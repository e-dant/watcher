#pragma once

// #include <unistd.h>  // isatty()
// #include <cstdio>  // fileno()
// #include <mutex>

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
namespace macos {
namespace {

enum class type {
  attr_owner,
  attr_other,
  other,
  path_rename,
  path_modify,
  path_create,
  path_destroy,
  path_other,
  path_is_dir,
  path_is_file,
  path_is_hard_link,
  path_is_sym_link,
};

using flag_pair = std::pair<FSEventStreamEventFlags, type>;

// clang-format off
inline constexpr auto flag_pair_count = 26;
inline constexpr std::array<flag_pair, flag_pair_count> flag_pair_container
  {
    /* basic information about what happened to some path.
       for now, this group is the important one. */
    flag_pair(kFSEventStreamEventFlagItemCreated,        type::path_create),
    flag_pair(kFSEventStreamEventFlagItemModified,       type::path_modify),
    flag_pair(kFSEventStreamEventFlagItemRemoved,        type::path_destroy),
    flag_pair(kFSEventStreamEventFlagItemRenamed,        type::path_rename),

    /* path information, i.e. whether the path is a file, directory, etc. */
    flag_pair(kFSEventStreamEventFlagItemIsDir,          type::path_is_dir),
    flag_pair(kFSEventStreamEventFlagItemIsFile,         type::path_is_file),
    flag_pair(kFSEventStreamEventFlagItemIsSymlink,      type::path_is_sym_link),
    flag_pair(kFSEventStreamEventFlagItemIsHardlink,     type::path_is_hard_link),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, type::path_is_hard_link),

    /* path attribute events, such as the owner and some xattr data. */
    flag_pair(kFSEventStreamEventFlagItemXattrMod,       type::attr_other),
    flag_pair(kFSEventStreamEventFlagOwnEvent,           type::attr_other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  type::attr_other),
    flag_pair(kFSEventStreamEventFlagItemInodeMetaMod,   type::attr_other),
    flag_pair(kFSEventStreamEventFlagItemChangeOwner,    type::attr_owner),

    /* some edge-cases which may be interesting later on. */
    flag_pair(kFSEventStreamEventFlagNone,               type::other),
    flag_pair(kFSEventStreamEventFlagMustScanSubDirs,    type::other),
    flag_pair(kFSEventStreamEventFlagUserDropped,        type::other),
    flag_pair(kFSEventStreamEventFlagKernelDropped,      type::other),
    flag_pair(kFSEventStreamEventFlagEventIdsWrapped,    type::other),
    flag_pair(kFSEventStreamEventFlagHistoryDone,        type::other),
    flag_pair(kFSEventStreamEventFlagRootChanged,        type::other),
    flag_pair(kFSEventStreamEventFlagMount,              type::other),
    flag_pair(kFSEventStreamEventFlagUnmount,            type::other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  type::other),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, type::other),
    flag_pair(kFSEventStreamEventFlagItemCloned,         type::other),
};
// clang-format on

void dumb_callback(ConstFSEventStreamRef, /* stream_ref (required) */
                   auto*,                 /* callback_info (required for cb) */
                   size_t os_event_count,
                   auto* os_event_paths,
                   const FSEventStreamEventFlags os_event_flags[],
                   const FSEventStreamEventId*) {
  auto decode_flags = [](const FSEventStreamEventFlags& flag_recv) {
    std::vector<type> translation;
    // this is a slow, dumb search.
    for (const flag_pair& it : flag_pair_container)
      if (flag_recv & it.first)
        translation.push_back(it.second);
    return translation;
  };

  auto log_event = [](const std::vector<type>& evs, const char* os_path) {
    std::cout << os_path << ": ";
    for (const auto& ev : evs) {
      switch (ev) {
        case (type::attr_other):
          std::cout << "type::attr_other\n";
          break;
        case (type::attr_owner):
          std::cout << "type::attr_owner\n";
          break;
        case (type::other):
          std::cout << "type::other\n";
          break;
        case (type::path_create):
          std::cout << "type::path_create\n";
          break;
        case (type::path_destroy):
          std::cout << "type::path_destroy\n";
          break;
        case (type::path_is_dir):
          std::cout << "type::path_is_dir\n";
          break;
        case (type::path_is_file):
          std::cout << "type::path_is_file\n";
          break;
        case (type::path_is_hard_link):
          std::cout << "type::path_is_hard_link\n";
          break;
        case (type::path_is_sym_link):
          std::cout << "type::path_is_sym_link\n";
          break;
        case (type::path_rename):
          std::cout << "type::path_rename\n";
          break;
        case (type::path_modify):
          std::cout << "type::path_modify\n";
          break;
        case (type::path_other):
          std::cout << "type::path_other\n";
          break;
      }
    }
  };

  time_t curr_time;
  time(&curr_time);

  for (size_t i = 0; i < os_event_count; ++i) {
    const auto _path_info_dict =
        static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(
            static_cast<CFArrayRef>(os_event_paths), static_cast<CFIndex>(i)));
    const auto _path_cfstring = static_cast<CFStringRef>(CFDictionaryGetValue(
        _path_info_dict, kFSEventStreamEventExtendedDataPathKey));
    const auto translated_path =
        CFStringGetCStringPtr(_path_cfstring, kCFStringEncodingUTF8);
    // auto cf_inode = static_cast<CFNumberRef>(CFDictionaryGetValue(
    //     _path_info_dict, kFSEventStreamEventExtendedFileIDKey));
    // unsigned long inode;
    // CFNumberGetValue(cf_inode, kCFNumberLongType, &inode);
    // std::cout << "_path_cfstring "
    //           << std::string(CFStringGetCStringPtr(_path_cfstring,
    //           kCFStringEncodingUTF8))
    //           << " (time/inode " << curr_time << "/" << inode << ")"
    //           << std::endl;
    log_event(decode_flags(os_event_flags[i]), translated_path);
  }
}

template <const auto delay_ms = 16>
auto mk_event_stream(const char* path) {
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
  std::cout << delay_s << "/delay_s" << std::endl;
  /* and the event stream flags */
  const auto event_stream_flags = kFSEventStreamCreateFlagFileEvents |
                                  kFSEventStreamCreateFlagUseExtendedData |
                                  kFSEventStreamCreateFlagUseCFTypes |
                                  kFSEventStreamCreateFlagNoDefer;
  /* to the OS, in the form of a request for an event stream. */
  return FSEventStreamCreate(
      nullptr,            // allocator
      &dumb_callback,     // callback; what to do
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

  // @todo use callback
  const auto event_stream = mk_event_stream<delay_ms>(path /*, dumb_callback*/);
  // request a high priority queue.
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
    else
      sleep_for(seconds(1));

  return dead_os_ev_queue(event_stream, event_queue);
}

/*
# Notes

## Event Stream Context

To set up a context with some parameters, something like (loosely from
`fswatch`) this could be used:
  ```
  std::unique_ptr<FSEventStreamContext> context(
      new FSEventStreamContext());
  context->version         = 0;
  context->info            = nullptr;
  context->retain          = nullptr;
  context->release         = nullptr;
  context->copyDescription = nullptr;
  ```
*/

}  // namespace macos
}  // namespace adapter
}  // namespace watcher
}  // namespace water