#pragma once

namespace water {
namespace watcher {
enum class platform_t {
  macos,
  mac_catalyst,
  ios,
  android,
  windows,
  linux,
  unknown,
};

inline constexpr platform_t platform
// apple
#if defined(__APPLE__)
#include <TargetConditionals.h>
// mac target os
#if defined(TARGET_OS_MACCATALYST)
    = platform_t::mac_catalyst;
#elif defined(TARGET_OS_MAC)
    = platform_t::macos;
#elif defined(TARGET_OS_IOS)
    = platform_t::ios;
#endif  // mac target os
// android
#elif defined(__ANDROID_API__)
    = platform_t::android;
// linux
#elif defined(__linux__)
    = platform_t::linux;
// windows
#elif defined(WIN32)
    = platform_t::windows;
#else
    = platform_t::unknown;
#endif

namespace literal {
using                                          // NOLINT
    water::watcher::platform,                  // NOLINT
    water::watcher::platform_t::unknown,       // NOLINT
    water::watcher::platform_t::mac_catalyst,  // NOLINT
    water::watcher::platform_t::macos,         // NOLINT
    water::watcher::platform_t::ios,           // NOLINT
    water::watcher::platform_t::android,       // NOLINT
    water::watcher::platform_t::linux,         // NOLINT
    water::watcher::platform_t::windows;       // NOLINT
}
}  // namespace watcher
}  // namespace water
