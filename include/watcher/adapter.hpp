#pragma once

/* @brief watcher/adapter
 * an adaptor "switch" that chooses the ideal adaptor for
 * the host platform (at compile-time). */

#include <watcher/adapter/hog.hpp>
#include <watcher/concepts.hpp>
#include <watcher/platform.hpp>

namespace water {
namespace watcher {
namespace adapter {

template <const auto delay_ms = 16>
inline bool run(const concepts::Path auto& path,
                const concepts::Callback auto& callback)
  requires std::is_integral_v<decltype(delay_ms)>
{
  using water::watcher::platform;
  if constexpr (platform == platform_t::mac_catalyst) {
    return hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::macos) {
    return hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::ios) {
    return hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::android) {
    return hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::linux) {
    return hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::windows) {
    return hog::run<delay_ms>(path, callback);
  } else if constexpr (platform == platform_t::unknown) {
    return hog::run<delay_ms>(path, callback);
  } else {
    return hog::run<delay_ms>(path, callback);
  }
}

}  // namespace adapter
}  // namespace watcher
}  // namespace water
