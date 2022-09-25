#pragma once

namespace water {
namespace watcher {

/* @todo consider adding platform versions */
enum class platform_t {
  mac_os,
  mac_catalyst,
  mac_ios,
  android,
  windows,
  linux,
  unknown,
};

inline constexpr platform_t platform
/* apple */
#if defined(__APPLE__)
#define PLATFORM_MAC TRUE
#include <TargetConditionals.h>
/* apple target */
#if defined(TARGET_OS_MACCATALYST)
    = platform_t::mac_catalyst;
#define PLATFORM_MAC_CATALYST TRUE
#elif defined(TARGET_OS_MAC)
    = platform_t::mac_os;
#define PLATFORM_MAC_OS TRUE
#elif defined(TARGET_OS_IOS)
    = platform_t::mac_ios;
#define PLATFORM_MAC_IOS TRUE
#endif /* apple target */
/* android */
#elif defined(__ANDROID_API__)
    = platform_t::android;
#define PLATFORM_ANDROID TRUE
/* linux */
#elif defined(__linux__)
    = platform_t::linux;
#define PLATFORM_LINUX TRUE
/* windows */
#elif defined(WIN32)
    = platform_t::windows;
#define PLATFORM_WINDOWS TRUE
#else
    = platform_t::unknown;
#define PLATFORM_UNKNOWN TRUE
#endif

namespace literal {
using                                         /* NOLINT */
    water::watcher::platform,                 /* NOLINT */
    water::watcher::platform_t::unknown,      /* NOLINT */
    water::watcher::platform_t::mac_catalyst, /* NOLINT */
    water::watcher::platform_t::mac_os,       /* NOLINT */
    water::watcher::platform_t::mac_ios,      /* NOLINT */
    water::watcher::platform_t::android,      /* NOLINT */
    water::watcher::platform_t::linux,        /* NOLINT */
    water::watcher::platform_t::windows;      /* NOLINT */
} /* namespace literal */
} /* namespace watcher */
} /* namespace water   */
