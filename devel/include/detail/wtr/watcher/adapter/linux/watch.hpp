#pragma once

/*  @brief wtr/detail/wtr/watcher/adapter/linux
    The Linux adapters. */

#if (defined(__linux__) || defined(__ANDROID_API__)) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

/* LINUX_VERSION_CODE
   KERNEL_VERSION */
#include <linux/version.h>
/* function */
#include <functional>
/* geteuid */
#include <unistd.h>
/* event
   callback
   inotify::watch
   fanotify::watch */
#include "wtr/watcher.hpp"

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {

/*
  @brief detail/wtr/watcher/adapter/watch

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

  We can use `fanotify` (and `inotify`, too) on a kernel
  version 5.9.0 or greater.

  If we can only use `inotify`, then we'll just use that.
  We only use `inotify` on Android and on kernel versions
  less than 5.9.0 and greater than/equal to 2.7.0.

  Below 2.7.0, you can use the `warthog` adapter by defining
  `WATER_WATCHER_USE_WARTHOG` at some point during the build
  or before including 'wtr/watcher.hpp'.
*/

inline bool watch(std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
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

} /* namespace adapter */
} /* namespace watcher */
} /* namespace wtr */
} /* namespace detail */

#endif
