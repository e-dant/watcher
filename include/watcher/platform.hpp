#pragma once

namespace water {
namespace watcher {

/* @todo consider adding platform versions */
enum class platform_t {
  linux_unknown,

  android,

  mac_catalyst,
  mac_os,
  mac_ios,
  mac_unknown,

  windows,

  unknown,
};

inline constexpr platform_t platform

/* linux */
#if defined(__linux__)
    = platform_t::linux_unknown;
# define PLATFORM_LINUX_ANY TRUE
# define PLATFORM_LINUX_UNKNOWN TRUE

/* android */
#elif defined(__ANDROID_API__)
    = platform_t::android;
# define PLATFORM_ANDROID TRUE

/* apple */
#elif defined(__APPLE__)
# define PLATFORM_MAC_ANY TRUE
# include <TargetConditionals.h>
/* apple target */
# if defined(TARGET_OS_MACCATALYST)
    = platform_t::mac_catalyst;
# define PLATFORM_MAC_CATALYST TRUE
# elif defined(TARGET_OS_MAC)
    = platform_t::mac_os;
# define PLATFORM_MAC_OS TRUE
# elif defined(TARGET_OS_IOS)
    = platform_t::mac_ios;
# define PLATFORM_MAC_IOS TRUE
# else
    = platform_t::mac_unknown;
# define PLATFORM_MAC_UNKNOWN TRUE
# endif /* apple target */

/* windows */
#elif defined(WIN32)
    = platform_t::windows;
# define PLATFORM_WINDOWS TRUE

/* unknown */
#else
    = platform_t::unknown;
# define PLATFORM_UNKNOWN TRUE
#endif

namespace literal {

using                                          /* NOLINT */
    water::watcher::platform,                  /* NOLINT */
    water::watcher::platform_t::mac_unknown,   /* NOLINT */
    water::watcher::platform_t::mac_catalyst,  /* NOLINT */
    water::watcher::platform_t::mac_ios,       /* NOLINT */
    water::watcher::platform_t::mac_os,        /* NOLINT */
    water::watcher::platform_t::android,       /* NOLINT */
    water::watcher::platform_t::linux_unknown, /* NOLINT */
    water::watcher::platform_t::windows,       /* NOLINT */
    water::watcher::platform_t::unknown;       /* NOLINT */

} /* namespace literal */
} /* namespace watcher */
} /* namespace water   */
