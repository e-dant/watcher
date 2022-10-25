#pragma once

/*
  @brief watcher/adapter/android

  We are exploring `fanotify` for a more efficient implementation on Linux and
  Android. Until a stable implementation has been made, we will use the
  `warthog` adapter on these systems.

  These kernel APIs are inaccurate and unstable.

  Work is being done to get most of `warthog`'s accuracy and most of
  `fanotify`'s efficiency.
*/

#include <watcher/platform.hpp>
#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#define WATER_WATCHER_USE_WARTHOG
#include <watcher/adapter/warthog/watch.hpp>
#endif
