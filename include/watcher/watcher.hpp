#pragma once

/*
  @brief watcher/watcher

  This is the public interface,
  and it's probably what you're looking for.

  Include and use this file.
*/

/* @todo consider using `std::invoke` */
/* #include <functional> */
#include <watcher/platform.hpp>
#if defined(PLATFORM_MAC_ANY)
#include <watcher/adapter/mac.hpp>
#elif defined(PLATFORM_LINUX_ANY)
#include <watcher/adapter/lin.hpp>
#elif defined(PLATFORM_ANDROID_ANY)
#include <watcher/adapter/andy.hpp>
#elif defined(PLATFORM_WINDOWS_ANY)
#include <watcher/adapter/win.hpp>
#elif defined(PLATFORM_UNKNOWN)
#include <watcher/adapter/warthog.hpp>
#else
#include <watcher/adapter/warthog.hpp>
#endif

namespace water {
namespace watcher {

/*
  @brief watcher/run

  @param closure (optional):
    A callback to call when the files being watched change.
    @see Callback

  @param path:
    The root path to watch for filesystem events.

  This is an adaptor "switch" that chooses the ideal adaptor
  for the host platform.

  Every adapter monitors `path_to_watch` for changes and
  executes the `callback` when they occur.

  The callback will be given an event object with all
  relevant information, such as the:
    - The (absolute) path
    - The type of the path object, i.e.:
      - File
      - Directory
      - Symbolic Link
      - Hard Link
      - Unknown
    - Filesystem event type, i.e.:
      - Create
      - Modify
      - Destroy
      - OS-Specific Events
      - Unknown
    - The event's timestamp
*/

template <const auto delay_ms = 16>
bool run(const char* path, const auto& callback) {
#if defined(PLATFORM_UNKNOWN)
  using adapter::warthog::run;

#elif defined(PLATFORM_MAC_ANY)
  using adapter::mac::run;

#elif defined(PLATFORM_LINUX_ANY)
  using adapter::lin::run;

#elif defined(PLATFORM_ANDROID_ANY)
  using adapter::andy::run;

#elif defined(PLATFORM_WINDOWS_ANY)
  using adapter::win::run;

#else
  using adapter::warthog::run;
#endif

  static_assert(delay_ms >= 0, "Negative time considered harmful.");

  return run<delay_ms>(path, callback);
}

namespace literal {
using water::watcher::run; /* NOLINT */
} /* namespace literal */
} /* namespace watcher */
} /* namespace water   */
