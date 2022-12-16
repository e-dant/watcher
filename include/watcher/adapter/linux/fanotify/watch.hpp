#pragma once

/*
  @brief wtr/watcher/<d>/adapter/linux/fanotify

  The Linux `fanotify` adapter.
*/

#include <climits>
#include <cstdio>
#include <cstring>
#include <watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
    || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

#include <fcntl.h>
#include <linux/fanotify.h>
#include <sys/epoll.h>
#include <sys/fanotify.h>
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
namespace fanotify {

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>
   Anonymous namespace for "private" things. */
namespace {

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/constants
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
       That's a good thumb-rule, and it might be the best
       value to use because there will be a possibly long
       character string (for the filename) in the event.
       We can infer some things about the size we need for
       the event buffer, but it's unlikely to be meaningful
       because of the variably sized character string being
       reported. We could use something like:
          event_buf_len
            = ((event_wait_queue_max + PATH_MAX)
            * (3 * sizeof(struct fanotify_event_metadata)));
       But that's a lot of flourish for 72 bytes that won't
       be meaningful. */
inline constexpr auto delay_ms = 16;
inline constexpr auto event_wait_queue_max = 1;
inline constexpr auto event_buf_len = event_wait_queue_max * PATH_MAX;

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/types
   - sys_resource_type
       An object representing an fanotify file descriptor,
       an epoll file descriptor, an epoll configuration,
       and whether or not these resources are valid. */
struct sys_resource_type
{
  bool valid;
  int watch_fd;
  int event_fd;
  struct epoll_event event_conf;
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns
   Functions:

     do_sys_resource_create
       -> sys_resource_type

     do_resource_release
       -> bool

     do_event_recv
       -> bool

     do_error
       -> bool
*/

inline auto do_sys_resource_create(std::filesystem::path const& path,
                                   event::callback const& callback) noexcept
    -> sys_resource_type;
inline auto do_resource_release(int watch_fd, int event_fd,
                                event::callback const& callback) noexcept
    -> bool;
inline auto do_event_recv(int watch_fd,
                          event::callback const& callback) noexcept -> bool;
inline auto do_error(auto const& msg, event::callback const& callback) noexcept
    -> bool
{
  callback({msg, event::what::other, event::kind::watcher});
  return false;
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_sys_resource_create
   Produces a `sys_resource_type` with the file descriptors from
   `fanotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto do_sys_resource_create(std::filesystem::path const& path,
                                   event::callback const& callback) noexcept
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
  auto const do_gather_path_marks
      = [](int const watch_fd, std::filesystem::path const& base_path,
           event::callback const& callback) -> std::vector<int> {
    using rdir_iterator = std::filesystem::recursive_directory_iterator;
    /* Follow symlinks, ignore paths which we don't have permissions for. */
    static constexpr auto dir_opt
        = std::filesystem::directory_options::skip_permission_denied
          & std::filesystem::directory_options::follow_directory_symlink;

    static constexpr auto marks_reserve_count = 256;

    auto dir_ec = std::error_code{};
    std::vector<int> marks;
    marks.reserve(marks_reserve_count);

    auto do_mark = [&](auto& dir) {
      int wd = fanotify_mark(watch_fd, FAN_MARK_ADD,
                             FAN_ONDIR
                                 /* | FAN_EVENT_ON_CHILD */
                                 | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                                 | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                             AT_FDCWD, dir.c_str());
      if (wd >= 0)
        marks.emplace_back(wd);
      else
        return false;
      return true;
    };

    if (!do_mark(base_path))
      return marks;
    else if (std::filesystem::is_directory(base_path, dir_ec))
      /* @todo @note
         Should we bail from within this loop if `do_mark` fails? */
      for (auto const& dir : rdir_iterator(base_path, dir_opt, dir_ec))
        if (!dir_ec)
          if (std::filesystem::is_directory(dir, dir_ec))
            if (!dir_ec)
              if (!do_mark(dir.path()))
                callback({"w/sys/path_unwatched@" / dir.path(),
                          event::what::other, event::kind::watcher});
    return marks;
  };

  /* Init Flags:
       Post-event reporting, non-blocking IO, and close-on-exec.
     Init Extra Flags:
       Read-only, large files, and close-on-exec.
     If we were making a filesystem auditor, we could use
     `FAN_CLASS_PRE_CONTENT|FAN_UNLIMITED_QUEUE|FAN_UNLIMITED_MARKS`. */
  /* clang-format off */
  int watch_fd
      = fanotify_init(FAN_CLASS_NOTIF
                      | FAN_REPORT_DFID_NAME,
                      O_RDONLY
                      | O_NONBLOCK
                      | O_CLOEXEC);
  if (watch_fd >= 0) {
    auto marks = do_gather_path_marks(watch_fd, path, callback);
    if (!marks.empty())
    {
      /* clang-format on */
      struct epoll_event event_conf
      {
        .events = EPOLLIN, .data { .fd = watch_fd }
      };

      int event_fd = epoll_create1(EPOLL_CLOEXEC);

      /* @note We could make the epoll and fanotify file descriptors
         non-blocking with `fcntl`. It's not clear if we can do this
         from their `*_init` calls. */
      /* fcntl(watch_fd, F_SETFL, O_NONBLOCK); */
      /* fcntl(event_fd, F_SETFL, O_NONBLOCK); */

      if (event_fd >= 0) {
        if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) >= 0) {
          return sys_resource_type{.valid = true,
                                   .watch_fd = watch_fd,
                                   .event_fd = event_fd,
                                   .event_conf = event_conf};
        } else {
          return do_error("e/sys/epoll_ctl", watch_fd, event_fd);
        }
      } else {
        return do_error("e/sys/epoll_create", watch_fd, event_fd);
      }
    } else {
      return do_error("e/sys/fanotify_mark", watch_fd);
    }
  } else {
    return do_error("e/sys/fanotify_init", watch_fd);
  }
}

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_resource_release
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

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_event_recv
   Reads through available (fanotify) filesystem events.
   Discerns their path and type.
   Calls the callback.
   Returns false on eventful errors.
*/
inline auto do_event_recv(int watch_fd,
                          event::callback const& callback) noexcept -> bool
{
  /* Read some events. */
  alignas(struct fanotify_event_metadata) char event_buf[event_buf_len];
  auto event_read = read(watch_fd, event_buf, sizeof(event_buf));

  /* Error */
  if (event_read < 0 && errno != EAGAIN)
    return do_error("e/sys/read", callback);
  /* Eventless */
  else if (event_read == 0)
    return true;
  /* Eventful */
  else {
    if (event_read == event_buf_len)
      callback(
          {"w/self/near_overflow", event::what::other, event::kind::watcher});

    /* Loop over everything in the event buffer. */
    for (struct fanotify_event_metadata* metadata
         = (struct fanotify_event_metadata*)event_buf;
         FAN_EVENT_OK(metadata, event_read);
         metadata = FAN_EVENT_NEXT(metadata, event_read))
    {
      struct fanotify_event_info_fid* dfid_info
          = (struct fanotify_event_info_fid*)(metadata + 1);

      /* The `metadata->fd` field contains either a file
         descriptor or the value `FAN_NOFD`. File descriptors
         are always greater than 0. `FAN_NOFD` represents an
         event queue overflow for `fanotify` listeners which
         are _not_ monitoring file handles, such as mount
         monitors. The file handle is in the metadata when an
         `fanotify` listener is monitoring events by their
         file handles.
         The `metadata->vers` field may differ between kernel
         versions, so we check it against what we have been
         compiled with. */
      if (metadata->fd == FAN_NOFD) {
        if (metadata->vers == FANOTIFY_METADATA_VERSION) {
          if (!(metadata->mask & FAN_Q_OVERFLOW)) {
            if (dfid_info->hdr.info_type == FAN_EVENT_INFO_TYPE_DFID_NAME) {
              /* The shenanigans we do here depend on this event being
                 `FAN_EVENT_INFO_TYPE_DFID_NAME`. The kernel passes us
                 some info about the directory and the directory entry
                 (the filename) of this event that doesn't exist when
                 other event types are reported. In particular, we need
                 a file descriptor to the directory (which we use
                 `readlink` on) and a character string representing the
                 name of the directory entry.
                 TLDR: We need information for the full path of the event,
                 information which is only reported inside this `if`. */
              int fd = open_by_handle_at(
                  AT_FDCWD, (struct file_handle*)dfid_info->handle,
                  O_RDONLY | O_CLOEXEC | O_PATH | O_NONBLOCK);
              if (fd > 0) {
                char path_accumulator[PATH_MAX];
                char procpath[128];
                snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", fd);
                auto const dirname_len
                    = readlink(procpath, path_accumulator,
                               sizeof(path_accumulator) - sizeof('\0'));
                close(fd);
                if (dirname_len > 0) {
                  path_accumulator[dirname_len] = '\0';

                  /* [ The following is (mostly) quoting from the kernel ]
                     Variable size struct for
                     dir file handle + child file handle + name

                     [ Omitting definition of `fanotify_info` here ]

                     (struct fanotify_fh) dir_fh starts at
                       buf[0]

                     (optional) dir2_fh starts at
                       buf[dir_fh_totlen]

                     (optional) file_fh starts at
                       buf[dir_fh_totlen + dir2_fh_totlen]

                     name starts at
                       buf[dir_fh_totlen + dir2_fh_totlen + file_fh_totlen]
                     ...

                     [ end quote ] */

                  char* name_info = (char*)(dfid_info + 1);
                  struct file_handle* dfid_fh
                      = (struct file_handle*)dfid_info->handle;
                  /* The kernel guarentees that there is a null-terminated
                     character string to the event's directory entry
                     after the file handle to the directory.
                     Confusing, right? */
                  char* filename
                      = (char*)(name_info + sizeof(struct file_handle)
                                + sizeof(dfid_fh->f_handle)
                                + sizeof(dfid_fh->handle_bytes)
                                + sizeof(dfid_fh->handle_type));
                  if (/* If a directory entry for this event exists */
                      filename != nullptr
                      /* And it's not the directory itself */
                      && strcmp(filename, ".") != 0)
                    /* Put it in the path accumulator */
                    snprintf(path_accumulator + dirname_len,
                             sizeof(path_accumulator) - dirname_len, "/%s",
                             filename);

                  if (metadata->mask & FAN_ONDIR) {
                    if (metadata->mask & FAN_CREATE)
                      callback({path_accumulator, event::what::create,
                                event::kind::dir});
                    else if (metadata->mask & FAN_MODIFY)
                      callback({path_accumulator, event::what::modify,
                                event::kind::dir});
                    else if (metadata->mask & FAN_DELETE)
                      callback({path_accumulator, event::what::destroy,
                                event::kind::dir});
                    else if (metadata->mask & FAN_MOVED_FROM)
                      callback({path_accumulator, event::what::rename,
                                event::kind::dir});
                    else if (metadata->mask & FAN_MOVED_TO)
                      callback({path_accumulator, event::what::rename,
                                event::kind::dir});
                    else if (metadata->mask & FAN_DELETE_SELF)
                      callback({path_accumulator, event::what::destroy,
                                event::kind::dir});
                    else if (metadata->mask & FAN_MOVE_SELF)
                      callback({path_accumulator, event::what::rename,
                                event::kind::dir});
                    else
                      callback({path_accumulator, event::what::other,
                                event::kind::dir});
                  } else {
                    if (metadata->mask & FAN_CREATE)
                      callback({path_accumulator, event::what::create,
                                event::kind::file});
                    else if (metadata->mask & FAN_MODIFY)
                      callback({path_accumulator, event::what::modify,
                                event::kind::file});
                    else if (metadata->mask & FAN_DELETE)
                      callback({path_accumulator, event::what::destroy,
                                event::kind::file});
                    else if (metadata->mask & FAN_MOVED_FROM)
                      callback({path_accumulator, event::what::rename,
                                event::kind::file});
                    else if (metadata->mask & FAN_MOVED_TO)
                      callback({path_accumulator, event::what::rename,
                                event::kind::file});
                    else if (metadata->mask & FAN_DELETE_SELF)
                      callback({path_accumulator, event::what::destroy,
                                event::kind::file});
                    else if (metadata->mask & FAN_MOVE_SELF)
                      callback({path_accumulator, event::what::rename,
                                event::kind::file});
                    else
                      callback({path_accumulator, event::what::other,
                                event::kind::file});
                  }
                } else {
                  return do_error("e/sys/readlink", callback);
                }
              } else {
                return do_error("e/sys/open", callback);
              }
            } else {
              callback({"w/self/strange_event", event::what::other,
                        event::kind::watcher});
            }
          } else {
            return do_error("e/sys/overflow", callback);
          }
        } else {
          return do_error("e/sys/kernel_version", callback);
        }
      } else {
        return do_error("e/sys/fanotify_strange_fd", callback);
      }
    }
  }
  return true;
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
  /* Gather these resources:
       - system resources
           For fanotify and epoll
       - event recieve list
           For receiving epoll events */

  struct sys_resource_type sr = do_sys_resource_create(path, callback);

  if (sr.valid) {
    struct epoll_event event_recv_list[event_wait_queue_max];

    /* Do this work until dead:
        - Await filesystem events
        - Invoke `callback` on errors and events */

    while (is_living()) {
      int event_count = epoll_wait(sr.event_fd, event_recv_list,
                                   event_wait_queue_max, delay_ms);
      if (event_count < 0)
        return do_resource_release(sr.watch_fd, sr.event_fd, callback)
               && do_error("e/sys/epoll_wait@" / path, callback);
      else if (event_count > 0)
        for (int n = 0; n < event_count; n++)
          if (event_recv_list[n].data.fd == sr.watch_fd)
            if (is_living())
              if (!do_event_recv(sr.watch_fd, callback))
                return do_resource_release(sr.watch_fd, sr.event_fd, callback)
                       && do_error("e/self/event_recv@" / path, callback);
    }
    return do_resource_release(sr.watch_fd, sr.event_fd, callback);
  } else {
    return do_resource_release(sr.watch_fd, sr.event_fd, callback)
           && do_error("e/self/sys_resource@" / path, callback);
  }
}

} /* namespace fanotify */
} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
          || defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
#endif /* if !defined(WATER_WATCHER_USE_WARTHOG) */
