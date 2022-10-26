#pragma once

/*
  @brief watcher/adapter/android

  The Android (Linux) `inotify` adapter.
*/

#include <watcher/platform.hpp>
#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#define WATER_WATCHER_USE_WARTHOG
#include <watcher/adapter/linux/watch.hpp>
#endif
