#pragma once

#include <CoreServices/CoreServices.h>
#include <unistd.h>  // isatty()
#include <array>
#include <chrono>
#include <cstdio>  // fileno()
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <watcher/concepts.hpp>

namespace water {
namespace watcher {
namespace adapter {
namespace macos {
namespace {
using std::string, std::array, std::pair;

enum class type {
  attr_owner,
  attr_other,
  other,
  path_rename,
  path_update,
  path_create,
  path_destroy,
  path_other,
  path_is_dir,
  path_is_file,
  path_is_hard_link,
  path_is_sym_link,
};

using flag_pair = pair<FSEventStreamEventFlags, type>;

inline constexpr array<flag_pair, 26> flags{
    //clang-format off
    flag_pair(kFSEventStreamEventFlagNone, type::other),
    flag_pair(kFSEventStreamEventFlagMustScanSubDirs, type::other),
    flag_pair(kFSEventStreamEventFlagUserDropped, type::other),
    flag_pair(kFSEventStreamEventFlagKernelDropped, type::other),
    flag_pair(kFSEventStreamEventFlagEventIdsWrapped, type::other),
    flag_pair(kFSEventStreamEventFlagHistoryDone, type::other),
    flag_pair(kFSEventStreamEventFlagRootChanged, type::other),
    flag_pair(kFSEventStreamEventFlagMount, type::other),
    flag_pair(kFSEventStreamEventFlagUnmount, type::other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod, type::other),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, type::other),
    flag_pair(kFSEventStreamEventFlagItemCloned, type::other),

    flag_pair(kFSEventStreamEventFlagItemXattrMod, type::attr_other),
    flag_pair(kFSEventStreamEventFlagOwnEvent, type::attr_other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod, type::attr_other),
    flag_pair(kFSEventStreamEventFlagItemInodeMetaMod, type::attr_other),

    flag_pair(kFSEventStreamEventFlagItemChangeOwner, type::attr_owner),

    /* for now, this block is the important one. */
    flag_pair(kFSEventStreamEventFlagItemCreated, type::path_create),
    flag_pair(kFSEventStreamEventFlagItemModified, type::path_update),
    flag_pair(kFSEventStreamEventFlagItemRemoved, type::path_destroy),
    flag_pair(kFSEventStreamEventFlagItemRenamed, type::path_rename),

    flag_pair(kFSEventStreamEventFlagItemIsDir, type::path_is_dir),
    flag_pair(kFSEventStreamEventFlagItemIsFile, type::path_is_file),
    flag_pair(kFSEventStreamEventFlagItemIsSymlink, type::path_is_sym_link),
    flag_pair(kFSEventStreamEventFlagItemIsHardlink, type::path_is_hard_link),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink,
              type::path_is_hard_link),

    //clang-format on
};

void callback(ConstFSEventStreamRef, /* stream_ref (required) */
              auto*,                 /* callback_info (required for cb) */
              size_t numEvents,
              auto* eventPaths,
              const FSEventStreamEventFlags eventFlags[],
              const FSEventStreamEventId*) {
  auto decode_flags =
      [](const FSEventStreamEventFlags& flag_recv) -> std::vector<type> {
    std::vector<type> evt_flags;
    // this is a slow, dumb search.
    for (const flag_pair& flag_pair : flags) {
      if (flag_recv & flag_pair.first) {
        evt_flags.push_back(flag_pair.second);
      }
    }
    return evt_flags;
  };

  auto log_event = [](const std::vector<type>& evs) {
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
        case (type::path_update):
          std::cout << "type::path_update\n";
          break;
        case (type::path_other):
          std::cout << "type::path_other\n";
          break;
      }
    }
  };

  time_t curr_time;
  time(&curr_time);

  for (size_t i = 0; i < numEvents; ++i) {
    auto path_info_dict = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(
        static_cast<CFArrayRef>(eventPaths), static_cast<CFIndex>(i)));
    auto path = static_cast<CFStringRef>(CFDictionaryGetValue(
        path_info_dict, kFSEventStreamEventExtendedDataPathKey));
    auto cf_inode = static_cast<CFNumberRef>(CFDictionaryGetValue(
        path_info_dict, kFSEventStreamEventExtendedFileIDKey));
    unsigned long inode;
    CFNumberGetValue(cf_inode, kCFNumberLongType, &inode);
    std::cout << "path "
              << std::string(CFStringGetCStringPtr(path, kCFStringEncodingUTF8))
              << " (time/inode " << curr_time << "/" << inode << ")"
              << std::endl;
    log_event(decode_flags(eventFlags[i]));
  }
}

template <const auto delay_ms = 16>
auto create_stream(const CFArrayRef& paths) {
  // std::unique_ptr<FSEventStreamContext> context(
  //     new FSEventStreamContext());
  // context->version         = 0;
  // context->info            = nullptr;
  // context->retain          = nullptr;
  // context->release         = nullptr;
  // context->copyDescription = nullptr;
  const auto event_stream_flag = kFSEventStreamCreateFlagFileEvents |
                                 kFSEventStreamCreateFlagUseExtendedData |
                                 kFSEventStreamCreateFlagUseCFTypes |
                                 kFSEventStreamCreateFlagNoDefer;
  const auto mk_stream = [&](const auto& delay_s) {
    const auto time_flag = kFSEventStreamEventIdSinceNow;
    return FSEventStreamCreate(
        nullptr,           // allocator
        &callback,         // callback; what to do
        nullptr,           // context (see note at top of this fn)
        paths,             // where to watch
        time_flag,         // since when (we choose since now)
        delay_s,           // "stutter" frequency; latency between polls
        event_stream_flag  // what data to gather and how
    );
  };

  if constexpr (delay_ms > 0)
    return mk_stream(delay_ms / 1000.0);
  else
    return mk_stream(0);
}
}  // namespace

template <const auto delay_ms = 16>
inline auto run(const concepts::Path auto& path,
                const concepts::Callback auto& callback) {
  const auto mk_cfstring = [](const auto& p) {
    return CFStringCreateWithCString(nullptr, p, kCFStringEncodingUTF8);
  };
  using std::this_thread::sleep_for, std::chrono::milliseconds;
  const auto alive_os_ev_queue = [](const FSEventStreamRef& event_stream,
                                    const auto& event_queue) {
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
  // the contortions here are to please darwin.
  // old-style cast because using a reinterpret_cast
  // would discard the path reference's const qualifier.
  // importantly, `path_as_refref` and its underlying types
  // *are* const qualified. using this old-style cast is ugly,
  // but it's also ok.
  const array<CFStringRef, 1> path_as_refref{mk_cfstring(path)};
  const auto event_stream = create_stream(CFArrayCreate(
      nullptr,                          // not sure
      (const void**)(&path_as_refref),  // path string(s)
      1,                                // number of paths
      &kCFTypeArrayCallBacks            // callback
      ));
  if (!event_stream)
    return false;
  // request a high priority queue.
  auto event_queue = dispatch_queue_create(
      "water.watcher.event_queue",
      dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL,
                                              QOS_CLASS_USER_INITIATED, -10));
  while (alive_os_ev_queue(event_stream, event_queue))
    if constexpr (delay_ms > 0)
      sleep_for(milliseconds(delay_ms));
  return dead_os_ev_queue(event_stream, event_queue);
}

}  // namespace macos
}  // namespace adapter
}  // namespace watcher
}  // namespace water