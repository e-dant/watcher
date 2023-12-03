#pragma once

#if defined(__APPLE__)

#include "wtr/watcher.hpp"
#include <chrono>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <random>
#include <string>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace {

// clang-format off
inline constexpr unsigned fsev_flag_path_file
  = kFSEventStreamEventFlagItemIsFile;
inline constexpr unsigned fsev_flag_path_dir
  = kFSEventStreamEventFlagItemIsDir;
inline constexpr unsigned fsev_flag_path_sym_link
  = kFSEventStreamEventFlagItemIsSymlink;
inline constexpr unsigned fsev_flag_path_hard_link
  = kFSEventStreamEventFlagItemIsHardlink
  | kFSEventStreamEventFlagItemIsLastHardlink;
inline constexpr unsigned fsev_flag_effect_create
  = kFSEventStreamEventFlagItemCreated;
inline constexpr unsigned fsev_flag_effect_remove
  = kFSEventStreamEventFlagItemRemoved;
inline constexpr unsigned fsev_flag_effect_modify
  = kFSEventStreamEventFlagItemModified
  | kFSEventStreamEventFlagItemInodeMetaMod
  | kFSEventStreamEventFlagItemFinderInfoMod
  | kFSEventStreamEventFlagItemChangeOwner
  | kFSEventStreamEventFlagItemXattrMod;
inline constexpr unsigned fsev_flag_effect_rename
  = kFSEventStreamEventFlagItemRenamed;
inline constexpr unsigned fsev_flag_effect_any
  = fsev_flag_effect_create
  | fsev_flag_effect_remove
  | fsev_flag_effect_modify
  | fsev_flag_effect_rename;
// clang-format on

struct argptr_type {
  using fspath = std::filesystem::path;
  /*  `fs::path` has no hash function, so we use this. */
  using pathset = std::unordered_set<std::string>;
  ::wtr::watcher::event::callback const callback{};
  std::shared_ptr<pathset> seen_created_paths{new pathset{}};
  std::shared_ptr<fspath> last_rename_from_path{new fspath{}};
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
      We'll check anyway, just in case Darwin lies.

      The dictionary contains looks like this:
        { "path": String
        , "fileID": Number
        }
      We can only call `CFStringGetCStringPtr()`
      on the `path` field. Not sure what function
      the `fileID` requires, or if it's different
      from what we'd get from `stat()`. (Is it an
      inode number?) Anyway, we seem to get this:
        -[__NSCFNumber length]: unrecognized ...
      Whenever we try to inspect it with Int or
      CStringPtr functions for CFStringGet...().
      The docs don't say much about these fields.
      I don't think they mention fileID at all.
  */
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

inline auto event_recv_one(
  argptr_type* ctx,
  std::filesystem::path const& path,
  unsigned flags)
{
  using path_type = enum ::wtr::watcher::event::path_type;
  using effect_type = enum ::wtr::watcher::event::effect_type;

  if (
    ! ctx                            // These checks are unfortunate
    || ! ctx->callback               // and absolutely necessary.
    || ! ctx->seen_created_paths     // Once in a while, Darwin will only
    || ! ctx->last_rename_from_path  // give *part* of the context to us.
    ) [[unlikely]]
    return;

  auto [callback, sc_paths, lrf_path] = *ctx;

  auto path_str = path.string();

  /*  A single path won't have different "types". */
  auto pt = flags & fsev_flag_path_file      ? path_type::file
          : flags & fsev_flag_path_dir       ? path_type::dir
          : flags & fsev_flag_path_sym_link  ? path_type::sym_link
          : flags & fsev_flag_path_hard_link ? path_type::hard_link
                                             : path_type::other;

  /*  We want to report odd events (even with an empty path)
      but we can bail early if we don't recognize the effect
      because everything else we do depends on that. */
  if (! (flags & fsev_flag_effect_any)) {
    callback({path, effect_type::other, pt});
    return;
  }

  /*  More than one effect might have happened to the
      same path. (Which is why we use non-exclusive `if`s.) */
  if (flags & fsev_flag_effect_create) {
    auto at = sc_paths->find(path_str);
    if (at == sc_paths->end()) {
      sc_paths->emplace(path_str);
      callback({path, effect_type::create, pt});
    }
  }
  if (flags & fsev_flag_effect_remove) {
    auto at = sc_paths->find(path_str);
    if (at != sc_paths->end()) {
      sc_paths->erase(at);
      callback({path, effect_type::destroy, pt});
    }
  }
  if (flags & fsev_flag_effect_modify) {
    callback({path, effect_type::modify, pt});
  }
  if (flags & fsev_flag_effect_rename) {
    /*  Assumes that the last "renamed-from" path
        if "honestly" correlated to the current
        "rename-to" path.
        For non-destructive rename events, we
        usually receive events in this order:
          1. A rename event on the "from-path"
          2. A rename event on the "to-path"
        As long as that pattern holds, we can
        store the first path in a set, look it
        up, test it against the current path
        for inequality, and check that it no
        longer exists -- In which case, we can
        say that we were renamed from that path
        to the current path.
        We want to store the last rename-from
        path in a set on the heap because the
        rename events might not be batched, and
        we don't want to trample on some other
        watcher with a static.
        This pattern breaks down if there are
        intervening rename events.
        For thoughts on recognizing destructive
        rename events, see this directory's
        notes (in the `notes.md` file).
    */
    auto differs = ! lrf_path->empty() && *lrf_path != path;
    auto missing = access(lrf_path->c_str(), F_OK) == -1;
    if (differs && missing) {
      callback({
        {*lrf_path, effect_type::rename, pt},
        {     path, effect_type::rename, pt}
      });
      lrf_path->clear();
    }
    else {
      *lrf_path = path;
    }
  }
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
  ConstFSEventStreamRef,      /*  `ConstFS..` is important */
  void* ctx,                  /*  Arguments passed to us */
  unsigned long count,        /*  Event count */
  void* paths,                /*  Paths with events */
  unsigned const* flags,      /*  Event flags */
  FSEventStreamEventId const* /*  event stream id */
  ) noexcept -> void
{
  auto ap = static_cast<argptr_type*>(ctx);
  if (paths && flags)
    for (unsigned long i = 0; i < count; i++)
      event_recv_one(ap, path_from_event_at(paths, i), flags[i]);
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
  ::wtr::watcher::event::callback const& callback,
  dispatch_queue_t queue) noexcept -> std::shared_ptr<sysres_type>
{
  using namespace std::chrono_literals;

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

  /*  todo: Do we need to release these?
      CFRelease(path_cfstring);
      CFRelease(path_array);
  */
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
      `fsev_flag_listen | kFSEventStreamCreateFlagNoDefer`.
      We're talking about saving a maximum latency of `delay_s`
      after some period of inactivity, which is not likely to
      be noticeable. I'm not sure what Darwin sets the "period
      of inactivity" to, and I'm not sure it matters. */
  static constexpr unsigned fsev_flag_listen =
    kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagUseExtendedData
    | kFSEventStreamCreateFlagUseCFTypes;

  /*  Request a filesystem event stream for `path` from the
      kernel. The event stream will call `event_recv` with
      `context` and some details about each filesystem event
      the kernel sees for the paths in `path_array`. */
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
    (0.016s).count(),
    /*  The event stream flags */
    fsev_flag_listen);

  if (stream && queue) {
    FSEventStreamSetDispatchQueue(stream, queue);
    FSEventStreamStart(stream);
    sysres->stream = stream;
    return sysres;
  }
  else
    return nullptr;
}

inline auto close_event_stream(std::shared_ptr<sysres_type> const& sr) noexcept
  -> bool
{
  /*  We want to handle any outstanding events before closing,
      so we flush the event stream before stopping it.
      `FSEventStreamInvalidate()` only needs to be called
      if we scheduled via `FSEventStreamScheduleWithRunLoop()`.
      That scheduling function is deprecated (as of macOS 13).
      Calling `FSEventStreamInvalidate()` fails an assertion
      and produces a warning in the console. However, calling
      `FSEventStreamRelease()` without first invalidating via
      `FSEventStreamInvalidate()` *also* fails an assertion,
      and produces a warning. I'm not sure what the right call
      to make here is. */
  return sr->stream
      && (FSEventStreamFlushSync(sr->stream),
          FSEventStreamStop(sr->stream),
          FSEventStreamInvalidate(sr->stream),
          FSEventStreamRelease(sr->stream),
          sr->stream = nullptr,
          true);
}

} /*  namespace */

inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  semabin const& is_living) noexcept -> bool
{
  auto queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  auto sr = open_event_stream(path, callback, queue);
  return sr && dispatch_semaphore_wait(is_living.sema, DISPATCH_TIME_FOREVER),
         close_event_stream(sr);
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr   */
} /*  namespace detail */

#endif
