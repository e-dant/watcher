#pragma once

/*  @brief wtr/watcher/<d>/adapter/linux/fanotify
    The Linux `fanotify` adapter. */

/*  WATER_WATCHER_PLATFORM_* */
#include <detail/wtr/watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_5_9_0) \
  && ! defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if ! defined(WATER_WATCHER_USE_WARTHOG)

#define WATER_WATCHER_ADAPTER_LINUX_FANOTIFY

/*  O_* */
#include <fcntl.h>
/*  EPOLL*
    epoll_ctl
    epoll_wait
    epoll_event
    epoll_create1 */
#include <sys/epoll.h>
/*  FAN_*
    fanotify_mark
    fanotify_init
    fanotify_event_metadata */
#include <sys/fanotify.h>
/*  open
    close
    readlink */
#include <unistd.h>
/*  errno */
#include <cerrno>
/*  PATH_MAX */
#include <climits>
/*  snprintf */
#include <cstdio>
/*  strerror */
#include <cstring>
/*  path
    is_directory
    directory_options
    recursive_directory_iterator */
#include <filesystem>
/*  function */
#include <functional>
/*  optional */
#include <optional>
/*  error_code */
#include <system_error>
/*  unordered_map */
#include <unordered_map>
/*  unordered_set */
#include <unordered_set>
/*  tuple
    make_tuple */
#include <tuple>
/*  event
    callback */
#include <wtr/watcher.hpp>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace fanotify {

/*  @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/constants
    - delay
      The delay, in milliseconds, while `epoll_wait` will
      'sleep' for until we are woken up. We usually check
      if we're still alive at that point.

    - event_wait_queue_max
      Number of events allowed to be given to recv
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
            * (3 * sizeof(fanotify_event_metadata)));
      But that's a lot of flourish for 72 bytes that won't
      be meaningful.

    - fan_init_flags:
      Post-event reporting, non-blocking IO and unlimited
      marks. We need sudo mode for the unlimited marks.
      If we were making a filesystem auditor, we might use:
          FAN_CLASS_PRE_CONTENT
          | FAN_UNLIMITED_QUEUE
          | FAN_UNLIMITED_MARKS

    - fan_init_opt_flags:
      Read-only, non-blocking, and close-on-exec. */
inline constexpr auto delay_ms = 16;
inline constexpr auto event_wait_queue_max = 1;
inline constexpr auto event_buf_len = PATH_MAX;
inline constexpr auto fan_init_flags = FAN_CLASS_NOTIF | FAN_REPORT_DFID_NAME
                                     | FAN_UNLIMITED_QUEUE
                                     | FAN_UNLIMITED_MARKS;
inline constexpr auto fan_init_opt_flags = O_RDONLY | O_NONBLOCK | O_CLOEXEC;

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/types
   - sys_resource_type
       An object holding:
         - An fanotify file descriptor
         - An epoll file descriptor
         - An epoll configuration
         - A set of watch marks (as returned by fanotify_mark)
         - A map of (sub)path handles to filesystem paths (names)
         - A boolean: whether or not the resources are valid */
using mark_set_type = std::unordered_set<int>;
using dir_map_type = std::unordered_map<unsigned long, std::filesystem::path>;

struct sys_resource_type {
  bool valid;
  int watch_fd;
  int event_fd;
  epoll_event event_conf;
  mark_set_type mark_set;
  dir_map_type dir_map;
};

inline auto mark(std::filesystem::path const& full_path,
                 int watch_fd,
                 mark_set_type& pmc) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd,
                         FAN_MARK_ADD,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                           | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD,
                         full_path.c_str());
  if (wd >= 0) {
    pmc.insert(wd);
    return true;
  }
  else
    return false;
};

inline auto mark(std::filesystem::path const& full_path,
                 sys_resource_type& sr,
                 unsigned long dir_hash) noexcept -> bool
{
  if (sr.dir_map.find(dir_hash) == sr.dir_map.end())
    return mark(full_path, sr.watch_fd, sr.mark_set)
        && sr.dir_map.emplace(dir_hash, full_path.parent_path()).second;

  else
    return mark(full_path, sr.watch_fd, sr.mark_set);
}

inline auto unmark(std::filesystem::path const& full_path,
                   int watch_fd,
                   mark_set_type& mark_set) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd,
                         FAN_MARK_REMOVE,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                           | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD,
                         full_path.c_str());
  auto const& at = mark_set.find(wd);

  if (wd >= 0 && at != mark_set.end()) {
    mark_set.erase(at);
    return true;
  }
  else
    return false;
};

inline auto unmark(std::filesystem::path const& full_path,
                   sys_resource_type& sr,
                   unsigned long dir_hash) noexcept -> bool
{
  auto const& at = sr.dir_map.find(dir_hash);

  if (at != sr.dir_map.end()) sr.dir_map.erase(at);

  return unmark(full_path, sr.watch_fd, sr.mark_set);
}

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/system_unfold
   Produces a `sys_resource_type` with the file descriptors from
   `fanotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto
system_unfold(std::filesystem::path const& path,
              ::wtr::watcher::event::callback const& callback) noexcept
  -> sys_resource_type
{
  namespace fs = ::std::filesystem;
  using evk = enum ::wtr::watcher::event::kind;
  using evw = enum ::wtr::watcher::event::what;

  auto do_error = [&callback](std::string const& msg,
                              fs::path const& path,
                              int watch_fd,
                              int event_fd = -1) noexcept -> sys_resource_type
  {
    callback(
      {msg.append("(").append(std::strerror(errno)).append(")@").append(path),
       evw::other,
       evk::watcher});

    return sys_resource_type{
      .valid = false,
      .watch_fd = watch_fd,
      .event_fd = event_fd,
      .event_conf = {.events = 0, .data = {.fd = watch_fd}},
      .mark_set = {},
      .dir_map = {},
    };
  };

  auto do_path_map_container_create =
    [](int const watch_fd,
       fs::path const& base_path,
       ::wtr::watcher::event::callback const& callback) -> mark_set_type
  {
    using diter = fs::recursive_directory_iterator;

    /* Follow symlinks, ignore paths which we don't have permissions for. */
    static constexpr auto dopt =
      fs::directory_options::skip_permission_denied
      & fs::directory_options::follow_directory_symlink;

    static constexpr auto rsrv_count = 1024;

    auto ec = std::error_code{};
    auto pmc = mark_set_type{};
    pmc.reserve(rsrv_count);

    if (mark(base_path, watch_fd, pmc))
      if (fs::is_directory(base_path, ec))
        if (! ec)
          for (auto& dir : diter(base_path, dopt, ec))
            if (! ec)
              if (fs::is_directory(dir, ec))
                if (! ec)
                  if (! mark(dir.path(), watch_fd, pmc))
                    callback({"w/sys/not_watched@" + base_path.string() + "@"
                                + dir.path().string(),
                              evw::other,
                              evk::watcher});

    return pmc;
  };

  int watch_fd = fanotify_init(fan_init_flags, fan_init_opt_flags);
  if (watch_fd >= 0) {
    auto pmc = do_path_map_container_create(watch_fd, path, callback);
    if (! pmc.empty()) {
      epoll_event event_conf{.events = EPOLLIN, .data{.fd = watch_fd}};

      int event_fd = epoll_create1(EPOLL_CLOEXEC);

      /* @note We could make the epoll and fanotify file descriptors
         non-blocking with `fcntl`. It's not clear if we can do this
         from their `*_init` calls. */
      /* fcntl(watch_fd, F_SETFL, O_NONBLOCK); */
      /* fcntl(event_fd, F_SETFL, O_NONBLOCK); */

      if (event_fd >= 0)
        if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) >= 0)
          return sys_resource_type{
            .valid = true,
            .watch_fd = watch_fd,
            .event_fd = event_fd,
            .event_conf = event_conf,
            .mark_set = std::move(pmc),
            .dir_map = {},
          };
        else
          return do_error("e/sys/epoll_ctl", path, watch_fd, event_fd);
      else
        return do_error("e/sys/epoll_create", path, watch_fd, event_fd);
    }
    else
      return do_error("e/sys/fanotify_mark", path, watch_fd);
  }
  else
    return do_error("e/sys/fanotify_init", path, watch_fd);
}

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/system_fold
   Close the file descriptors `watch_fd` and `event_fd`. */
inline auto system_fold(sys_resource_type& sr) noexcept -> bool
{
  return ! (close(sr.watch_fd) && close(sr.event_fd));
}

/*  @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/raise
    Promotes an event to a full path and a hash.

    This uses a path from our cache, in the system resource object `sr`, if
    available. Otherwise, it will try to read a directory and directory entry.
    Sometimes, such as on a directory being destroyed and the file not being
    able to be opened, the directory entry is the only event info. We can still
    get the rest of the path in those cases, however, by looking up the cached
    "upper" directory that event belongs to.

    Using the cache is helpful in other cases, too. This is an averaged bench of
    the three branches this function can go down:
      - Cache:            2427ns
      - Dir:              8905ns
      - Dir + Dir Entry:  31966ns

    The shenanigans we do here depend on this event being
    `FAN_EVENT_INFO_TYPE_DFID_NAME`. The kernel passes us
    some info about the directory and the directory entry
    (the filename) of this event that doesn't exist when
    other event types are reported. In particular, we need
    a file descriptor to the directory (which we use
    `readlink` on) and a character string representing the
    name of the directory entry.
    TLDR: We need information for the full path of the event,
    information which is only reported inside this `if`.

    From the kernel:
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

    The kernel guarentees that there is a null-terminated
    character string to the event's directory entry
    after the file handle to the directory.
    Confusing, right? */
inline auto raise(sys_resource_type& sr,
                  fanotify_event_metadata const* metadata) noexcept
  -> std::tuple<std::filesystem::path, unsigned long>
{
  namespace fs = ::std::filesystem;

  auto path_imbue = [](char* path_accum,
                       fanotify_event_info_fid const* dfid_info,
                       file_handle* dir_fh,
                       ssize_t dir_name_len = 0) noexcept -> void
  {
    char* name_info = (char*)(dfid_info + 1);
    char* file_name = static_cast<char*>(
      name_info + sizeof(file_handle) + sizeof(dir_fh->f_handle)
      + sizeof(dir_fh->handle_bytes) + sizeof(dir_fh->handle_type));

    if (file_name && std::strcmp(file_name, ".") != 0)
      std::snprintf(path_accum + dir_name_len,
                    PATH_MAX - dir_name_len,
                    "/%s",
                    file_name);
  };

  auto dir_fid_info = ((fanotify_event_info_fid const*)(metadata + 1));

  auto dir_fh = (file_handle*)(dir_fid_info->handle);

  /* The sum of the handle's bytes. A low-quality hash.
     Unreliable after the directory's inode is recycled. */
  unsigned long dir_hash = std::abs(dir_fh->handle_type);
  for (decltype(dir_fh->handle_bytes) i = 0; i < dir_fh->handle_bytes; i++)
    dir_hash += *(dir_fh->f_handle + i);

  auto const& cache = sr.dir_map.find(dir_hash);

  if (cache != sr.dir_map.end()) {
    /* We already have a path name, use it */
    char path_buf[PATH_MAX];
    auto dir_name = cache->second.c_str();
    auto dir_name_len = cache->second.string().length();

    /* std::snprintf(path_buf, sizeof(path_buf), "%s", dir_name); */
    std::memcpy(path_buf, dir_name, dir_name_len);
    path_imbue(path_buf, dir_fid_info, dir_fh, dir_name_len);

    return std::make_tuple(fs::path{std::move(path_buf)}, dir_hash);
  }
  else {
    /* We can get a path name, so get that and use it */
    char path_buf[PATH_MAX];
    int fd = open_by_handle_at(AT_FDCWD,
                               dir_fh,
                               O_RDONLY | O_CLOEXEC | O_PATH | O_NONBLOCK);
    if (fd > 0) {
      char fs_proc_path[128];
      std::snprintf(fs_proc_path, sizeof(fs_proc_path), "/proc/self/fd/%d", fd);
      ssize_t dirname_len =
        readlink(fs_proc_path, path_buf, sizeof(path_buf) - sizeof('\0'));
      close(fd);

      if (dirname_len > 0) {
        /* Put the directory name in the path accumulator.
           Passing `dirname_len` has the effect of putting
           the event's filename in the path buffer as well. */
        path_buf[dirname_len] = '\0';
        path_imbue(path_buf, dir_fid_info, dir_fh, dirname_len);

        return std::make_tuple(fs::path{std::move(path_buf)}, dir_hash);
      }
      else

        return std::make_tuple(fs::path{}, 0);
    }
    else {
      path_imbue(path_buf, dir_fid_info, dir_fh);

      return std::make_tuple(fs::path{std::move(path_buf)}, dir_hash);
    }
  }
};

inline auto raise(fanotify_event_metadata const* m) noexcept
{
  using evk = enum ::wtr::watcher::event::kind;
  using evw = enum ::wtr::watcher::event::what;

  return std::make_tuple(

    m->mask & FAN_CREATE   ? evw::create
    : m->mask & FAN_DELETE ? evw::destroy
    : m->mask & FAN_MODIFY ? evw::modify
    : m->mask & FAN_MOVE   ? evw::rename
                           : evw::other,

    m->mask & FAN_ONDIR ? evk::dir : evk::file);
}

/*  @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/send
    Send events to the user.

    This is the important part.
    Most of the other code is
    a layer of translation
    between us and the kernel. */
inline auto send(sys_resource_type& sr,
                 fanotify_event_metadata const* m,
                 ::wtr::watcher::event::callback const& callback) noexcept
  -> bool
{
  using evk = enum ::wtr::watcher::event::kind;
  using evw = enum ::wtr::watcher::event::what;

  auto [path, hash] = raise(sr, m);

  auto [what, kind] = raise(m);

  auto tend

    = hash ? kind == evk::dir

             ? what == evw::create  ? mark(path, sr, hash)
             : what == evw::destroy ? unmark(path, sr, hash)
                                    : true
             : true

           : false;

  return

    tend ? callback({path, what, kind}),

    true

         : false;
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/recv
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
inline auto recv(sys_resource_type& sr,
                 std::filesystem::path const& base_path,
                 ::wtr::watcher::event::callback const& callback) noexcept
  -> bool
{
  enum class state { ok, none, err };

  auto do_error = [&base_path,
                   &callback](std::string const& msg) noexcept -> bool
  {
    callback({msg + "@" + base_path,
              ::wtr::watcher::event::what::other,
              ::wtr::watcher::event::kind::watcher});
    return false;
  };

  /* Read some events. */
  alignas(fanotify_event_metadata) char event_buf[event_buf_len];
  auto event_read = read(sr.watch_fd, event_buf, sizeof(event_buf));

  switch (event_read > 0    ? state::ok
          : event_read == 0 ? state::none
          : errno == EAGAIN ? state::none
                            : state::err) {
    case state::ok : {
      /* Loop over everything in the event buffer. */
      for (auto* metadata = (fanotify_event_metadata const*)event_buf;
           FAN_EVENT_OK(metadata, event_read);
           metadata = FAN_EVENT_NEXT(metadata, event_read))
        if (metadata->fd == FAN_NOFD)
          if (metadata->vers == FANOTIFY_METADATA_VERSION)
            if (! (metadata->mask & FAN_Q_OVERFLOW))
              if (((fanotify_event_info_fid*)(metadata + 1))->hdr.info_type
                  == FAN_EVENT_INFO_TYPE_DFID_NAME)

                /* Send the events we recieve. */
                return send(sr, metadata, callback);

              else
                return ! do_error("w/self/event_info");
            else
              return do_error("e/sys/overflow");
          else
            return do_error("e/sys/kernel_version");
        else
          return do_error("e/sys/wrong_event_fd");
    } break;

    case state::none : return true; break;

    case state::err : return do_error("e/sys/read"); break;
  }

  /* Unreachable */
  return false;
};

/*  @brief wtr/watcher/<d>/adapter/watch
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
    A function to decide whether we're dead. */
inline bool watch(std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  using evk = enum ::wtr::watcher::event::kind;
  using evw = enum ::wtr::watcher::event::what;

  auto die = [&path, &callback](sys_resource_type&& sr) noexcept -> bool
  {
    return system_fold(sr)
           ? (callback(
                {"s/self/die@" + path.string(), evw::other, evk::watcher}),
              true)
           : (callback(
                {"e/self/die@" + path.string(), evw::other, evk::watcher}),
              false);
  };

  auto do_error = [&path, &callback](sys_resource_type&& sr,
                                     std::string const& msg) -> bool
  {
    callback({msg + "@" + path, evw::other, evk::watcher});

    die(sr);

    return false;
  };

  /*  While living, with
      - System resources for fanotify and epoll
      - An event list for receiving epoll events

      Do
      - Await filesystem events
      - Invoke `callback` on errors and events */

  sys_resource_type sr = system_unfold(path, callback);

  epoll_event event_recv_list[event_wait_queue_max];

  if (sr.valid) [[likely]] {
    while (is_living()) [[likely]]

    {
      int event_count = epoll_wait(sr.event_fd,
                                   event_recv_list,
                                   event_wait_queue_max,
                                   delay_ms);
      if (event_count < 0)
        return do_error(sr, "e/sys/epoll_wait");

      else if (event_count > 0) [[likely]]
        for (int n = 0; n < event_count; n++)
          if (event_recv_list[n].data.fd == sr.watch_fd) [[likely]]
            if (is_living()) [[likely]]
              if (! recv(sr, path, callback)) [[unlikely]]
                return do_error(sr, "e/self/event_recv");
    }

    return die(sr);
  }

  else
    return do_error(sr, "e/self/sys_resource");
}

} /* namespace fanotify */
} /* namespace adapter */
} /* namespace watcher */
} /* namespace wtr */
} /* namespace detail */

#endif /* !defined(WATER_WATCHER_USE_WARTHOG) */
#endif /* defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_5_9_0) \
          && !defined(WATER_WATCHER_PLATFORM_ANDROID_ANY) */
