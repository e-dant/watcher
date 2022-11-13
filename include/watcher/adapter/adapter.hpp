#pragma once

#include <mutex>
#include <watcher/event.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

namespace {
static bool watcher_alive = false;            /* NOLINT */
static auto watcher_alive_mtx = std::mutex{}; /* NOLINT */
} /* namespace */

inline constexpr auto delay_ms = 16;

/*
  @brief watcher/adapter/is_living

  A predicate to evaluate whether `watch` should die.

  Likely may be overloaded by the user in the future.
*/

inline bool is_living()
{
  watcher_alive_mtx.lock();
  bool _ = watcher_alive;
  watcher_alive_mtx.unlock();
  return _;
}

/*
  @brief watcher/adapter/can_watch

  Call this before `watch` to ensure only one `watch` exists.

  It might do other things or be removed at some point.
*/

inline bool make_living()
{
  bool ok = true;
  watcher_alive_mtx.lock();

  if (watcher_alive)
    ok = false;

  else
    watcher_alive = true;

  watcher_alive_mtx.unlock();
  return ok;
}

/*
  @brief watcher/adapter/watch
  Monitors `path` for changes.
  Invokes `callback` with an `event` when they happen.
  `watch` stops when asked to or irrecoverable errors occur.
  All events, including errors, are passed to `callback`.

  @param path:
   A `path` to watch for filesystem events.

  @param callback:
   A `callback` to invoke with an `event` object
   when the files being watched change.
*/

inline bool watch(const char* path, event::callback const& callback);

/*
  @brief watcher/adapter/die

  Invokes `callback` immediately before destroying itself.
*/

inline bool die(event::callback const& callback)
{
  bool ok = true;

  {
    watcher_alive_mtx.lock();

    if (watcher_alive == true)
      watcher_alive = false;

    else
      ok = false;

    watcher_alive_mtx.unlock();
  }

  if (ok)
    callback(
        event::event{"s/self/die", event::what::destroy, event::kind::watcher});

  else
    callback(
        event::event{"e/self/die", event::what::destroy, event::kind::watcher});

  return ok;
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

/* clang-format off */

#include <watcher/platform.hpp>

#include <watcher/adapter/windows/watch.hpp>
#include <watcher/adapter/darwin/watch.hpp>
#include <watcher/adapter/linux/watch.hpp>
#include <watcher/adapter/android/watch.hpp>

#include <watcher/adapter/warthog/watch.hpp>

/* clang-format on */
