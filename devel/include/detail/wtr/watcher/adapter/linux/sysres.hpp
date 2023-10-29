#pragma once

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include "wtr/watcher.hpp"
#include <cstring>
#include <dirent.h>
#include <functional>
#include <stdio.h>
#include <string>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace detail::wtr::watcher::adapter {

struct ep {
  /*  Number of events allowed to be
      given to do_event_recv. The same
      value is usually returned by a
      call to `epoll_wait`). Any number
      between 1 and INT_MAX should be
      fine... But lower is fine for us.
      We don't lose events if we 'miss'
      them, the events are still waiting
      in the next call to `epoll_wait`.
  */
  static constexpr auto q_ulim = 64;
  /*  The delay, in milliseconds, while
      `epoll_wait` will 'pause' us for
      until we are woken up. We use the
      time after this to check if we're
      still alive, then re-enter epoll.
  */
  static constexpr auto wake_ms = 16;

  int fd = -1;
  epoll_event interests[q_ulim]{};
};

inline auto do_error =
  [](std::string&& msg, char const* const path, auto const& cb) -> bool
{
  using et = enum ::wtr::watcher::event::effect_type;
  using pt = enum ::wtr::watcher::event::path_type;
  cb({msg + path, et::other, pt::watcher});
  return false;
};

inline auto do_warn =
  [](std::string&& msg, auto const& path, auto const& cb) -> bool
{ return (do_error(std::move(msg), path, cb), true); };

inline auto make_ep =
  [](char const* const base_path, auto const& cb, int ev_fd) -> ep
{
  auto do_error = [&](auto&& msg)
  { return (adapter::do_error(msg, base_path, cb), ep{}); };
#if __ANDROID_API__
  int fd = epoll_create(1);
#else
  int fd = epoll_create1(EPOLL_CLOEXEC);
#endif
  auto want_ev = epoll_event{.events = EPOLLIN, .data{.fd = ev_fd}};
  int ec = epoll_ctl(fd, EPOLL_CTL_ADD, ev_fd, &want_ev);
  return fd < 0 ? do_error("e/sys/epoll_create@")
       : ec < 0 ? (close(fd), do_error("e/sys/epoll_ctl@"))
                : ep{.fd = fd};
};

inline auto is_dir(char const* const path) -> bool
{
  struct stat s;
  return stat(path, &s) == 0 && S_ISDIR(s.st_mode);
}

inline auto strany = [](char const* const s, auto... cmp) -> bool
{ return ((strcmp(s, cmp) == 0) || ...); };

/*  $ echo time wtr.watcher / -ms 1
      | sudo bash -E
      ...
      real 0m25.094s
      user 0m4.091s
      sys  0m20.856s

    $ sudo find / -type d
      | wc -l
      ...
      784418

    We could parallelize this, but
    it's never going to be instant.

    It might be worth it to start
    watching before we're done this
    hot find-and-mark path, despite
    not having a full picture.
*/
template<class Fn>
inline auto walkdir_do(char const* const path, Fn const& f) -> void
{
  auto pappend = [&](char* head, char* tail)
  { return snprintf(head, PATH_MAX, "%s/%s", path, tail); };
  if (DIR* d = opendir(path)) {
    f(path);
    while (dirent* de = readdir(d)) {
      char next[PATH_MAX];
      char real[PATH_MAX];
      if (de->d_type != DT_DIR) continue;
      if (strany(de->d_name, ".", "..")) continue;
      if (pappend(next, de->d_name) <= 0) continue;
      if (! realpath(next, real)) continue;
      walkdir_do(real, f);
    }
    (void)closedir(d);
  }
}

inline auto close_sysres = [](auto& sr) -> bool
{
  sr.ok = false;
  auto closed = close(sr.ke.fd) == 0 && close(sr.ep.fd) == 0;
  sr.ke.fd = sr.ep.fd = -1;
  return closed;
};

} /*  namespace detail::wtr::watcher::adapter */

#endif
