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
using std::string;
using std::vector;

enum class type {
  attr_modify,
  other,
  path_create,
  path_destroy,
  path_is_dir,
  path_is_file,
  path_is_hard_link,
  path_is_sym_link,
  path_rename,
  path_update,
  perm_owner,
};

using flag_pair = std::pair<FSEventStreamEventFlags, type>;

inline constexpr std::array<flag_pair, 26> flags{
    flag_pair(kFSEventStreamEventFlagNone, type::other),
    flag_pair(kFSEventStreamEventFlagMustScanSubDirs,
              type::other),
    flag_pair(kFSEventStreamEventFlagUserDropped,
              type::other),
    flag_pair(kFSEventStreamEventFlagKernelDropped,
              type::other),
    flag_pair(kFSEventStreamEventFlagEventIdsWrapped,
              type::other),
    flag_pair(kFSEventStreamEventFlagHistoryDone,
              type::other),
    flag_pair(kFSEventStreamEventFlagRootChanged,
              type::other),
    flag_pair(kFSEventStreamEventFlagMount, type::other),
    flag_pair(kFSEventStreamEventFlagUnmount, type::other),
    flag_pair(kFSEventStreamEventFlagItemChangeOwner,
              type::perm_owner),
    flag_pair(kFSEventStreamEventFlagItemCreated,
              type::path_create),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,
              type::other),
    flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,
              type::attr_modify),
    flag_pair(kFSEventStreamEventFlagItemInodeMetaMod,
              type::attr_modify),
    flag_pair(kFSEventStreamEventFlagItemIsDir,
              type::path_is_dir),
    flag_pair(kFSEventStreamEventFlagItemIsFile,
              type::path_is_file),
    flag_pair(kFSEventStreamEventFlagItemIsSymlink,
              type::path_is_sym_link),
    flag_pair(kFSEventStreamEventFlagItemModified,
              type::path_update),
    flag_pair(kFSEventStreamEventFlagItemRemoved,
              type::path_destroy),
    flag_pair(kFSEventStreamEventFlagItemRenamed,
              type::path_rename),
    flag_pair(kFSEventStreamEventFlagItemXattrMod,
              type::attr_modify),
    flag_pair(kFSEventStreamEventFlagOwnEvent,
              type::attr_modify),
    flag_pair(kFSEventStreamEventFlagItemIsHardlink,
              type::path_is_hard_link),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink,
              type::path_is_hard_link),
    flag_pair(kFSEventStreamEventFlagItemIsLastHardlink,
              type::other),
    flag_pair(kFSEventStreamEventFlagItemCloned,
              type::other),
};

void callback(
    ConstFSEventStreamRef /* stream_ref (required) */,
    auto* /* callback_info (required for cb) */,
    size_t numEvents, auto* eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId*) {
  auto decode_flags = [](FSEventStreamEventFlags flag_recv)
      -> vector<type> {
    vector<type> evt_flags;
    for (const flag_pair& flag_pair : flags) {
      if (flag_recv & flag_pair.first) {
        evt_flags.push_back(flag_pair.second);
      }
    }
    return evt_flags;
  };

  auto log_event = [](vector<type> evs) {
    for (const auto& ev : evs) {
      switch (ev) {
        case (type::attr_modify):
          std::cout << "type::attr_modify\n";
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
        case (type::perm_owner):
          std::cout << "type::perm_owner\n";
          break;
      }
    }
  };

  time_t curr_time;
  time(&curr_time);

  for (size_t i = 0; i < numEvents; ++i) {
    auto path_info_dict = static_cast<CFDictionaryRef>(
        CFArrayGetValueAtIndex(
            static_cast<CFArrayRef>(eventPaths),
            static_cast<CFIndex>(i)));
    auto path
        = static_cast<CFStringRef>(CFDictionaryGetValue(
            path_info_dict,
            kFSEventStreamEventExtendedDataPathKey));
    auto cf_inode
        = static_cast<CFNumberRef>(CFDictionaryGetValue(
            path_info_dict,
            kFSEventStreamEventExtendedFileIDKey));
    unsigned long inode;
    CFNumberGetValue(cf_inode, kCFNumberLongType, &inode);
    std::cout << "got event at path "
              << std::string(CFStringGetCStringPtr(
                     path, kCFStringEncodingUTF8))
              << " at time " << curr_time << " at inode "
              << inode << std::endl;
    log_event(decode_flags(eventFlags[i]));
  }
}

template <const auto delay_ms = 16>
auto create_stream(CFArrayRef& paths) {
  // std::unique_ptr<FSEventStreamContext> context(
  //     new FSEventStreamContext());
  // context->version         = 0;
  // context->info            = nullptr;
  // context->retain          = nullptr;
  // context->release         = nullptr;
  // context->copyDescription = nullptr;
  const auto event_stream_flag
      = kFSEventStreamCreateFlagFileEvents
        | kFSEventStreamCreateFlagUseExtendedData
        | kFSEventStreamCreateFlagUseCFTypes
        | kFSEventStreamCreateFlagNoDefer;
  const auto mk_stream = [&](const auto& delay_s) {
    std::cout << "Creating FSEvent stream...\n";
    const auto time_flag = kFSEventStreamEventIdSinceNow;
    return FSEventStreamCreate(nullptr, &callback, nullptr,
                               paths, time_flag, delay_s,
                               event_stream_flag);
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
    return CFStringCreateWithCString(nullptr, p,
                                     kCFStringEncodingUTF8);
  };
  using std::this_thread::sleep_for,
      std::chrono::milliseconds;

  const auto rm_event_stream
      = [](FSEventStreamRef& event_stream_ref) {
          std::cout << "Stopping event stream...\n";
          FSEventStreamStop(event_stream_ref);
          std::cout << "Invalidating event stream...\n";
          FSEventStreamInvalidate(event_stream_ref);
          std::cout << "Releasing event stream...\n";
          FSEventStreamRelease(event_stream_ref);
          event_stream_ref = nullptr;
        };

  const auto rm_event_queue = [](const auto& event_queue) {
    dispatch_release(event_queue);
  };

  // vector<CFStringRef>
  std::array<CFStringRef, 1> path_container_basic{
      mk_cfstring(path)};

  if (path_container_basic.empty())
    return false;

  CFArrayRef path_container
      = CFArrayCreate(nullptr,
                      reinterpret_cast<const void**>(
                          &path_container_basic[0]),
                      static_cast<const CFIndex>(
                          path_container_basic.size()),
                      &kCFTypeArrayCallBacks);

  auto event_stream = create_stream(path_container);

  if (!event_stream)
    std::cerr << "Event stream could not be created.";

  // Creating dispatch queue
  auto queue = dispatch_queue_create("fswatch_event_queue",
                                     nullptr);
  FSEventStreamSetDispatchQueue(event_stream, queue);

  std::cout << "Starting event stream...\n";
  FSEventStreamStart(event_stream);

  while (true)
    if constexpr (delay_ms > 0)
      sleep_for(milliseconds(delay_ms));

  rm_event_stream(event_stream);
  rm_event_queue(queue);

  return true;
}

}  // namespace macos
}  // namespace adapter
}  // namespace watcher
}  // namespace water