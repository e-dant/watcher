#pragma once

/*
  @brief wtr/watcher/detail/adapter/linux

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
#include <watcher/event.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

/* @brief wtr/watcher/detail/adapter/linux/<a>
   Anonymous namespace for "private" things. */
namespace {

/* @brief wtr/watcher/detail/adapter/linux/<a>/constants
   - event_max_count
       Number of events allowed to be given to do_scan
       (returned by `epoll_wait`). Any number between 1
       and some large number should be fine. We don't
       lose events if we 'miss' them, the events are
       still waiting in the next call to `epoll_wait`.
   - in_init_opt
       Use non-blocking IO.
   - in_watch_opt
       Everything we can get.
   - scan_buf_len:
       4096, which is a typical page size.
   @todo
   - Measure perf of IN_ALL_EVENTS
   - Handle move events properly.
     - Use IN_MOVED_TO
     - Use event::<something> */
inline constexpr auto event_max_count = 1;
inline constexpr auto scan_buf_len = 4096;
inline constexpr auto in_init_opt = IN_NONBLOCK;
inline constexpr auto in_watch_opt
    = IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_Q_OVERFLOW;

/* @brief wtr/watcher/detail/adapter/linux/<a>/types
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
  // struct epoll_event event_list[event_max_count];
};

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns
   Functions:

     do_path_map_create
       -> path_map_type

     do_sys_resource_create
       -> sys_resource_type

     do_resource_release
       -> bool

     do_scan
       -> bool
*/

inline auto do_path_map_create(std::filesystem::path const& base_path,
                               int const watch_fd,
                               event::callback const& callback) noexcept
    -> path_map_type;
inline auto do_sys_resource_create(event::callback const& callback) noexcept
    -> sys_resource_type;
inline auto do_resource_release(int watch_fd, int event_fd,
                                event::callback const& callback) noexcept
    -> bool;
inline auto do_scan(int watch_fd, path_map_type& path_map,
                    event::callback const& callback) noexcept -> bool;

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_path_map_create
   If the path given is a directory
     - find all directories above the base path given.
     - ignore nonexistent directories.
     - return a map of watch descriptors -> directories.
   If `path` is a file
     - return it as the only value in a map.
     - the watch descriptor key should always be 1. */
inline auto do_path_map_create(std::filesystem::path const& base_path,
                               int const watch_fd,
                               event::callback const& callback) noexcept
    -> path_map_type
{
  using rdir_iterator = std::filesystem::recursive_directory_iterator;
  /* Follow symlinks, ignore paths which we don't have permissions for. */
  constexpr auto dir_opt
      = std::filesystem::directory_options::skip_permission_denied
        & std::filesystem::directory_options::follow_directory_symlink;

  constexpr auto path_map_reserve_count = 256;

  auto dir_ec = std::error_code{};
  path_map_type path_map;
  path_map.reserve(path_map_reserve_count);

  auto do_mark = [&](auto& dir) {
    int wd = inotify_add_watch(watch_fd, dir.c_str(), in_watch_opt);
    return wd < 0
        ? [&](){
            callback({"e/sys/inotify_add_watch",
                     event::what::other, event::kind::watcher});
            return false; }()
        : [&](){
            path_map[wd] = dir;
            return true;  }();
  };

  if (!do_mark(base_path))
    return path_map_type{};
  else if (std::filesystem::is_directory(base_path, dir_ec))
    /* @todo @note
       Should we bail from within this loop if `do_mark` fails? */
    for (auto const& dir : rdir_iterator(base_path, dir_opt, dir_ec))
      if (!dir_ec)
        if (std::filesystem::is_directory(dir, dir_ec))
          if (!dir_ec) do_mark(dir.path());
  return path_map;
};

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_sys_resource_create
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
        = epoll_create(event_max_count);
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

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_resource_release
   Close the file descriptors `watch_fd` and `event_fd`.
   Invoke `callback` on errors. */
inline auto do_resource_release(int watch_fd, int event_fd,
                                event::callback const& callback) noexcept
    -> bool
{
  auto const watch_fd_close_ok = close(watch_fd) == 0;
  auto const event_fd_close_ok = close(event_fd) == 0;
  if (!watch_fd_close_ok)
    callback(
        {"e/sys/close/watch_fd", event::what::other, event::kind::watcher});
  if (!event_fd_close_ok)
    callback(
        {"e/sys/close/event_fd", event::what::other, event::kind::watcher});
  return watch_fd_close_ok && event_fd_close_ok;
}

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_scan
   Reads through available (inotify) filesystem events.
   Discerns their path and type.
   Calls the callback.
   Returns false on eventful errors.

   @todo
   Return new directories when they appear,
   Consider running and returning `find_dirs` from here.
   Remove destroyed watches. */
inline auto do_scan(int watch_fd, path_map_type& path_map,
                    event::callback const& callback) noexcept -> bool
{
  alignas(struct inotify_event) char buf[scan_buf_len];

  enum class event_recv_status { eventful, eventless, error };

  auto const lift_event_recv = [](int fd, char* buf) {
    /* Read some events. */
    ssize_t len = read(fd, buf, scan_buf_len);

    /* EAGAIN means no events were found.
       We return `eventless` in that case. */
    if (len < 0 && errno != EAGAIN)
      return std::make_pair(event_recv_status::error, len);
    else if (len <= 0)
      return std::make_pair(event_recv_status::eventless, len);
    else
      return std::make_pair(event_recv_status::eventful, len);
  };

  /* Loop while events can be read from the inotify file descriptor. */
  while (true) {
    /* Read events */
    auto [status, len] = lift_event_recv(watch_fd, buf);
    /* Handle the errored, eventless and eventful reads */
    switch (status) {
      case event_recv_status::eventful:
        /* Loop over all events in the buffer. */
        const struct inotify_event* event_recv;
        for (char* ptr = buf; ptr < buf + len;
             ptr += sizeof(struct inotify_event) + event_recv->len)
        {
          event_recv = (const struct inotify_event*)ptr;

          /* @todo
             Consider using std::filesystem here. */
          auto const path_kind = event_recv->mask & IN_ISDIR
                                     ? event::kind::dir
                                     : event::kind::file;
          int path_wd = event_recv->wd;
          auto event_dir_path = path_map.find(path_wd)->second;
          auto event_base_name = std::filesystem::path(event_recv->name);
          auto event_path = event_dir_path / event_base_name;

          if (event_recv->mask & IN_Q_OVERFLOW) {
            callback({"e/self/overflow@" / event_dir_path, event::what::other,
                      event::kind::watcher});
          } else if (event_recv->mask & IN_CREATE) {
            callback({event_path, event::what::create, path_kind});
            if (path_kind == event::kind::dir) {
              int new_watch_fd = inotify_add_watch(watch_fd, event_path.c_str(),
                                                   in_watch_opt);
              path_map[new_watch_fd] = event_path;
            }
          } else if (event_recv->mask & IN_DELETE) {
            callback({event_path, event::what::destroy, path_kind});
            /* @todo rm watch, rm path map entry */
          } else if (event_recv->mask & IN_MOVE) {
            callback({event_path, event::what::rename, path_kind});
          } else if (event_recv->mask & IN_MODIFY) {
            callback({event_path, event::what::modify, path_kind});
          } else {
            callback({event_path, event::what::other, path_kind});
          }
        }
        /* We don't want to return here. We run until `eventless`. */
        break;
      case event_recv_status::error:
        callback({"e/sys/read", event::what::other, event::kind::watcher});
        return false;
        break;
      case event_recv_status::eventless: return true; break;
    }
  }
}

} /* namespace */

/*
  @brief watcher/detail/adapter/watch
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
       - delay
           In milliseconds, for epoll
       - system resources
           For inotify and epoll
       - event recieve list
           For receiving epoll events
       - path map
           For event to path lookups */

  static constexpr auto delay_ms = 16;

  struct sys_resource_type sr = do_sys_resource_create(callback);
  if (sr.valid) {
    auto path_map = do_path_map_create(path, sr.watch_fd, callback);
    if (path_map.size() > 0) {
      struct epoll_event event_recv_list[event_max_count];

      /* Do this work until dead:
          - Await filesystem events
          - Invoke `callback` on errors and events */

      while (is_living()) {
        int event_count
            = epoll_wait(sr.event_fd, event_recv_list, event_max_count, delay_ms);
        if (event_count < 0)
          return do_resource_release(sr.watch_fd, sr.event_fd, callback)
                 && do_error("e/sys/epoll_wait@" / path);
        else if (event_count > 0)
          for (int n = 0; n < event_count; n++)
            if (event_recv_list[n].data.fd == sr.watch_fd)
              if (is_living())
                if (!do_scan(sr.watch_fd, path_map, callback))
                  return do_resource_release(sr.watch_fd, sr.event_fd, callback)
                         && do_error("e/self/scan@" / path);
      }
      return do_resource_release(sr.watch_fd, sr.event_fd, callback);
    } else {
      return do_resource_release(sr.watch_fd, sr.event_fd, callback)
             && do_error("e/self/path_map@" / path);
    }
  } else {
    return do_resource_release(sr.watch_fd, sr.event_fd, callback)
           && do_error("e/self/sys_resource@" / path);
  }
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
          || defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
#endif /* if !defined(WATER_WATCHER_USE_WARTHOG) */
