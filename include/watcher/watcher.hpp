#pragma once

/* @brief watcher/watcher
 * the public interface.
 * include and use this file. */

#include <watcher/adapter/hog.hpp>
#include <watcher/adapter/macos.hpp>
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

 @param path: The root path to watch for filesystem events.

 This is an adaptor "switch" that chooses the ideal adaptor
 for the host platform.

 Every adapter monitors `path_to_watch` for changes and
 executes the `closure` when they happen.
*/

template <const auto delay_ms = 16>
bool run(const concepts::Path auto& path,
         const concepts::Callback auto& callback) requires
    std::is_integral_v<decltype(delay_ms)> {
  using water::watcher::platform;
  if constexpr (platform == platform_t::unknown) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::mac_catalyst) {
    return adapter::macos::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::macos) {
    return adapter::macos::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::ios) {
    return adapter::macos::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::android) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::linux) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::windows) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else {
    return adapter::hog::run<delay_ms>(path, callback);
  }
}

}  // namespace watcher
}  // namespace water
