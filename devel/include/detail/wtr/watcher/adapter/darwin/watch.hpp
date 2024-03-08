#pragma once

#if defined(__APPLE__)

#include "wtr/watcher.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace {

// clang-format off

/*  If we want less "sleepy" time after a period of time
    without receiving filesystem events, we could OR like:
    `fsev_flag_listen | kFSEventStreamCreateFlagNoDefer`.
    We're talking about saving a maximum latency of `delay_s`
    after some period of inactivity, which is not likely to
    be noticeable. I'm not sure what Darwin sets the "period
    of inactivity" to, and I'm not sure it matters. */
inline constexpr unsigned fsev_listen_for
  = kFSEventStreamCreateFlagFileEvents
  | kFSEventStreamCreateFlagUseExtendedData
  | kFSEventStreamCreateFlagUseCFTypes;
inline constexpr auto fsev_listen_since
  = kFSEventStreamEventIdSinceNow;
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

struct ContextData {
  using fspath = std::filesystem::path;
  /*  `fs::path` has no hash function, so we use this. */
  using pathset = std::unordered_set<std::string>;

  ::wtr::watcher::event::callback const& callback{};
  pathset* seen_created_paths{nullptr};
  fspath* last_rename_path{nullptr};
  Synchronized::Primitive* in_use{nullptr};
};

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
inline auto path_from_event_at(void* event_recv_paths, unsigned long i)
  -> std::filesystem::path
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

inline auto event_recv_one(
  ContextData& ctx,
  std::filesystem::path const& path,
  unsigned flags)
{
  using ::wtr::watcher::event;
  using path_type = enum ::wtr::watcher::event::path_type;
  using effect_type = enum ::wtr::watcher::event::effect_type;

  auto cb = ctx.callback;

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
    cb({path, effect_type::other, pt});
    return;
  }

  /*  More than one thing can happen to the same path.
      (So these `if`s are not exclusive.) */

  if (flags & fsev_flag_effect_create) {
    auto et = effect_type::create;
    auto at = ctx.seen_created_paths->find(path_str);
    if (at == ctx.seen_created_paths->end()) {
      ctx.seen_created_paths->emplace(path_str);
      cb({path, et, pt});
    }
  }
  if (flags & fsev_flag_effect_remove) {
    auto et = effect_type::destroy;
    auto at = ctx.seen_created_paths->find(path_str);
    if (at != ctx.seen_created_paths->end()) {
      ctx.seen_created_paths->erase(at);
      cb({path, et, pt});
    }
  }
  if (flags & fsev_flag_effect_modify) {
    auto et = effect_type::modify;
    cb({path, et, pt});
  }
  if (flags & fsev_flag_effect_rename) {
    /*  Assumes that the last "renamed-from" path
        is "honestly" correlated to the current
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

    auto et = effect_type::rename;
    auto lr_path = *ctx.last_rename_path;
    auto differs = ! lr_path.empty() && lr_path != path;
    auto missing = access(lr_path.c_str(), F_OK) == -1;
    if (differs && missing) {
      cb({
        {lr_path, et, pt},
        {   path, et, pt}
      });
      ctx.last_rename_path->clear();
    }
    else {
      *ctx.last_rename_path = path;
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
  void* maybe_ctx,            /*  Arguments passed to us */
  unsigned long count,        /*  Event count */
  void* paths,                /*  Paths with events */
  unsigned const* flags,      /*  Event flags */
  FSEventStreamEventId const* /*  A unique stream id */
  ) -> void
{
  auto data_ok = paths      /*  These checks are unfortunate, */
              && flags      /*  but they are also necessary. */
              && maybe_ctx; /*  Once in a blue moon, this doesn't exist */
  if (! data_ok) return;
  auto ctx = *static_cast<ContextData*>(maybe_ctx);
  auto synced = Synchronized::try_from(*ctx.in_use);
  if (! synced) return;
  for (unsigned long i = 0; i < count; i++) {
    auto path = path_from_event_at(paths, i);
    auto flag = flags[i];
    event_recv_one(ctx, path, flag);
  }
}

static_assert(event_recv == FSEventStreamCallback{event_recv});

inline auto open_event_stream(
  std::filesystem::path const& path,
  dispatch_queue_t queue,
  void* ctx) -> FSEventStreamRef
{
  auto context = FSEventStreamContext{
    .version = 0,               /*  FSEvents.h: "Only valid value is zero." */
    .info = ctx,                /*  The context; Our "argument pointer". */
    .retain = nullptr,          /*  Not needed; We manage the lifetimes. */
    .release = nullptr,         /*  Same reason as `.retain` */
    .copyDescription = nullptr, /*  Optional string for debugging. */
  };

  /*  path_cfstring and path_array appear to be managed by the
      FSEventStream, are always null when we go to close the
      stream, and shouldn't be freed before then. Would seem
      to me like we'd need to release them (in re. the Create
      rule), but we don't appear to. */

  void const* path_cfstring =
    CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);

  /*  A predefined structure which is (from CFArray.h) --
      "appropriate when the values in a CFArray are CFTypes" */
  static auto const cf_arr_ty = kCFTypeArrayCallBacks;

  CFArrayRef path_array = CFArrayCreate(
    nullptr,        /*  A custom allocator is optional */
    &path_cfstring, /*  Data: A ptr-ptr of (in our case) strings */
    1,              /*  We're just storing one path here */
    &cf_arr_ty      /*  The type of the data we're storing */
  );

  /*  Request a filesystem event stream for `path` from the
      kernel. The event stream will call `event_recv` with
      `context` and some details about each filesystem event
      the kernel sees for the paths in `path_array`. */

  auto stream = FSEventStreamCreate(
    nullptr,           /*  A custom allocator is optional */
    &event_recv,       /*  A callable to invoke on changes */
    &context,          /*  The callable's arguments (context) */
    path_array,        /*  The path(s) we were asked to watch */
    fsev_listen_since, /*  The time "since when" we watch */
    0.016,             /*  Seconds between scans *after inactivity* */
    fsev_listen_for    /*  Which event types to send up to us */
  );

  if (stream && queue && ctx) {
    FSEventStreamSetDispatchQueue(stream, queue);
    FSEventStreamStart(stream);
    return stream;
  }
  else
    return nullptr;
}

inline auto wait(semabin const& sb)
{
  auto s = sb.state();
  if (s == semabin::pending) {
    dispatch_semaphore_wait(sb.sem, DISPATCH_TIME_FOREVER);
    return semabin::released;
  }
  else
    return s;
}

/*  Bugs, footguns

    1.
    When we flood the filesystem with events, Darwin may choose,
    for reason I don't fully understand, to tell us about events
    after we are long gone. Maybe the FSEvent stream (which should
    be f'ing closed) sometimes ignores us having asking it to stop.
    Maybe some tasks in the dispatch queue are not being cleared
    properly. I'm not sure.

    Creating events as quickly as possible while a few short-lived
    FSEvents streams are open is usually enough to reproduce this.
    The recipe to reproduce seems to be a ton of filesystem events
    ongoing while some FSEvent streams are open, then closing the
    streams either during or shortly after the events stop happening
    to the filesystem, but before they have all been reported.

    A minimal-ish reproducer is in /etc/wip-fsevents-issue.

    Whatever the reason, sometimes, Darwin seems happy call into
    our event handler with resource after we have left the memory
    space those resources belong to. I consider that a bug somewhere
    in FSEvents, Dispatch or maybe something deeper.

    At one point, I thought that purging events for the device
    would help. Even that fails under sufficiently high load.
    The positive side effect may have effectively been a sleep.
    Sometimes I even consider adding a deliberate  sleep here.
    Because time is not a synchronization primitive, that will
    also eventually fail. Though, if it's the best we can do,
    despite the kernel, maybe it's worth it. I'm not sure.

    Before that, I added a bunch of synchronization primitives
    to the context and made the lifetime of the context a bit
    transactional. We'd check on atomic flags which in the event
    reception loop to be sure we were alive there, after which
    we'd set an idle flag which the `watch` function would wait
    on before cleaning up. All well and good, but then again,
    it's not like any of that memory *even exists* when Darwin
    calls into it after we've asked it to stop and left.

    There's a minimal-ish reproducer in /etc/wip-fsevents-issue.

    "Worse is better."

    2.
    We want to handle any outstanding events before closing,
    so we flush the event stream before stopping it.
    `FSEventStreamInvalidate()` only needs to be called
    if we scheduled via `FSEventStreamScheduleWithRunLoop()`.
    That scheduling function is deprecated (as of macOS 13).
    Calling `FSEventStreamInvalidate()` fails an assertion
    and produces a warning in the console. However, calling
    `FSEventStreamRelease()` without first invalidating via
    `FSEventStreamInvalidate()` also fails an assertion and
    produces a warning. Releasing seems safer than not, so
    we'll do that.
*/
inline auto close_event_stream(FSEventStreamRef stream, ContextData& ctx)
  -> bool
{
  if (! stream) return false;
  auto _ = Synchronized::eventually_from(*ctx.in_use);
  FSEventStreamFlushSync(stream);
  FSEventStreamStop(stream);
  FSEventStreamInvalidate(stream);
  FSEventStreamRelease(stream);
  stream = nullptr;
  return true;
}

} /*  namespace */

/*  Lifetimes
    We will ensure that the queue, context and callback
    are alive at least until we close the event stream.
    They are shared between us and the system. The system
    is self-inconsistent but well-meaning. Sometimes, Apple
    will use our resources after we've asked it not to. I
    consider that a bug somewhere in FSEvents, Dispatch or
    maybe something deeper. There's a minimal-ish reproducer
    in the `/etc/wip-fsevents-issue` directory.  */
inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& cb,
  semabin const& is_living) -> bool
{
  static auto queue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);
  auto seen_created_paths = ContextData::pathset{};
  auto last_rename_path = ContextData::fspath{};
  auto in_use = Synchronized::Primitive{};
  auto ctx = ContextData{cb, &seen_created_paths, &last_rename_path, &in_use};

  auto fsevs = open_event_stream(path, queue, &ctx);

  auto state_ok = wait(is_living) == semabin::released;
  auto close_ok = close_event_stream(fsevs, ctx);
  return state_ok && close_ok;
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr   */
} /*  namespace detail */

#endif
