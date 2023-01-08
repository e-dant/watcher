#pragma once

/*
  @brief wtr/watcher/<d>/adapter/linux/fanotify

  The Linux `fanotify` adapter.
*/

#include <watcher/detail/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_5_9_0) \
    && !defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

#define WATER_WATCHER_ADAPTER_LINUX_FANOTIFY

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/fanotify.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstring>
/* function */
#include <filesystem>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>
/* event
   callback */
#include <watcher/watcher.hpp>

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
inline constexpr auto event_buf_len
    = PATH_MAX;  // event_wait_queue_max * PATH_MAX;

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/types
   - sys_resource_type
       An object holding:
         - An fanotify file descriptor
         - An epoll file descriptor
         - An epoll configuration
         - A set of watch marks (as returned by fanotify_mark)
         - A map of (sub)path handles to filesystem paths (names)
         - A boolean representing the validity of these resources */
using path_mark_container_type = std::unordered_set<int>;
using dir_name_container_type
    = std::unordered_map<unsigned long, std::filesystem::path>;

struct sys_resource_type
{
  bool valid;
  int watch_fd;
  int event_fd;
  struct epoll_event event_conf;
  path_mark_container_type path_mark_container;
  dir_name_container_type dir_name_container;
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns

     do_sys_resource_create
       -> sys_resource_type

     do_sys_resource_destroy
       -> bool

     do_event_recv
       -> bool

     do_path_mark_add
       -> bool

     do_path_mark_remove
       -> bool

     do_error
       -> bool

     do_warning
       -> bool
*/

inline auto do_sys_resource_create(std::filesystem::path const&,
                                   event::callback const&) noexcept
    -> sys_resource_type;
inline auto do_sys_resource_destroy(sys_resource_type&,
                                    std::filesystem::path const&,
                                    event::callback const&) noexcept -> bool;
inline auto do_event_recv(sys_resource_type&, std::filesystem::path const&,
                          event::callback const&) noexcept -> bool;
inline auto do_path_mark_add(std::filesystem::path const&, int,
                             path_mark_container_type&) noexcept -> bool;
inline auto do_path_mark_remove(std::filesystem::path const&, int,
                                path_mark_container_type&) noexcept -> bool;
inline auto do_error(auto const&, auto const&, event::callback const&) noexcept
    -> bool;
inline auto do_warning(auto const&, auto const&,
                       event::callback const&) noexcept -> bool;

inline auto do_path_mark_add(std::filesystem::path const& full_path,
                             int watch_fd,
                             path_mark_container_type& pmc) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd, FAN_MARK_ADD,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                             | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD, full_path.c_str());
  if (wd >= 0)
    pmc.insert(wd);
  else
    return false;
  return true;
};

inline auto do_path_mark_remove(std::filesystem::path const& full_path,
                                int watch_fd,
                                path_mark_container_type& pmc) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd, FAN_MARK_REMOVE,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                             | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD, full_path.c_str());
  auto const at = pmc.find(wd);
  if (wd >= 0 && at != pmc.end())
    pmc.erase(at);
  else
    return false;
  return true;
};

inline auto do_error(auto const& error, auto const& path,
                     event::callback const& callback) noexcept -> bool
{
  auto msg = std::string(error)
                 .append("(")
                 .append(strerror(errno))
                 .append(")@")
                 .append(path);
  callback({msg, event::what::other, event::kind::watcher});
  return false;
};

inline auto do_warning(auto const& error, auto const& path,
                       event::callback const& callback) noexcept -> bool
{
  auto msg = std::string(error).append("@").append(path);
  callback({msg, event::what::other, event::kind::watcher});
  return true;
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_sys_resource_create
   Produces a `sys_resource_type` with the file descriptors from
   `fanotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto do_sys_resource_create(std::filesystem::path const& path,
                                   event::callback const& callback) noexcept
    -> sys_resource_type
{
  auto const do_error = [&callback](auto const& error, auto const& path,
                                    int watch_fd, int event_fd = -1) {
    auto msg = std::string(error)
                   .append("(")
                   .append(strerror(errno))
                   .append(")@")
                   .append(path);
    callback({msg, event::what::other, event::kind::watcher});
    return sys_resource_type{
        .valid = false,
        .watch_fd = watch_fd,
        .event_fd = event_fd,
        .event_conf = {.events = 0, .data = {.fd = watch_fd}},
        .path_mark_container = {},
        .dir_name_container = {},
    };
  };
  auto const do_path_map_container_create
      = [](int const watch_fd, std::filesystem::path const& watch_base_path,
           event::callback const& callback) -> path_mark_container_type {
    using rdir_iterator = std::filesystem::recursive_directory_iterator;

    /* Follow symlinks, ignore paths which we don't have permissions for. */
    static constexpr auto dir_opt
        = std::filesystem::directory_options::skip_permission_denied
          & std::filesystem::directory_options::follow_directory_symlink;

    static constexpr auto rsrv_count = 1024;

    /* auto dir_ec = std::error_code{}; */
    path_mark_container_type pmc;
    pmc.reserve(rsrv_count);

    if (!do_path_mark_add(watch_base_path, watch_fd, pmc))
      return pmc;

    else if (std::filesystem::is_directory(watch_base_path))
      try {
        for (auto const& dir : rdir_iterator(watch_base_path, dir_opt))
          if (std::filesystem::is_directory(dir))
            if (!do_path_mark_add(dir.path(), watch_fd, pmc))
              do_warning(
                  "w/sys/not_watched",
                  std::string(watch_base_path).append("@").append(dir.path()),
                  callback);
      } catch (...) {
        do_warning("w/sys/not_watched", watch_base_path, callback);
      }
    return pmc;
  };

  /* Init Flags:
       Post-event reporting, non-blocking IO and unlimited marks.
     Init Extra Flags:
       Read-only and close-on-exec.
     If we were making a filesystem auditor, we could use
     `FAN_CLASS_PRE_CONTENT|FAN_UNLIMITED_QUEUE|FAN_UNLIMITED_MARKS`. */
  /* clang-format off */
  int watch_fd
      = fanotify_init(FAN_CLASS_NOTIF
                      | FAN_REPORT_DFID_NAME
                      | FAN_UNLIMITED_QUEUE
                      | FAN_UNLIMITED_MARKS,
                      O_RDONLY
                      | O_NONBLOCK
                      | O_CLOEXEC);
  if (watch_fd >= 0) {
    auto pmc = do_path_map_container_create(watch_fd, path, callback);
    if (!pmc.empty())
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
          return sys_resource_type{
              .valid = true,
              .watch_fd = watch_fd,
              .event_fd = event_fd,
              .event_conf = event_conf,
              .path_mark_container = std::move(pmc),
              .dir_name_container = {},
          };
        } else {
          return do_error("e/sys/epoll_ctl", path, watch_fd, event_fd);
        }
      } else {
        return do_error("e/sys/epoll_create", path, watch_fd, event_fd);
      }
    } else {
      return do_error("e/sys/fanotify_mark", path, watch_fd);
    }
  } else {
    return do_error("e/sys/fanotify_init", path, watch_fd);
  }
}

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_sys_resource_destroy
   Close the resources in the system resource type `sr`.
   Invoke `callback` on errors. */
inline auto do_sys_resource_destroy(
    sys_resource_type& sr, std::filesystem::path const& watch_base_path,
    event::callback const& callback) noexcept -> bool
{
  auto const watch_fd_close_ok = close(sr.watch_fd) == 0;
  auto const event_fd_close_ok = close(sr.event_fd) == 0;
  if (!watch_fd_close_ok)
    do_error("e/sys/close/watch_fd", watch_base_path, callback);
  if (!event_fd_close_ok)
    do_error("e/sys/close/event_fd", watch_base_path, callback);
  return watch_fd_close_ok && event_fd_close_ok;
}

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/lift_event_path
   This lifts a path from our cache, in the system resource object `sr`, if
   available. Otherwise, it will try to read a directory and directory entry.
   Sometimes, such as on a directory being destroyed and the file not being
   able to be opened, the directory entry is the only event info. We can still
   get the rest of the path in those cases, however, by looking up the cached
   "upper" directory that event belongs to.
   Using the cache is helpful in other cases, too. This is an averaged bench of
   the three paths this function can go down:
     - Cache:            2427ns
     - Dir:              8905ns
     - Dir + Dir Entry:  31966ns */
inline auto lift_event_path(sys_resource_type& sr,
                            decltype(fanotify_event_metadata::mask) const& mask,
                            auto const& dfid_info,
                            std::filesystem::path const& watch_base_path,
                            event::callback const& callback) noexcept
    -> std::optional<std::filesystem::path>
{
  auto const& nip
      = [&]() -> std::optional<
                  std::pair<std::filesystem::path const, unsigned long const>> {
    /* The shenanigans we do here depend on this event being
       `FAN_EVENT_INFO_TYPE_DFID_NAME`. The kernel passes us
       some info about the directory and the directory entry
       (the filename) of this event that doesn't exist when
       other event types are reported. In particular, we need
       a file descriptor to the directory (which we use
       `readlink` on) and a character string representing the
       name of the directory entry.
       TLDR: We need information for the full path of the event,
       information which is only reported inside this `if`.

           [ The following is (mostly) quoting from the kernel ]
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

           [ end quote ]


       The kernel guarentees that there is a null-terminated
       character string to the event's directory entry
       after the file handle to the directory.
       Confusing, right?
    */

    auto const path_accum_append
        = [](auto& path_accum, auto const& dfid_info, auto const& dir_fh,
             auto const& dirname_len) -> void {
      char* name_info = (char*)(dfid_info + 1);
      char* filename
          = (char*)(name_info + sizeof(struct file_handle)
                    + sizeof(dir_fh->f_handle) + sizeof(dir_fh->handle_bytes)
                    + sizeof(dir_fh->handle_type));
      if (filename != nullptr && strcmp(filename, ".") != 0)
        snprintf(path_accum + dirname_len, sizeof(path_accum) - dirname_len,
                 "/%s", filename);
    };

    auto const path_accum_front = [](auto& path_accum, auto const& dfid_info,
                                     auto const& dir_fh) -> void {
      char* name_info = (char*)(dfid_info + 1);
      char* filename
          = (char*)(name_info + sizeof(struct file_handle)
                    + sizeof(dir_fh->f_handle) + sizeof(dir_fh->handle_bytes)
                    + sizeof(dir_fh->handle_type));
      if (filename != nullptr && strcmp(filename, ".") != 0)
        snprintf(path_accum, sizeof(path_accum), "/%s", filename);
    };

    auto const dir_fh = (struct file_handle*)dfid_info->handle;

    unsigned long dir_id{(unsigned long)(std::abs(dir_fh->handle_type))};

    for (unsigned i = 0; i < dir_fh->handle_bytes; i++)
      dir_id += dir_fh->f_handle[i];

    auto const dit = sr.dir_name_container.find(dir_id);

    if (dit != sr.dir_name_container.end()) {
      /* We already have a path name, use it */
      char path_accum[PATH_MAX];
      auto dirname = dit->second.c_str();
      auto dirname_len = dit->second.string().length();
      snprintf(path_accum, sizeof(path_accum), "%s", dirname);
      path_accum_append(path_accum, dfid_info, dir_fh, dirname_len);

      return std::make_pair(std::filesystem::path(path_accum), dir_id);

    } else {
      /* We can get a path name, so get that and use it */
      char path_accum[PATH_MAX];
      int fd = open_by_handle_at(AT_FDCWD, dir_fh,
                                 O_RDONLY | O_CLOEXEC | O_PATH | O_NONBLOCK);
      if (fd > 0) {
        char procpath[128];
        snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", fd);
        auto const dirname_len
            = readlink(procpath, path_accum, sizeof(path_accum) - sizeof('\0'));
        close(fd);

        if (dirname_len > 0) {
          path_accum[dirname_len] = '\0';
          /* Put the directory name in the path accumulator.
             Then, if a directory entry for this event exists
             And it's not the directory itself
             Put it in the path accumulator. */
          path_accum_append(path_accum, dfid_info, dir_fh, dirname_len);

          return std::make_pair(std::filesystem::path(path_accum), dir_id);
        }

        do_warning("w/sys/readlink", watch_base_path, callback);
        return std::nullopt;
      } else {
        path_accum_front(path_accum, dfid_info, dir_fh);

        return std::make_pair(std::filesystem::path(path_accum), dir_id);
      }
    }
  }();

  if (nip.has_value()) {
    auto& [full_path, dir_id] = nip.value();
    if (mask & FAN_ONDIR) {
      if (mask & FAN_CREATE) {
        auto const& at = sr.dir_name_container.find(dir_id);
        if (at == sr.dir_name_container.end()) {
          sr.dir_name_container.emplace(dir_id, full_path.parent_path());
        }
        do_path_mark_add(full_path, sr.watch_fd, sr.path_mark_container);
      } else if (mask & FAN_DELETE) {
        do_path_mark_remove(full_path, sr.watch_fd, sr.path_mark_container);
        auto const& at = sr.dir_name_container.find(dir_id);
        if (at != sr.dir_name_container.end()) sr.dir_name_container.erase(at);
      }
    }
    return full_path;
  }
  return std::nullopt;
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/event_send
   This is the heart of the adapter.
   Everything flows from here. */
inline auto event_send(decltype(fanotify_event_metadata::mask) const& mask,
                       std::optional<std::filesystem::path> const& path,
                       event::callback const& callback) noexcept -> void
{
  using namespace wtr::watcher::event;

  if (path.has_value()) {
    auto const& p = path.value();
    if (mask & FAN_ONDIR) {
      if (mask & FAN_CREATE)
        callback({p, what::create, kind::dir});
      else if (mask & FAN_MODIFY)
        callback({p, what::modify, kind::dir});
      else if (mask & FAN_DELETE)
        callback({p, what::destroy, kind::dir});
      else if (mask & FAN_MOVED_FROM)
        callback({p, what::rename, kind::dir});
      else if (mask & FAN_MOVED_TO)
        callback({p, what::rename, kind::dir});
      else
        callback({p, what::other, kind::dir});

    } else {
      if (mask & FAN_CREATE)
        callback({p, what::create, kind::file});
      else if (mask & FAN_MODIFY)
        callback({p, what::modify, kind::file});
      else if (mask & FAN_DELETE)
        callback({p, what::destroy, kind::file});
      else if (mask & FAN_MOVED_FROM)
        callback({p, what::rename, kind::file});
      else if (mask & FAN_MOVED_TO)
        callback({p, what::rename, kind::file});
      else
        callback({p, what::other, kind::file});
    }
  } else {
    do_warning("w/self/no_path", "", callback);
  }
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_event_recv
   Reads through available (fanotify) filesystem events.
   Discerns their path and type.
   Calls the callback.
   Returns false on eventful errors.
   @note
   The `metadata->fd` field contains either a file
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
inline auto do_event_recv(sys_resource_type& sr,
                          std::filesystem::path const& watch_base_path,
                          event::callback const& callback) noexcept -> bool
{
  /* Read some events. */
  alignas(struct fanotify_event_metadata) char event_buf[event_buf_len];
  auto event_read = read(sr.watch_fd, event_buf, sizeof(event_buf));

  /* Error */
  if (event_read < 0 && errno != EAGAIN)
    return do_error("e/sys/read", watch_base_path, callback);

  /* Eventful */
  else if (event_read > 0)
    /* Loop over everything in the event buffer. */
    for (auto* metadata = (struct fanotify_event_metadata const*)event_buf;
         FAN_EVENT_OK(metadata, event_read);
         metadata = FAN_EVENT_NEXT(metadata, event_read))
      if (metadata->fd == FAN_NOFD)
        if (metadata->vers == FANOTIFY_METADATA_VERSION)
          if (!(metadata->mask & FAN_Q_OVERFLOW))
            if (((fanotify_event_info_fid*)(metadata + 1))->hdr.info_type
                == FAN_EVENT_INFO_TYPE_DFID_NAME)

              /* This is the important part:
                 Send the events we recieve.
                 Everything before here here
                 is a layer of translation
                 between us and the kernel. */
              event_send(
                  metadata->mask,
                  lift_event_path(sr, metadata->mask,
                                  ((fanotify_event_info_fid*)(metadata + 1)),
                                  watch_base_path, callback),
                  callback);

            else
              return do_warning("w/self/event_info", watch_base_path, callback);
          else
            return do_error("e/sys/overflow", watch_base_path, callback);
        else
          return do_error("e/sys/kernel_version", watch_base_path, callback);
      else
        return do_error("e/sys/wrong_event_fd", watch_base_path, callback);

  /* Eventless */
  else
    return true;

  /* Value after looping */
  return true;
};

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
                  std::function<bool()> const& is_living) noexcept
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
        return do_sys_resource_destroy(sr, path, callback)
               && do_error("e/sys/epoll_wait", path, callback);
      else if (event_count > 0)
        for (int n = 0; n < event_count; n++)
          if (event_recv_list[n].data.fd == sr.watch_fd)
            if (is_living())
              if (!do_event_recv(sr, path, callback))
                return do_sys_resource_destroy(sr, path, callback)
                       && do_error("e/self/event_recv", path, callback);
    }
    return do_sys_resource_destroy(sr, path, callback);
  } else {
    return do_sys_resource_destroy(sr, path, callback)
           && do_error("e/self/sys_resource", path, callback);
  }
}

} /* namespace fanotify */
} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* !defined(WATER_WATCHER_USE_WARTHOG) */
#endif /* defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_5_9_0) \
          && !defined(WATER_WATCHER_PLATFORM_ANDROID_ANY) */
