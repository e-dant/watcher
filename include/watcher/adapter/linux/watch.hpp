#pragma once

/*
  @brief wtr/watcher/detail/adapter/linux

  The Linux adapters.
*/

#include <watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
    || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

/* function */
#include <functional>
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
  If the user is (effectively) root, and not on Android,
  the we'll use `fanotify`. If not, we'll use `inotify`.

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
*/
inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  return
#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
      inotify::watch(path, callback, is_living);
#else
      geteuid() == 0 ? fanotify::watch(path, callback, is_living)
                     : inotify::watch(path, callback, is_living);
#endif
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
          || defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
#endif /* if !defined(WATER_WATCHER_USE_WARTHOG) */
