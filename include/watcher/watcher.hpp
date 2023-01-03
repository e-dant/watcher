#pragma once

/*
  @brief watcher/watcher

  This is the public interface.
  Include and use this file.
*/

/* clang-format off */
#include <watcher/platform.hpp>
#include <watcher/event.hpp>
#include <watcher/adapter/windows/watch.hpp>
#include <watcher/adapter/darwin/watch.hpp>
#include <watcher/adapter/linux/watch.hpp>
#include <watcher/adapter/android/watch.hpp>
#include <watcher/adapter/warthog/watch.hpp>
#include <watcher/adapter/adapter.hpp>
#include <watcher/watch.hpp>
/* clang-format on */
