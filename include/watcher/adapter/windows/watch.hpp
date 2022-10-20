#pragma once

/*
  @brief watcher/adapter/windows

  Work is planned for a `ReadDirectoryChangesW`-based adapter for Windows.
*/

#include <watcher/platform.hpp>
#if defined(PLATFORM_WINDOWS_ANY)
#define WATER_WATCHER_USE_WARTHOG
#include <watcher/adapter/warthog/watch.hpp>
#endif
