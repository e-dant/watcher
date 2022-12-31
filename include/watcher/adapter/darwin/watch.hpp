#pragma once

#include <watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_MAC_ANY)

/*
  @brief watcher/adapter/darwin

  The Darwin `FSEvent` adapter.
*/

/* CF*
   kFS*
   FS*
   dispatch_queue* */
#include <CoreServices/CoreServices.h>
/* array */
#include <array>
/* milliseconds */
#include <chrono>
/* path */
#include <filesystem>
/* numeric_limits */
#include <limits>
/* random_device
   mt19937
   uniform_int_distribution */
#include <random>
/* string
   to_string */
#include <string>
/* sleep_for */
#include <thread>
/* vector */
#include <vector>
/* unordered_set */
#include <unordered_set>
/* watch */
#include <watcher/adapter/adapter.hpp>
/* event::* */
#include <watcher/event.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

using flag_what_pair_type = std::pair<FSEventStreamEventFlags, event::what>;
using flag_kind_pair_type = std::pair<FSEventStreamEventFlags, event::kind>;

inline constexpr auto delay_ms = 16;
inline constexpr auto flag_what_pair_count = 4;
inline constexpr auto flag_kind_pair_count = 5;

/* basic information about what happened to some path.
   this group is the important one.
   See note [Extra Event Flags] */

/* clang-format off */
inline constexpr std::array<flag_what_pair_type, flag_what_pair_count>
    flag_what_pair{
      flag_what_pair_type(kFSEventStreamEventFlagItemCreated,        event::what::create),
      flag_what_pair_type(kFSEventStreamEventFlagItemModified,       event::what::modify),
      flag_what_pair_type(kFSEventStreamEventFlagItemRemoved,        event::what::destroy),
      flag_what_pair_type(kFSEventStreamEventFlagItemRenamed,        event::what::rename),
    };
/* clang-format on */

/* clang-format off */
inline constexpr std::array<flag_kind_pair_type, flag_kind_pair_count>
    flag_kind_pair{
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsDir,          event::kind::dir),
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsFile,         event::kind::file),
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsSymlink,      event::kind::sym_link),
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsHardlink,     event::kind::hard_link),
      flag_kind_pair_type(kFSEventStreamEventFlagItemIsLastHardlink, event::kind::hard_link),
    };
/* clang-format on */

auto do_make_event_stream(auto const& path, auto const& callback) noexcept
{
  /*  The contortions here are to please darwin.
      importantly, `path_as_refref` and its underlying types
      *are* const qualified. using void** is not ok. but it's also ok. */
  void const* path_cfstring
      = CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);

  /* We pass along the path we were asked to watch, */
  auto const path_cfarray_cfstring
      = CFArrayCreate(nullptr,               /* not sure */
                      &path_cfstring,        /* path string(s) */
                      1,                     /* number of paths */
                      &kCFTypeArrayCallBacks /* callback */
      );

  /* the time point from which we want to monitor events (which is now), */
  static constexpr auto time_flag = kFSEventStreamEventIdSinceNow;

  /* the delay, in seconds */
  static constexpr auto delay_s = delay_ms > 0 ? delay_ms / 1000.0 : 0.0;

  /* and the event stream flags */
  static constexpr auto event_stream_flags
      = kFSEventStreamCreateFlagFileEvents
        | kFSEventStreamCreateFlagUseExtendedData
        | kFSEventStreamCreateFlagUseCFTypes
      // | kFSEventStreamCreateFlagNoDefer
      ;

  /* to the OS, requesting a file event stream which uses our callback. */
  return FSEventStreamCreate(
      nullptr,               /* Allocator */
      callback,              /* Callback; what to do */
      nullptr,               /* Context (see note [event stream context]) */
      path_cfarray_cfstring, /* Where to watch */
      time_flag,             /* Since when (we choose since now) */
      delay_s,               /* Time between fs event scans after inactivity */
      event_stream_flags     /* What data to gather and how */
  );
}

} /* namespace */

inline bool watch(auto const& path, event::callback const& callback,
                  auto const& is_living) noexcept
{
  using seen_created_paths_type = std::unordered_set<std::string>;
  static seen_created_paths_type seen_created_paths{};

  static auto callback_hook = callback;

  auto const callback_adapter
      = [](ConstFSEventStreamRef const,   /* stream_ref */
           void*,                         /* callback_info */
           size_t const event_recv_count, /* event count */
           auto* event_recv_paths,        /* paths with events */
           FSEventStreamEventFlags const* event_recv_flags, /* event flags */
           FSEventStreamEventId const* /* event stream id */
           ) -> void {
    /* @note
       Sometimes events are batched together and re-sent
       (despite having already been sent).
       Example:
         [first batch of events from the os]
         file 'a' created
         -> create event for 'a' is sent
         [some tiny delay, 1 ms or so]
         [second batch of events from the os]
         file 'a' destroyed
         -> create event for 'a' is sent
         -> destroy event for 'a' is sent
       So, we filter out duplicate events when they're sent
       in a batch. We do this by storing and pruning the
       set of paths which we've seen created. */
    auto const& lift_what_kind_pairs
        = [](std::string const& this_path,
              FSEventStreamEventFlags const& flag_recv)
        -> std::vector<std::pair<event::what, event::kind>> {
      std::vector<std::pair<event::what, event::kind>> wks{};
      wks.reserve(flag_what_pair_count + flag_kind_pair_count);

      /* The event's kind doesn't (seem to) vary in batched events.
         (Why would it?) */

      event::kind k = [&flag_recv]() {
        if (flag_recv & kFSEventStreamEventFlagItemIsFile) {
          return event::kind::file;
        } else if (flag_recv & kFSEventStreamEventFlagItemIsDir) {
          return event::kind::dir;
        } else if (flag_recv & kFSEventStreamEventFlagItemIsSymlink) {
          return event::kind::sym_link;
        } else if (flag_recv & kFSEventStreamEventFlagItemIsHardlink) {
          return event::kind::hard_link;
        } else if (flag_recv & kFSEventStreamEventFlagItemIsLastHardlink) {
          return event::kind::hard_link;
        } else {
          return event::kind::other;
        }
      }();

      /* What happened to some path is often batched.
         (Hence the non-exclusive `if`s.) */

      if (flag_recv & kFSEventStreamEventFlagItemCreated) {
        bool seen_created = seen_created_paths.contains(this_path);
        if (!seen_created) {
          seen_created_paths.insert(this_path);
          wks.emplace_back(std::make_pair(event::what::create, k));
        }
      }
      if (flag_recv & kFSEventStreamEventFlagItemRemoved) {
        bool seen_created = seen_created_paths.contains(this_path);
        if (seen_created) {
          seen_created_paths.erase(this_path);
          wks.emplace_back(std::make_pair(event::what::destroy, k));
        }
      }
      if (flag_recv & kFSEventStreamEventFlagItemModified) {
        wks.emplace_back(std::make_pair(event::what::modify, k));
      }
      if (flag_recv & kFSEventStreamEventFlagItemRenamed) {
        wks.emplace_back(std::make_pair(event::what::rename, k));
      }

      return wks;
    };

    for (size_t i = 0; i < event_recv_count; i++) {
      std::filesystem::path const& event_recv_path = [&event_recv_paths, &i]() {
        CFStringRef event_recv_path_cfdict = [&event_recv_paths, &i]() {
          CFDictionaryRef event_recv_path_cfarray
              = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(
                  static_cast<CFArrayRef>(event_recv_paths),
                  static_cast<CFIndex>(i)));
          return static_cast<CFStringRef>(CFDictionaryGetValue(
              event_recv_path_cfarray, kFSEventStreamEventExtendedDataPathKey));
        }();
        char const* event_recv_path_cstr = CFStringGetCStringPtr(
            event_recv_path_cfdict, kCFStringEncodingUTF8);
        return std::filesystem::path(event_recv_path_cstr);
      }();

      if (!event_recv_path.empty()) {
        auto wks = lift_what_kind_pairs(event_recv_path.string(),
                                        event_recv_flags[i]);
        for (auto& wk : wks) {
          callback_hook(
              wtr::watcher::event::event{event_recv_path, wk.first, wk.second});
        }
      }
    }
  };

  auto const do_make_event_queue
      = [](char const* event_queue_name) -> dispatch_queue_t {
    /* Request a high priority queue */
    dispatch_queue_t event_queue = dispatch_queue_create(
        event_queue_name,
        dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL,
                                                QOS_CLASS_USER_INITIATED, -10));
    return event_queue;
  };

  auto const do_make_event_handler_alive
      = [](FSEventStreamRef const& event_stream,
           dispatch_queue_t const& event_queue) -> bool {
    if (!event_stream || !event_queue) return false;
    FSEventStreamSetDispatchQueue(event_stream, event_queue);
    FSEventStreamStart(event_stream);
    return event_queue ? true : false;
  };

  auto const do_make_event_handler_dead
      = [](FSEventStreamRef const& event_stream,
           dispatch_queue_t const& event_queue) -> void {
    FSEventStreamStop(event_stream);
    FSEventStreamInvalidate(event_stream);
    FSEventStreamRelease(event_stream);
    dispatch_release(event_queue);
    /* Assuming macOS > 10.8 or iOS > 6.0,
       we don't need to check for null on the
       dispatch queue (our `event_queue`).
       https://developer.apple.com/documentation
       /dispatch/1496328-dispatch_release */
  };

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<size_t> dis(0,
                                            std::numeric_limits<size_t>::max());

  auto const event_queue_name
      = ("wtr.watcher.event_queue." + std::to_string(dis(gen)));

  auto const event_stream = do_make_event_stream(path, callback_adapter);
  auto const event_queue = do_make_event_queue(event_queue_name.c_str());

  if (!do_make_event_handler_alive(event_stream, event_queue)) return false;

  while (is_living())
    if constexpr (delay_ms > 0)
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

  do_make_event_handler_dead(event_stream, event_queue);

  /* Here, `true` means we were alive at all.
     Errors are handled through the callback. */
  return true;
}

inline bool watch(char const* path, event::callback const& callback,
                  auto const& is_living) noexcept
{
  return watch(std::filesystem::path(path), callback, is_living);
}

/* clang-format off */
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

## Extra Event Flags

```
    // path information, i.e. whether the path is a file, directory, etc.
    // we can get this info much more easily later on in `wtr/watcher/event`.

    flag_pair(kFSEventStreamEventFlagItemIsDir,          event::what::dir),
    flag_pair(kFSEventStreamEventFlagItemIsFile,         event::what::file),
    flag_pair(kFSEventStreamEventFlagItemIsSymlink,      event::what::sym_link),
    flag_pair(kFSEventStreamEventFlagItemIsHardlink,     event::what::hard_link),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, event::what::hard_link),

    // path attribute events, such as the owner and some xattr data.
    // will be worthwhile soon to implement these.
    // @todo this.
    flag_pair(kFSEventStreamEventFlagItemXattrMod,       event::what::other),
    flag_pair(kFSEventStreamEventFlagOwnEvent,           event::what::other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::other),
    flag_pair(kFSEventStreamEventFlagItemInodeMetaMod,   event::what::other),

    // some edge-cases which may be interesting later on.
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

    // for debugging, it may be useful to print everything.
    if (flag_recv & kFSEventStreamEventFlagMount)
      std::cout << "mount" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagUnmount)
      std::cout << "unmount" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagNone)
      std::cout << "none" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemCreated)
      std::cout << "created" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemModified)
      std::cout << "modified" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemRemoved)
      std::cout << "removed" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemRenamed)
      std::cout << "renamed" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemCloned)
      std::cout << "cloned" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagHistoryDone)
      std::cout << "history done" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagKernelDropped)
      std::cout << "kernel dropped" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagUserDropped)
      std::cout << "user dropped" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagOwnEvent)
      std::cout << "own event" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagRootChanged)
      std::cout << "root changed" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagEventIdsWrapped)
      std::cout << "event ids wrapped" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemChangeOwner)
      std::cout << "change owner" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemXattrMod)
      std::cout << "xattr mod" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemFinderInfoMod)
      std::cout << "finder info mod" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagItemInodeMetaMod)
      std::cout << "inode meta mod" << std::endl;
    if (flag_recv & kFSEventStreamEventFlagMustScanSubDirs)
      std::cout << "must scan sub dirs" << std::endl;
```
*/
/* clang-format on */

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr   */

#endif /* if defined(WATER_WATCHER_PLATFORM_MAC_ANY) */
