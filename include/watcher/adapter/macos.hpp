#pragma once

#include <CoreServices/CoreServices.h>
#include <unistd.h>  // isatty()
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

enum class fsevent_type {
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

using fsevent_flag_pair
    = std::pair<FSEventStreamEventFlags, fsevent_type>;

static const vector<fsevent_flag_pair> fsevent_flags{
    {kFSEventStreamEventFlagNone, fsevent_type::other},
    {kFSEventStreamEventFlagMustScanSubDirs,
     fsevent_type::other},
    {kFSEventStreamEventFlagUserDropped,
     fsevent_type::other},
    {kFSEventStreamEventFlagKernelDropped,
     fsevent_type::other},
    {kFSEventStreamEventFlagEventIdsWrapped,
     fsevent_type::other},
    {kFSEventStreamEventFlagHistoryDone,
     fsevent_type::other},
    {kFSEventStreamEventFlagRootChanged,
     fsevent_type::other},
    {kFSEventStreamEventFlagMount, fsevent_type::other},
    {kFSEventStreamEventFlagUnmount, fsevent_type::other},
    {kFSEventStreamEventFlagItemChangeOwner,
     fsevent_type::perm_owner},
    {kFSEventStreamEventFlagItemCreated,
     fsevent_type::path_create},
    {kFSEventStreamEventFlagItemFinderInfoMod,
     fsevent_type::other},
    {kFSEventStreamEventFlagItemFinderInfoMod,
     fsevent_type::attr_modify},
    {kFSEventStreamEventFlagItemInodeMetaMod,
     fsevent_type::attr_modify},
    {kFSEventStreamEventFlagItemIsDir,
     fsevent_type::path_is_dir},
    {kFSEventStreamEventFlagItemIsFile,
     fsevent_type::path_is_file},
    {kFSEventStreamEventFlagItemIsSymlink,
     fsevent_type::path_is_sym_link},
    {kFSEventStreamEventFlagItemModified,
     fsevent_type::path_update},
    {kFSEventStreamEventFlagItemRemoved,
     fsevent_type::path_destroy},
    {kFSEventStreamEventFlagItemRenamed,
     fsevent_type::path_rename},
    {kFSEventStreamEventFlagItemXattrMod,
     fsevent_type::attr_modify},
    {kFSEventStreamEventFlagOwnEvent,
     fsevent_type::attr_modify},
    {kFSEventStreamEventFlagItemIsHardlink,
     fsevent_type::path_is_hard_link},
    {kFSEventStreamEventFlagItemIsLastHardlink,
     fsevent_type::path_is_hard_link},
    {kFSEventStreamEventFlagItemIsLastHardlink,
     fsevent_type::other},
    {kFSEventStreamEventFlagItemCloned,
     fsevent_type::other},
};

void fsevent_callback(
    ConstFSEventStreamRef /* stream_ref (required) */,
    auto* /* callback_info (required for cb) */,
    size_t numEvents, auto* eventPaths,
    const FSEventStreamEventFlags eventFlags[],
    const FSEventStreamEventId*) {
  auto decode_flags = [](FSEventStreamEventFlags flag_recv)
      -> vector<fsevent_type> {
    vector<fsevent_type> evt_flags;
    for (const fsevent_flag_pair& flag_pair :
         fsevent_flags) {
      if (flag_recv & flag_pair.first) {
        evt_flags.push_back(flag_pair.second);
      }
    }
    return evt_flags;
  };

  auto log_event = [](vector<fsevent_type> evs) {
    for (const auto& ev : evs) {
      switch (ev) {
        case (fsevent_type::attr_modify):
          std::cout << "fsevent_type::attr_modify\n";
          break;
        case (fsevent_type::other):
          std::cout << "fsevent_type::other\n";
          break;
        case (fsevent_type::path_create):
          std::cout << "fsevent_type::path_create\n";
          break;
        case (fsevent_type::path_destroy):
          std::cout << "fsevent_type::path_destroy\n";
          break;
        case (fsevent_type::path_is_dir):
          std::cout << "fsevent_type::path_is_dir\n";
          break;
        case (fsevent_type::path_is_file):
          std::cout << "fsevent_type::path_is_file\n";
          break;
        case (fsevent_type::path_is_hard_link):
          std::cout << "fsevent_type::path_is_hard_link\n";
          break;
        case (fsevent_type::path_is_sym_link):
          std::cout << "fsevent_type::path_is_sym_link\n";
          break;
        case (fsevent_type::path_rename):
          std::cout << "fsevent_type::path_rename\n";
          break;
        case (fsevent_type::path_update):
          std::cout << "fsevent_type::path_update\n";
          break;
        case (fsevent_type::perm_owner):
          std::cout << "fsevent_type::perm_owner\n";
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
auto fsevent_create_stream(CFArrayRef& paths) {
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
    return FSEventStreamCreate(nullptr, &fsevent_callback,
                               nullptr, paths, time_flag,
                               delay_s, event_stream_flag);
  };

  if constexpr (delay_ms > 0)
    return mk_stream(delay_ms / 1000.0);
  else
    return mk_stream(0);
}
}  // namespace

template <const auto delay_ms = 16>
inline auto run(const char* path,
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

  vector<CFStringRef> path_container_basic{
      mk_cfstring(path)};

  if (path_container_basic.empty())
    return false;

  CFArrayRef path_container = CFArrayCreate(
      nullptr,
      reinterpret_cast<const void**>(
          &path_container_basic[0]),
      static_cast<CFIndex>(path_container_basic.size()),
      &kCFTypeArrayCallBacks);

  auto event_stream = fsevent_create_stream(path_container);

  if (!event_stream)
    std::cerr << "Event stream could not be created.";

  // Creating dispatch queue
  auto fsevent_queue = dispatch_queue_create(
      "fswatch_event_queue", nullptr);
  FSEventStreamSetDispatchQueue(event_stream,
                                fsevent_queue);

  std::cout << "Starting event stream...\n";
  FSEventStreamStart(event_stream);

  while (true)
    if constexpr (delay_ms > 0)
      sleep_for(milliseconds(delay_ms));

  rm_event_stream(event_stream);
  rm_event_queue(fsevent_queue);

  return true;
}

}  // namespace macos
}  // namespace adapter
}  // namespace watcher
}  // namespace water