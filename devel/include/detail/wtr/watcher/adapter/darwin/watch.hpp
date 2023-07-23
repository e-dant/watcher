#pragma once

#if defined(__APPLE__)

#include "wtr/watcher.hpp"
#include <chrono>
#include <CoreServices/CoreServices.h>
#include <cstdio>
#include <filesystem>
#include <functional>
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
  ::wtr::watcher::event::callback const& callback;
  std::unordered_set<std::string>* seen_created_paths;
};

inline constexpr auto delay_ms = std::chrono::milliseconds(16);
inline constexpr auto delay_s =
  std::chrono::duration_cast<std::chrono::seconds>(delay_ms);
inline constexpr auto has_delay = delay_ms.count() > 0;

inline constexpr auto time_flag = kFSEventStreamEventIdSinceNow;

/*  We could OR `event_stream_flags` with `kFSEventStreamCreateFlagNoDefer`
    if we want less "sleepy" time after a period of no filesystem events.
    But we're talking about saving a maximum latency of `delay_ms` after
    some period of inactivity -- very small. */
inline constexpr unsigned event_stream_flags =
  kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagUseExtendedData
  | kFSEventStreamCreateFlagUseCFTypes;

inline constexpr unsigned kFSEventStreamEventFlagItemIsAnyHardLink =
  kFSEventStreamEventFlagItemIsHardlink
  | kFSEventStreamEventFlagItemIsLastHardlink;

inline std::tuple<FSEventStreamRef, dispatch_queue_t>
event_stream_open(std::filesystem::path const& path,
                  FSEventStreamCallback funcptr,
                  argptr_type const& funcptr_args) noexcept
{
  static constexpr CFIndex path_array_size = 1;
  static constexpr auto queue_priority = -10;

  auto funcptr_context =
    FSEventStreamContext{0, (void*)&funcptr_args, nullptr, nullptr, nullptr};
  /*  Creating this untyped array of strings is unavoidable.
      `path_cfstring` and `path_cfarray_cfstring` must be temporaries because
      `CFArrayCreate` takes the address of a string and `FSEventStreamCreate`
      the address of an array (of strings). */
  void const* path_cfstring =
    CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
  CFArrayRef path_array = CFArrayCreate(nullptr,
                                        &path_cfstring,
                                        path_array_size,
                                        &kCFTypeArrayCallBacks);

  /*  The event queue name doesn't seem to need to be unique.
      We try to make a unique name anyway, just in case.
      The event queue name will be:
        = "wtr" + [0, 28) character number
      And will always be a string between 5 and 32-characters long:
        = 3 (prefix) + [1, 28] (digits) + 1 (null char from snprintf) */
  char queue_name[3 + 28 + 1]{};
  std::mt19937 gen(std::random_device{}());
  std::snprintf(queue_name,
                sizeof(queue_name),
                "wtr%zu",
                std::uniform_int_distribution<size_t>(
                  0,
                  std::numeric_limits<size_t>::max())(gen));

  /*  Request a file event stream for `path` from the kernel
      which invokes `funcptr` with `funcptr_context` on events. */
  FSEventStreamRef stream = FSEventStreamCreate(
    nullptr,           /* Custom allocator, optional */
    funcptr,           /* A callable to invoke on changes */
    &funcptr_context,  /* The callable's arguments (context). */
    path_array,        /* The path we were asked to watch */
    time_flag,         /* The time "since when" we receive events */
    delay_s.count(),   /* The time between scans after inactivity */
    event_stream_flags /* The event stream flags */
  );

  /*  Request a (very) high priority queue. */
  dispatch_queue_t queue = dispatch_queue_create(
    queue_name,
    dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL,
                                            QOS_CLASS_USER_INITIATED,
                                            queue_priority));

  FSEventStreamSetDispatchQueue(stream, queue);

  FSEventStreamStart(stream);

  return std::make_tuple(stream, queue);
}

/*  @note
    The functions we use to close the stream and queue take `_Nonnull`
    parameters, so we should be able to take `const&` for our arguments.
    We don't because it would be misleading. `stream` and `queue` are
    eventually null (and always invalid) after the calls we make here.
    @note
    Assuming macOS > 10.8 or iOS > 6.0, we don't need to check for null on the
    dispatch queue after we release it:
      https://developer.apple.com/documentation/dispatch/1496328-dispatch_release
*/
inline bool event_stream_close(
  std::tuple<FSEventStreamRef, dispatch_queue_t>&& resources) noexcept
{
  auto [stream, queue] = resources;
  if (stream) {
    FSEventStreamStop(stream);
    FSEventStreamInvalidate(stream);
    FSEventStreamRelease(stream);
    if (queue) {
      dispatch_release(queue);
      return true;
    }
    else
      return false;
  }
  else
    return false;
}

inline std::filesystem::path path_from_event_at(void* event_recv_paths,
                                                unsigned long i) noexcept
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

  auto cstr =
    CFStringGetCStringPtr(static_cast<CFStringRef>(CFDictionaryGetValue(
                            static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(
                              static_cast<CFArrayRef>(event_recv_paths),
                              static_cast<CFIndex>(i))),
                            kFSEventStreamEventExtendedDataPathKey)),
                          kCFStringEncodingUTF8);

  return cstr ? fs::path{cstr} : fs::path{};
}

/*  @note
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
inline void event_recv(ConstFSEventStreamRef,    /* `ConstFS..` is important */
                       void* arg_ptr,            /* Arguments passed to us */
                       unsigned long recv_count, /* Event count */
                       void* recv_paths,         /* Paths with events */
                       unsigned int const* recv_flags, /* Event flags */
                       FSEventStreamEventId const*     /* event stream id */
                       ) noexcept
{
  using evk = enum ::wtr::watcher::event::kind;
  using evw = enum ::wtr::watcher::event::what;

  if (arg_ptr && recv_paths) {

    auto [callback, seen_created] = *static_cast<argptr_type*>(arg_ptr);

    for (unsigned long i = 0; i < recv_count; i++) {
      auto path = path_from_event_at(recv_paths, i);

      if (! path.empty()) {
        /*  `path` has no hash function, so we use a string. */
        auto path_str = path.string();

        decltype(*recv_flags) flag = recv_flags[i];

        /*  A single path won't have different "kinds". */
        auto k = flag & kFSEventStreamEventFlagItemIsFile    ? evk::file
               : flag & kFSEventStreamEventFlagItemIsDir     ? evk::dir
               : flag & kFSEventStreamEventFlagItemIsSymlink ? evk::sym_link
               : flag & kFSEventStreamEventFlagItemIsAnyHardLink
                 ? evk::hard_link
                 : evk::other;

        /*  More than one thing might have happened to the same path.
            (Which is why we use non-exclusive `if`s.) */
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

} /* namespace */

inline bool watch(std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  using evk = enum ::wtr::watcher::event::kind;
  using evw = enum ::wtr::watcher::event::what;
  using std::this_thread::sleep_for;

  auto seen_created_paths = std::unordered_set<std::string>{};
  auto event_recv_argptr = argptr_type{callback, &seen_created_paths};

  auto stream_resources =
    event_stream_open(path, event_recv, event_recv_argptr);

  while (is_living())
    if constexpr (has_delay) sleep_for(delay_ms);

  auto died_ok = event_stream_close(std::move(stream_resources));

  if (died_ok)
    callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});

  else
    callback({"e/self/die@" + path.string(), evw::destroy, evk::watcher});

  return died_ok;
}

} /* namespace adapter */
} /* namespace watcher */
} /* namespace wtr   */
} /* namespace detail */

#endif
