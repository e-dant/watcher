#pragma once

/* @brief watcher/watcher
 * the public interface.
 * include and use this file. */

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <watcher/concepts.hpp>
#include <watcher/platform.hpp>
#include <watcher/adapter/hog.hpp>
#include <watcher/status.hpp>

namespace water {

namespace watcher {

/* @brief watcher/run
 * @param closure (optional):
 *  A callback to perform when the files
 *  being watched change.
 *  @see Callback
 * Monitors `path_to_watch` for changes.
 * Executes the given closure when they
 * happen. */
template <const auto delay_ms = 16>
bool run(const concepts::Path auto& path,
         const concepts::Callback auto& callback) requires
    std::is_integral_v<decltype(delay_ms)> {
  using water::watcher::platform;
  if constexpr (platform == platform_t::mac_catalyst) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::macos) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::ios) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::android) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::linux) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::windows) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::unknown) {
    return adapter::hog::run<delay_ms>(path, callback);
  } else {
    return adapter::hog::run<delay_ms>(path, callback);
  }
}

}  // namespace watcher
}  // namespace water
