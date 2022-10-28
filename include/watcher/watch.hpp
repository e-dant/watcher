#pragma once

#include <watcher/adapter/adapter.hpp>
#include <watcher/event.hpp>

namespace water {
namespace watcher {

/*
  @brief watcher/watch

  Implements `water::watcher::watch`.

  There are two things the user needs:
    - The `watch` function
    - The `die` function
    - The `event` structure

  That's it, and this is one of them.

  Happy hacking.

  @param callback (optional):
    Something (such as a function or closure) to be called
    whenever events occur in the paths being watched.

  @param path:
    The root path to watch for filesystem events.

  This is an adaptor "switch" that chooses the ideal adaptor
  for the host platform.

  Every adapter monitors `path` for changes and invoked the
  `callback` with an `event` object when they occur.

  The `event` object will contain the:
    - Path -- Which is always relative.
    - Path type -- one of:
      - File
      - Directory
      - Symbolic Link
      - Hard Link
      - Unknown
    - Event type -- one of:
      - Create
      - Modify
      - Destroy
      - OS-Specific Events
      - Unknown
    - Event time -- In nanoseconds since epoch
*/

static bool watch(const char* path, event::callback const& callback)
{
  return detail::adapter::can_watch() ? detail::adapter::watch(path, callback)
                                      : false;
}

static bool watch(const char* path, event::callback const& living_cb,
                  event::callback const& dying_cb)
{
  using namespace water::watcher::detail;

  auto ok = adapter::can_watch() ? adapter::watch(path, living_cb) : false;

  adapter::die(dying_cb);

  return ok;
}

/*
  @brief watcher/die

  Stops the `watch`.
  Destroys itself.
*/
static bool die()
{
  using whatever = const event::event&;
  return detail::adapter::die([](whatever) -> void {});
}

/*
  @brief watcher/die

  Stops the `watch`.
  Calls `callback`,
  then destroys itself.
*/
static bool die(event::callback const& callback)
{
  return detail::adapter::die(callback);
}

} /* namespace watcher */
} /* namespace water   */
