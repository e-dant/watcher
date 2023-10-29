#pragma once

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include "wtr/watcher.hpp"
#include <linux/version.h>
#include <unistd.h>

namespace detail::wtr::watcher::adapter {

/*  We'll run and keep running as long as
    we have:
    - A lifetime the user hasn't ended
    - System resources for inotify and epoll
    - An event buffer to `read()` into */
inline auto watch =
  [](auto const& path, auto const& cb, auto& is_living) noexcept -> bool
{
#if (KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE) && ! __ANDROID_API__
  bool fanotify_usable = geteuid() == 0;
#elif (KERNEL_VERSION(2, 7, 0) <= LINUX_VERSION_CODE) || __ANDROID_API__
  bool fanotify_usable = false;
#else
#error "Define 'WATER_WATCHER_USE_WARTHOG' on kernel versions < 2.7.0"
#endif

  return fanotify_usable ? fanotify::watch(path.c_str(), cb, is_living)
                         : inotify::watch(path.c_str(), cb, is_living);
};

} /*  namespace detail::wtr::watcher::adapter */

#endif
