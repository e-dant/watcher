#pragma once

/*
  @brief watcher/adapter/linux

  The Linux `inotify` adapter.
*/

#include <watcher/platform.hpp>
#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY)

#include <errno.h>
#include <string.h>  //strerror
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <vector>
#include <watcher/adapter/adapter.hpp>
#include <watcher/event.hpp>

namespace water {
namespace watcher {
namespace detail {
namespace adapter {
namespace { /* anonymous namespace for "private" variables */

/*
  Types
*/

using dir_opt_type = std::filesystem::directory_options;

/*
  Constants
*/

inline constexpr auto in_watch_opt =
    /* @todo measure perf of IN_ALL_EVENTS */
    IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_Q_OVERFLOW;
inline constexpr auto in_init_opt = IN_NONBLOCK;
inline constexpr dir_opt_type dir_opt =
    std::filesystem::directory_options::skip_permission_denied &
    std::filesystem::directory_options::follow_directory_symlink;

/*
  Functions
*/

/* Reads through available (inotify) filesystem events.
   Discerns their path and type.
   Calls the callback. */
inline void scan_directory(int fd,
                           int* wd,
                           auto const& callback) {
  /* 4096 is a typical page size. */
  static constexpr auto buf_len = 4096;
  alignas(struct inotify_event) char buf[buf_len];

  enum class read_event_status { eventful, eventless, error };

  auto const read_event = [](auto fd, auto buf) {
    /* Read some events. */
    ssize_t len = read(fd, buf, buf_len);

    /* EAGAIN means no events were found.
       We return `eventless` in this case. */
    if (len < 0 && errno != EAGAIN)
      return std::make_pair(read_event_status::error, len);
    else if (len <= 0)
      return std::make_pair(read_event_status::eventless, len);
    else
      return std::make_pair(read_event_status::eventful, len);
  };

  /* Loop while events can be read from inotify file descriptor. */
  while (true) {
    /* Read events */
    auto [status, len] = read_event(fd, buf);
    /* Handle the errored, eventless and eventful reads */
    switch (status) {
      case read_event_status::eventful:
        /* Loop over all events in the buffer. */
        const struct inotify_event* os_event;
        for (char* ptr = buf; ptr < buf + len;
             ptr += sizeof(struct inotify_event) + os_event->len) {
          os_event = (const struct inotify_event*)ptr;

          /* @todo use std::filesystem here. */
          auto const path_kind =
              os_event->mask & IN_ISDIR ? event::kind::dir : event::kind::file;

          if (os_event->mask & IN_Q_OVERFLOW)
            callback(event::event{"e:overflow", event::what::other,
                                  event::kind::watcher});
          else if (os_event->mask & IN_CREATE)
            callback(
                event::event{os_event->name, event::what::create, path_kind});
          else if (os_event->mask & IN_DELETE)
            callback(
                event::event{os_event->name, event::what::destroy, path_kind});
          else if (os_event->mask & IN_MOVE)
            callback(
                event::event{os_event->name, event::what::rename, path_kind});
          else if (os_event->mask & IN_MODIFY)
            callback(
                event::event{os_event->name, event::what::modify, path_kind});
          else
            callback(
                event::event{os_event->name, event::what::other, path_kind});
        }
        break;
      case read_event_status::error:
        callback(
            event::event{"e:read", event::what::other, event::kind::watcher});
        break;
      case read_event_status::eventless:
        break;
    }
  }
}

} /* namespace */

static bool is_living();  // NOLINT

/*
  @brief watcher/adapter/linux/watch

  @param callback
   A callback to perform when the files
   being watched change.

  Monitors `path` for changes.

  Invokes `callback` with an `event`
  when they happen.
*/
static bool watch(const char* base_path, event::callback const& callback) {
  /* If `path` is a directory, try to iterate through its contents.
     handle errors by ignoring nonexistent directories.
     If `path` is a file, return it as the only element in a vector. */
  using path_container_type = std::vector<std::string>;
  auto const&& find_dirs = [](char const* path) -> path_container_type {
    using rdir_iterator = std::filesystem::recursive_directory_iterator;
    auto dir_ec = std::error_code{};

    if (!std::filesystem::is_directory(path, dir_ec))
      return path_container_type{std::string{!dir_ec ? path : ""}};

    path_container_type paths;
    for (auto const& dir_it : rdir_iterator(path, dir_opt, dir_ec))
      if (!dir_ec)
        if (std::filesystem::is_directory(dir_it, dir_ec))
          if (!dir_ec)
            paths.emplace_back(dir_it.path());

    return paths;
  };

  const path_container_type path_container = find_dirs(base_path);

  for (auto const& path : path_container) {
    if (path.empty()) {
      callback(event::event{"e:path:empty", event::what::other,
                            event::kind::watcher});
      return false;
    }
  }

  /* Create the file descriptor for accessing the inotify API. */
  auto fd_watch = inotify_init1(in_init_opt);
  if (fd_watch < 0) {
    callback(event::event{"e:inotify:init", event::what::other,
                          event::kind::watcher});
    return false;
  }

  /* Memory for watch descriptors. */

  int* wd = (int*)calloc(path_container.size(), sizeof(int));
  if (wd == NULL) {
    callback(
        event::event{"e:calloc", event::what::other, event::kind::watcher});
    return false;
  }

  /* Mark directories for events */

  for (int i = 0; i < path_container.size(); i++) {
    wd[i] =
        inotify_add_watch(fd_watch, path_container.at(i).c_str(), in_watch_opt);
    if (wd[i] < 0)
      return false;
  }

  /* Epoll File descriptors to await receipt of inotify events. */

  struct epoll_event ev {
    .events = EPOLLIN, .data { .fd = fd_watch }
  };

  struct epoll_event events[path_container.size()];

  auto epfd = epoll_create1(EPOLL_CLOEXEC);

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd_watch, &ev) < 0) {
    callback(
        event::event{"e:epoll:ctl", event::what::other, event::kind::watcher});
    return false;
  }

  /* Await events */

  while (is_living()) {
    auto nfds = epoll_wait(epfd, events, 4096, 500);
    if (nfds == -1) {
      callback(event::event{"e:epoll:wait", event::what::other,
                            event::kind::watcher});
      return false;
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == fd_watch) {
        /* Read inotify events and queue them. */
        scan_directory(fd_watch, wd, callback);
      }
    }

    /* Or, with poll:
    auto const poll_num = poll(fds_poll, fds_poll_count, -1);
    if (poll_num == -1 && errno != EINTR) {
      std::cerr << "error: poll" << std::endl;
      return false;
    }

    // On a valid poll,
    if (poll_num > 0) {
      // with a new inotify event,
      if (fds_poll[0].revents & POLLIN) {
        // scan the directory.
        scan_directory(fd_watch, wd, callback);
      }
    }
    */
  }

  /* Close inotify file descriptor. */
  if (close(fd_watch) < 0) {
    callback(event::event{"e:close", event::what::other, event::kind::watcher});
    return false;
  }

  free(wd);

  return true;
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace water */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
