#pragma once

#include <watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_MAC_ANY)

/*
  @brief watcher/adapter/darwin

  The Darwin `FSEvent` adapter.
*/

/* CF*
   kFS*
   FS*
   dispatch_queue* */
#include <CoreServices/CoreServices.h>
/* array */
#include <array>
/* milliseconds */
#include <chrono>
/* function */
#include <functional>
/* path */
#include <filesystem>
/* numeric_limits */
#include <limits>
/* unique_ptr */
#include <memory>
/* random_device
   mt19937
   uniform_int_distribution */
#include <random>
/* string
   to_string */
#include <string>
/* sleep_for */
#include <thread>
/* tuple
   make_tuple */
#include <tuple>
/* vector */
#include <vector>
/* unordered_set */
#include <unordered_set>
/* event
   callback */
#include <watcher/watcher.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

using seen_created_paths_type = std::unordered_set<std::string>;

struct cbcp_type
{
  event::callback const& callback;
  seen_created_paths_type* seen_created_paths;
};

inline constexpr auto delay_ms{std::chrono::milliseconds(16)};
inline constexpr auto delay_s{
    std::chrono::duration_cast<std::chrono::seconds>(delay_ms)};
inline constexpr auto has_delay{delay_ms.count() > 0};
inline constexpr auto flag_what_pair_count{4};
inline constexpr auto flag_kind_pair_count{5};

inline std::string event_queue_name()
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<size_t> dis(0,
                                            std::numeric_limits<size_t>::max());
  return ("wtr.watcher.event_queue." + std::to_string(dis(gen)));
}

inline dispatch_queue_t do_event_queue_create(std::string const& eqn
                                              = event_queue_name())
{
  /* Request a high priority queue */
  dispatch_queue_t event_queue = dispatch_queue_create(
      eqn.c_str(), dispatch_queue_attr_make_with_qos_class(
                       DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, -10));
  return event_queue;
}

inline bool do_event_queue_stream(dispatch_queue_t const& event_queue,
                                  FSEventStreamRef const& event_stream)
{
  if (!event_stream || !event_queue) return false;
  FSEventStreamSetDispatchQueue(event_stream, event_queue);
  FSEventStreamStart(event_stream);
  return event_queue ? true : false;
}

inline void do_event_resources_close(FSEventStreamRef& event_stream,
                                     dispatch_queue_t& event_queue)
{
  FSEventStreamStop(event_stream);
  FSEventStreamInvalidate(event_stream);
  FSEventStreamRelease(event_stream);
  dispatch_release(event_queue);
  /* Assuming macOS > 10.8 or iOS > 6.0,
     we don't need to check for null on the
     dispatch queue (our `event_queue`).
     https://developer.apple.com/documentation
     /dispatch/1496328-dispatch_release */
}

inline FSEventStreamRef do_event_stream_create(
    std::filesystem::path const& path, FSEventStreamContext* callback_context,
    auto const& event_stream_callback_adapter) noexcept
{
  /* The contortions here are to please darwin. The variable
     `path_cfarray_cfstring` and its underlying types should
     still be const-qualified, but I don't know the computer
     origami to ensure that. */
  void const* path_cfstring
      = CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);

  /* We pass along the path we were asked to watch, */
  CFArrayRef path_cfarray_cfstring
      = CFArrayCreate(nullptr,               /* not sure */
                      &path_cfstring,        /* path string(s) */
                      1,                     /* number of paths */
                      &kCFTypeArrayCallBacks /* callback */
      );

  /* the time point from which we want to monitor events (which is now), */
  static constexpr auto time_flag = kFSEventStreamEventIdSinceNow;

  /* and the event stream flags */
  static constexpr auto event_stream_flags
      = kFSEventStreamCreateFlagFileEvents
        | kFSEventStreamCreateFlagUseExtendedData
        | kFSEventStreamCreateFlagUseCFTypes
      // | kFSEventStreamCreateFlagNoDefer
      ;

  /* to the OS, requesting a file event stream which uses our callback. */
  return FSEventStreamCreate(
      nullptr,                       /* Allocator */
      event_stream_callback_adapter, /* `callback` */
      callback_context,              /* State for `callback` */
      path_cfarray_cfstring,         /* Where to watch */
      time_flag,                     /* Since when (we choose since now) */
      delay_s.count(),   /* Time between fs event scans after inactivity */
      event_stream_flags /* What data to gather and how */
  );
}

inline void callback_adapter(
    ConstFSEventStreamRef,                /* `ConstFS..` is important */
    void* callback_context,               /* Context */
    unsigned long event_recv_count,       /* Event count */
    void* event_recv_paths,               /* Paths with events */
    const unsigned int* event_recv_flags, /* Event flags */
    FSEventStreamEventId const* /* event stream id */)
{
  /* @note
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
  auto const& lift_what_kind_pairs
      = [&](std::string const& this_path,
            FSEventStreamEventFlags const& flag_recv,
            seen_created_paths_type* seen_created_paths)
      -> std::vector<std::pair<event::what, event::kind>> {
    std::vector<std::pair<event::what, event::kind>> wks{};
    wks.reserve(flag_what_pair_count + flag_kind_pair_count);

    /* The event's kind doesn't (seem to) vary in batched events.
       (Why would it?) */

    event::kind k = [&flag_recv]() {
      if (flag_recv & kFSEventStreamEventFlagItemIsFile) {
        return event::kind::file;
      } else if (flag_recv & kFSEventStreamEventFlagItemIsDir) {
        return event::kind::dir;
      } else if (flag_recv & kFSEventStreamEventFlagItemIsSymlink) {
        return event::kind::sym_link;
      } else if (flag_recv & kFSEventStreamEventFlagItemIsHardlink) {
        return event::kind::hard_link;
      } else if (flag_recv & kFSEventStreamEventFlagItemIsLastHardlink) {
        return event::kind::hard_link;
      } else {
        return event::kind::other;
      }
    }();

    /* What happened to some path is often batched.
       (Hence the non-exclusive `if`s.) */

    if (flag_recv & kFSEventStreamEventFlagItemCreated) {
      bool seen_created = seen_created_paths->contains(this_path);
      if (!seen_created) {
        seen_created_paths->insert(this_path);
        wks.emplace_back(std::make_pair(event::what::create, k));
      }
    }
    if (flag_recv & kFSEventStreamEventFlagItemRemoved) {
      bool seen_created = seen_created_paths->contains(this_path);
      if (seen_created) {
        seen_created_paths->erase(this_path);
        wks.emplace_back(std::make_pair(event::what::destroy, k));
      }
    }
    if (flag_recv & kFSEventStreamEventFlagItemModified) {
      wks.emplace_back(std::make_pair(event::what::modify, k));
    }
    if (flag_recv & kFSEventStreamEventFlagItemRenamed) {
      wks.emplace_back(std::make_pair(event::what::rename, k));
    }

    return wks;
  };

  cbcp_type* cbcp = (cbcp_type*)callback_context;

  event::callback const& callback = cbcp->callback;

  seen_created_paths_type* seen_created_paths = cbcp->seen_created_paths;

  for (size_t i = 0; i < event_recv_count; i++) {
    std::filesystem::path const& event_recv_path = [&event_recv_paths, &i]() {
      CFStringRef event_recv_path_cfdict = [&event_recv_paths, &i]() {
        CFDictionaryRef event_recv_path_cfarray = static_cast<CFDictionaryRef>(
            CFArrayGetValueAtIndex(static_cast<CFArrayRef>(event_recv_paths),
                                   static_cast<CFIndex>(i)));
        return static_cast<CFStringRef>(CFDictionaryGetValue(
            event_recv_path_cfarray, kFSEventStreamEventExtendedDataPathKey));
      }();
      char const* event_recv_path_cstr = CFStringGetCStringPtr(
          event_recv_path_cfdict, kCFStringEncodingUTF8);
      return std::filesystem::path(event_recv_path_cstr);
    }();

    if (!event_recv_path.empty()) {
      auto wks = lift_what_kind_pairs(event_recv_path.string(),
                                      event_recv_flags[i], seen_created_paths);
      for (auto& wk : wks) {
        callback(
            wtr::watcher::event::event{event_recv_path, wk.first, wk.second});
      }
    }
  }
}

} /* namespace */

inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  auto seen_created_paths = seen_created_paths_type{};

  auto cbcp = cbcp_type{callback, &seen_created_paths};

  auto callback_context
      = FSEventStreamContext{0, (void*)&cbcp, nullptr, nullptr, nullptr};

  auto event_stream
      = do_event_stream_create(path, &callback_context, callback_adapter);

  auto event_queue = do_event_queue_create();

  if (do_event_queue_stream(event_queue, event_stream)) {
    while (is_living())
      if constexpr (has_delay) std::this_thread::sleep_for(delay_ms);

    do_event_resources_close(event_stream, event_queue);

    /* Here, `true` means we were alive at all.
       Most errors are reported through the callback. */
    return true;

  } else
    return false;
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr   */

#endif /* if defined(WATER_WATCHER_PLATFORM_MAC_ANY) */

