#pragma once

namespace water {
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
#if defined(__linux__)
    = platform_t::linux_unknown;
# define WATER_WATCHER_PLATFORM_LINUX_ANY TRUE
# define WATER_WATCHER_PLATFORM_LINUX_UNKNOWN TRUE

/* android */
#elif defined(__ANDROID_API__)
    = platform_t::android;
# define WATER_WATCHER_PLATFORM_ANDROID_ANY TRUE
# define WATER_WATCHER_PLATFORM_ANDROID_UNKNOWN TRUE

/* apple */
#elif defined(__APPLE__)
# define WATER_WATCHER_PLATFORM_MAC_ANY TRUE
# include <TargetConditionals.h>
/* apple target */
# if defined(TARGET_OS_MACCATALYST)
    = platform_t::mac_catalyst;
# define WATER_WATCHER_PLATFORM_MAC_CATALYST TRUE
# elif defined(TARGET_OS_MAC)
    = platform_t::mac_os;
# define WATER_WATCHER_PLATFORM_MAC_OS TRUE
# elif defined(TARGET_OS_IOS)
    = platform_t::mac_ios;
# define WATER_WATCHER_PLATFORM_MAC_IOS TRUE
# else
    = platform_t::mac_unknown;
# define WATER_WATCHER_PLATFORM_MAC_UNKNOWN TRUE
# endif /* apple target */

/* windows */
#elif defined(WIN32)
    = platform_t::windows;
# define WATER_WATCHER_PLATFORM_WINDOWS_ANY TRUE
# define WATER_WATCHER_PLATFORM_WINDOWS_UNKNOWN TRUE

/* unknown */
#else
    = platform_t::unknown;
# define WATER_WATCHER_PLATFORM_UNKNOWN TRUE
#endif

/* clang-format on */

} /* namespace detail */
} /* namespace watcher */
} /* namespace water   */

/*
  @brief watcher/event

  There are two things the user needs:
    - The `watch` function
    - The `event` object

  The `event` object is used to pass information about
  filesystem events to the (user-supplied) callback
  given to `watch`.

  The `event` object will contain the:
    - Path -- Which is always relative.
    - Path type -- one of:
      - File
      - Directory
      - Symbolic Link
      - Hard Link
      - Unknown
    - Event type -- one of:
      - Create
      - Modify
      - Destroy
      - OS-Specific Events
      - Unknown
    - Event time -- In nanoseconds since epoch

  Happy hacking.
*/

/*
  @brief watcher/event/types
  - water::watcher::event::kind
  - water::watcher::event::what
  - water::watcher::event::event
  - water::watcher::event::callback
*/

/* std::ostream */
#include <ostream>

/* std::chrono::system_clock::now,
   std::chrono::duration_cast,
   std::chrono::system_clock,
   std::chrono::nanoseconds,
   std::chrono::time_point */
#include <chrono>

namespace water {
namespace watcher {
namespace event {

/*
  @brief water/watcher/event/what

  A structure intended to represent
  what has happened to some path
  at the moment of some affecting event.
*/

enum class what {
  /* the essential happenings */
  rename,
  modify,
  create,
  destroy,

  /* extended happenings:
     path attributes */
  owner,

  /* catch-all */
  other,
};

/*
  @brief water/watcher/event/kind

  The essential path types
*/
enum class kind {
  /* the essential path types */
  dir,
  file,
  hard_link,
  sym_link,

  /* the special path types */
  watcher,

  /* catch-all */
  other,
};

struct event {
  /*
    I like these names. Very human.
    'what happen'
    'event kind'
  */
  const char* where;
  const enum what what;
  const enum kind kind;
  const long long when;

  event(const char* where, const enum what happen, const enum kind kind)
      : where{where},

        what{happen},

        kind{kind},

        /* wow! thanks chrono! */
        when{std::chrono::duration_cast<std::chrono::nanoseconds>(
                 std::chrono::time_point<std::chrono::system_clock>{
                     std::chrono::system_clock::now()}
                     .time_since_epoch())
                 .count()} {}

  ~event() noexcept = default;

  /* @brief water/watcher/event/<<

     prints out where, what and kind.
     formats the output as a json object. */
  friend std::ostream& operator<<(std::ostream& os, const event& e) {
    /* clang-format off */
    auto const what_repr = [&]() {
      switch (e.what) {
        case what::rename:  return "rename";
        case what::modify:  return "modify";
        case what::create:  return "create";
        case what::destroy: return "destroy";
        case what::owner:   return "owner";
        case what::other:   return "other";
        default:            return "other";
      }
    }();

    auto const kind_repr = [&]() {
      switch (e.kind) {
        case kind::dir:       return "dir";
        case kind::file:      return "file";
        case kind::hard_link: return "hard_link";
        case kind::sym_link:  return "sym_link";
        case kind::watcher:   return "watcher";
        case kind::other:     return "other";
        default:              return "other";
      }
    }();

    return os << R"(")" << e.when << R"(":)"
              << "{"
                  << R"("where":")" << e.where   << R"(",)"
                  << R"("what":")"  << what_repr << R"(",)"
                  << R"("kind":")"  << kind_repr << R"(")"
              << "}";
    /* clang-format on */
  }
};

/*
  @brief watcher/event/callback

  Ensure the adapters recieve events and return nothing.

  Maybe this should be inferred. I'm not sure.
*/
using callback = void (*)(const event&);

} /* namespace event */
} /* namespace watcher */
} /* namespace water   */


namespace water {
namespace watcher {
namespace detail {
namespace adapter {

static bool watcher_alive = false;

/*
  @brief watcher/adapter/is_living

  A predicate to evaluate whether `watch` should die.

  Likely may be overloaded by the user in the future.
*/

static bool is_living() {
  return watcher_alive ? true : false;
}

/*
  @brief watcher/adapter/can_watch

  Call this before `watch` to ensure only one `watch` exists.

  It might do other things or be removed at some point.
*/

static bool can_watch() {
  if (watcher_alive)
    return false;
  else
    watcher_alive = true;
  return true;
}

/*
  @brief watcher/adapter/die

  Invokes `callback` immediately before destroying itself.
*/

static bool die(event::callback const& callback) {
  callback(event::event{"", event::what::destroy, event::kind::watcher});
  if (watcher_alive) {
    watcher_alive = false;
    return true;
  } else {
    return false;
  }
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace water */

/* clang-format off */




/* clang-format on */

/*
  @brief watcher/adapter/windows

  Work is planned for a `ReadDirectoryChangesW`-based adapter for Windows.
*/

#if defined(WATER_WATCHER_PLATFORM_WINDOWS_ANY)
#define WATER_WATCHER_USE_WARTHOG
#endif


#if defined(WATER_WATCHER_PLATFORM_MAC_ANY)

/*
  @brief watcher/adapter/darwin

  The Darwin `FSEvent` adapter.
*/

#include <CoreServices/CoreServices.h>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace water {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

using flag_pair = std::pair<FSEventStreamEventFlags, event::what>;

/* clang-format off */
inline constexpr auto flag_pair_count = 26;
inline constexpr std::array<flag_pair, flag_pair_count> flag_pair_container
  {
    /* basic information about what happened to some path.
       for now, this group is the important one. */
    flag_pair(kFSEventStreamEventFlagItemCreated,        event::what::create),
    flag_pair(kFSEventStreamEventFlagItemModified,       event::what::modify),
    flag_pair(kFSEventStreamEventFlagItemRemoved,        event::what::destroy),
    flag_pair(kFSEventStreamEventFlagItemRenamed,        event::what::rename),

    /* path information, i.e. whether the path is a file, directory, etc.
       we can get this info much more easily later on in `water/watcher/event`. */
    /* flag_pair(kFSEventStreamEventFlagItemIsDir,          event::what::dir),      */
    /* flag_pair(kFSEventStreamEventFlagItemIsFile,         event::what::file),     */
    /* flag_pair(kFSEventStreamEventFlagItemIsSymlink,      event::what::sym_link), */
    /* flag_pair(kFSEventStreamEventFlagItemIsHardlink,     event::what::hard_link),*/
    /* flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, event::what::hard_link),*/

    /* path attribute events, such as the owner and some xattr data.
       will be worthwhile soon to implement these.
       @todo(next weekend) this. */
    flag_pair(kFSEventStreamEventFlagItemChangeOwner,    event::what::owner),
    flag_pair(kFSEventStreamEventFlagItemXattrMod,       event::what::other),
    flag_pair(kFSEventStreamEventFlagOwnEvent,           event::what::other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::other),
    flag_pair(kFSEventStreamEventFlagItemInodeMetaMod,   event::what::other),

    /* some edge-cases which may be interesting later on. */
    flag_pair(kFSEventStreamEventFlagNone,               event::what::other),
    flag_pair(kFSEventStreamEventFlagMustScanSubDirs,    event::what::other),
    flag_pair(kFSEventStreamEventFlagUserDropped,        event::what::other),
    flag_pair(kFSEventStreamEventFlagKernelDropped,      event::what::other),
    flag_pair(kFSEventStreamEventFlagEventIdsWrapped,    event::what::other),
    flag_pair(kFSEventStreamEventFlagHistoryDone,        event::what::other),
    flag_pair(kFSEventStreamEventFlagRootChanged,        event::what::other),
    flag_pair(kFSEventStreamEventFlagMount,              event::what::other),
    flag_pair(kFSEventStreamEventFlagUnmount,            event::what::other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::other),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, event::what::other),
    flag_pair(kFSEventStreamEventFlagItemCloned,         event::what::other),
};
/* clang-format on */

template <auto const delay_ms = 16>
auto mk_event_stream(const char* path, auto const& callback) {
  /*  the contortions here are to please darwin.
      importantly, `path_as_refref` and its underlying types
      *are* const qualified. using void** is not ok. but it's also ok. */
  const void* _path_ref =
      CFStringCreateWithCString(nullptr, path, kCFStringEncodingUTF8);
  const void** _path_refref{&_path_ref};

  /* We pass along the path we were asked to watch, */
  auto const translated_path =
      CFArrayCreate(nullptr,               /* not sure */
                    _path_refref,          /* path string(s) */
                    1,                     /* number of paths */
                    &kCFTypeArrayCallBacks /* callback */
      );
  /* the time point from which we want to monitor events (which is now), */
  auto const time_flag = kFSEventStreamEventIdSinceNow;
  /* the delay, in seconds */
  static constexpr auto delay_s = []() {
    if constexpr (delay_ms > 0)
      return (delay_ms / 1000.0);
    else
      return (0);
  }();

  /* and the event stream flags */
  auto const event_stream_flags = kFSEventStreamCreateFlagFileEvents |
                                  kFSEventStreamCreateFlagUseExtendedData |
                                  kFSEventStreamCreateFlagUseCFTypes |
                                  kFSEventStreamCreateFlagNoDefer;
  /* to the OS, requesting a file event stream which uses our callback. */
  return FSEventStreamCreate(
      nullptr,           /* allocator */
      callback,          /* callback; what to do */
      nullptr,           /* context (see note [event stream context]) */
      translated_path,   /* where to watch */
      time_flag,         /* since when (we choose since now) */
      delay_s,           /* time between fs event scans */
      event_stream_flags /* what data to gather and how */
  );
}

} /* namespace */

template <auto const delay_ms = 16>
inline bool watch(const char* path, auto const& callback) {
  using std::chrono::seconds, std::chrono::milliseconds,
      std::this_thread::sleep_for, std::filesystem::is_regular_file,
      std::filesystem::is_directory, std::filesystem::is_symlink,
      std::filesystem::exists;
  static auto callback_hook = callback;
  auto const callback_adapter =
      [](ConstFSEventStreamRef, /* stream_ref
                                   (required) */
         auto*,                 /* callback_info (required) */
         size_t os_event_count, auto* os_event_paths,
         const FSEventStreamEventFlags os_event_flags[],
         const FSEventStreamEventId*) {
        auto decode_flags = [](const FSEventStreamEventFlags& flag_recv) {
          std::vector<event::what> translation;
          /* @todo this is a slow, dumb search. fix it. */
          for (const flag_pair& it : flag_pair_container)
            if (flag_recv & it.first)
              translation.push_back(it.second);
          return translation;
        };

        for (decltype(os_event_count) i = 0; i < os_event_count; ++i) {
          auto const _path_info_dict = static_cast<CFDictionaryRef>(
              CFArrayGetValueAtIndex(static_cast<CFArrayRef>(os_event_paths),
                                     static_cast<CFIndex>(i)));
          auto const _path_cfstring =
              static_cast<CFStringRef>(CFDictionaryGetValue(
                  _path_info_dict, kFSEventStreamEventExtendedDataPathKey));
          const char* translated_path =
              CFStringGetCStringPtr(_path_cfstring, kCFStringEncodingUTF8);
          /* see note [inode and time] for some extra stuff that can be done
           * here. */
          auto const discern_kind = [](const char* path) {
            return exists(path) ? is_regular_file(path) ? event::kind::file
                                  : is_directory(path)  ? event::kind::dir
                                  : is_symlink(path)    ? event::kind::sym_link
                                                        : event::kind::other
                                : event::kind::other;
          };
          for (auto const& flag_it : decode_flags(os_event_flags[i]))
            if (translated_path != nullptr)
              callback_hook(water::watcher::event::event{
                  translated_path, flag_it, discern_kind(translated_path)});
        }
      };

  auto const alive_os_ev_queue = [](const FSEventStreamRef& event_stream,
                                    auto const& event_queue) {
    if (!event_stream || !event_queue)
      return false;
    FSEventStreamSetDispatchQueue(event_stream, event_queue);
    FSEventStreamStart(event_stream);
    return event_queue ? true : false;
  };

  auto const dead_os_ev_queue = [](const FSEventStreamRef& event_stream,
                                   auto const& event_queue) {
    FSEventStreamStop(event_stream);
    FSEventStreamInvalidate(event_stream);
    FSEventStreamRelease(event_stream);
    dispatch_release(event_queue);
    return event_queue ? false : true;
  };

  auto const event_stream = mk_event_stream<delay_ms>(path, callback_adapter);

  /* request a high priority queue */
  auto const event_queue = dispatch_queue_create(
      "water.watcher.event_queue",
      dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL,
                                              QOS_CLASS_USER_INITIATED, -10));

  if (alive_os_ev_queue(event_stream, event_queue))
    while (is_living())
      /* this does nothing to affect processing, but this thread doesn't need to
         run an infinite loop aggressively. It can wait, with some latency,
         until the queue stops, and then clean itself up. */
      if constexpr (delay_ms > 0)
        sleep_for(milliseconds(delay_ms));

  // callback(water::watcher::event::event{"", event::what::destroy,
  // event::kind::watcher});
  return dead_os_ev_queue(event_stream, event_queue);
}

/*
# Notes

## Event Stream Context

To set up a context with some parameters, something like this, from the
`fswatch` project repo, could be used:

  ```cpp
  std::unique_ptr<FSEventStreamContext> context(
      new FSEventStreamContext());
  context->version         = 0;
  context->info            = nullptr;
  context->retain          = nullptr;
  context->release         = nullptr;
  context->copyDescription = nullptr;
  ```

## Inode and Time

To grab the inode and time information about an event, something like this, also
from `fswatch`, could be used:

  ```cpp
  time_t curr_time;
  time(&curr_time);
  auto cf_inode = static_cast<CFNumberRef>(CFDictionaryGetValue(
      _path_info_dict, kFSEventStreamEventExtendedFileIDKey));
  unsigned long inode;
  CFNumberGetValue(cf_inode, kCFNumberLongType, &inode);
  std::cout << "_path_cfstring "
            << std::string(CFStringGetCStringPtr(_path_cfstring,
            kCFStringEncodingUTF8))
            << " (time/inode " << curr_time << "/" << inode << ")"
            << std::endl;
  ```
*/

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace water   */

#endif /* if defined(WATER_WATCHER_PLATFORM_MAC_ANY) */

/*
  @brief watcher/adapter/linux

  We are exploring `fanotify` for a more efficient implementation on Linux and Android.
  Until a stable implementation has been made, we will use the `warthog` adapter on these systems.

  These kernel APIs are inaccurate and unstable.

  Work is being done to get most of `warthog`'s accuracy and most of `fanotify`'s efficiency.
*/

#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY)
#define WATER_WATCHER_USE_WARTHOG
#endif

/*
  @brief watcher/adapter/android

  We are exploring `fanotify` for a more efficient implementation on Linux and Android.
  Until a stable implementation has been made, we will use the `warthog` adapter on these systems.

  These kernel APIs are inaccurate and unstable.

  Work is being done to get most of `warthog`'s accuracy and most of `fanotify`'s efficiency.
*/

#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#define WATER_WATCHER_USE_WARTHOG
#endif

#if defined(WATER_WATCHER_PLATFORM_UNKNOWN) || \
    defined(WATER_WATCHER_USE_WARTHOG)

/*
  @brief watcher/adapter/warthog

  A reasonably dumb adapter that works on any platform.

  This adapter beats `kqueue`, but it doesn't bean recieving
  filesystem events directly from the OS.

  This is the fallback adapter on platforms that either
    - Only support `kqueue`
    - Only support the C++ standard library
*/

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>

namespace water {
namespace watcher {
namespace detail {
namespace adapter {
namespace { /* anonymous namespace for "private" things */
/* clang-format off */

inline constexpr std::filesystem::directory_options
  dir_opt = 
    /* This is ridiculous */
    std::filesystem::directory_options::skip_permission_denied 
    & std::filesystem::directory_options::follow_directory_symlink;

using bucket_type = std::unordered_map<std::string, std::filesystem::file_time_type>;

/* clang-format on */

/*  @brief watcher/adapter/warthog/scan
    - Scans `path` for changes.
    - Updates our bucket to match the changes.
    - Calls `send_event` when changes happen.
    - Returns false if the file tree cannot be scanned. */
static bool scan(const char* path,
                 auto const& send_event,
                 bucket_type& bucket) {
  /* @brief watcher/adapter/warthog/scan_file
     - Scans a (single) file for changes.
     - Updates our bucket to match the changes.
     - Calls `send_event` when changes happen.
     - Returns false if the file cannot be scanned. */
  auto const scan_file = [&](const char* file, auto const& send_event) -> bool {
    using std::filesystem::exists, std::filesystem::is_regular_file,
        std::filesystem::last_write_time;
    if (exists(file) && is_regular_file(file)) {
      auto ec = std::error_code{};
      /* grabbing the file's last write time */
      auto const timestamp = last_write_time(file, ec);
      if (ec) {
        /* the file changed while we were looking at it. so, we call the
         * closure, indicating destruction, and remove it from the bucket. */
        send_event(event::event{file, event::what::destroy, event::kind::file});
        if (bucket.contains(file))
          bucket.erase(file);
      }
      /* if it's not in our bucket, */
      else if (!bucket.contains(file)) {
        /* we put it in there and call the closure, indicating creation. */
        bucket[file] = timestamp;
        send_event(event::event{file, event::what::create, event::kind::file});
      }
      /* otherwise, it is already in our bucket. */
      else {
        /* we update the file's last write time, */
        if (bucket[file] != timestamp) {
          bucket[file] = timestamp;
          /* and call the closure on them, indicating modification */
          send_event(
              event::event{file, event::what::modify, event::kind::file});
        }
      }
      return true;
    } /* if the path doesn't exist, we nudge the callee with `false` */
    else
      return false;
  };

  /* @brief watcher/adapter/warthog/scan_directory
     - Scans a (single) directory for changes.
     - Updates our bucket to match the changes.
     - Calls `send_event` when changes happen.
     - Returns false if the directory cannot be scanned. */
  auto const scan_directory = [&](const char* dir,
                                  auto const& send_event) -> bool {
    using std::filesystem::recursive_directory_iterator,
        std::filesystem::is_directory;
    /* if this thing is a directory */
    if (is_directory(dir)) {
      /* try to iterate through its contents */
      auto dir_it_ec = std::error_code{};
      for (auto const& file :
           recursive_directory_iterator(dir, dir_opt, dir_it_ec))
        /* while handling errors */
        if (dir_it_ec)
          return false;
        else
          /* msvc complains without this cast */
          scan_file((const char*)(file.path().c_str()), send_event);
      return true;
    } else
      return false;
  };

  return scan_directory(path, send_event) ? true
         : scan_file(path, send_event)    ? true
                                          : false;
};

/* @brief water/watcher/warthog/tend_bucket
   If the bucket is empty, try to populate it.
   otherwise, prune it. */
static bool tend_bucket(const char* path,
                        auto const& send_event,
                        bucket_type& bucket) {
  /*  @brief watcher/adapter/warthog/populate
      @param path - path to monitor for
      Creates a file map, the "bucket", from `path`. */
  auto const populate = [&](const char* path) -> bool {
    using std::filesystem::exists, std::filesystem::is_directory,
        std::filesystem::recursive_directory_iterator,
        std::filesystem::last_write_time;
    /* this happens when a path was changed while we were reading it.
     there is nothing to do here; we prune later. */
    auto dir_it_ec = std::error_code{};
    auto lwt_ec = std::error_code{};
    if (exists(path)) {
      /* this is a directory */
      if (is_directory(path)) {
        for (auto const& file :
             recursive_directory_iterator(path, dir_opt, dir_it_ec)) {
          if (!dir_it_ec) {
            auto const lwt = last_write_time(file, lwt_ec);
            if (!lwt_ec)
              bucket[file.path().string()] = lwt;
            else
              /* @todo use this practice elsewhere or make a fn for it
                 otherwise, this might be confusing and inconsistent. */
              bucket[file.path().string()] = last_write_time(path);
          }
        }
      }
      /* this is a file */
      else {
        bucket[path] = last_write_time(path);
      }
    } else {
      return false;
    }
    return true;
  };

  /*  @brief watcher/adapter/warthog/prune
      Removes files which no longer exist from our bucket. */
  auto const prune = [&](const char* path, auto const& send_event) -> bool {
    using std::filesystem::exists, std::filesystem::is_regular_file,
        std::filesystem::is_directory, std::filesystem::is_symlink;
    auto bucket_it = bucket.begin();
    /* while looking through the bucket's contents, */
    while (bucket_it != bucket.end()) {
      /* check if the stuff in our bucket exists anymore. */
      exists(bucket_it->first)
          /* if so, move on. */
          ? std::advance(bucket_it, 1)
          /* if not, call the closure, indicating destruction,
             and remove it from our bucket. */
          : [&]() {
              send_event(event::event{bucket_it->first.c_str(),
                                      event::what::destroy,
                                      is_regular_file(path) ? event::kind::file
                                      : is_directory(path)  ? event::kind::dir
                                      : is_symlink(path) ? event::kind::sym_link
                                                         : event::kind::other});
              /* bucket, erase it! */
              bucket_it = bucket.erase(bucket_it);
            }();
    }
    return true;
  };

  return bucket.empty() ? populate(path)            ? true
                          : prune(path, send_event) ? true
                                                    : false
                        : true;
};

} /* namespace */

/*
  @brief watcher/adapter/warthog/watch

  @param path:
   A path to watch for changes.

  @param callback:
   A callback to perform when the files
   being watched change.

  Monitors `path` for changes.

  Calls `callback` with an `event` when they happen.

  Unless it should stop, or errors present, `watch` recurses.
*/
static bool watch(const char* path, event::callback const& callback) {
  using std::this_thread::sleep_for, std::chrono::milliseconds;
  /* First, sleep for delay_ms.

     Then, keep running if:
       - The bucket is doing well, and
       - No errors occured while scanning,

     Otherwise, stop and return false. */

  static constexpr auto delay_ms = 16;
  static bucket_type bucket;

  if constexpr (delay_ms > 0)
    sleep_for(milliseconds(delay_ms));

  return is_living() ? tend_bucket(path, callback, bucket)
                           ? scan(path, callback, bucket)
                                 ? watch(path, callback)
                                 : false
                           : false
                     : true;
  /* clang-format off */
  /*
  // This had issues being called late.
  return is_living()
             ? tend_bucket(path, callback, bucket)
                   ? scan(path, callback, bucket)
                        ? watch(path, callback, is_living)
                        : dying(callback)
                   : dying(callback)
             : dying(callback);
  */
  /* clang-format on */
}

// May be unnecessary for now.
// In the future, we could allow overloads and defaults like this.

// static bool watch(const char* path, event::callback const& callback) {
//   return watch(path, callback,
//                []() -> bool { return watcher_alive ? true : false; });
// }

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace water */

#endif /* if defined(WATER_WATCHER_PLATFORM_UNKNOWN) */


namespace water {
namespace watcher {

/*
  @brief watcher/watch

  Implements `water::watcher::watch`.

  There are two things the user needs:
    - The `watch` function
    - The `event` structure

  That's it, and this is one of them.

  Happy hacking.

  @param callback (optional):
    Something (such as a function or closure) to be called
    whenever events occur in the paths being watched.

  @param path:
    The root path to watch for filesystem events.

  This is an adaptor "switch" that chooses the ideal adaptor
  for the host platform.

  Every adapter monitors `path` for changes and invoked the
  `callback` with an `event` object when they occur.

  The `event` object will contain the:
    - Path -- Which is always relative.
    - Path type -- one of:
      - File
      - Directory
      - Symbolic Link
      - Hard Link
      - Unknown
    - Event type -- one of:
      - Create
      - Modify
      - Destroy
      - OS-Specific Events
      - Unknown
    - Event time -- In nanoseconds since epoch
*/

static bool watch(const char* path, event::callback const& callback) {
  return detail::adapter::can_watch() ? detail::adapter::watch(path, callback)
                                      : false;
}

/*
  @brief watcher/die

  Stops the `watch`.
  Destroys itself.
*/
static bool die() {
  using whatever = const event::event&;
  return detail::adapter::die([](whatever) -> void {});
}

/*
  @brief watcher/die

  Stops the `watch`.
  Calls `callback`,
  then destroys itself.
*/
static bool die(event::callback const& callback) {
  return detail::adapter::die(callback);
}

} /* namespace watcher */
} /* namespace water   */
