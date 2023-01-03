#pragma once

/*
  @brief wtr/watcher/<d>/adapter/linux/inotify

  The Linux `inotify` adapter.
*/

#include <watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
    || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <watcher/adapter/adapter.hpp>
/* event
   callback */
#include <watcher/event.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace inotify {

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>
   Anonymous namespace for "private" things. */
namespace {

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/constants
   - delay
       The delay, in milliseconds, while `epoll_wait` will
       'sleep' for until we are woken up. We usually check
       if we're still alive at that point.
   - event_wait_queue_max
       Number of events allowed to be given to do_event_recv
       (returned by `epoll_wait`). Any number between 1
       and some large number should be fine. We don't
       lose events if we 'miss' them, the events are
       still waiting in the next call to `epoll_wait`.
   - event_buf_len:
       For our event buffer, 4096 is a typical page size
       and sufficiently large to hold a great many events.
       That's a good thumb-rule.
   - in_init_opt
       Use non-blocking IO.
   - in_watch_opt
       Everything we can get.
   @todo
   - Measure perf of IN_ALL_EVENTS
   - Handle move events properly.
     - Use IN_MOVED_TO
     - Use event::<something> */
inline constexpr auto delay_ms = 16;
inline constexpr auto event_wait_queue_max = 1;
inline constexpr auto event_buf_len = 4096;
inline constexpr auto in_init_opt = IN_NONBLOCK;
inline constexpr auto in_watch_opt
    = IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_Q_OVERFLOW;

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/types
   - path_map_type
       An alias for a map of file descriptors to paths.
   - sys_resource_type
       An object representing an inotify file descriptor,
       an epoll file descriptor, an epoll configuration,
       and whether or not these resources are valid. */
using path_map_type = std::unordered_map<int, std::filesystem::path>;
struct sys_resource_type
{
  bool valid;
  int watch_fd;
  int event_fd;
  struct epoll_event event_conf;
};

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns
   Functions:

     do_path_map_create
       -> path_map_type

     do_sys_resource_create
       -> sys_resource_type

     do_resource_release
       -> bool

     do_event_recv
       -> bool
*/

inline auto do_path_map_create(int const watch_fd,
                               std::filesystem::path const& watch_base_path,
                               event::callback const& callback) noexcept
    -> path_map_type;
inline auto do_sys_resource_create(event::callback const& callback) noexcept
    -> sys_resource_type;
inline auto do_resource_release(int watch_fd, int event_fd,
                                std::filesystem::path const& watch_base_path,
                                event::callback const& callback) noexcept
    -> bool;
inline auto do_event_recv(int watch_fd, path_map_type& path_map,
                          std::filesystem::path const& watch_base_path,
                          event::callback const& callback) noexcept -> bool;

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_path_map_create
   If the path given is a directory
     - find all directories above the base path given.
     - ignore nonexistent directories.
     - return a map of watch descriptors -> directories.
   If `path` is a file
     - return it as the only value in a map.
     - the watch descriptor key should always be 1. */
inline auto do_path_map_create(int const watch_fd,
                               std::filesystem::path const& watch_base_path,
                               event::callback const& callback) noexcept
    -> path_map_type
{
  using rdir_iterator = std::filesystem::recursive_directory_iterator;
  /* Follow symlinks, ignore paths which we don't have permissions for. */
  static constexpr auto dir_opt
      = std::filesystem::directory_options::skip_permission_denied
        & std::filesystem::directory_options::follow_directory_symlink;

  static constexpr auto path_map_reserve_count = 256;

  auto dir_ec = std::error_code{};
  path_map_type path_map;
  path_map.reserve(path_map_reserve_count);

  auto do_mark = [&](auto& dir) {
    int wd = inotify_add_watch(watch_fd, dir.c_str(), in_watch_opt);
    return wd > 0 ? path_map.emplace(wd, dir).first != path_map.end() : false;
  };

  if (!do_mark(watch_base_path))
    return path_map_type{};
  else if (std::filesystem::is_directory(watch_base_path, dir_ec))
    /* @todo @note
       Should we bail from within this loop if `do_mark` fails? */
    for (auto const& dir : rdir_iterator(watch_base_path, dir_opt, dir_ec))
      if (!dir_ec)
        if (std::filesystem::is_directory(dir, dir_ec))
          if (!dir_ec)
            if (!do_mark(dir.path()))
              callback({"w/sys/path_unwatched@" / dir.path(),
                        event::what::other, event::kind::watcher});
  return path_map;
};

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_sys_resource_create
   Produces a `sys_resource_type` with the file descriptors from
   `inotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto do_sys_resource_create(event::callback const& callback) noexcept
    -> sys_resource_type
{
  auto const do_error
      = [&callback](auto const& msg, int watch_fd, int event_fd = -1) {
          callback({msg, event::what::other, event::kind::watcher});
          return sys_resource_type{
              .valid = false,
              .watch_fd = watch_fd,
              .event_fd = event_fd,
              .event_conf = {.events = 0, .data = {.fd = watch_fd}}};
        };

  int watch_fd
#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
      = inotify_init();
#elif defined(WATER_WATCHER_PLATFORM_LINUX_ANY)
      = inotify_init1(in_init_opt);
#endif

  if (watch_fd >= 0) {
    struct epoll_event event_conf
    {
      .events = EPOLLIN, .data { .fd = watch_fd }
    };

    int event_fd
#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
        = epoll_create(event_wait_queue_max);
#elif defined(WATER_WATCHER_PLATFORM_LINUX_ANY)
        = epoll_create1(EPOLL_CLOEXEC);
#endif

    if (event_fd >= 0)
      if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) >= 0)
        return sys_resource_type{.valid = true,
                                 .watch_fd = watch_fd,
                                 .event_fd = event_fd,
                                 .event_conf = event_conf};
      else
        return do_error("e/sys/epoll_ctl", watch_fd, event_fd);
    else
      return do_error("e/sys/epoll_create", watch_fd, event_fd);
  } else {
    return do_error("e/sys/inotify_init", watch_fd);
  }
}

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_resource_release
   Close the file descriptors `watch_fd` and `event_fd`.
   Invoke `callback` on errors. */
inline auto do_resource_release(int watch_fd, int event_fd,
                                std::filesystem::path const& watch_base_path,
                                event::callback const& callback) noexcept
    -> bool
{
  auto const watch_fd_close_ok = close(watch_fd) == 0;
  auto const event_fd_close_ok = close(event_fd) == 0;
  if (!watch_fd_close_ok)
    callback({"e/sys/close/watch_fd@" / watch_base_path, event::what::other,
              event::kind::watcher});
  if (!event_fd_close_ok)
    callback({"e/sys/close/event_fd@" / watch_base_path, event::what::other,
              event::kind::watcher});
  return watch_fd_close_ok && event_fd_close_ok;
}

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_event_recv
   Reads through available (inotify) filesystem events.
   Discerns their path and type.
   Calls the callback.
   Returns false on eventful errors.

   @todo
   Return new directories when they appear,
   Consider running and returning `find_dirs` from here.
   Remove destroyed watches. */
inline auto do_event_recv(int watch_fd, path_map_type& path_map,
                          std::filesystem::path const& watch_base_path,
                          event::callback const& callback) noexcept -> bool
{
  alignas(struct inotify_event) char buf[event_buf_len];

  enum class event_recv_state { eventful, eventless, error };

  auto const lift_this_event = [](int fd, char* buf) {
    /* Read some events. */
    ssize_t len = read(fd, buf, event_buf_len);

    /* EAGAIN means no events were found.
       We return `eventless` in that case. */
    if (len < 0 && errno != EAGAIN)
      return std::make_pair(event_recv_state::error, len);
    else if (len <= 0)
      return std::make_pair(event_recv_state::eventless, len);
    else
      return std::make_pair(event_recv_state::eventful, len);
  };

  /* Loop while events can be read from the inotify file descriptor. */
  while (true) {
    /* Read events */
    auto [status, len] = lift_this_event(watch_fd, buf);
    /* Handle the errored, eventless and eventful reads */
    switch (status) {
      case event_recv_state::eventful:
        /* Loop over all events in the buffer. */
        const struct inotify_event* this_event;
        for (char* ptr = buf; ptr < buf + len;
             ptr += sizeof(struct inotify_event) + this_event->len)
        {
          this_event = (const struct inotify_event*)ptr;

          /* @todo
             Consider using std::filesystem here. */
          auto const path_kind = this_event->mask & IN_ISDIR
                                     ? event::kind::dir
                                     : event::kind::file;
          int path_wd = this_event->wd;
          auto event_dir_path = path_map.find(path_wd)->second;
          auto event_base_name = std::filesystem::path(this_event->name);
          auto event_path = event_dir_path / event_base_name;

          if (this_event->mask & IN_Q_OVERFLOW) {
            callback({"e/self/overflow@" / watch_base_path, event::what::other,
                      event::kind::watcher});
          } else if (this_event->mask & IN_CREATE) {
            callback({event_path, event::what::create, path_kind});
            if (path_kind == event::kind::dir) {
              int new_watch_fd = inotify_add_watch(watch_fd, event_path.c_str(),
                                                   in_watch_opt);
              path_map[new_watch_fd] = event_path;
            }
          } else if (this_event->mask & IN_DELETE) {
            callback({event_path, event::what::destroy, path_kind});
            /* @todo rm watch, rm path map entry */
          } else if (this_event->mask & IN_MOVE) {
            callback({event_path, event::what::rename, path_kind});
          } else if (this_event->mask & IN_MODIFY) {
            callback({event_path, event::what::modify, path_kind});
          } else {
            callback({event_path, event::what::other, path_kind});
          }
        }
        /* We don't want to return here. We run until `eventless`. */
        break;
      case event_recv_state::error:
        callback({"e/sys/read@" / watch_base_path, event::what::other,
                  event::kind::watcher});
        return false;
        break;
      case event_recv_state::eventless: return true; break;
    }
  }
}

} /* namespace */

/*
  @brief wtr/watcher/<d>/adapter/watch
  Monitors `path` for changes.
  Invokes `callback` with an `event` when they happen.
  `watch` stops when asked to or irrecoverable errors occur.
  All events, including errors, are passed to `callback`.

  @param path
    A filesystem path to watch for events.

  @param callback
    A function to invoke with an `event` object
    when the files being watched change.

  @param is_living
    A function to decide whether we're dead.
*/
inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  auto const& is_living) noexcept
{
  auto const do_error = [&callback](auto const& msg) -> bool {
    callback({msg, event::what::other, event::kind::watcher});
    return false;
  };

  /* Gather these resources:
       - system resources
           For inotify and epoll
       - event recieve list
           For receiving epoll events
       - path map
           For event to path lookups */

  struct sys_resource_type sr = do_sys_resource_create(callback);
  if (sr.valid) {
    auto path_map = do_path_map_create(sr.watch_fd, path, callback);
    if (path_map.size() > 0) {
      struct epoll_event event_recv_list[event_wait_queue_max];

      /* Do this work until dead:
          - Await filesystem events
          - Invoke `callback` on errors and events */

      while (is_living()) {
        int event_count = epoll_wait(sr.event_fd, event_recv_list,
                                     event_wait_queue_max, delay_ms);
        if (event_count < 0)
          return do_resource_release(sr.watch_fd, sr.event_fd, path, callback)
                 && do_error("e/sys/epoll_wait@" / path);
        else if (event_count > 0)
          for (int n = 0; n < event_count; n++)
            if (event_recv_list[n].data.fd == sr.watch_fd)
              if (is_living())
                if (!do_event_recv(sr.watch_fd, path_map, path, callback))
                  return do_resource_release(sr.watch_fd, sr.event_fd, path,
                                             callback)
                         && do_error("e/self/event_recv@" / path);
      }
      return do_resource_release(sr.watch_fd, sr.event_fd, path, callback);
    } else {
      return do_resource_release(sr.watch_fd, sr.event_fd, path, callback)
             && do_error("e/self/path_map@" / path);
    }
  } else {
    return do_resource_release(sr.watch_fd, sr.event_fd, path, callback)
           && do_error("e/self/sys_resource@" / path);
  }
}

} /* namespace inotify */
} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
          || defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
#endif /* if !defined(WATER_WATCHER_USE_WARTHOG) */
