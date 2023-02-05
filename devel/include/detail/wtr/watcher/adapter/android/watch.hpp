#pragma once

/*
  @brief watcher/adapter/android

  The Android (Linux) `inotify` adapter.
*/

/* WATER_WATCHER_PLATFORM_* */
#include <detail/wtr/watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#include <detail/wtr/watcher/adapter/linux/watch.hpp>
#endif
