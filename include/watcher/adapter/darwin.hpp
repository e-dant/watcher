#pragma once

/*
  @brief watcher/adapter/darwin

  An efficient adapter for darwin.
*/

#include <CoreServices/CoreServices.h>
#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <watcher/concepts.hpp>
#include <watcher/event.hpp>

namespace water {
namespace watcher {
namespace adapter {
namespace darwin {
namespace {

using flag_pair = std::pair<FSEventStreamEventFlags, event::what>;

// clang-format off
inline constexpr auto flag_pair_count = 26;
inline constexpr std::array<flag_pair, flag_pair_count> flag_pair_container
  {
    /* basic information about what happened to some path.
       for now, this group is the important one. */
    flag_pair(kFSEventStreamEventFlagItemCreated,        event::what::path_create),
    flag_pair(kFSEventStreamEventFlagItemModified,       event::what::path_modify),
    flag_pair(kFSEventStreamEventFlagItemRemoved,        event::what::path_destroy),
    flag_pair(kFSEventStreamEventFlagItemRenamed,        event::what::path_rename),

    // /* path information, i.e. whether the path is a file, directory, etc. */
    // flag_pair(kFSEventStreamEventFlagItemIsDir,          event::what::path_is_dir),
    // flag_pair(kFSEventStreamEventFlagItemIsFile,         event::what::path_is_file),
    // flag_pair(kFSEventStreamEventFlagItemIsSymlink,      event::what::path_is_sym_link),
    // flag_pair(kFSEventStreamEventFlagItemIsHardlink,     event::what::path_is_hard_link),
    // flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, event::what::path_is_hard_link),

    /* path attribute events, such as the owner and some xattr data. */
    flag_pair(kFSEventStreamEventFlagItemXattrMod,       event::what::attr_other),
    flag_pair(kFSEventStreamEventFlagOwnEvent,           event::what::attr_other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::attr_other),
    flag_pair(kFSEventStreamEventFlagItemInodeMetaMod,   event::what::attr_other),
    flag_pair(kFSEventStreamEventFlagItemChangeOwner,    event::what::attr_owner),

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
// clang-format on

template <const auto delay_ms = 16>
auto mk_event_stream(const char* path, const auto& callback) {
  // the contortions here are to please darwin.
  // importantly, `path_as_refref` and its underlying types
  // *are* const qualified. using void** is not ok. but it's also ok.
  const void* _path_ref =
      CFStringCreateWithCString(nullptr, path, kCFStringEncodingUTF8);
  const void** _path_refref{&_path_ref};

  /* We pass along the path we were asked to watch, */
  const auto translated_path = CFArrayCreate(nullptr,       // not sure
                                             _path_refref,  // path string(s)
                                             1,             // number of paths
                                             &kCFTypeArrayCallBacks  // callback
  );
  /* the time point from which we want to monitor events (which is now), */
  const auto time_flag = kFSEventStreamEventIdSinceNow;
  /* the delay, in seconds */
  static constexpr auto delay_s = []() {
    if constexpr (delay_ms > 0)
      return (delay_ms / 1000.0);
    else
      return (0);
  }();

  /* and the event stream flags */
  const auto event_stream_flags = kFSEventStreamCreateFlagFileEvents |
                                  kFSEventStreamCreateFlagUseExtendedData |
                                  kFSEventStreamCreateFlagUseCFTypes |
                                  kFSEventStreamCreateFlagNoDefer;
  /* to the OS, requesting a file event stream which uses our callback. */
  return FSEventStreamCreate(
      nullptr,            // allocator
      callback,           // callback; what to do
      nullptr,            // context (see note [event stream context])
      translated_path,    // where to watch
      time_flag,          // since when (we choose since now)
      delay_s,            // time between fs event scans
      event_stream_flags  // what data to gather and how
  );
}

}  // namespace

template <const auto delay_ms = 16>
inline auto run(const concepts::Path auto& path,
                const concepts::Callback auto& callback) {
  using std::chrono::seconds, std::chrono::milliseconds,
      std::this_thread::sleep_for;
  static auto callback_hook = callback;
  const auto callback_adapter = [](ConstFSEventStreamRef, /* stream_ref
                                                             (required) */
                                   auto*, /* callback_info (required) */
                                   size_t os_event_count, auto* os_event_paths,
                                   const FSEventStreamEventFlags
                                       os_event_flags[],
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
      const auto _path_info_dict = static_cast<CFDictionaryRef>(
          CFArrayGetValueAtIndex(static_cast<CFArrayRef>(os_event_paths),
                                 static_cast<CFIndex>(i)));
      const auto _path_cfstring = static_cast<CFStringRef>(CFDictionaryGetValue(
          _path_info_dict, kFSEventStreamEventExtendedDataPathKey));
      const char* translated_path =
          CFStringGetCStringPtr(_path_cfstring, kCFStringEncodingUTF8);
      /* see note [inode and time] for some extra stuff that can be done
       * here. */
      for (const auto& flag_it : decode_flags(os_event_flags[i]))
        callback_hook(water::watcher::event::event{translated_path, flag_it});
    }
  };

  const auto alive_os_ev_queue = [](const FSEventStreamRef& event_stream,
                                    const auto& event_queue) {
    if (!event_stream || !event_queue)
      return false;
    FSEventStreamSetDispatchQueue(event_stream, event_queue);
    FSEventStreamStart(event_stream);
    return event_queue ? true : false;
  };

  const auto dead_os_ev_queue = [](const FSEventStreamRef& event_stream,
                                   const auto& event_queue) {
    FSEventStreamStop(event_stream);
    FSEventStreamInvalidate(event_stream);
    FSEventStreamRelease(event_stream);
    dispatch_release(event_queue);
    return event_queue ? false : true;
  };

  // const auto cba = callback_adapter{callback};

  const auto event_stream = mk_event_stream<delay_ms>(path, callback_adapter);

  /* request a high priority queue */
  const auto event_queue = dispatch_queue_create(
      "water.watcher.event_queue",
      dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL,
                                              QOS_CLASS_USER_INITIATED, -10));

  while (alive_os_ev_queue(event_stream, event_queue))
    /* this does nothing to affect processing, but this thread doesn't need to
       run an infinite loop aggressively. It can wait, with some latency, until
       the queue stops, and then clean itself up. */
    if constexpr (delay_ms > 0)
      sleep_for(milliseconds(delay_ms));

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

}  // namespace darwin
}  // namespace adapter
}  // namespace watcher
}  // namespace water