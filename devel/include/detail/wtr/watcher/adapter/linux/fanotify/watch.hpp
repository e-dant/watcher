#pragma once

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE) && ! __ANDROID_API__

#include "wtr/watcher.hpp"
#include <atomic>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <optional>
#include <sys/epoll.h>
#include <sys/fanotify.h>
#include <system_error>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace detail::wtr::watcher::adapter::fanotify {

/*  - fan_init_flags:
      Post-event reporting, non-blocking IO and unlimited
      marks. We need sudo mode for the unlimited marks.
      If we were making a filesystem auditor, we might use:
          FAN_CLASS_PRE_CONTENT
          | FAN_UNLIMITED_QUEUE
          | FAN_UNLIMITED_MARKS
    - fan_init_io_flags:
      Read-only, non-blocking, and close-on-exec. */

struct ev_fa {
  static constexpr auto buf_len = 4096;
  static constexpr auto c_ulim = buf_len / sizeof(fanotify_event_metadata);
  static constexpr auto recv_flags = FAN_ONDIR | FAN_CREATE | FAN_MODIFY
                                   | FAN_MOVED_FROM | FAN_MOVED_TO | FAN_DELETE
                                   | FAN_DELETE_SELF | FAN_MOVE_SELF;
  static constexpr auto init_flags = FAN_CLASS_NOTIF | FAN_REPORT_DFID_NAME
                                   | FAN_UNLIMITED_QUEUE | FAN_UNLIMITED_MARKS;
  static constexpr auto init_io_flags = O_RDONLY | O_CLOEXEC;  // O_NONBLOCK
  int fd = -1;
  alignas(fanotify_event_metadata) char buf[buf_len]{0};
};

struct sysres {
  bool ok = false;
  ev_fa ke{};
  adapter::ep ep{};
};

inline auto do_mark =
  [](char const* const dirpath, int fa_fd, auto const& cb) noexcept -> bool
{
  auto do_error = [&]() -> bool
  { return adapter::do_error("w/sys/not_watched@", dirpath, cb); };
  char real[PATH_MAX];
  if (! realpath(dirpath, real)) return do_error();
  int anonymous_wd =
    fanotify_mark(fa_fd, FAN_MARK_ADD, ev_fa::recv_flags, AT_FDCWD, real);
  return anonymous_wd == 0 ? true : is_dir(real) ? do_error() : false;
};

/*  Grabs the resources we need from `fanotify`
    and `epoll`. Marks itself invalid on errors,
    sends diagnostics on warnings and errors.
    Notes:
    We could make the epoll and fanotify file
    descriptors non-blocking with `fcntl`. It's
    not clear if we can do this from their
    `*_init` calls or if we need them to be non-
    blocking with our threaded architecture.
      fcntl(fa_fd, F_SETFL, O_NONBLOCK);
      fcntl(ep_fd, F_SETFL, O_NONBLOCK);
*/
inline auto make_sysres(
  char const* const base_path,
  ::wtr::watcher::event::callback const& cb) noexcept -> sysres
{
  auto do_error = [&](auto&& msg, int fa_fd, int ep_fd = -1) -> sysres
  {
    adapter::do_error(msg, base_path, cb);
    return sysres{
      .ok = false,
      .ke = {.fd = fa_fd},
      .ep = {.fd = ep_fd},
    };
  };

  int fa_fd = fanotify_init(ev_fa::init_flags, ev_fa::init_io_flags);
  if (fa_fd < 1) return do_error("e/sys/fanotify_init@", fa_fd);
  int ep_fd = epoll_create1(EPOLL_CLOEXEC);
  if (ep_fd < 1) return do_error("e/sys/epoll_create@", fa_fd, ep_fd);
  auto want_ev = epoll_event{.events = EPOLLIN, .data{.fd = fa_fd}};
  int ctl_ec = epoll_ctl(ep_fd, EPOLL_CTL_ADD, fa_fd, &want_ev);
  if (ctl_ec != 0) return do_error("e/sys/epoll_ctl@", fa_fd, ep_fd);

  adapter::walkdir_do(
    base_path,
    [&](auto const& dir) { do_mark(dir, fa_fd, cb); });

  return sysres{
    .ok = true,
    .ke{.fd = fa_fd},
    .ep{.fd = ep_fd},
  };
}

inline auto close_sysres(sysres& sr) noexcept -> bool
{
  sr.ok = false;
  auto ok = close(sr.ke.fd) == 0 && close(sr.ep.fd) == 0;
  sr.ke.fd = sr.ep.fd = -1;
  return ok;
}

/*  Parses a full path from an event's metadata.
    The shenanigans we do here depend on this event being
    `FAN_EVENT_INFO_TYPE_DFID_NAME`. The kernel passes us
    some info about the directory and the directory entry
    (the filename) of this event that doesn't exist when
    other event types are reported. In particular, we need
    a file descriptor to the directory (which we use
    `readlink` on) and a character string representing the
    name of the directory entry.
    TLDR: We need information for the full path of the
    event, information which is only reported inside this
    `if`.
    From the kernel:
    Variable size struct for
    dir file handle + child file handle + name
    [ Omitting definition of `fanotify_info` here ]
    (struct fanotify_fh)
      dir_fh starts at
        buf[0] (optional)
      dir2_fh starts at
        buf[dir_fh_totlen] (optional)
      file_fh starts at
        buf[dir_fh_totlen+dir2_fh_totlen]
      name starts at
        buf[dir_fh_totlen+dir2_fh_totlen+file_fh_totlen]
      ...
    The kernel guarentees that there is a null-terminated
    character string to the event's directory entry
    after the file handle to the directory.
    Confusing, right? */
inline auto pathof(fanotify_event_metadata const* const mtd) noexcept
  -> std::filesystem::path
{
  constexpr size_t path_ulim = PATH_MAX - sizeof('\0');
  auto dir_info = (fanotify_event_info_fid*)(mtd + 1);
  auto dir_fh = (file_handle*)(dir_info->handle);

  char path_buf[PATH_MAX];
  ssize_t file_name_offset = 0;

  /*  Directory name */
  {
    constexpr int ofl = O_RDONLY | O_CLOEXEC | O_PATH;
    int fd = open_by_handle_at(AT_FDCWD, dir_fh, ofl);
    if (fd > 0) {
      /*  If we have a pid with more than 128 digits... Well... */
      char fs_ev_pidpath[128];
      snprintf(fs_ev_pidpath, sizeof(fs_ev_pidpath), "/proc/self/fd/%d", fd);
      file_name_offset = readlink(fs_ev_pidpath, path_buf, path_ulim);
      close(fd);
    }
    path_buf[file_name_offset] = 0;
    /*  todo: Is is an error if we can't get the directory name? */
  }

  /*  File name ("Directory entry")
      If we wrote the directory name before here, we
      can start writing the file name after its offset. */
  {
    if (file_name_offset > 0) {
      char* file_name = ((char*)dir_fh->f_handle + dir_fh->handle_bytes);
      auto not_selfdir = strcmp(file_name, ".") != 0;
      auto beg = path_buf + file_name_offset;
      auto end = PATH_MAX - file_name_offset;
      if (file_name && not_selfdir) snprintf(beg, end, "/%s", file_name);
    }
  }
  return {path_buf};
}

inline auto
peek(fanotify_event_metadata const* const mtd, size_t read_len) noexcept
  -> fanotify_event_metadata const*
{
  auto evlen = mtd->event_len;
  auto next = (fanotify_event_metadata*)((char*)mtd + evlen);
  return FAN_EVENT_OK(next, read_len - evlen) ? next : nullptr;
}

inline auto
parse_event(fanotify_event_metadata const* m, size_t read_len) noexcept
  -> ::wtr::watcher::event
{
  using ev = ::wtr::watcher::event;
  using ev_pt = enum ev::path_type;
  using ev_et = enum ev::effect_type;
  auto n = peek(m, read_len);
  auto pt = m->mask & FAN_ONDIR ? ev_pt::dir : ev_pt::file;
  auto et = m->mask & FAN_CREATE ? ev_et::create
          : m->mask & FAN_DELETE ? ev_et::destroy
          : m->mask & FAN_MODIFY ? ev_et::modify
          : m->mask & FAN_MOVE   ? ev_et::rename
                                 : ev_et::other;
  auto isfromto = [et](unsigned a, unsigned b) -> bool
  { return et == ev_et::rename && a & FAN_MOVED_FROM && b & FAN_MOVED_TO; };
  auto e = [et, pt](auto*... m) -> ev { return ev(ev(pathof(m), et, pt)...); };
  return ! n                        ? e(m)
       : isfromto(m->mask, n->mask) ? e(m, n)
       : isfromto(n->mask, m->mask) ? e(n, m)
                                    : e(m);
}

/*  Send events to the user.
    This is the important part.
    Most of the other code is
    a layer of translation
    between us and the kernel. */
inline auto send(
  ::wtr::watcher::event const& ev,
  sysres& sr,
  ::wtr::watcher::event::callback const& cb) noexcept -> bool
{
  using e = ::wtr::watcher::event;
  using ev_pt = enum e::path_type;
  using ev_et = enum e::effect_type;
  auto [pn, et, pt, _t, _a] = ev;
  auto update_ok = pt == ev_pt::dir && et == ev_et::create
                   ? do_mark(pn.c_str(), sr.ke.fd, cb)
                   : true;
  return (cb(ev), update_ok);
}

/*  Read some events from what fanotify gives
    us. Sends (the valid) events to the user's
    callback. Send a diagnostic to the user on
    warnings and errors. Returns false on errors.
    True otherwise.
    Notes:
    The `metadata->fd` field contains either a
    file descriptor or the value `FAN_NOFD`. File
    descriptors are always greater than 0 (but we
    will get the FAN_NOFD value, and we can grab
    other information from the /proc/self/<fd>
    filesystem, which we do in `pathof()`).
    `FAN_NOFD` represents an event queue overflow
    for `fanotify` listeners which are *not*
    monitoring file handles, such as mount/volume
    watchers. The file handle is in the metadata
    when an `fanotify` listener is monitoring
    events by their file handles.
    The `metadata->vers` field may differ between
    kernel versions, so we check it against the
    version we were compiled with. */
inline auto do_event_recv(
  std::filesystem::path const& base_path,
  ::wtr::watcher::event::callback const& cb,
  sysres& sr) noexcept -> bool
{
  auto do_error = [&](auto&& msg) noexcept -> bool
  { return adapter::do_error(msg, base_path.c_str(), cb); };
  auto do_warn = [&](auto&& msg) noexcept -> bool { return ! do_error(msg); };
  auto ev_info = [](fanotify_event_metadata* m) noexcept
  { return (fanotify_event_info_fid*)(m + 1); };
  auto ev_has_dirname = [&](fanotify_event_metadata* m) noexcept -> bool
  { return ev_info(m)->hdr.info_type == FAN_EVENT_INFO_TYPE_DFID_NAME; };

  unsigned ev_c = 0;
  memset(sr.ke.buf, 0, sr.ke.buf_len);
  int read_len = read(sr.ke.fd, sr.ke.buf, sr.ke.buf_len);
  if (read_len <= 0 && errno != EAGAIN)
    return true;
  else if (read_len < 0)
    return do_error("e/sys/read@");
  else
    for (auto* mtd = (fanotify_event_metadata*)(sr.ke.buf);
         FAN_EVENT_OK(mtd, read_len);
         mtd = FAN_EVENT_NEXT(mtd, read_len))
      if (ev_c++ > sr.ke.c_ulim)
        return do_error("e/sys/bad_count@");
      else if (mtd->vers != FANOTIFY_METADATA_VERSION)
        return do_error("e/sys/kernel_version@");
      else if (mtd->fd != FAN_NOFD)
        return do_warn("w/sys/bad_fd@");
      else if (mtd->mask & FAN_Q_OVERFLOW)
        return do_warn("w/sys/overflow@");
      else if (! ev_has_dirname(mtd))
        return do_warn("w/sys/bad_info@");
      else
        send(parse_event(mtd, read_len), sr, cb);
  return true;
}

inline auto watch(
  char const* const path,
  ::wtr::watcher::event::callback const& cb,
  std::atomic<bool>& is_living) noexcept -> bool
{
  auto sr = make_sysres(path, cb);
  auto do_error = [&](auto&& msg) -> bool
  { return (close_sysres(sr), adapter::do_error(msg, path, cb)); };

  if (! sr.ok) return do_error("e/self/resource@");
  while (is_living) {
    int ep_c =
      epoll_wait(sr.ep.fd, sr.ep.interests, sr.ep.q_ulim, sr.ep.wake_ms);
    if (ep_c < 0)
      return do_error("e/sys/epoll_wait@");
    else if (ep_c > 0) [[likely]]
      for (int n = 0; n < ep_c; n++)
        if (sr.ep.interests[n].data.fd == sr.ke.fd) [[likely]]
          if (! do_event_recv(path, cb, sr)) [[unlikely]]
            return do_error("e/self/event_recv@");
  }

  return close_sysres(sr);
}

} /* namespace detail::wtr::watcher::adapter::fanotify */

#endif
#endif
