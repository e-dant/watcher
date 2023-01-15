#pragma once

/*
  @brief watcher/adapter/android

  The Android (Linux) `inotify` adapter.
*/

/* WATER_WATCHER_PLATFORM_* */
#include <watcher/detail/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#include <watcher/detail/adapter/linux/watch.hpp>
#endif
