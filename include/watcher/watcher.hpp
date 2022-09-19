#pragma once

/*
  @brief watcher/watcher

  This is the public interface,
  and it's probably what you're looking for.

  Include and use this file.
*/

// #include <functional> @todo invoke
#include <watcher/adapter/darwin.hpp>
#include <watcher/adapter/warthog.hpp>
#include <watcher/concepts.hpp>
#include <watcher/event.hpp>
#include <watcher/platform.hpp>

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
  executes the `closure` when they happen.
*/

template <const auto delay_ms = 16>
bool run(const concepts::Path auto& path,
         const concepts::Callback auto& callback) {
  static_assert(delay_ms >= 0, "Negative time considered harmful.");
  using namespace water::watcher::literal;

  /* ternary if tempting. */

  if constexpr (platform == unknown)
    return adapter::warthog::run<delay_ms>(path, callback);

  else if constexpr (platform == mac_catalyst)
    return adapter::darwin::run<delay_ms>(path, callback);

  else if constexpr (platform == macos)
    return adapter::darwin::run<delay_ms>(path, callback);

  else if constexpr (platform == ios)
    return adapter::darwin::run<delay_ms>(path, callback);

  else if constexpr (platform == android)
    return adapter::warthog::run<delay_ms>(path, callback);

  else if constexpr (platform == linux)
    return adapter::warthog::run<delay_ms>(path, callback);

  else if constexpr (platform == windows)
    return adapter::warthog::run<delay_ms>(path, callback);

  else
    return adapter::warthog::run<delay_ms>(path, callback);
}

namespace literal {
using water::watcher::run;  // NOLINT
}  // namespace literal
}  // namespace watcher
}  // namespace water
