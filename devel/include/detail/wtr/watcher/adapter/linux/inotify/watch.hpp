#pragma once

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (KERNEL_VERSION(2, 7, 0) <= LINUX_VERSION_CODE) || __ANDROID_API__

#include "wtr/watcher.hpp"
#include <filesystem>
#include <limits.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>

namespace detail::wtr::watcher::adapter::inotify {

// clang-format off
struct ke_in_ev {
  /*  The maximum length of an inotify
      event. Inotify will add at least
      one null byte to the end of the
      event to terminate the path name,
      but *also* to align the next event
      on an 8-byte boundary. We add one
      here to account for that byte, in
      this case for alignment.
  */
  static constexpr unsigned one_ulim = sizeof(inotify_event) + NAME_MAX + 1;
  static_assert(one_ulim % 8 == 0, "alignment");
  /*  Practically, this buffer is large
      enough for most read calls we would
      ever need to make. It's many times
      the size of the largest possible
      event, and those events are rare.
      Note that this isn't scaled with
      our `ep_q_sz` because this buffer
      length has nothing to do with an
      `epoll` event loop and everything
      to do with `read()` calls which
      fill a buffer with batched events.
  */
  static constexpr unsigned buf_len = 4096;
  static_assert(buf_len > one_ulim * 8, "capacity");
  /*  The upper limit of how many events
      we could possibly read into a buffer
      which is `buf_len` bytes large.
      Practically, if we come half-way
      close to this value, we should
      be skeptical of the `read`.
  */
  static constexpr unsigned c_ulim = buf_len / sizeof(inotify_event);
  static_assert(c_ulim == 256);
  /*  These are the kinds of events which
      we're intersted in. Inotify *should*
      only send us these events, but that's
      not always the case. What's more, we
      sometimes receive events not in the
      IN_ALL_EVENTS mask. We'll ask inotify
      for these events and filter more if
      needed later on.
      These are all the supported events:
        IN_ACCESS
        IN_ATTRIB
        IN_CLOSE_WRITE
        IN_CLOSE_NOWRITE
        IN_CREATE
        IN_DELETE
        IN_DELETE_SELF
        IN_MODIFY
        IN_MOVE_SELF
        IN_MOVED_FROM
        IN_MOVED_TO
        IN_OPEN
  */
  static constexpr unsigned recv_mask
    = IN_CREATE
    | IN_DELETE
    | IN_DELETE_SELF
    | IN_MODIFY
    | IN_MOVE_SELF
    | IN_MOVED_FROM
    | IN_MOVED_TO;

  int fd = -1;
  using paths = std::unordered_map<int, std::filesystem::path>;
  paths dm{};
  alignas(inotify_event) char ev_buf[buf_len]{0};
  int rm_wd_buf[buf_len]{0};

  static_assert(sizeof(ev_buf) % sizeof(inotify_event) == 0, "alignment");
};

// clang-format on

struct sysres {
  result ok = result::e;
  ke_in_ev ke{};
  semabin const& il{};
  adapter::ep ep{};
};

inline auto do_mark =
  [](char const* const dirpath, int dirfd, auto& dm, auto const& cb) -> result
{
  auto e = result::w_sys_not_watched;
  char real[PATH_MAX];
  int wd = realpath(dirpath, real) && is_dir(real)
           ? inotify_add_watch(dirfd, real, ke_in_ev::recv_mask)
           : -1;
  if (wd > 0)
    return dm.emplace(wd, real), result::complete;
  else
    return send_msg(e, dirpath, cb), e;
};

inline auto make_sysres = [](
                            char const* const base_path,
                            auto const& cb,
                            semabin const& living) -> sysres
{
  auto make_inotify = [](result* ok) -> int
  {
    if (*ok >= result::e) return -1;
    int in_fd = inotify_init();
    if (in_fd < 0) *ok = result::e_sys_api_inotify;
    return in_fd;
  };

  auto make_dm = [&](result* ok, int in_fd) -> ke_in_ev::paths
  {
    auto dm = ke_in_ev::paths{};
    if (*ok >= result::e) return dm;
    walkdir_do(base_path, [&](auto dir) { do_mark(dir, in_fd, dm, cb); });
    if (dm.empty()) *ok = result::e_self_noent;
    return dm;
  };

  auto make_ep = [&](result* ok, int in_fd, int il_fd) -> ep
  {
    if (*ok >= result::e) return ep{};
    auto ep = adapter::make_ep(in_fd, il_fd);
    if (ep.fd < 0) *ok = result::e_sys_api_epoll;
    return ep;
  };

  auto ok = result::pending;
  auto in_fd = make_inotify(&ok);
  auto dm = make_dm(&ok, in_fd);
  auto ep = make_ep(&ok, in_fd, living.fd);
  return sysres{
    .ok = ok,
    .ke{
        .fd = in_fd,
        .dm = std::move(dm),
        },
    .il = living,
    .ep = ep,
  };
};

inline auto peek = [](
                     inotify_event const* const in_ev,
                     inotify_event const* const ev_tail) -> inotify_event*
{
  auto len_to_next = sizeof(inotify_event) + (in_ev ? in_ev->len : 0);
  auto next = (inotify_event*)((char*)in_ev + len_to_next);
  return next < ev_tail ? next : nullptr;
};

struct parsed {
  ::wtr::watcher::event ev;
  inotify_event* next = nullptr;
};

inline auto parse_ev = [](
                         std::filesystem::path const& dirname,
                         inotify_event const* const in,
                         inotify_event const* const tail) -> parsed
{
  using ev = ::wtr::watcher::event;
  using ev_pt = enum ev::path_type;
  using ev_et = enum ev::effect_type;
  auto pathof = [&](inotify_event const* const m)
  { return dirname / std::filesystem::path{m->name}; };
  auto pt = in->mask & IN_ISDIR ? ev_pt::dir : ev_pt::file;
  auto et = in->mask & IN_CREATE ? ev_et::create
          : in->mask & IN_DELETE ? ev_et::destroy
          : in->mask & IN_MOVE   ? ev_et::rename
          : in->mask & IN_MODIFY ? ev_et::modify
                                 : ev_et::other;
  auto isassoc = [&](auto* a, auto* b) -> bool
  { return b && b->cookie && b->cookie == a->cookie && et == ev_et::rename; };
  auto isfromto = [](auto* a, auto* b) -> bool
  { return (a->mask & IN_MOVED_FROM) && (b->mask & IN_MOVED_TO); };
  auto one = [&](auto* a, auto* next) -> parsed {
    return {ev(pathof(a), et, pt), next};
  };
  auto assoc = [&](auto* a, auto* b) -> parsed {
    return {ev(ev(pathof(a), et, pt), ev(pathof(b), et, pt)), peek(b, tail)};
  };
  auto next = peek(in, tail);
  return ! isassoc(in, next) ? one(in, next)
       : isfromto(in, next)  ? assoc(in, next)
       : isfromto(next, in)  ? assoc(next, in)
                             : one(in, next);
};

struct defer_dm_rm_wd {
  ke_in_ev& ke;
  size_t back_idx = 0;

  /*  It is impossible to exceed our buffer
      because we store as many events as are
      possible to receive from `read()`.
      If we do exceed the indices, then some
      system invariant has been violated. */
  inline auto push(int wd) -> bool
  {
    if (back_idx < ke.buf_len)
      return ke.rm_wd_buf[back_idx++] = wd, true;
    else
      return false;
  };

  inline defer_dm_rm_wd(ke_in_ev& ke)
      : ke{ke} {};

  inline ~defer_dm_rm_wd()
  {
    for (size_t i = 0; i < back_idx; ++i) {
      auto wd = ke.rm_wd_buf[i];
      auto at = ke.dm.find(wd);
      if (at != ke.dm.end()) ke.dm.erase(at);
    }
  };
};

/*  Parses each event's path name,
    path type and effect.
    Looks for the directory path
    in a map of watch descriptors
    to directory paths.
    Updates the path map, adding
    new directories as they are
    created, and removing them
    as they are destroyed.

    Forward events and errors to
    the user. Returns on errors
    and when eventless.

    Event notes:
    Phantom Events --
    An event from an unmarked path,
    or a path which we didn't mark,
    was somehow reported to us.
    These events are rare, but they
    do happen. We won't be able to
    parse this event's real path.
    The ->name field seems to be
    null on these events, and we
    won't have a directory path to
    prepend it with. I'm not sure
    if we should try to parse the
    other fields, or if they would
    be interesting to the user.
    This may change.

    Impossible Events --
    These events are relatively
    rare, but they happen more
    than I think they should. We
    usually see these during
    high-throughput flurries of
    events. Maybe there is an
    error in our implementation?

    Deferred Events --
    Inotify closes the removed watch
    descriptors itself. We want to
    keep parity with inotify in our
    path map. That way, we can be
    in agreement about which watch
    descriptors map to which paths.
    We need to postpone removing
    this watch and, possibly, its
    watch descriptor from our path
    map until we're done with this
    event batch. Self-destroy events
    might come before we read other
    events that would map the watch
    descriptor to a path. Because
    we need to look the directory
    path up in the path map, we
    will defer its removal.

    Other notes:
    Sometimes we can fail to mark
    a new directory on its create
    event. This can happen if the
    directory is removed quickly
    after being created.
    In that case, we are unlikely
    to lose any path names on
    future events because events
    won't happen in that directory.
    If this happens for some other
    reason, we're in trouble.
*/
inline auto do_ev_recv = [](auto const& cb, sysres& sr) -> result
{
  auto is_parity_lost = [](unsigned msk) -> bool
  { return msk & IN_DELETE_SELF && ! (msk & IN_MOVE_SELF); };
  auto is_real_event = [](unsigned msk) -> bool
  {
    bool has_any = msk & ke_in_ev::recv_mask;
    bool is_self_info = msk & (IN_IGNORED | IN_DELETE_SELF | IN_MOVE_SELF);
    return has_any && ! is_self_info;
  };

  auto read_len = read(sr.ke.fd, sr.ke.ev_buf, sizeof(sr.ke.ev_buf));
  if (read_len < 0 && errno != EAGAIN)
    return result::e_sys_api_read;
  else {
    auto const* in_ev = (inotify_event*)(sr.ke.ev_buf);
    auto const* const in_ev_tail = (inotify_event*)(sr.ke.ev_buf + read_len);
    unsigned in_ev_c = 0;
    auto dmrm = defer_dm_rm_wd{sr.ke};
    while (in_ev && in_ev < in_ev_tail) {
      auto in_ev_next = peek(in_ev, in_ev_tail);
      unsigned msk = in_ev->mask;
      auto dmhit = sr.ke.dm.find(in_ev->wd);
      if (in_ev_c++ > ke_in_ev::c_ulim)
        return result::e_sys_ret;
      else if (is_parity_lost(msk) && ! dmrm.push(in_ev->wd))
        return result::e_sys_ret;
      else if (dmhit == sr.ke.dm.end())
        send_msg(result::w_sys_phantom, "", cb);
      else if (msk & IN_Q_OVERFLOW)
        send_msg(result::w_sys_q_overflow, dmhit->second.c_str(), cb);
      else if (is_real_event(msk)) {
        auto [ev, next] = parse_ev(dmhit->second, in_ev, in_ev_tail);
        if (msk & IN_ISDIR && msk & IN_CREATE)
          do_mark(ev.path_name.c_str(), sr.ke.fd, sr.ke.dm, cb);
        cb(ev);
        in_ev_next = next;
      }
      in_ev = in_ev_next;
    }
    return result::pending;
  }
};

} /*  namespace detail::wtr::watcher::adapter::inotify */

#endif
#endif
