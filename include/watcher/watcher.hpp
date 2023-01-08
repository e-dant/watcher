#pragma once

/*
  @brief watcher/watcher

  This is the public interface.
  Include and use this file.
*/

/* clang-format off */
#include <watcher/detail/platform.hpp>
#include <watcher/detail/event.hpp>
#include <watcher/detail/adapter/windows/watch.hpp>
#include <watcher/detail/adapter/darwin/watch.hpp>
#include <watcher/detail/adapter/linux/fanotify/watch.hpp>
#include <watcher/detail/adapter/linux/inotify/watch.hpp>
#include <watcher/detail/adapter/linux/watch.hpp>
#include <watcher/detail/adapter/android/watch.hpp>
#include <watcher/detail/adapter/warthog/watch.hpp>
#include <watcher/detail/adapter/adapter.hpp>
#include <watcher/detail/watch.hpp>
/* clang-format on */
