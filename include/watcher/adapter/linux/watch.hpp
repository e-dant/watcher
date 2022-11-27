#pragma once

/*
  @brief wtr/watcher/detail/adapter/linux

  The Linux `inotify` adapter.
*/

#include <watcher/platform.hpp>
#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY)

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <watcher/adapter/adapter.hpp>
#include <watcher/event.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {
/* @brief wtr/watcher/detail/adapter/linux/<a>
   Anonymous namespace for "private" things. */

/* @brief wtr/watcher/detail/adapter/linux/<a>/types
   Types:
     - string
     - dir_opt_type
     - path_container_type */
using std::string;
using dir_opt_type = std::filesystem::directory_options;
using path_container_type = std::unordered_map<int, std::string>;

/* @brief wtr/watcher/detail/adapter/linux/<a>/constants
   Constants:
     - event_max_count
     - in_init_opt
     - in_watch_opt
     - dir_opt */
inline constexpr auto event_max_count = 1;
inline constexpr auto path_container_reserve_count = 256;
inline constexpr auto in_init_opt = IN_NONBLOCK;
/* @todo
   Measure perf of IN_ALL_EVENTS */
/* @todo
   Handle move events properly.
     - Use IN_MOVED_TO
     - Use event::<something> */
inline constexpr auto in_watch_opt
    = IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_Q_OVERFLOW;
inline constexpr dir_opt_type dir_opt
    = std::filesystem::directory_options::skip_permission_denied
      & std::filesystem::directory_options::follow_directory_symlink;

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns
   Functions:

     do_path_container_create
       -> optional < path_container_type >

     do_event_resource_create
       -> optional < tuple < epoll_event, epoll_event*, int > >

     do_watch_fd_create
       -> optional < int >

     do_watch_fd_release
       -> bool

     do_event_wait_recv
       -> bool

     do_scan
       -> bool
*/

auto do_path_container_create(string const& base_path, int const watch_fd,
                              event::callback const& callback)
    -> std::optional<path_container_type>;
auto do_event_resource_create(int const watch_fd,
                              event::callback const& callback)
    -> std::optional<std::tuple<epoll_event, epoll_event*, int>>;
auto do_watch_fd_create(event::callback const& callback) -> std::optional<int>;
auto do_watch_fd_release(int watch_fd, event::callback const& callback) -> bool;
auto do_event_wait_recv(int watch_fd, int event_fd, epoll_event* event_list,
                        path_container_type& path_container, string const& path,
                        event::callback const& callback,
                        auto const& in_is_living) -> bool;
auto do_scan(int fd, path_container_type& path_container,
             event::callback const& callback) -> bool;

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_path_container_create
   If the path given is a directory
     - find all directories above the base path given.
     - ignore nonexistent directories.
     - return a map of watch descriptors -> directories.
   If `path` is a file
     - return it as the only value in a map.
     - the watch descriptor key should always be 1. */
auto do_path_container_create(string const& base_path, /* NOLINT */
                              int const watch_fd,
                              event::callback const& callback)
    -> std::optional<path_container_type>
{
  using rdir_iterator = std::filesystem::recursive_directory_iterator;

  auto dir_ec = std::error_code{};
  path_container_type path_map;
  path_map.reserve(path_container_reserve_count);

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
    return std::nullopt;
  else if (std::filesystem::is_directory(base_path, dir_ec))
    /* @todo @note
       Should we bail from within this loop if `do_mark` fails? */
    for (auto const& dir : rdir_iterator(base_path, dir_opt, dir_ec))
      if (!dir_ec)
        if (std::filesystem::is_directory(dir, dir_ec))
          if (!dir_ec) do_mark(dir.path());
  return path_map;
};

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_event_resource_create
   Return and initializes epoll events and file descriptors,
   which are the resources needed for an epoll_wait loop.
   Or return nothing. */
auto do_event_resource_create(int const watch_fd, /* NOLINT */
                              event::callback const& callback)
    -> std::optional<std::tuple<epoll_event, epoll_event*, int>>
{
  struct epoll_event event_conf
  {
    .events = EPOLLIN, .data
    {
      .fd = watch_fd
    }
  };
  struct epoll_event event_list[event_max_count];
  int event_fd = epoll_create1(EPOLL_CLOEXEC);

  if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) < 0) {
    callback({"e/sys/epoll_create1", event::what::other, event::kind::watcher});
    return std::nullopt;
  } else
    return std::make_tuple(event_conf, event_list, event_fd);
};

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_watch_fd_create
   Return an (optional) file descriptor
   which may access the inotify api. */
auto do_watch_fd_create(event::callback const& callback) /* NOLINT */
    -> std::optional<int>
{
  int watch_fd
#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY)
      = inotify_init1(in_init_opt);
#elif defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
      = inotify_init();
#endif

  if (watch_fd < 0) {
    callback(
        {"e/sys/inotify_init1?", event::what::other, event::kind::watcher});
    return std::nullopt;
  } else
    return watch_fd;
};

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_watch_fd_release
   Close the file descriptor `fd_watch`,
   Invoke `callback` on errors. */
auto do_watch_fd_release(int watch_fd, /* NOLINT */
                         event::callback const& callback) -> bool
{
  if (close(watch_fd) < 0) {
    callback({"e/sys/close", event::what::other, event::kind::watcher});
    return false;
  } else
    return true;
}

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_event_wait_recv
   - Await filesystem events.
   - When available, scan them.
   - Invoke the callback on errors. */
auto do_event_wait_recv(/* NOLINT */
                        int watch_fd,
                        int event_fd, /* Note the separate file descriptors for
                                         inotify and epoll */
                        epoll_event* event_list,
                        path_container_type& path_container,
                        string const& base_path,
                        event::callback const& callback,
                        auto const& in_is_living) -> bool
{
  auto const do_event_dispatch
      = [&watch_fd, &event_fd, &event_list, &path_container, &callback]() {
          /* The more time asleep, the better,
             as long as we don't sleep forever,
             because we may need to die. */
          int const event_count
              = epoll_wait(event_fd, event_list, event_max_count, delay_ms);

          if (event_count >= 0) {
            for (int n = 0; n < event_count; n++)
              if (event_list[n].data.fd == watch_fd)
                if (!do_scan(watch_fd, path_container, callback)) return false;
            /* We return true on eventless invocations. */
            return true;
          } else {
            return false;
          }
        };

  auto const do_event_error = [&callback]() {
    perror("epoll_wait");
    callback({"e/sys/epoll_wait", event::what::other, event::kind::watcher});
    /* We always return false on errors. */
    return false;
  };

  return
      /* If we are alive */
      in_is_living(base_path)
          /* Dispatch pending events to `do_scan` */
          ? do_event_dispatch()
                /* And keep running */
                ? do_event_wait_recv(watch_fd, event_fd, event_list,
                                     path_container, base_path, callback,
                                     in_is_living)
                /* Otherwise, send an error */
                : do_event_error()
          /* Death by natural causes */
          : true;

  /* if (is_ok) */
  /*   return */
  /*     do_event_wait_recv(watch_fd, event_fd, event_list, path_container, */
  /*                             base_path, callback, in_is_living); */
  /* else */
  /*   return false; */
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
auto do_scan(int watch_fd, /* NOLINT */
             path_container_type& path_container,
             event::callback const& callback) -> bool
{
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
          auto event_base_path = path_container.find(path_wd)->second;
          auto event_path = string(event_base_path + "/" + event_recv->name);

          if (event_recv->mask & IN_Q_OVERFLOW)
            callback(
                {"e/self/overflow", event::what::other, event::kind::watcher});
          else if (event_recv->mask & IN_CREATE)
            callback({event_path, event::what::create, path_kind});
          else if (event_recv->mask & IN_DELETE)
            callback({event_path, event::what::destroy, path_kind});
          else if (event_recv->mask & IN_MOVE)
            callback({event_path, event::what::rename, path_kind});
          else if (event_recv->mask & IN_MODIFY)
            callback({event_path, event::what::modify, path_kind});
          else
            callback({event_path, event::what::other, path_kind});
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

/* @brief wtr/watcher/detail/adapter/linux/fns/watch
   Monitors `base_path` for changes.
   Invokes `callback` with an `event` when they happen.

   @param base_path
   The path to watch for filesystem events.

   @param callback
   A callback to perform when the files
   being watched change. */
inline bool watch(auto const& path, event::callback const& callback,
                  auto const& in_is_living)
{
  /*
     Values
   */

  auto watch_fd_optional = do_watch_fd_create(callback);

  if (watch_fd_optional.has_value()) {
    auto watch_fd = watch_fd_optional.value();

    auto&& path_container_optional
        = do_path_container_create(path, watch_fd, callback);

    if (path_container_optional.has_value()) {
      /* Find all directories above the base path given.
         Make a map of watch descriptors -> paths. */
      path_container_type&& path_container
          = std::move(path_container_optional.value());

      auto&& event_tuple = do_event_resource_create(watch_fd, callback);

      if (event_tuple.has_value()) {
        /* auto&& event_conf = std::get<0>(event_tuple.value()); */
        auto event_list = std::get<1>(event_tuple.value());
        auto event_fd = std::get<2>(event_tuple.value());

        /*
           Work
         */

        /* Watch until dead. */
        do_event_wait_recv(watch_fd, event_fd, event_list, path_container, path,
                           callback, in_is_living);
        return do_watch_fd_release(watch_fd, callback);
      } else
        return do_watch_fd_release(watch_fd, callback);
    } else
      return do_watch_fd_release(watch_fd, callback);
  } else
    return false;
}

inline bool watch(char const* path, event::callback const& callback,
                  auto const& is_living)
{
  return watch(std::string(path), callback, is_living);
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
