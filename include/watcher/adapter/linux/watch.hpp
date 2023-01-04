#pragma once

/*
  @brief wtr/watcher/detail/adapter/linux

  The Linux adapters.
*/

#include <watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_2_7_0) \
    || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

/* function */
#include <functional>
/* LINUX_VERSION_CODE
   KERNEL_VERSION */
#include <linux/version.h>
/* geteuid */
#include <unistd.h>
/* event
   callback
   fanotify::watch
   inotify::watch */
#include <watcher/watcher.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

/*
  @brief watcher/detail/adapter/watch

  Monitors `path` for changes.
  Invokes `callback` with an `event` when they happen.
  `watch` stops when asked to or unrecoverable errors occur.
  All events, including errors, are passed to `callback`.

  @param path
    A filesystem path to watch for events.

  @param callback
    A function to invoke with an `event` object
    when the files being watched change.

  @param is_living
    A function to decide whether we're dead.

  @note
  If we have a kernel that can use either `fanotify` or
  `inotify`, then we will use `fanotify` if the user is
  (effectively) root.

  If we can only use `fanotify` or `inotify`, then we'll
  use them. We only use `inotify` on Android.

  There should never be a system that can use `fanotify`,
  but not `inotify`. It's just here for completeness.
*/

inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  return

#if defined(WATER_WATCHER_ADAPTER_LINUX_FANOTIFY) \
    && defined(WATER_WATCHER_ADAPTER_LINUX_INOTIFY)

      geteuid() == 0 ? fanotify::watch(path, callback, is_living)
                     : inotify::watch(path, callback, is_living);

#elif defined(WATER_WATCHER_ADAPTER_LINUX_FANOTIFY)

      fanotify::watch(path, callback, is_living);

#elif defined(WATER_WATCHER_ADAPTER_LINUX_INOTIFY)

      inotify::watch(path, callback, is_living);

#else

#error "Define 'WATER_WATCHER_USE_WARTHOG' on kernel versions < 2.7"

#endif
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_2_7_0) \
          || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY) */
#endif /* !defined(WATER_WATCHER_USE_WARTHOG) */
