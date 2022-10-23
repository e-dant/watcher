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
     - do_scan */

/* @brief water/watcher/detail/adapter/linux/<anonymous>/functions/do_scan
   Reads through available (inotify) filesystem events.
   Discerns their path and type.
   Calls the callback.
   Returns false on eventful errors.

   @todo
   Return new directories when they appear,
   Consider running and returning `find_dirs` from here.
   Remove destroyed watches. */
inline bool do_scan(int fd, event::callback const& callback) {
  /* 4096 is a typical page size. */
  static constexpr auto buf_len = 4096;
  alignas(struct inotify_event) char buf[buf_len];

  enum class event_recv_status { eventful, eventless, error };

  auto const lift_event_recv = [](int fd, char* buf) {
    /* Read some events. */
    ssize_t len = read(fd, buf, buf_len);

    /* EAGAIN means no events were found.
       We return `eventless` in this case. */
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
    auto [status, len] = lift_event_recv(fd, buf);
    /* Handle the errored, eventless and eventful reads */
    switch (status) {
      case event_recv_status::eventful:
        /* Loop over all events in the buffer. */
        const struct inotify_event* event_recv;
        for (char* ptr = buf; ptr < buf + len;
             ptr += sizeof(struct inotify_event) + event_recv->len) {
          event_recv = (const struct inotify_event*)ptr;

          /* @todo
             Consider using std::filesystem here. */
          auto const path_kind = event_recv->mask & IN_ISDIR
                                     ? event::kind::dir
                                     : event::kind::file;

          if (event_recv->mask & IN_Q_OVERFLOW)
            callback(event::event{"e/self/overflow", event::what::other,
                                  event::kind::watcher});
          else if (event_recv->mask & IN_CREATE)
            callback(
                event::event{event_recv->name, event::what::create, path_kind});
          else if (event_recv->mask & IN_DELETE)
            callback(event::event{event_recv->name, event::what::destroy,
                                  path_kind});
          else if (event_recv->mask & IN_MOVE)
            callback(
                event::event{event_recv->name, event::what::rename, path_kind});
          else if (event_recv->mask & IN_MODIFY)
            callback(
                event::event{event_recv->name, event::what::modify, path_kind});
          else
            callback(
                event::event{event_recv->name, event::what::other, path_kind});
        }
        break;
      case event_recv_status::error:
        callback(event::event{"e/sys/read", event::what::other,
                              event::kind::watcher});
        return false;
        break;
      case event_recv_status::eventless:
        return true;
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
         event::callback const& callback) {
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
  auto const&& event_create_resource = [](int const watch_fd,
                                          event::callback const& callback)
      -> std::optional<std::tuple<epoll_event, epoll_event*, int>> {
    struct epoll_event event_conf {
      .events = EPOLLIN, .data { .fd = watch_fd }
    };
    struct epoll_event event_list[event_max_count];
    auto const event_fd = epoll_create1(EPOLL_CLOEXEC);

    if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) < 0) {
      callback(event::event{"e/sys/epoll_ctl", event::what::other,
                            event::kind::watcher});
      return std::nullopt;
    }
    return std::move(std::make_tuple(event_conf, event_list, event_fd));
  };

  /* Close the file descriptor `fd_watch`,
     Invoke `callback` on errors. */
  auto const do_release_resource = [](int watch_fd,
                                      event::callback const& callback) {
    if (close(watch_fd) < 0) {
      callback(event::event{"e/sys/close", event::what::other,
                            event::kind::watcher});
      return false;
    }
    return true;
  };

  /* Find all directories above the base path given. */
  path_container_type const&& path_container = do_find_dirs(base_path);

  /* Inotify API file descriptor. */
  auto watch_fd = inotify_init1(in_init_opt);
  if (watch_fd < 0) {
    callback(event::event{"e/sys/inotify_init1", event::what::other,
                          event::kind::watcher});
    return false;
  }

  /* Create memory for watch descriptors.
     Mark directories for event notifications. */
  auto const&& do_watch_wd_container_create =
      [](int const watch_fd, path_container_type const& path_container,
         event::callback const& callback) -> std::optional<int*> {
    /* Create memory for watch descriptors. */
    int* watch_wd_container = (int*)calloc(path_container.size(), sizeof(int));
    if (watch_wd_container == nullptr) {
      callback(event::event{"e/sys/calloc", event::what::other,
                            event::kind::watcher});
      return std::nullopt;
    }

    /* Mark directories for event notifications. */
    for (int i = 0; i < path_container.size(); i++) {
      watch_wd_container[i] = inotify_add_watch(
          watch_fd, path_container.at(i).c_str(), in_watch_opt);
      if (watch_wd_container[i] < 0) {
        callback(event::event{"e/sys/inotify_add_watch", event::what::other,
                              event::kind::watcher});
        free(watch_wd_container);
        return std::nullopt;
      }
    }

    return std::move(watch_wd_container);
  };

  auto const&& watch_wd_container_optional =
      do_watch_wd_container_create(watch_fd, path_container, callback);

  if (watch_wd_container_optional.has_value()) {
    auto&& watch_wd_container = watch_wd_container_optional.value();
    auto&& event_tuple = event_create_resource(watch_fd, callback);

    if (event_tuple.has_value()) {
      /* auto event_conf = std::get<0>(event_tuple.value()); */
      auto&& event_list = std::get<1>(event_tuple.value());
      auto&& event_fd = std::get<2>(event_tuple.value());

      /* Await filesystem events.
         When available, scan them.
         Invoke the callback on errors. */
      auto const do_event_recv = [](int event_fd, auto& event_list,
                                    int watch_fd,
                                    event::callback const& callback) {
        int const event_count =
            epoll_wait(event_fd, event_list, event_max_count, delay_ms);

        auto const do_event_dispatch = [&]() {
          for (int n = 0; n < event_count; ++n)
            if (event_list[n].data.fd == watch_fd)
              return do_scan(watch_fd, callback);
          /* We return true on eventless invocations. */
          return true;
        };

        auto const do_event_error = [&]() {
          callback(event::event{"e/sys/epoll_wait", event::what::other,
                                event::kind::watcher});
          /* We always return false here so that
             it trickles through the call stack. */
          return false;
        };

        return event_count < 0 ? do_event_error() : do_event_dispatch();
      };

      /* Loop until dead. */
      while (is_living() &&
             do_event_recv(event_fd, event_list, watch_fd, callback))
        continue;

      free(watch_wd_container);
      return do_release_resource(watch_fd, callback);
    }
  }

  /* Catch-all. We only get here if `is_living()` starts as false. */
  return do_release_resource(watch_fd, callback);
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace water */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
