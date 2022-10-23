#pragma once

/*
  @brief watcher/adapter/linux

  The Linux `inotify` adapter.
*/

#include <watcher/platform.hpp>
#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY)

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <vector>
#include <watcher/adapter/adapter.hpp>
#include <watcher/event.hpp>

namespace water {
namespace watcher {
namespace detail {
namespace adapter {
namespace {
/* @brief water/watcher/detail/adapter/linux/<anonymous>
   Anonymous namespace for "private" things. */

/* @brief water/watcher/detail/adapter/linux/<anonymous>/types
   Types:
     - dir_opt_type
     - path_container_type */
using dir_opt_type = std::filesystem::directory_options;
using path_container_type = std::vector<std::string>;

/* @brief water/watcher/detail/adapter/linux/<anonymous>/constants
   Constants:
     - event_max_count
     - in_init_opt
     - in_watch_opt
     - dir_opt */
inline constexpr auto event_max_count = 1;
inline constexpr auto in_init_opt = IN_NONBLOCK;
/* @todo
   Measure perf of IN_ALL_EVENTS */
/* @todo
   Handle move events properly.
     - Use IN_MOVED_TO
     - Use event::<something> */
inline constexpr auto in_watch_opt =
    IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_Q_OVERFLOW;
inline constexpr dir_opt_type dir_opt =
    std::filesystem::directory_options::skip_permission_denied &
    std::filesystem::directory_options::follow_directory_symlink;

/* @brief water/watcher/detail/adapter/linux/<anonymous>/functions
   Functions:
     - scan */

/* @brief water/watcher/detail/adapter/linux/<anonymous>/functions/scan
   Reads through available (inotify) filesystem events.
   Discerns their path and type.
   Calls the callback.

   @todo
   Return new directories when they appear,
   Consider running and returning `find_dirs` from here.
   Remove destroyed watches. */
inline void scan(int fd, int* wd_memory, auto const& callback) {
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

  /* Loop while events can be read from the inotify file descriptor. */
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

          /* @todo
             Consider using std::filesystem here. */
          auto const path_kind =
              os_event->mask & IN_ISDIR ? event::kind::dir : event::kind::file;

          if (os_event->mask & IN_Q_OVERFLOW)
            callback(event::event{"e/self/overflow", event::what::other,
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
        callback(event::event{"e/sys/read", event::what::other,
                              event::kind::watcher});
        return;
        break;
      case read_event_status::eventless:
        return;
        break;
    }
  }
}

} /* namespace */

/* @note water/watcher/detail/adapter/is_living
   This symbol is defined in watcher/adapter/adapter.hpp
   But clangd has a tough time finding it while editing. */
static bool is_living(); /* NOLINT */

/* @brief water/watcher/detail/adapter/linux/watch
   Monitors `base_path` for changes.
   Invokes `callback` with an `event` when they happen.

   @param base_path
   The path to watch for filesystem events.

   @param callback
   A callback to perform when the files
   being watched change. */
static bool watch(const char* base_path, event::callback const& callback) {
  /* If `path` is a directory, try to iterate through its contents.
     handle errors by ignoring nonexistent directories.
     If `path` is a file, return it as the only element in a vector. */
  auto const&& do_find_dirs = [](char const* path) -> path_container_type {
    using rdir_iterator = std::filesystem::recursive_directory_iterator;
    auto dir_ec = std::error_code{};

    if (std::filesystem::is_directory(path, dir_ec)) {
      path_container_type paths;
      paths.reserve(256);
      paths.emplace_back(path);
      for (auto const& dir_it : rdir_iterator(path, dir_opt, dir_ec))
        if (!dir_ec)
          if (std::filesystem::is_directory(dir_it, dir_ec))
            if (!dir_ec)
              paths.emplace_back(dir_it.path());
      return paths;
    } else {
      return path_container_type{std::string{!dir_ec ? path : ""}};
    }
  };

  /* Check that all paths in `path_container` are valid. */
  auto const do_path_container_check_valid =
      [](const path_container_type& path_container,
         const event::callback& callback) {
        for (auto const& path : path_container)
          if (path.empty()) {
            callback(event::event{"e/self/path/empty", event::what::other,
                                  event::kind::watcher});
            return false;
          }
        return true;
      };

  /* Return and initializes epoll events and file descriptors.
     Return the necessary resources for an epoll_wait loop.
     Or return nothing. */
  auto const&& event_epoll_init = [](int const fd_watch,
                                     event::callback const& callback)
      -> std::optional<std::tuple<epoll_event, epoll_event*, int>> {
    struct epoll_event ev {
      .events = EPOLLIN, .data { .fd = fd_watch }
    };
    struct epoll_event events[event_max_count];
    auto fd_event_poll = epoll_create1(EPOLL_CLOEXEC);

    if (epoll_ctl(fd_event_poll, EPOLL_CTL_ADD, fd_watch, &ev) < 0) {
      callback(event::event{"e/sys/epoll_ctl", event::what::other,
                            event::kind::watcher});
      return std::nullopt;
    }
    return std::move(std::make_tuple(ev, events, fd_event_poll));
  };

  /* Close the file descriptor `fd_watch`,
     Free the memory used by `wd_memory`,
     Invokes `callback` if an error happened. */
  const auto do_release_resource = [](int fd_watch, int* wd_memory,
                                      const event::callback& callback) {
    if (close(fd_watch) < 0) {
      callback(
          event::event{"e/sys/close", event::what::other, event::kind::watcher});
      return false;
    }
    free(wd_memory);
    return true;
  };

  /* Find all directories above the base path given. */
  const path_container_type path_container = do_find_dirs(base_path);

  /* Inotify API file descriptor. */
  auto fd_watch = inotify_init1(in_init_opt);
  if (fd_watch < 0) {
    callback(event::event{"e/sys/inotify_init1", event::what::other,
                          event::kind::watcher});
    return false;
  }

  /* Memory for watch descriptors. */
  int* wd = (int*)calloc(path_container.size(), sizeof(int));
  if (wd == nullptr) {
    callback(
        event::event{"e/sys/calloc", event::what::other, event::kind::watcher});
    return false;
  }

  /* Mark directories for events. */
  /* @todo put this in `find_dirs` */
  for (int i = 0; i < path_container.size(); i++) {
    wd[i] =
        inotify_add_watch(fd_watch, path_container.at(i).c_str(), in_watch_opt);
    if (wd[i] < 0)
      return false;
  }

  auto event_tuple = event_epoll_init(fd_watch, callback);

  if (event_tuple.has_value()) {
    auto event_conf = std::get<0>(event_tuple.value());
    auto event_list = std::get<1>(event_tuple.value());
    auto fd_event = std::get<2>(event_tuple.value());

    /* Await filesystem events.
       When available, scan them.
       Invoke the callback on errors. */
    auto const do_event_loop = [&]() {
      auto const event_count =
          epoll_wait(fd_event, event_list, event_max_count, delay_ms);
      auto const do_event_dispatch = [&]() {
        for (int n = 0; n < event_count; ++n)
          if (event_list[n].data.fd == fd_watch)
            scan(fd_watch, wd, callback);
        return true;
      };

      auto const do_event_error = [](const event::callback& callback) {
        callback(event::event{"e/sys/epoll_wait", event::what::other,
                              event::kind::watcher});
        return false;
      };

      return event_count < 0 ? do_event_error(callback) : do_event_dispatch();
    };

    /* Loop until dead. */
    while (is_living() && do_event_loop()) {
    }

    return do_release_resource(fd_watch, wd, callback);
  }

  /* Catch-all. We only get here if `is_living()` starts as false. */
  return do_release_resource(fd_watch, wd, callback);
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace water */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
