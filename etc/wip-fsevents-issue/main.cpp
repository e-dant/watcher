#include <array>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <tuple>

using namespace std;

using Ctx = set<string>;

auto path_from_event_at(void* event_recv_paths, unsigned long i)
  -> filesystem::path
{
  if (event_recv_paths)
    if (
      void const* from_arr = CFArrayGetValueAtIndex(
        static_cast<CFArrayRef>(event_recv_paths),
        static_cast<CFIndex>(i)))
      if (
        void const* from_dict = CFDictionaryGetValue(
          static_cast<CFDictionaryRef>(from_arr),
          kFSEventStreamEventExtendedDataPathKey))
        if (
          char const* as_cstr = CFStringGetCStringPtr(
            static_cast<CFStringRef>(from_dict),
            kCFStringEncodingUTF8))
          return {as_cstr};

  return {};
}

auto event_recv(
  ConstFSEventStreamRef,
  void* maybe_ctx,
  unsigned long count,
  void* paths,
  unsigned const* flags,
  FSEventStreamEventId const*) -> void
{
  if (maybe_ctx && paths && flags) {
    auto ctx = static_cast<Ctx*>(maybe_ctx);
    for (unsigned long i = 0; i < count; i++) {
      auto ev_path = path_from_event_at(paths, i);
      auto [at, inserted] = ctx->insert(ev_path);
      assert(inserted);
      cout << *at << "\n";
    }
  }
}

auto open_event_stream(
  filesystem::path const& path,
  dispatch_queue_t queue,
  void* ctx) -> FSEventStreamRef
{
  auto context = FSEventStreamContext{
    .version = 0,
    .info = ctx,
    .retain = nullptr,
    .release = nullptr,
    .copyDescription = nullptr,
  };

  void const* path_cfstring =
    CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
  auto const path_array =
    CFArrayCreate(nullptr, &path_cfstring, 1, &kCFTypeArrayCallBacks);

  auto fsev_listen_since = kFSEventStreamEventIdSinceNow;
  unsigned fsev_listen_for = kFSEventStreamCreateFlagFileEvents
                           | kFSEventStreamCreateFlagUseExtendedData
                           | kFSEventStreamCreateFlagUseCFTypes;
  FSEventStreamRef stream = FSEventStreamCreate(
    nullptr,
    &event_recv,
    &context,
    path_array,
    fsev_listen_since,
    0.016,
    fsev_listen_for);

  if (stream && queue) {
    FSEventStreamSetDispatchQueue(stream, queue);
    FSEventStreamStart(stream);
    return stream;
  }
  else
    return nullptr;
}

auto close_event_stream(FSEventStreamRef s) -> bool
{
  if (s) {
    FSEventStreamFlushSync(s);
    FSEventStreamStop(s);
    auto event_id = FSEventsGetCurrentEventId();
    auto device = FSEventStreamGetDeviceBeingWatched(s);
    FSEventsPurgeEventsForDeviceUpToEventId(device, event_id);
    FSEventStreamInvalidate(s);
    FSEventStreamRelease(s);
    s = nullptr;
    return true;
  }
  else
    return false;
}

auto main() -> int
{
  auto queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  constexpr auto n_watchers = 5;
  constexpr auto event_count = 500;

  using Watcher = tuple<unique_ptr<FSEventStreamRef>, unique_ptr<Ctx>>;

  // Some number of watchers
  auto watchers = array<Watcher, n_watchers>{};
  for (auto& w : watchers) {
    auto ctx = make_unique<Ctx>();
    auto watcher = make_unique<FSEventStreamRef>(
      open_event_stream("/tmp", queue, ctx.get()));
    w = {std::move(watcher), std::move(ctx)};
  }

  // Some events in the background
  for (auto i = 0; i < event_count; i++) {
    auto path = "/tmp/hi" + to_string(i);
    auto _ = ofstream(path);  // touch
    filesystem::remove(path);
  }

  // Clean up (we never get here during a crash)
  cout << "ok" << endl;
  for (auto& w : watchers) {
    auto& [watcher, ctx] = w;
    auto owned = watcher.release();
    close_event_stream(*owned);
  }

  return 0;
}
