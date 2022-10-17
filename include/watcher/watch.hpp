#pragma once

#include <watcher/adapter/android/watch.hpp>
#include <watcher/adapter/linux/watch.hpp>
#include <watcher/adapter/darwin/watch.hpp>
#include <watcher/adapter/warthog/watch.hpp>
#include <watcher/adapter/windows/watch.hpp>

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

template <const auto delay_ms = 16>
[[nodiscard("Wise to check if this (boolean) function was successful.")]]

bool watch(
    const char* path,
    const auto& callback) {
  static_assert(delay_ms >= 0, "Negative time considered harmful.");
  return detail::adapter::watch<delay_ms>(path, callback);
}

} /* namespace watcher */
} /* namespace water   */
