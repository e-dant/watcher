#pragma once

#include <watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_MAC_ANY)

/*
  @brief watcher/adapter/darwin

  The Darwin `FSEvent` adapter.
*/

#include <CoreServices/CoreServices.h>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <watcher/adapter/adapter.hpp>
#include <watcher/event.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

using flag_pair = std::pair<FSEventStreamEventFlags, event::what>;

/* clang-format off */
inline constexpr auto flag_pair_count = 4;
inline constexpr std::array<flag_pair, flag_pair_count> flag_pair_container
  {
    /* basic information about what happened to some path.
       this group is the important one.
       See note [Extra Event Flags] */
    flag_pair(kFSEventStreamEventFlagItemCreated,        event::what::create),
    flag_pair(kFSEventStreamEventFlagItemModified,       event::what::modify),
    flag_pair(kFSEventStreamEventFlagItemRemoved,        event::what::destroy),
    flag_pair(kFSEventStreamEventFlagItemRenamed,        event::what::rename),
};
/* clang-format on */

auto do_make_event_stream(auto const& path, auto const& callback)
{
  /*  the contortions here are to please darwin.
      importantly, `path_as_refref` and its underlying types
      *are* const qualified. using void** is not ok. but it's also ok. */
  void const* path_cfstring
      = CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);

  /* We pass along the path we were asked to watch, */
  auto const translated_path
      = CFArrayCreate(nullptr,               /* not sure */
                      &path_cfstring,        /* path string(s) */
                      1,                     /* number of paths */
                      &kCFTypeArrayCallBacks /* callback */
      );

  /* the time point from which we want to monitor events (which is now), */
  auto const time_flag = kFSEventStreamEventIdSinceNow;

  /* the delay, in seconds */
  static constexpr auto delay_s = delay_ms > 0 ? delay_ms / 1000.0 : 0.0;

  /* and the event stream flags */
  auto const event_stream_flags = kFSEventStreamCreateFlagFileEvents
                                  | kFSEventStreamCreateFlagUseExtendedData
                                  | kFSEventStreamCreateFlagUseCFTypes
                                  | kFSEventStreamCreateFlagNoDefer;
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

inline bool watch(auto const& path, event::callback const& callback,
                  auto const& is_living)
{
  using std::chrono::seconds, std::chrono::milliseconds, std::to_string,
      std::string, std::this_thread::sleep_for,
      std::filesystem::is_regular_file, std::filesystem::is_directory,
      std::filesystem::is_symlink, std::filesystem::exists;

  static auto callback_hook = callback;

  auto const callback_adapter
      = [](ConstFSEventStreamRef const,   /* stream_ref */
           auto*,                         /* callback_info */
           size_t const event_recv_count, /* event count */
           auto* event_recv_paths,        /* paths with events */
           FSEventStreamEventFlags const* event_recv_what, /* event flags */
           FSEventStreamEventId const*                     /* event stream id */
        ) {
          auto decode_flags = [](FSEventStreamEventFlags const& flag_recv) {
            std::vector<event::what> translation;
            /* @todo this is a slow, dumb search. fix it. */
            for (flag_pair const& it : flag_pair_container)
              if (flag_recv & it.first) translation.push_back(it.second);
            return translation;
          };

          for (size_t i = 0; i < event_recv_count; i++) {
            char const* event_path = [&]() {
              auto const&& event_path_from_cfdict = [&]() {
                auto const&& event_path_from_cfarray
                    = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(
                        static_cast<CFArrayRef>(event_recv_paths),
                        static_cast<CFIndex>(i)));
                return static_cast<CFStringRef>(CFDictionaryGetValue(
                    event_path_from_cfarray,
                    kFSEventStreamEventExtendedDataPathKey));
              }();
              return CFStringGetCStringPtr(event_path_from_cfdict,
                                           kCFStringEncodingUTF8);
            }();
            /* see note [inode and time]
               for some extra stuff that can be done here. */
            auto const lift_event_kind = [](auto const& path) {
              return exists(path) ? is_regular_file(path) ? event::kind::file
                                    : is_directory(path)  ? event::kind::dir
                                    : is_symlink(path) ? event::kind::sym_link
                                                       : event::kind::other
                                  : event::kind::other;
            };
            for (auto const& what_it : decode_flags(event_recv_what[i]))
              if (event_path != nullptr)
                callback_hook(wtr::watcher::event::event{
                    event_path, what_it, lift_event_kind(event_path)});
          }
        };

  auto const do_make_event_queue = [](char const* event_queue_name) {
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
           dispatch_queue_t const& event_queue) {
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

  /* @todo This should be the mersenne twister */
  auto const event_queue_name
      = ("wtr.watcher.event_queue." + to_string(std::rand()));

  auto const event_stream = do_make_event_stream(path, callback_adapter);
  auto const event_queue = do_make_event_queue(event_queue_name.c_str());

  if (!do_make_event_handler_alive(event_stream, event_queue)) return false;

  while (is_living())
    if constexpr (delay_ms > 0) sleep_for(milliseconds(delay_ms));

  do_make_event_handler_dead(event_stream, event_queue);

  /* We shouldn't call `is_living` more than we need to.
     Bad runs are returned above. */
  return true;
}

inline bool watch(char const* path, event::callback const& callback,
                  auto const& is_living)
{
  return watch(std::string(path), callback, is_living);
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

## Extra Event Flags

```
    // path information, i.e. whether the path is a file, directory, etc.
    // we can get this info much more easily later on in `wtr/watcher/event`.

    // flag_pair(kFSEventStreamEventFlagItemIsDir,          event::what::dir),
    // flag_pair(kFSEventStreamEventFlagItemIsFile,         event::what::file),
    // flag_pair(kFSEventStreamEventFlagItemIsSymlink, event::what::sym_link),
    // flag_pair(kFSEventStreamEventFlagItemIsHardlink, event::what::hard_link),
    // flag_pair(kFSEventStreamEventFlagItemIsLastHardlink,
event::what::hard_link),

    // path attribute events, such as the owner and some xattr data.
    // will be worthwhile soon to implement these.
    // @todo this.
    // flag_pair(kFSEventStreamEventFlagItemXattrMod,       event::what::other),
    // flag_pair(kFSEventStreamEventFlagOwnEvent,           event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemInodeMetaMod,   event::what::other),

    // some edge-cases which may be interesting later on.
    // flag_pair(kFSEventStreamEventFlagNone,               event::what::other),
    // flag_pair(kFSEventStreamEventFlagMustScanSubDirs,    event::what::other),
    // flag_pair(kFSEventStreamEventFlagUserDropped,        event::what::other),
    // flag_pair(kFSEventStreamEventFlagKernelDropped,      event::what::other),
    // flag_pair(kFSEventStreamEventFlagEventIdsWrapped,    event::what::other),
    // flag_pair(kFSEventStreamEventFlagHistoryDone,        event::what::other),
    // flag_pair(kFSEventStreamEventFlagRootChanged,        event::what::other),
    // flag_pair(kFSEventStreamEventFlagMount,              event::what::other),
    // flag_pair(kFSEventStreamEventFlagUnmount,            event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemCloned,         event::what::other),
```

*/

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr   */

#endif /* if defined(WATER_WATCHER_PLATFORM_MAC_ANY) */
