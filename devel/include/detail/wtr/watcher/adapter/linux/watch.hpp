#pragma once

#if (defined(__linux__) || __ANDROID_API__) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include "wtr/watcher.hpp"
#include <linux/version.h>
#include <unistd.h>

namespace detail::wtr::watcher::adapter {

inline auto watch =
  [](auto const& path, auto const& cb, auto const& is_living) -> bool
{
  auto p = path.c_str();
#if (KERNEL_VERSION(5, 9, 0) <= LINUX_VERSION_CODE) && ! __ANDROID_API__
  return geteuid() == 0 ? fanotify::watch(p, cb, is_living)
                        : inotify::watch(p, cb, is_living);
#elif (KERNEL_VERSION(2, 7, 0) <= LINUX_VERSION_CODE) || __ANDROID_API__
  return inotify::watch(p, cb, is_living);
#else
#error "Define 'WATER_WATCHER_USE_WARTHOG' on kernel versions < 2.7.0"
#endif
};

} /*  namespace detail::wtr::watcher::adapter */

#endif
