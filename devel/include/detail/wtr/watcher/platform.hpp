#pragma once

namespace detail {
namespace wtr {
namespace watcher {

enum class platform_type {
  /* Linux */
  linux_kernel,
  linux_kernel_unknown,

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

inline constexpr platform_type platform

/* linux */
# if defined(__linux__) && !defined(__ANDROID_API__)
#  define WATER_WATCHER_PLATFORM_LINUX_KERNEL_ANY TRUE

/* LINUX_VERSION_CODE
   KERNEL_VERSION */
#  include <linux/version.h>

/* linux >= 5.9.0 */
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_5_9_0
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_UNKNOWN FALSE
#  endif
/* linux >= 2.7.0 */
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 7, 0)
    = platform_type::linux_kernel;
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_2_7_0
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_UNKNOWN FALSE
/* linux unknown */
#  else
    = platform_type::linux_kernel_unknown;
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_UNKNOWN TRUE
#  endif

/* android */
# elif defined(__ANDROID_API__)
    = platform_type::android;
#  define WATER_WATCHER_PLATFORM_ANDROID_ANY TRUE
#  define WATER_WATCHER_PLATFORM_ANDROID_UNKNOWN TRUE

/* apple */
# elif defined(__APPLE__)
#  define WATER_WATCHER_PLATFORM_MAC_ANY TRUE

/* TARGET_OS_* */
#  include <TargetConditionals.h>

/* apple mac catalyst */
#  if defined(TARGET_OS_MACCATALYST)
    = platform_type::mac_catalyst;
#  define WATER_WATCHER_PLATFORM_MAC_CATALYST TRUE
/* apple macos, osx */
#  elif defined(TARGET_OS_MAC)
    = platform_type::mac_os;
#  define WATER_WATCHER_PLATFORM_MAC_OS TRUE
/* apple ios */
#  elif defined(TARGET_OS_IOS)
    = platform_type::mac_ios;
#  define WATER_WATCHER_PLATFORM_MAC_IOS TRUE
/* apple unknown */
#  else
    = platform_type::mac_unknown;
#  define WATER_WATCHER_PLATFORM_MAC_UNKNOWN TRUE
#  endif /* apple target */

/* windows */
# elif defined(WIN32) || defined(_WIN32)
    = platform_type::windows;
#  define WATER_WATCHER_PLATFORM_WINDOWS_ANY TRUE
#  define WATER_WATCHER_PLATFORM_WINDOWS_UNKNOWN TRUE

/* unknown */
# else
# warning "host platform is unknown"
    = platform_type::unknown;
#  define WATER_WATCHER_PLATFORM_UNKNOWN TRUE
# endif

/* clang-format on */

} /* namespace watcher */
} /* namespace wtr   */
} /* namespace detail */
