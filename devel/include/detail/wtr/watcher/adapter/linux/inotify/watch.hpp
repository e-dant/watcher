#pragma once

#if (defined(__linux__) || defined(__ANDROID_API__)) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 7, 0)) || defined(__ANDROID_API__)

#include "wtr/watcher.hpp"
#include <atomic>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <system_error>
#include <tuple>
#include <unistd.h>
#include <unordered_map>

namespace detail::wtr::watcher::adapter::inotify {

/*  - pathmap_type
        An alias for a map of file descriptors to paths.
    - sysres_type
        An object representing an inotify file descriptor,
        an epoll file descriptor, an epoll configuration,
        and whether or not the resources are ok */
using pathmap_type = std::unordered_map<int, std::filesystem::path>;

struct sysres_type {
  /*  Number of events allowed to be
      given to do_event_recv. The same
      value is usually returned by a
      call to `epoll_wait`). Any number
      between 1 and INT_MAX should be
      fine... But 1 is fine for us.
      We don't lose events if we 'miss'
      them, the events are still waiting
      in the next call to `epoll_wait`.
  */
  static constexpr auto ep_q_sz = 1;
  /*  The delay, in milliseconds, while
      `epoll_wait` will 'pause' us for
      until we are woken up. We use the
      time after this to check if we're
      still alive, then re-enter epoll.
  */
  static constexpr auto ep_delay_ms = 16;
  /*  Practically, this buffer is large
      enough for most read calls we would
      ever need to make. It's eight times
      the size of the largest possible
      event, and those events are rare.
      Note that this isn't scaled with
      our `eq_q_sz` because events are
      batched together elsewhere as well.
  */
  static constexpr auto ev_buf_len = (sizeof(inotify_event) + NAME_MAX + 1) * 8;
  /*  The upper limit of how many events
      we could possibly read into a buffer
      which is `ev_buf_len` bytes large.
      Practically, if we come half-way
      close to this value, we should
      be skeptical of the `read`.
  */
  static constexpr auto ev_c_ulim = ev_buf_len / sizeof(inotify_event);
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
  static constexpr unsigned in_ev_mask = IN_CREATE | IN_DELETE | IN_DELETE_SELF
                                       | IN_MODIFY | IN_MOVE_SELF
                                       | IN_MOVED_FROM | IN_MOVED_TO;

  bool ok = false;
  int in_fd = -1;
  int ep_fd = -1;
  epoll_event ep_interests[ep_q_sz]{};
  pathmap_type dm{};
  alignas(inotify_event) char ev_buf[ev_buf_len]{0};
};

inline auto do_error =
  [](auto&& msg, auto const& path, auto const& callback) noexcept -> bool
{
  using et = enum ::wtr::watcher::event::effect_type;
  using pt = enum ::wtr::watcher::event::path_type;
  callback({msg + path.string(), et::other, pt::watcher});
  return false;
};

inline auto do_warn =
  [](auto&& msg, auto const& path, auto const& callback) noexcept -> bool
{ return (do_error(std::move(msg), path, callback), true); };

inline auto do_mark =
  [](auto& dm, int dirfd, auto const& dirname) noexcept -> bool
{
  if (! std::filesystem::is_directory(dirname)) return false;
  int wd = inotify_add_watch(dirfd, dirname.c_str(), sysres_type::in_ev_mask);
  return wd > 0 ? dm.emplace(wd, dirname).first != dm.end() : false;
};

/*  If the path given is a directory
      - find all directories above the base path given.
      - ignore nonexistent directories.
      - return a map of watch descriptors -> directories.
    If `path` is a file
      - return it as the only value in a map.
      - the watch descriptor key should always be 1. */
inline auto make_pathmap = [](
                             auto const& base_path,
                             auto const& callback,
                             int inotify_watch_fd) noexcept -> pathmap_type
{
  namespace fs = std::filesystem;
  using dopt = fs::directory_options;
  using diter = fs::recursive_directory_iterator;
  using fs::is_directory;
  constexpr auto fs_dir_opt =
    dopt::skip_permission_denied & dopt::follow_directory_symlink;
  auto dm = pathmap_type{};
  dm.reserve(256);
  auto do_mark = [&](auto d) noexcept
  { return inotify::do_mark(dm, inotify_watch_fd, d); };
  try {
    if (is_directory(base_path) && do_mark(base_path))
      for (auto dir : diter(base_path, fs_dir_opt))
        if (is_directory(dir) && ! do_mark(dir.path()))
          do_warn("w/sys/not_watched@", base_path, callback);
  } catch (...) {}
  return dm;
};

inline auto make_sysres =
  [](auto const& base_path, auto const& callback) noexcept -> sysres_type
{
  auto do_error = [&](auto&& msg, int in_fd, int ep_fd = -1)
  {
    return (
      inotify::do_error(std::move(msg), base_path, callback),
      sysres_type{
        .ok = false,
        .in_fd = in_fd,
        .ep_fd = ep_fd,
      });
  };
#if defined(__ANDROID_API__)
  int in_fd = inotify_init();
#else
  /*  todo: Do we need to be non-blocking? */
  int in_fd = inotify_init1(IN_NONBLOCK);
#endif
  if (in_fd < 0) return do_error("e/sys/inotify_init@", in_fd);
#if defined(__ANDROID_API__)
  int ep_fd = epoll_create(1);
#else
  int ep_fd = epoll_create1(EPOLL_CLOEXEC);
#endif
  if (ep_fd < 0) return do_error("e/sys/epoll_create@", in_fd, ep_fd);
  auto dm = make_pathmap(base_path, callback, in_fd);
  if (dm.empty()) return do_error("e/self/pathmap@", in_fd, ep_fd);
  auto want_ev = epoll_event{.events = EPOLLIN, .data{.fd = in_fd}};
  int ctlec = epoll_ctl(ep_fd, EPOLL_CTL_ADD, in_fd, &want_ev);
  if (ctlec < 0) return do_error("e/sys/epoll_ctl@", in_fd, ep_fd);
  return sysres_type{
    .ok = true,
    .in_fd = in_fd,
    .ep_fd = ep_fd,
    .dm = std::move(dm),
  };
};

inline auto close_sysres = [](auto& sr) noexcept -> bool
{
  sr.ok |= close(sr.in_fd) == 0;
  sr.ok |= close(sr.ep_fd) == 0;
  return sr.ok;
};

/*  Event notes:
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
inline auto do_event_recv_one = [](
                                  auto const& base_path,
                                  auto const& callback,
                                  sysres_type& sr,
                                  size_t read_len) noexcept -> bool
{
  auto do_warn = [&](auto&& msg) noexcept -> bool
  { return inotify::do_warn(std::move(msg), base_path, callback); };
  auto do_error = [&](auto&& msg) noexcept -> bool
  { return inotify::do_error(std::move(msg), base_path, callback); };
  std::vector<int> defer_close{};
  auto do_deferred = [&]() noexcept
  {
    auto ok = true;
    for (auto wd : defer_close) {
      /*  No need to check rm_watch for errors
          because there is a very good chance
          that inotify closed the fd by itself.
          It's just here in case it didn't. */
      inotify_rm_watch(sr.in_fd, wd);
      if (sr.dm.find(wd) == sr.dm.end())
        do_error("w/self/impossible_event@");
      else
        ok |= sr.dm.erase(wd);
    }
    return ok;
  };
  auto const* event = (inotify_event*)(sr.ev_buf);
  auto const* const tail = (inotify_event*)(sr.ev_buf + read_len);
  while (event < tail) {
    unsigned ev_c = 0;
    unsigned msk = event->mask;
    auto d = sr.dm.find(event->wd);
    enum {
      e_lim,
      w_lim,
      phantom,
      impossible,
      ignore,
      self_del,
      self_delmov,
      eventful,
    } event_state = ev_c++ > sysres_type::ev_c_ulim                ? e_lim
                  : msk & IN_Q_OVERFLOW                            ? w_lim
                  : d == sr.dm.end()                               ? phantom
                  : ! (msk & sysres_type::in_ev_mask)              ? impossible
                  : msk & IN_IGNORED                               ? ignore
                  : msk & IN_DELETE_SELF && ! (msk & IN_MOVE_SELF) ? self_del
                  : msk & IN_DELETE_SELF || msk & IN_MOVE_SELF     ? self_delmov
                                                                   : eventful;
    switch (event_state) {
      case e_lim : return (do_deferred(), do_error("e/sys/event_lim@"));
      case w_lim : do_warn("w/sys/event_lim@"); break;
      case phantom : do_warn("w/sys/phantom_event@"); break;
      case impossible : break;
      case ignore : break;
      case self_del : defer_close.push_back(event->wd); break;
      case self_delmov : break;
      case eventful : {
        using pt = enum ::wtr::watcher::event::path_type;
        using et = enum ::wtr::watcher::event::effect_type;
        auto path_name = d->second / event->name;
        auto path_type = msk & IN_ISDIR ? pt::dir : pt::file;
        auto effect_type = msk & IN_CREATE     ? et::create
                         : msk & IN_DELETE     ? et::destroy
                         : msk & IN_MOVED_FROM ? et::rename
                         : msk & IN_MOVED_TO   ? et::rename
                         : msk & IN_MODIFY     ? et::modify
                                               : et::other;
        if (msk & IN_ISDIR && msk & IN_CREATE) {
          if (! do_mark(sr.dm, sr.in_fd, path_name))
            do_warn("w/sys/add_watch@");
        }
        callback({path_name, effect_type, path_type});  // <- Magic happens
      } break;
    }
    auto full_len = sizeof(inotify_event) + event->len;
    event = (inotify_event*)((char*)event + full_len);
  }
  return do_deferred();
};

/*  Reads through available (inotify) filesystem events.
    There might be several events from a single read.
    Three possible states:
     - eventful: there are events to read
     - eventless: there are no events to read
     - error: there was an error reading events
    The EAGAIN "error" means there is nothing
    to read. We count that as 'eventless'.
    Discerns each event's full path and type.
    Looks for the full path in `dm`, our map of
    watch descriptors to directory paths.
    Updates the path map, adding the directories
    with `create` events and removing the ones
    with `destroy` events.
    Forward events and errors to the user.
    Return when eventless.
    @todo
    Consider running and returning `find_dirs` from here.
    Remove destroyed watches. */
inline auto do_event_recv = [](
                              auto const& base_path,
                              auto const& callback,
                              sysres_type& sr) noexcept -> bool
{
  memset(sr.ev_buf, 0, sizeof(sr.ev_buf));
  auto read_len = read(sr.in_fd, sr.ev_buf, sizeof(sr.ev_buf));
  enum {
    eventful,
    eventless,
    error
  } read_state = read_len > 0 && *sr.ev_buf ? eventful
               : read_len == 0              ? eventless
               : errno == EAGAIN            ? eventless
                                            : error;
  switch (read_state) {
    case eventless : return true;
    case error : return do_error("e/sys/read@", base_path, callback);
    case eventful : {
      return do_event_recv_one(base_path, callback, sr, read_len);
    }
  }

  assert(! "Unreachable");
  return false;
};

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  std::atomic<bool>& is_living) noexcept -> bool
{
  /*  While:
      - A lifetime the user hasn't ended
      - A historical map of watch descriptors
        to long paths (for event reporting)
      - System resources for inotify and epoll
      - An event buffer for events from epoll
      - We're alive
      Do:
      - Await filesystem events
      - Invoke `callback` on errors and events */

  auto sr = make_sysres(path, callback);

  auto do_error = [&](auto&& msg)
  {
    return (
      close_sysres(sr),
      inotify::do_error(std::move(msg), path, callback));
  };

  if (sr.ok) {
    while (is_living) {
      int ev_count = epoll_wait(
        sr.ep_fd,
        sr.ep_interests,
        sysres_type::ep_q_sz,
        sysres_type::ep_delay_ms);
      if (ev_count < 0)
        return do_error("e/sys/epoll_wait@");
      else if (ev_count > 0)
        for (int n = 0; n < ev_count; n++)
          if (sr.ep_interests[n].data.fd == sr.in_fd)
            if (! do_event_recv(path, callback, sr))
              return do_error("e/self/event_recv@");
    }
  }
  return close_sysres(sr);
}

} /* namespace detail::wtr::watcher::adapter::inotify */

#endif
#endif
