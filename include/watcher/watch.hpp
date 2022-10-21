#pragma once

#include <watcher/adapter/android/watch.hpp>
#include <watcher/adapter/darwin/watch.hpp>
#include <watcher/adapter/linux/watch.hpp>
#include <watcher/adapter/warthog/watch.hpp>
#include <watcher/adapter/windows/watch.hpp>
#include <watcher/event.hpp>

namespace water {
namespace watcher {

/*
  @brief watcher/watch

  Implements `water::watcher::watch`.

  There are two things the user needs:
    - The `watch` function
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

using callback_type = void (*)(const event::event&);

static bool watch(const char* path, callback_type const& callback) {
  return detail::adapter::live() ? detail::adapter::watch(path, callback)
                                 : false;
}

static bool watch(const char* path,
                  callback_type const& callback,
                  auto const& until) {
  return detail::adapter::live() ? detail::adapter::watch(path, callback, until)
                                 : false;
}

static bool die() {
  using whatever = const event::event&;
  return detail::adapter::die([](whatever) -> void {});
}

static bool die(callback_type const& callback) {
  return detail::adapter::die(callback);
}

} /* namespace watcher */
} /* namespace water   */
