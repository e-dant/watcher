#pragma once

#include <watcher/event.hpp>

namespace water {
namespace watcher {
namespace detail {
namespace adapter {

inline constexpr auto delay_ms = 16;

static bool watcher_alive = false;

/*
  @brief watcher/adapter/is_living

  A predicate to evaluate whether `watch` should die.

  Likely may be overloaded by the user in the future.
*/

static bool is_living() {
  return watcher_alive ? true : false;
}

/*
  @brief watcher/adapter/can_watch

  Call this before `watch` to ensure only one `watch` exists.

  It might do other things or be removed at some point.
*/

static bool can_watch() {
  if (watcher_alive)
    return false;
  else
    watcher_alive = true;
  return true;
}

/*
  @brief watcher/adapter/watch

  @param path:
   A path to watch for changes.

  @param callback:
   A callback to perform when the files
   being watched change.

  Monitors `path` for changes.

  Calls `callback` with an `event` when they happen.

  Unless it should stop, or errors present, `watch` recurses.
*/

static bool watch(const char* path, event::callback const& callback);

/*
  @brief watcher/adapter/die

  Invokes `callback` immediately before destroying itself.
*/

static bool die(event::callback const& callback) {
  callback(event::event{"", event::what::destroy, event::kind::watcher});
  if (watcher_alive) {
    watcher_alive = false;
    return true;
  } else {
    return false;
  }
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace water */

/* clang-format off */

#include <watcher/platform.hpp>

#include <watcher/adapter/windows/watch.hpp>
#include <watcher/adapter/darwin/watch.hpp>
#include <watcher/adapter/linux/watch.hpp>
#include <watcher/adapter/android/watch.hpp>

#include <watcher/adapter/warthog/watch.hpp>

/* clang-format on */
