#pragma once

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (KERNEL_VERSION(2, 7, 0) <= LINUX_VERSION_CODE) || __ANDROID_API__

#include "wtr/watcher.hpp"
#include <atomic>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <system_error>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace detail::wtr::watcher::adapter::inotify {

struct ev_in {
  /*  The maximum length of an inotify
      event. Inotify will add at least
      one null byte to the end of the
      event to terminate the path name,
      but *also* to align the next event
      on an 8-byte boundary. We add one
      here to account for that byte, in
      this case for alignment.
  */
  static constexpr auto one_ulim = sizeof(inotify_event) + NAME_MAX + 1;
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
  static constexpr auto buf_len = 4096;
  static_assert(buf_len > one_ulim * 8, "capacity");
  /*  The upper limit of how many events
      we could possibly read into a buffer
      which is `ev_buf_len` bytes large.
      Practically, if we come half-way
      close to this value, we should
      be skeptical of the `read`.
  */
  static constexpr auto c_ulim = buf_len / sizeof(inotify_event);
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
  static constexpr unsigned recv_mask = IN_CREATE | IN_DELETE | IN_DELETE_SELF
                                      | IN_MODIFY | IN_MOVE_SELF | IN_MOVED_FROM
                                      | IN_MOVED_TO;

  int fd = -1;
  using paths = std::unordered_map<int, std::filesystem::path>;
  paths dm{};
  alignas(inotify_event) char buf[buf_len]{0};
};

struct sysres {
  bool ok = false;
  ev_in ke{};
  adapter::ep ep{};
};

inline auto do_mark = [](
                        char const* const dirpath,
                        int dirfd,
                        auto& dm,
                        auto const& cb) noexcept -> bool
{
  auto do_error = [&]() -> bool
  { return adapter::do_error("w/sys/not_watched@", dirpath, cb); };
  char real[PATH_MAX];
  if (! realpath(dirpath, real)) return do_error();
  int wd = inotify_add_watch(dirfd, real, ev_in::recv_mask);
  return wd > 0       ? (dm.emplace(wd, real), true)
       : is_dir(real) ? do_error()
                      : false;
};

inline auto make_sysres =
  [](char const* const base_path, auto const& cb) noexcept -> sysres
{
  auto do_error = [&](std::string&& msg = "e/self/resource@")
  { return (adapter::do_error(std::move(msg), base_path, cb), sysres{}); };
  /*  todo: Do we really need to be non-blocking? */
  int in_fd = inotify_init();
  if (in_fd < 0) return do_error("e/sys/inotify_init@");
  auto dm = ev_in::paths{};
  adapter::walkdir_do(
    base_path,
    [&](char const* const dir) { do_mark(dir, in_fd, dm, cb); });
  if (dm.empty()) (close(in_fd), do_error());
  auto ep = adapter::make_ep(base_path, cb, in_fd);
  if (ep.fd < 0) return (close(in_fd), do_error());
  return sysres{
    .ok = true,
    .ke{
        .fd = in_fd,
        .dm = std::move(dm),
        },
    .ep = ep,
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
    An event, probably from some
    other recent instance of
    inotify, somehow got to us.
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
    We need to postpone removing
    this watch and, possibly, its
    watch descriptor from our path
    map until we're dont with this
    even loop. Self-destroy events
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
inline auto do_event_recv =
  [](char const* const base_path, auto const& cb, sysres& sr) noexcept -> bool
{
  auto do_error = [&](auto&& msg) noexcept -> bool
  { return adapter::do_error(msg, base_path, cb); };
  auto do_warn = [&](auto&& msg) noexcept -> bool { return ! do_error(msg); };
  auto defer_close = std::vector<int>{};
  auto do_deferred = [&]() noexcept
  {
    /*  No need to check rm_watch for errors
        because there is a very good chance
        that inotify closed the fd by itself.
        It's just here in case it didn't. */
    auto ok = true;
    for (auto wd : defer_close)
      ok |=
        ((inotify_rm_watch(sr.ke.fd, wd), sr.ke.dm.find(wd) != sr.ke.dm.end()))
          ? sr.ke.dm.erase(wd)
          : do_error("w/self/impossible_event@");
    return ok;
  };

  memset(sr.ke.buf, 0, sr.ke.buf_len);
  auto read_len = read(sr.ke.fd, sr.ke.buf, sizeof(sr.ke.buf));
  auto const* e = (inotify_event*)(sr.ke.buf);
  auto const* const ev_tail = (inotify_event*)(sr.ke.buf + read_len);
  if (read_len < 0 && errno != EAGAIN) return do_error("e/sys/read@");
  while (e && e < ev_tail) {
    unsigned ev_c = 0;
    unsigned msk = e->mask;
    auto d = sr.ke.dm.find(e->wd);
    auto n = [&]()
    {
      auto nv = sizeof(inotify_event) + e->len;
      auto p = (inotify_event*)((char*)e + nv);
      return p < ev_tail ? p : nullptr;
    }();
    enum {
      e_lim,
      w_lim,
      phantom,
      impossible,
      ignore,
      self_del,
      self_delmov,
      eventful,
    } recv_state = ev_c++ > ev_in::c_ulim                         ? e_lim
                 : msk & IN_Q_OVERFLOW                            ? w_lim
                 : d == sr.ke.dm.end()                            ? phantom
                 : ! (msk & ev_in::recv_mask)                     ? impossible
                 : msk & IN_IGNORED                               ? ignore
                 : msk & IN_DELETE_SELF && ! (msk & IN_MOVE_SELF) ? self_del
                 : msk & IN_DELETE_SELF || msk & IN_MOVE_SELF     ? self_delmov
                                                                  : eventful;
    switch (recv_state) {
      case e_lim : return (do_deferred(), do_error("e/sys/event_lim@"));
      case w_lim : do_warn("w/sys/event_lim@"); break;
      case phantom : do_warn("w/sys/phantom_event@"); break;
      case impossible : break;
      case ignore : break;
      case self_del : defer_close.push_back(e->wd); break;
      case self_delmov : break;
      case eventful : {
        using ev = ::wtr::watcher::event;
        using ev_pt = enum ev::path_type;
        using ev_et = enum ev::effect_type;
        auto pathof = [dirname = d->second](char const* const filename)
        { return dirname / std::filesystem::path{filename}; };
        auto do_mark = [&]() {
          return inotify::do_mark(
            pathof(e->name).c_str(),
            sr.ke.fd,
            sr.ke.dm,
            cb);
        };
        auto pt = msk & IN_ISDIR ? ev_pt::dir : ev_pt::file;
        auto et = msk & IN_CREATE     ? ev_et::create
                : msk & IN_DELETE     ? ev_et::destroy
                : msk & IN_MOVED_FROM ? ev_et::rename
                : msk & IN_MOVED_TO   ? ev_et::rename
                : msk & IN_MODIFY     ? ev_et::modify
                                      : ev_et::other;
        auto a = [&]() -> ev { return {pathof(e->name), et, pt}; };
        auto b = [&]() -> ev { return {pathof(n->name), et, pt}; };
        enum {
          one_markable,
          assoc_ltr,
          assoc_rtl,
          one,
        } send_as = (et == ev_et::rename && n && n->cookie == e->cookie)
                    ? (e->mask & IN_MOVED_FROM) && (n->mask & IN_MOVED_TO)
                      ? assoc_ltr
                      : assoc_rtl
                  : (et == ev_et::create && pt == ev_pt::dir) ? one_markable
                                                              : one;
        send_as == one_markable ? (do_mark(), cb(a()))
        : send_as == assoc_ltr  ? cb({a(), b()})
        : send_as == assoc_rtl  ? cb({b(), a()})
                                : cb(a());
      } break;
    }
    e = n;
  }
  return do_deferred();
};

inline auto watch(
  char const* const path,
  ::wtr::watcher::event::callback const& cb,
  std::atomic<bool>& is_living) noexcept -> bool
{
  auto sr = make_sysres(path, cb);
  auto do_error = [&](auto&& msg) -> bool
  { return (close_sysres(sr), adapter::do_error(msg, path, cb)); };

  if (! sr.ok) return do_error("e/self/resource@");
  while (is_living) {
    int ep_c =
      epoll_wait(sr.ep.fd, sr.ep.interests, sr.ep.q_ulim, sr.ep.wake_ms);
    if (ep_c < 0)
      return do_error("e/sys/epoll_wait@");
    else if (ep_c > 0) [[likely]]
      for (int n = 0; n < ep_c; n++)
        if (sr.ep.interests[n].data.fd == sr.ke.fd) [[likely]]
          if (! do_event_recv(path, cb, sr)) [[unlikely]]
            return do_error("e/self/event_recv@");
  }

  return close_sysres(sr);
}

} /* namespace detail::wtr::watcher::adapter::inotify */

#endif
#endif
