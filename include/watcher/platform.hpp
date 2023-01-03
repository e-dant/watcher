#pragma once

namespace wtr {
namespace watcher {
namespace detail {

/* @todo add platform versions */
enum class platform_t {
  /* Linux */
  linux_unknown,

  /* Android */
  android,

  /* Darwin */
  mac_catalyst,
  mac_os,
  mac_ios,
  mac_unknown,

  /* Windows */
  windows,

  /* Unkown */
  unknown,
};

/* clang-format off */

inline constexpr platform_t platform

/* linux */
# if defined(__linux__) && !defined(__ANDROID_API__)
    = platform_t::linux_unknown;
#  define WATER_WATCHER_PLATFORM_LINUX_ANY TRUE
#  define WATER_WATCHER_PLATFORM_LINUX_UNKNOWN TRUE

/* android */
# elif defined(__ANDROID_API__)
    = platform_t::android;
#  define WATER_WATCHER_PLATFORM_ANDROID_ANY TRUE
#  define WATER_WATCHER_PLATFORM_ANDROID_UNKNOWN TRUE

/* apple */
# elif defined(__APPLE__)
#  define WATER_WATCHER_PLATFORM_MAC_ANY TRUE
#  include <TargetConditionals.h>
/* apple target */
#  if defined(TARGET_OS_MACCATALYST)
    = platform_t::mac_catalyst;
#  define WATER_WATCHER_PLATFORM_MAC_CATALYST TRUE
#  elif defined(TARGET_OS_MAC)
    = platform_t::mac_os;
#  define WATER_WATCHER_PLATFORM_MAC_OS TRUE
#  elif defined(TARGET_OS_IOS)
    = platform_t::mac_ios;
#  define WATER_WATCHER_PLATFORM_MAC_IOS TRUE
#  else
    = platform_t::mac_unknown;
#  define WATER_WATCHER_PLATFORM_MAC_UNKNOWN TRUE
#  endif /* apple target */

/* windows */
# elif defined(WIN32) || defined(_WIN32)
    = platform_t::windows;
#  define WATER_WATCHER_PLATFORM_WINDOWS_ANY TRUE
#  define WATER_WATCHER_PLATFORM_WINDOWS_UNKNOWN TRUE

/* unknown */
# else
# warning "host platform is unknown"
    = platform_t::unknown;
#  define WATER_WATCHER_PLATFORM_UNKNOWN TRUE
# endif

/* clang-format on */

} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr   */
