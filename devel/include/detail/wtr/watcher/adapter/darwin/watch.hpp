#pragma once

#if defined(__APPLE__)

#include "wtr/watcher.hpp"
#include <atomic>
#include <chrono>
#include <CoreServices/CoreServices.h>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace {

struct argptr_type {
  using pathset = std::unordered_set<std::string>;
  ::wtr::watcher::event::callback const callback{};
  std::shared_ptr<pathset> seen_created_paths{new pathset{}};
};

struct sysres_type {
  FSEventStreamRef stream{};
  argptr_type argptr{};
};

inline auto path_from_event_at(void* event_recv_paths, unsigned long i) noexcept
  -> std::filesystem::path
{
  /*  We make a path from a C string...
      In an array, in a dictionary...
      Without type safety...
      Because most of darwin's apis are `void*`-typed.

      We should be guarenteed that nothing in here is
      or can be null, but I'm skeptical. We ask Darwin
      for utf8 strings from a dictionary of utf8 strings
      which it gave us. Nothing should be able to be null.
      We'll check anyway, just in case Darwin lies. */

  namespace fs = std::filesystem;

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
          return fs::path{as_cstr};
  return fs::path{};
}

/*  Sometimes events are batched together and re-sent
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
inline auto event_recv(
  ConstFSEventStreamRef,          /*  `ConstFS..` is important */
  void* argptr,                   /*  Arguments passed to us */
  unsigned long recv_count,       /*  Event count */
  void* recv_paths,               /*  Paths with events */
  unsigned int const* recv_flags, /*  Event flags */
  FSEventStreamEventId const*     /*  event stream id */
  ) noexcept -> void
{
  using evk = enum ::wtr::watcher::event::path_type;
  using evw = enum ::wtr::watcher::event::effect_type;

  static constexpr unsigned any_hard_link =
    kFSEventStreamEventFlagItemIsHardlink
    | kFSEventStreamEventFlagItemIsLastHardlink;

  if (argptr && recv_paths) {

    auto [callback, seen_created] = *static_cast<argptr_type*>(argptr);

    for (unsigned long i = 0; i < recv_count; i++) {
      auto path = path_from_event_at(recv_paths, i);

      if (! path.empty()) {
        /*  `path` has no hash function, so we use a string. */
        auto path_str = path.string();

        decltype(*recv_flags) flag = recv_flags[i];

        /*  A single path won't have different "types". */
        auto k = flag & kFSEventStreamEventFlagItemIsFile    ? evk::file
               : flag & kFSEventStreamEventFlagItemIsDir     ? evk::dir
               : flag & kFSEventStreamEventFlagItemIsSymlink ? evk::sym_link
               : flag & any_hard_link                        ? evk::hard_link
                                                             : evk::other;

        /*  More than one thing might have happened to the
            same path. (Which is why we use non-exclusive `if`s.) */
        if (flag & kFSEventStreamEventFlagItemCreated) {
          if (seen_created->find(path_str) == seen_created->end()) {
            seen_created->emplace(path_str);
            callback({path, evw::create, k});
          }
        }
        if (flag & kFSEventStreamEventFlagItemRemoved) {
          auto const& seen_created_at = seen_created->find(path_str);
          if (seen_created_at != seen_created->end()) {
            seen_created->erase(seen_created_at);
            callback({path, evw::destroy, k});
          }
        }
        if (flag & kFSEventStreamEventFlagItemModified) {
          callback({path, evw::modify, k});
        }
        if (flag & kFSEventStreamEventFlagItemRenamed) {
          callback({path, evw::rename, k});
        }
      }
    }
  }
}

/*  Make sure that event_recv has the same type as, or is
    convertible to, an FSEventStreamCallback. We don't use
    `is_same_v()` here because `event_recv` is `noexcept`.
    Side note: Is an exception qualifier *really* part of
    the type? Or, is it a "path_type"? Something else?
    We want this assertion for nice compiler errors. */
static_assert(FSEventStreamCallback{event_recv} == event_recv);

inline auto open_event_stream(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback) noexcept
  -> std::tuple<std::shared_ptr<sysres_type>, bool>
{
  using namespace std::chrono_literals;
  using std::chrono::duration_cast, std::chrono::seconds;

  auto sysres =
    std::make_shared<sysres_type>(sysres_type{nullptr, argptr_type{callback}});

  auto context = FSEventStreamContext{
    /*  FSEvents.h:
        "Currently the only valid value is zero." */
    .version = 0,
    /*  The "actual" context; our "argument pointer".
        This must be alive until we clear the event stream. */
    .info = static_cast<void*>(&sysres->argptr),
    /*  An optional "retention" callback. We don't need this
        because we manage `argptr`'s lifetime ourselves. */
    .retain = nullptr,
    /*  An optional "release" callback, not needed for the same
        reasons as the retention callback. */
    .release = nullptr,
    /*  An optional string for debugging purposes. */
    .copyDescription = nullptr,
  };

  void const* path_cfstring =
    CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
  CFArrayRef path_array = CFArrayCreate(
    /*  A custom allocator is optional */
    nullptr,
    /*  Data: A pointer-to-pointer of (in our case) strings */
    &path_cfstring,
    /*  We're just storing one path here */
    1,
    /*  A predefined structure which is "appropriate for use
        when the values in a CFArray are all CFTypes" (CFArray.h) */
    &kCFTypeArrayCallBacks);

  /*  If we want less "sleepy" time after a period of time
      without receiving filesystem events, we could OR like:
      `event_stream_flags | kFSEventStreamCreateFlagNoDefer`.
      We're talking about saving a maximum latency of `delay_s`
      after some period of inactivity, which is not likely to
      be noticeable. I'm not sure what Darwin sets the "period
      of inactivity" to, and I'm not sure it matters. */
  static constexpr unsigned event_stream_flags =
    kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagUseExtendedData
    | kFSEventStreamCreateFlagUseCFTypes;

  /*  Request a filesystem event stream for `path` from the
      kernel. The event stream will call `event_recv` with
      `context` and some details about each filesystem event
      the kernel sees for the paths in `path_array`. */
  if (
    FSEventStreamRef stream = FSEventStreamCreate(
      /*  A custom allocator is optional */
      nullptr,
      /*  A callable to invoke on changes */
      &event_recv,
      /*  The callable's arguments (context) */
      &context,
      /*  The path(s) we were asked to watch */
      path_array,
      /*  The time "since when" we receive events */
      kFSEventStreamEventIdSinceNow,
      /*  The time between scans *after inactivity* */
      duration_cast<seconds>(16ms).count(),
      /*  The event stream flags */
      event_stream_flags)) {
    FSEventStreamSetDispatchQueue(
      stream,
      /*  We don't need to retain, maintain or release this
          dispatch queue. It's a global system queue, and it
          outlives us. */
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

    FSEventStreamStart(stream);

    sysres->stream = stream;

    return {sysres, true};
  }
  else {
    return {{}, false};
  }
}

inline auto
close_event_stream(std::shared_ptr<sysres_type> const& sysres) noexcept -> bool
{
  if (sysres->stream) {
    /*  We want to handle any outstanding events before closing. */
    FSEventStreamFlushSync(sysres->stream);
    /*  `FSEventStreamInvalidate()` only needs to be called
        if we scheduled via `FSEventStreamScheduleWithRunLoop()`.
        That scheduling function is deprecated (as of macOS 13).
        Calling `FSEventStreamInvalidate()` fails an assertion
        and produces a warning in the console. However, calling
        `FSEventStreamRelease()` without first invalidating via
        `FSEventStreamInvalidate()` *also* fails an assertion,
        and produces a warning. I'm not sure what the right call
        to make here is. */
    FSEventStreamStop(sysres->stream);
    FSEventStreamInvalidate(sysres->stream);
    FSEventStreamRelease(sysres->stream);
    sysres->stream = nullptr;
    return true;
  }
  else
    return false;
}

inline auto block_while(std::atomic<bool>& b)
{
  using namespace std::chrono_literals;
  using std::this_thread::sleep_for;

  while (b) sleep_for(16ms);
}

} /*  namespace */

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  std::atomic<bool>& is_living) noexcept -> bool
{
  auto&& [sysres, ok] = open_event_stream(path, callback);
  return ok ? (block_while(is_living), close_event_stream(sysres)) : false;
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr   */
} /*  namespace detail */

#endif
