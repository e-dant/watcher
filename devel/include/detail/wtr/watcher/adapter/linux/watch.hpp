#pragma once

#if (defined(__linux__) || defined(__ANDROID_API__)) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include "wtr/watcher.hpp"
#include <atomic>
#include <functional>
#include <linux/version.h>
#include <unistd.h>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {

/*  If we have a kernel that can use either `fanotify` or
    `inotify`, then we will use `fanotify` if the user is
    (effectively) root.
    We can use `fanotify` (and `inotify`, too) on a kernel
    version 5.9.0 or greater.
    If we can only use `inotify`, then we'll just use that.
    We only use `inotify` on Android and on kernel versions
    less than 5.9.0 and greater than/equal to 2.7.0.
    Below 2.7.0, you can use the `warthog` adapter by
    defining `WATER_WATCHER_USE_WARTHOG` at some point during
    the build or before including 'wtr/watcher.hpp'. */

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  std::atomic_bool& is_living) noexcept -> bool
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)) \
  && ! defined(__ANDROID_API__)
  return geteuid() == 0 ? fanotify::watch(path, callback, is_living)
                        : inotify::watch(path, callback, is_living);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 7, 0)) \
  || defined(__ANDROID_API__)
  return inotify::watch(path, callback, is_living);
#else
#error "Define 'WATER_WATCHER_USE_WARTHOG' on kernel versions < 2.7.0"
#endif
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr */
} /*  namespace detail */

#endif
