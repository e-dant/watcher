#ifndef W973564ED9F278A21F3E12037288412FBAF175F889
#define W973564ED9F278A21F3E12037288412FBAF175F889

namespace wtr {
namespace watcher {
namespace detail {

/* @todo add platform versions */
enum class platform_t {
  /* Linux */
  linux_unknown,

  /* Android */
  android,

  /* Darwin */
  mac_catalyst,
  mac_os,
  mac_ios,
  mac_unknown,

  /* Windows */
  windows,

  /* Unkown */
  unknown,
};

/* clang-format off */

inline constexpr platform_t platform

/* linux */
# if defined(__linux__) && !defined(__ANDROID_API__)
    = platform_t::linux_unknown;
#  define WATER_WATCHER_PLATFORM_LINUX_ANY TRUE
#  define WATER_WATCHER_PLATFORM_LINUX_UNKNOWN TRUE

/* android */
# elif defined(__ANDROID_API__)
    = platform_t::android;
#  define WATER_WATCHER_PLATFORM_ANDROID_ANY TRUE
#  define WATER_WATCHER_PLATFORM_ANDROID_UNKNOWN TRUE

/* apple */
# elif defined(__APPLE__)
#  define WATER_WATCHER_PLATFORM_MAC_ANY TRUE
#  include <TargetConditionals.h>
/* apple target */
#  if defined(TARGET_OS_MACCATALYST)
    = platform_t::mac_catalyst;
#  define WATER_WATCHER_PLATFORM_MAC_CATALYST TRUE
#  elif defined(TARGET_OS_MAC)
    = platform_t::mac_os;
#  define WATER_WATCHER_PLATFORM_MAC_OS TRUE
#  elif defined(TARGET_OS_IOS)
    = platform_t::mac_ios;
#  define WATER_WATCHER_PLATFORM_MAC_IOS TRUE
#  else
    = platform_t::mac_unknown;
#  define WATER_WATCHER_PLATFORM_MAC_UNKNOWN TRUE
#  endif /* apple target */

/* windows */
# elif defined(WIN32) || defined(_WIN32)
    = platform_t::windows;
#  define WATER_WATCHER_PLATFORM_WINDOWS_ANY TRUE
#  define WATER_WATCHER_PLATFORM_WINDOWS_UNKNOWN TRUE

/* unknown */
# else
# warning "host platform is unknown"
    = platform_t::unknown;
#  define WATER_WATCHER_PLATFORM_UNKNOWN TRUE
# endif

/* clang-format on */

} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr   */

/* @brief watcher/event
   There are only three things the user needs:
    - The `die` function
    - The `watch` function
    - The `event` object

   The `event` object is used to pass information about
   filesystem events to the (user-supplied) callback
   given to `watch`.

   The `event` object will contain the:
     - Path, which is always absolute.
     - Type, one of:
       - dir
       - file
       - hard_link
       - sym_link
       - watcher
       - other
     - Event type, one of:
       - rename
       - modify
       - create
       - destroy
       - owner
       - other
     - Event time in nanoseconds since epoch

   The `watcher` type is special.
   Events with this type will include messages from
   the watcher. You may recieve error messages or
   important status updates.

   Happy hacking. */

/* - std::ostream */
#include <ostream>

/* - std::chrono::system_clock::now,
   - std::chrono::duration_cast,
   - std::chrono::system_clock,
   - std::chrono::nanoseconds,
   - std::chrono::time_point */
#include <chrono>

/* - std::filesystem::path */
#include <filesystem>

/* - std::function */
#include <functional>

namespace wtr {
namespace watcher {
namespace event {

namespace {
using std::function, std::chrono::duration_cast, std::chrono::nanoseconds,
    std::chrono::time_point, std::chrono::system_clock;
} /* namespace */

/* @brief watcher/event/types
   - wtr::watcher::event
   - wtr::watcher::event::kind
   - wtr::watcher::event::what
   - wtr::watcher::event::callback */

/* @brief wtr/watcher/event/what
   A structure intended to represent
   what has happened to some path
   at the moment of some affecting event. */
enum class what {
  /* The essentials */
  rename,
  modify,
  create,
  destroy,

  /* The attributes */
  owner,

  /* Catch-all */
  other,
};

/* @brief wtr/watcher/event/kind
   The essential kinds of paths. */
enum class kind {
  /* The essentials */
  dir,
  file,
  hard_link,
  sym_link,

  /* The specials */
  watcher,

  /* Catch-all */
  other,
};

namespace {
inline auto what_repr(enum what const& w)
{
  switch (w) {
    case what::rename: return "rename";
    case what::modify: return "modify";
    case what::create: return "create";
    case what::destroy: return "destroy";
    case what::owner: return "owner";
    case what::other: return "other";
    default: return "other";
  }
}

inline auto kind_repr(enum kind const& k)
{
  switch (k) {
    case kind::dir: return "dir";
    case kind::file: return "file";
    case kind::hard_link: return "hard_link";
    case kind::sym_link: return "sym_link";
    case kind::watcher: return "watcher";
    case kind::other: return "other";
    default: return "other";
  }
}
} /* namespace */

struct event
{
  /* I like these names. Very human.
     'what happen'
     'event kind' */
  const std::filesystem::path where;
  const enum what what;
  const enum kind kind;
  const long long when{
      duration_cast<nanoseconds>(
          time_point<system_clock>{system_clock::now()}.time_since_epoch())
          .count()};

  event(std::filesystem::path const where, enum what const what,
        enum kind const kind) noexcept
      : where{where}, what{what}, kind{kind} {};

  ~event() noexcept = default;

  /* @brief wtr/watcher/event/==
     Compares event objects for equivalent
     `where`, `what` and `kind` values. */
  friend bool operator==(event const& lhs, event const& rhs) noexcept
  {
    /* True if */
    return
        /* The path */
        lhs.where == rhs.where
        /* And what happened */
        && lhs.what == rhs.what
        /* And the kind of path */
        && lhs.kind == rhs.kind
        /* And the time */
        && lhs.when == rhs.when;
    /* Are the same. */
  };

  /* @brief wtr/watcher/event/!=
     Not == */
  friend bool operator!=(event const& lhs, event const& rhs) noexcept
  {
    return !(lhs == rhs);
  };

  /* @brief wtr/watcher/event/<<
     Streams out `where`, `what` and `kind`.
     Formats the stream as a json object. */
  friend std::ostream& operator<<(std::ostream& os, event const& ev) noexcept
  {
    /* clang-format off */
    return os << R"(")" << ev.when << R"(":)"
              << "{"
                  << R"("where":)" << ev.where      << R"(,)"
                  << R"("what":")"  << what_repr(ev.what) << R"(",)"
                  << R"("kind":")"  << kind_repr(ev.kind) << R"(")"
              << "}";
    /* clang-format on */
  }
};

/* @brief wtr/watcher/event/<<
   Streams out a `what` value. */
inline std::ostream& operator<<(std::ostream& os, enum what const& w) noexcept
{
  return os << "\"" << what_repr(w) << "\"";
}

/* @brief wtr/watcher/event/<<
   Streams out a `kind` value. */
inline std::ostream& operator<<(std::ostream& os, enum kind const& k) noexcept
{
  return os << "\"" << kind_repr(k) << "\"";
}

/* @brief watcher/event/callback
   Ensure the adapters can recieve events
   and will return nothing. */
using callback = function<void(event const&)>;

} /* namespace event */
} /* namespace watcher */
} /* namespace wtr   */

/* obj: path */
#include <filesystem>
/* obj: mutex */
#include <mutex>
/* obj: string */
#include <string>
/* obj: unordered_map */
#include <unordered_map>
/* fn: watch
   fn: die */

/* clang-format off */
/* clang-format on */

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

inline bool adapter(std::filesystem::path const& path,
                    event::callback const& callback, bool const& msg) noexcept
{
  /* @brief
     This container is used to count the number of watchers on some path.
     The key is a hash of a path.
     The value is the count of living watchers on a path. */
  static auto living_container{std::unordered_map<std::string, size_t>{}};

  /* @brief
     A primitive thread synchrony tool, a mutex, for the watcher container. */
  static auto living_container_mtx{std::mutex{}};

  /* @brief
     Some path as a string. We need the path as a string to store it as the
     key in a map because `std::filesystem::path` doesn't know how to hash,
     but `std::string` does. */
  auto const& path_str = path.string();

  /* @brief
     A predicate given to the watchers.
     True if living, false if dead.

     The watchers are expected to die promptly,
     but safely, when this returns false. */
  auto const& is_living = [&path_str]() -> bool {
    auto _ = std::scoped_lock{living_container_mtx};

    return living_container.contains(path_str);
  };

  /* @brief
     Increment the watch count on a unique path.
     If the count is 0, insert the path into the set
     of living watchers.

     Always returns true. */
  auto const& live = [&path_str, &callback]() -> bool {
    auto _ = std::scoped_lock{living_container_mtx};

    living_container[path_str] = living_container.contains(path_str)
                                     ? living_container.at(path_str) + 1
                                     : 1;

    callback(
        {"s/self/live@" + path_str, event::what::create, event::kind::watcher});

    return true;
  };

  /* @brief
     Decrement the watch count on a unique path.
     If the count is 0, erase the path from the set
     of living watchers.

     When a path's count is 0, the watchers on that path die.

     Returns false if the `path` is not being watched. */
  auto const& die = [&path_str, &callback]() -> bool {
    auto _ = std::scoped_lock{living_container_mtx};

    bool dead = true;

    if (living_container.contains(path_str)) {
      auto& at = living_container.at(path_str);

      if (at > 1)
        at -= 1;

      else
        living_container.erase(path_str);

    } else
      dead = false;

    callback({(dead ? "s/self/die@" : "e/self/die@") + path_str,
              event::what::destroy, event::kind::watcher});

    return dead;
  };

  /* @brief
     We know two messages:
       - Tell a watcher to live
       - Tell a watcher to die
     There may be more messages in the future. */
  if (msg)
    return live() && watch(path, callback, is_living);

  else
    return die();
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

/*
  @brief watcher/adapter/windows

  The Windows `ReadDirectoryChangesW` adapter.
*/

#if defined(WATER_WATCHER_PLATFORM_WINDOWS_ANY)

/* ReadDirectoryChangesW
   CreateIoCompletionPort
   CreateFileW
   CreateEventW
   GetQueuedCompletionStatus
   ResetEvent
   GetLastError
   WideCharToMultiByte */
#include <windows.h>
/* milliseconds */
#include <chrono>
/* path */
#include <filesystem>
/* string
   wstring */
#include <string>
/* this_thread::sleep_for */
#include <thread>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

inline constexpr auto delay_ms = std::chrono::milliseconds(16);
inline constexpr auto delay_ms_dw = static_cast<DWORD>(delay_ms.count());
inline constexpr auto has_delay = delay_ms > std::chrono::milliseconds(0);

/* I think the default page size in Windows is 64kb,
   so 65536 might also work well. */
inline constexpr auto event_buf_len_max = 8192;

/* Hold resources necessary to recieve and send filesystem events. */
class watch_event_proxy
{
 public:
  bool is_valid{true};

  std::filesystem::path path;

  wchar_t path_name[256]{L""};

  HANDLE path_handle{nullptr};

  HANDLE event_completion_token{nullptr};

  HANDLE event_token{CreateEventW(nullptr, true, false, nullptr)};

  OVERLAPPED event_overlap{};

  FILE_NOTIFY_INFORMATION event_buf[event_buf_len_max];

  DWORD event_buf_len_ready{0};

  watch_event_proxy(std::filesystem::path const& path) noexcept : path{path}
  {
    memcpy(path_name, path.c_str(), path.string().size());

    path_handle = CreateFileW(
        path.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (path_handle)
      event_completion_token
          = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

    if (event_completion_token)
      is_valid = CreateIoCompletionPort(path_handle, event_completion_token,
                                        (ULONG_PTR)path_handle, 1)
                 && ResetEvent(event_token);
  }

  ~watch_event_proxy() noexcept
  {
    if (event_token) CloseHandle(event_token);
    if (event_completion_token) CloseHandle(event_completion_token);
  }
};

inline bool is_valid(watch_event_proxy& w) noexcept
{
  return w.is_valid && w.event_buf != nullptr;
}

inline bool has_event(watch_event_proxy& w) noexcept
{
  return w.event_buf_len_ready != 0;
}

inline bool do_event_recv(watch_event_proxy& w,
                          event::callback const& callback) noexcept
{
  using namespace wtr::watcher::event;

  w.event_buf_len_ready = 0;
  DWORD bytes_returned = 0;
  memset(&w.event_overlap, 0, sizeof(OVERLAPPED));

  auto read_ok = ReadDirectoryChangesW(
      w.path_handle, w.event_buf, event_buf_len_max, true,
      FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION
          | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE
          | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES
          | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME,
      &bytes_returned, &w.event_overlap, nullptr);

  if (w.event_buf && read_ok) {
    w.event_buf_len_ready = bytes_returned > 0 ? bytes_returned : 0;
    return true;
  } else {
    switch (GetLastError()) {
      case ERROR_IO_PENDING:
        w.event_buf_len_ready = 0;
        w.is_valid = false;
        callback({"e/sys/read/pending", what::other, kind::watcher});
        break;
      default: callback({"e/sys/read", what::other, kind::watcher}); break;
    }
    return false;
  }
}

inline bool do_event_send(watch_event_proxy& w,
                          event::callback const& callback) noexcept
{
  FILE_NOTIFY_INFORMATION* buf = w.event_buf;

  if (is_valid(w)) {
    while (buf + sizeof(FILE_NOTIFY_INFORMATION) <= buf + w.event_buf_len_ready)
    {
      if (buf->FileNameLength % 2 == 0) {
        auto where
            = w.path / std::wstring{buf->FileName, buf->FileNameLength / 2};

        auto what = [&buf]() {
          switch (buf->Action) {
            case FILE_ACTION_MODIFIED: return event::what::modify;
            case FILE_ACTION_ADDED: return event::what::create;
            case FILE_ACTION_REMOVED: return event::what::destroy;
            case FILE_ACTION_RENAMED_OLD_NAME: return event::what::rename;
            case FILE_ACTION_RENAMED_NEW_NAME: return event::what::rename;
            default: return event::what::other;
          }
        }();

        auto kind = [&where]() {
          try {
            return std::filesystem::is_directory(where) ? event::kind::dir
                                                        : event::kind::file;
          } catch (...) {
            return event::kind::other;
          }
        }();

        callback({where, what, kind});

        if (buf->NextEntryOffset == 0)
          break;
        else
          buf = (FILE_NOTIFY_INFORMATION*)((uint8_t*)buf
                                           + buf->NextEntryOffset);
      }
    }
    return true;
  } else {
    return false;
  }
}

} /* namespace */

/* while living
   watch for events
   return when dead
   true if no errors */

inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  auto w = watch_event_proxy{path};

  if (is_valid(w)) {
    do_event_recv(w, callback);

    while (is_valid(w) && has_event(w)) {
      do_event_send(w, callback);
    }

    while (is_living()) {
      ULONG_PTR completion_key{0};
      LPOVERLAPPED overlap{nullptr};

      bool complete = GetQueuedCompletionStatus(
          w.event_completion_token, &w.event_buf_len_ready, &completion_key,
          &overlap, delay_ms_dw);

      if (complete && overlap) {
        while (is_valid(w) && has_event(w)) {
          do_event_send(w, callback);
          do_event_recv(w, callback);
        }
      }
    }

    return true;
  } else {
    return false;
  }
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif


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
/* watch */
/* event::* */

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
                  auto const& is_living) noexcept
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

/*
  @brief wtr/watcher/detail/adapter/linux

  The Linux adapters.
*/


#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
    || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

#include <unistd.h>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

/*
  @brief watcher/detail/adapter/watch
  If the user is (effectively) root, and not on Android,
  the we'll use `fanotify`. If not, we'll use `inotify`.

  Monitors `path` for changes.
  Invokes `callback` with an `event` when they happen.
  `watch` stops when asked to or unrecoverable errors occur.
  All events, including errors, are passed to `callback`.

  @param path
    A filesystem path to watch for events.

  @param callback
    A function to invoke with an `event` object
    when the files being watched change.

  @param is_living
    A function to decide whether we're dead.
*/
inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  auto const& is_living) noexcept
{
  return
#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
      inotify::watch(path, callback, is_living);
#else
      geteuid() == 0 ? fanotify::watch(path, callback, is_living)
                     : inotify::watch(path, callback, is_living);
#endif
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
          || defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
#endif /* if !defined(WATER_WATCHER_USE_WARTHOG) */

/*
  @brief watcher/adapter/android

  The Android (Linux) `inotify` adapter.
*/

#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#endif

#if defined(WATER_WATCHER_PLATFORM_UNKNOWN) \
    || defined(WATER_WATCHER_USE_WARTHOG)

/*
  @brief watcher/adapter/warthog

  A reasonably dumb adapter that works on any platform.

  This adapter beats `kqueue`, but it doesn't bean recieving
  filesystem events directly from the OS.

  This is the fallback adapter on platforms that either
    - Only support `kqueue` (`warthog` beats `kqueue`)
    - Only support the C++ standard library
*/

/* type: milliseconds */
#include <chrono>
/* obj: string */
#include <string>
/* lots of stuff */
#include <filesystem>
/* obj: error_code */
#include <system_error>
/* fn: this_thread::sleep_for */
#include <thread>
/* obj: unordered_map */
#include <unordered_map>
/* type: event::callback
   obj: event::event */

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace { /* anonymous namespace for "private" things */
/* clang-format off */

inline constexpr std::filesystem::directory_options
  scan_dir_options = 
    /* This is ridiculous */
    std::filesystem::directory_options::skip_permission_denied 
    & std::filesystem::directory_options::follow_directory_symlink;

using bucket_type = std::unordered_map<std::string, std::filesystem::file_time_type>;

/* clang-format on */

/*  @brief watcher/adapter/warthog/scan
    - Scans `path` for changes.
    - Updates our bucket to match the changes.
    - Calls `send_event` when changes happen.
    - Returns false if the file tree cannot be scanned. */
inline bool scan(std::filesystem::path const& path, auto const& send_event,
                 bucket_type& bucket) noexcept
{
  /* @brief watcher/adapter/warthog/scan_file
     - Scans a (single) file for changes.
     - Updates our bucket to match the changes.
     - Calls `send_event` when changes happen.
     - Returns false if the file cannot be scanned. */
  auto const scan_file
      = [&](std::filesystem::path const& file, auto const& send_event) -> bool {
    using std::filesystem::exists, std::filesystem::is_regular_file,
        std::filesystem::last_write_time;
    if (exists(file) && is_regular_file(file)) {
      auto ec = std::error_code{};
      /* grabbing the file's last write time */
      auto const timestamp = last_write_time(file, ec);
      if (ec) {
        /* the file changed while we were looking at it. so, we call the
         * closure, indicating destruction, and remove it from the bucket. */
        send_event(event::event{file, event::what::destroy, event::kind::file});
        if (bucket.contains(file)) bucket.erase(file);
      }
      /* if it's not in our bucket, */
      else if (!bucket.contains(file))
      {
        /* we put it in there and call the closure, indicating creation. */
        bucket[file] = timestamp;
        send_event(event::event{file, event::what::create, event::kind::file});
      }
      /* otherwise, it is already in our bucket. */
      else
      {
        /* we update the file's last write time, */
        if (bucket[file] != timestamp) {
          bucket[file] = timestamp;
          /* and call the closure on them, indicating modification */
          send_event(
              event::event{file, event::what::modify, event::kind::file});
        }
      }
      return true;
    } /* if the path doesn't exist, we nudge the callee with `false` */
    else
      return false;
  };

  /* @brief watcher/adapter/warthog/scan_directory
     - Scans a (single) directory for changes.
     - Updates our bucket to match the changes.
     - Calls `send_event` when changes happen.
     - Returns false if the directory cannot be scanned. */
  auto const scan_directory
      = [&](std::filesystem::path const& dir, auto const& send_event) -> bool {
    using std::filesystem::recursive_directory_iterator,
        std::filesystem::is_directory;
    /* if this thing is a directory */
    if (is_directory(dir)) {
      /* try to iterate through its contents */
      auto dir_it_ec = std::error_code{};
      for (auto const& file :
           recursive_directory_iterator(dir, scan_dir_options, dir_it_ec))
        /* while handling errors */
        if (dir_it_ec)
          return false;
        else
          scan_file(file.path(), send_event);
      return true;
    } else
      return false;
  };

  return scan_directory(path, send_event) ? true
         : scan_file(path, send_event)    ? true
                                          : false;
};

/* @brief wtr/watcher/warthog/tend_bucket
   If the bucket is empty, try to populate it.
   otherwise, prune it. */
inline bool tend_bucket(std::filesystem::path const& path,
                        auto const& send_event, bucket_type& bucket) noexcept
{
  /*  @brief watcher/adapter/warthog/populate
      @param path - path to monitor for
      Creates a file map, the "bucket", from `path`. */
  auto const populate = [&](std::filesystem::path const& path) -> bool {
    using std::filesystem::exists, std::filesystem::is_directory,
        std::filesystem::recursive_directory_iterator,
        std::filesystem::last_write_time;
    /* this happens when a path was changed while we were reading it.
     there is nothing to do here; we prune later. */
    auto dir_it_ec = std::error_code{};
    auto lwt_ec = std::error_code{};
    if (exists(path)) {
      /* this is a directory */
      if (is_directory(path)) {
        for (auto const& file :
             recursive_directory_iterator(path, scan_dir_options, dir_it_ec))
        {
          if (!dir_it_ec) {
            auto const lwt = last_write_time(file, lwt_ec);
            if (!lwt_ec)
              bucket[file.path()] = lwt;
            else
              /* @todo use this practice elsewhere or make a fn for it
                 otherwise, this might be confusing and inconsistent. */
              bucket[file.path()] = last_write_time(path);
          }
        }
      }
      /* this is a file */
      else
      {
        bucket[path] = last_write_time(path);
      }
    } else {
      return false;
    }
    return true;
  };

  /*  @brief watcher/adapter/warthog/prune
      Removes files which no longer exist from our bucket. */
  auto const prune
      = [&](std::filesystem::path const& path, auto const& send_event) -> bool {
    using std::filesystem::exists, std::filesystem::is_regular_file,
        std::filesystem::is_directory, std::filesystem::is_symlink;
    auto bucket_it = bucket.begin();
    /* while looking through the bucket's contents, */
    while (bucket_it != bucket.end()) {
      /* check if the stuff in our bucket exists anymore. */
      exists(bucket_it->first)
          /* if so, move on. */
          ? std::advance(bucket_it, 1)
          /* if not, call the closure, indicating destruction,
             and remove it from our bucket. */
          : [&]() {
              send_event(event::event{bucket_it->first, event::what::destroy,
                                      is_regular_file(path) ? event::kind::file
                                      : is_directory(path)  ? event::kind::dir
                                      : is_symlink(path) ? event::kind::sym_link
                                                         : event::kind::other});
              /* bucket, erase it! */
              bucket_it = bucket.erase(bucket_it);
            }();
    }
    return true;
  };

  return bucket.empty() ? populate(path)            ? true
                          : prune(path, send_event) ? true
                                                    : false
                        : true;
};

} /* namespace */

/*
  @brief watcher/adapter/warthog/watch

  @param path:
   A path to watch for changes.

  @param callback:
   A callback to perform when the files
   being watched change.

  Monitors `path` for changes.

  Calls `callback` with an `event` when they happen.

  Unless it should stop, or errors present, `watch` recurses.
*/

inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  auto const& is_living) noexcept
{
  using std::this_thread::sleep_for, std::chrono::milliseconds;
  /* Sleep for `delay_ms`.

     Then, keep running if
       - We are alive
       - The bucket is doing well
       - No errors occured while scanning

     Otherwise, stop and return false. */

  bucket_type bucket;

  static constexpr auto delay_ms = 16;

  if constexpr (delay_ms > 0) sleep_for(milliseconds(delay_ms));

  return is_living() ? tend_bucket(path, callback, bucket)
                           ? scan(path, callback, bucket)
                                 ? watch(path, callback, is_living)
                                 : false
                           : false
                     : true;
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_UNKNOWN) */

/* type: path */
#include <filesystem>
/* fn: watch_ctl */
/* type: callback */

namespace wtr {
namespace watcher {

/* @brief wtr/watcher/watch

   @param path:
     The root path to watch for filesystem events.

   @param living_cb (optional):
     Something (such as a closure) to be called when events
     occur in the path being watched.

   This is an adaptor "switch" that chooses the ideal adaptor
   for the host platform.

   Every adapter monitors `path` for changes and invokes the
   `callback` with an `event` object when they occur.

   There are three things the user needs:
     - The `die` function
     - The `watch` function
     - The `event` structure

   That's it.

   Happy hacking. */
inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback) noexcept
{
  return detail::adapter::adapter(path, callback, true);
}

/* @brief wtr/watcher/die

   Stops a watcher at `path`.
   Calls `callback` with status messages.
   True if newly dead. */
inline bool die(
    std::filesystem::path const& path,
    event::callback const& callback = [](event::event) -> void {}) noexcept
{
  return detail::adapter::adapter(path, callback, false);
}

} /* namespace watcher */
} /* namespace wtr   */

/*
  @brief wtr/watcher/<d>/adapter/linux/fanotify

  The Linux `fanotify` adapter.
*/


#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
    || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/fanotify.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace fanotify {

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>
   Anonymous namespace for "private" things. */
namespace {

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/constants
   - delay
       The delay, in milliseconds, while `epoll_wait` will
       'sleep' for until we are woken up. We usually check
       if we're still alive at that point.
   - event_wait_queue_max
       Number of events allowed to be given to do_event_recv
       (returned by `epoll_wait`). Any number between 1
       and some large number should be fine. We don't
       lose events if we 'miss' them, the events are
       still waiting in the next call to `epoll_wait`.
   - event_buf_len:
       For our event buffer, 4096 is a typical page size
       and sufficiently large to hold a great many events.
       That's a good thumb-rule, and it might be the best
       value to use because there will be a possibly long
       character string (for the filename) in the event.
       We can infer some things about the size we need for
       the event buffer, but it's unlikely to be meaningful
       because of the variably sized character string being
       reported. We could use something like:
          event_buf_len
            = ((event_wait_queue_max + PATH_MAX)
            * (3 * sizeof(struct fanotify_event_metadata)));
       But that's a lot of flourish for 72 bytes that won't
       be meaningful. */
inline constexpr auto delay_ms = 16;
inline constexpr auto event_wait_queue_max = 1;
inline constexpr auto event_buf_len
    = PATH_MAX;  // event_wait_queue_max * PATH_MAX;

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/types
   - sys_resource_type
       An object holding:
         - An fanotify file descriptor
         - An epoll file descriptor
         - An epoll configuration
         - A set of watch marks (as returned by fanotify_mark)
         - A map of (sub)path handles to filesystem paths (names)
         - A boolean representing the validity of these resources */
using path_mark_container_type = std::unordered_set<int>;
using dir_name_container_type
    = std::unordered_map<unsigned long, std::filesystem::path>;

struct sys_resource_type
{
  bool valid;
  int watch_fd;
  int event_fd;
  struct epoll_event event_conf;
  path_mark_container_type path_mark_container;
  dir_name_container_type dir_name_container;
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns

     do_sys_resource_create
       -> sys_resource_type

     do_sys_resource_destroy
       -> bool

     do_event_recv
       -> bool

     do_path_mark_add
       -> bool

     do_path_mark_remove
       -> bool

     do_error
       -> bool

     do_warning
       -> bool
*/

inline auto do_sys_resource_create(std::filesystem::path const&,
                                   event::callback const&) noexcept
    -> sys_resource_type;
inline auto do_sys_resource_destroy(sys_resource_type&,
                                    std::filesystem::path const&,
                                    event::callback const&) noexcept -> bool;
inline auto do_event_recv(sys_resource_type&, std::filesystem::path const&,
                          event::callback const&) noexcept -> bool;
inline auto do_path_mark_add(std::filesystem::path const&, int,
                             path_mark_container_type&) noexcept -> bool;
inline auto do_path_mark_remove(std::filesystem::path const&, int,
                                path_mark_container_type&) noexcept -> bool;
inline auto do_error(auto const&, auto const&, event::callback const&) noexcept
    -> bool;
inline auto do_warning(auto const&, auto const&,
                       event::callback const&) noexcept -> bool;

inline auto do_path_mark_add(std::filesystem::path const& full_path,
                             int watch_fd,
                             path_mark_container_type& pmc) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd, FAN_MARK_ADD,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                             | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD, full_path.c_str());
  if (wd >= 0)
    pmc.insert(wd);
  else
    return false;
  return true;
};

inline auto do_path_mark_remove(std::filesystem::path const& full_path,
                                int watch_fd,
                                path_mark_container_type& pmc) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd, FAN_MARK_REMOVE,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                             | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD, full_path.c_str());
  auto const at = pmc.find(wd);
  if (wd >= 0 && at != pmc.end())
    pmc.erase(at);
  else
    return false;
  return true;
};

inline auto do_error(auto const& error, auto const& path,
                     event::callback const& callback) noexcept -> bool
{
  auto msg = std::string(error)
                 .append("(")
                 .append(strerror(errno))
                 .append(")@")
                 .append(path);
  callback({msg, event::what::other, event::kind::watcher});
  return false;
};

inline auto do_warning(auto const& error, auto const& path,
                       event::callback const& callback) noexcept -> bool
{
  auto msg = std::string(error).append("@").append(path);
  callback({msg, event::what::other, event::kind::watcher});
  return true;
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_sys_resource_create
   Produces a `sys_resource_type` with the file descriptors from
   `fanotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto do_sys_resource_create(std::filesystem::path const& path,
                                   event::callback const& callback) noexcept
    -> sys_resource_type
{
  auto const do_error = [&callback](auto const& error, auto const& path,
                                    int watch_fd, int event_fd = -1) {
    auto msg = std::string(error)
                   .append("(")
                   .append(strerror(errno))
                   .append(")@")
                   .append(path);
    callback({msg, event::what::other, event::kind::watcher});
    return sys_resource_type{
        .valid = false,
        .watch_fd = watch_fd,
        .event_fd = event_fd,
        .event_conf = {.events = 0, .data = {.fd = watch_fd}},
        .path_mark_container = {},
        .dir_name_container = {},
    };
  };
  auto const do_path_map_container_create
      = [](int const watch_fd, std::filesystem::path const& watch_base_path,
           event::callback const& callback) -> path_mark_container_type {
    using rdir_iterator = std::filesystem::recursive_directory_iterator;

    /* Follow symlinks, ignore paths which we don't have permissions for. */
    static constexpr auto dir_opt
        = std::filesystem::directory_options::skip_permission_denied
          & std::filesystem::directory_options::follow_directory_symlink;

    static constexpr auto rsrv_count = 1024;

    /* auto dir_ec = std::error_code{}; */
    path_mark_container_type pmc;
    pmc.reserve(rsrv_count);

    if (!do_path_mark_add(watch_base_path, watch_fd, pmc))
      return pmc;

    else if (std::filesystem::is_directory(watch_base_path))
      try {
        for (auto const& dir : rdir_iterator(watch_base_path, dir_opt))
          if (std::filesystem::is_directory(dir))
            if (!do_path_mark_add(dir.path(), watch_fd, pmc))
              do_warning(
                  "w/sys/not_watched",
                  std::string(watch_base_path).append("@").append(dir.path()),
                  callback);
      } catch (...) {
        do_warning("w/sys/not_watched", watch_base_path, callback);
      }
    return pmc;
  };

  /* Init Flags:
       Post-event reporting, non-blocking IO and unlimited marks.
     Init Extra Flags:
       Read-only and close-on-exec.
     If we were making a filesystem auditor, we could use
     `FAN_CLASS_PRE_CONTENT|FAN_UNLIMITED_QUEUE|FAN_UNLIMITED_MARKS`. */
  /* clang-format off */
  int watch_fd
      = fanotify_init(FAN_CLASS_NOTIF
                      | FAN_REPORT_DFID_NAME
                      | FAN_UNLIMITED_QUEUE
                      | FAN_UNLIMITED_MARKS,
                      O_RDONLY
                      | O_NONBLOCK
                      | O_CLOEXEC);
  if (watch_fd >= 0) {
    auto pmc = do_path_map_container_create(watch_fd, path, callback);
    if (!pmc.empty())
    {
      /* clang-format on */
      struct epoll_event event_conf
      {
        .events = EPOLLIN, .data { .fd = watch_fd }
      };

      int event_fd = epoll_create1(EPOLL_CLOEXEC);

      /* @note We could make the epoll and fanotify file descriptors
         non-blocking with `fcntl`. It's not clear if we can do this
         from their `*_init` calls. */
      /* fcntl(watch_fd, F_SETFL, O_NONBLOCK); */
      /* fcntl(event_fd, F_SETFL, O_NONBLOCK); */

      if (event_fd >= 0) {
        if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) >= 0) {
          return sys_resource_type{
              .valid = true,
              .watch_fd = watch_fd,
              .event_fd = event_fd,
              .event_conf = event_conf,
              .path_mark_container = std::move(pmc),
              .dir_name_container = {},
          };
        } else {
          return do_error("e/sys/epoll_ctl", path, watch_fd, event_fd);
        }
      } else {
        return do_error("e/sys/epoll_create", path, watch_fd, event_fd);
      }
    } else {
      return do_error("e/sys/fanotify_mark", path, watch_fd);
    }
  } else {
    return do_error("e/sys/fanotify_init", path, watch_fd);
  }
}

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_sys_resource_destroy
   Close the resources in the system resource type `sr`.
   Invoke `callback` on errors. */
inline auto do_sys_resource_destroy(
    sys_resource_type& sr, std::filesystem::path const& watch_base_path,
    event::callback const& callback) noexcept -> bool
{
  auto const watch_fd_close_ok = close(sr.watch_fd) == 0;
  auto const event_fd_close_ok = close(sr.event_fd) == 0;
  if (!watch_fd_close_ok)
    do_error("e/sys/close/watch_fd", watch_base_path, callback);
  if (!event_fd_close_ok)
    do_error("e/sys/close/event_fd", watch_base_path, callback);
  return watch_fd_close_ok && event_fd_close_ok;
}

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/lift_event_path
   This lifts a path from our cache, in the system resource object `sr`, if
   available. Otherwise, it will try to read a directory and directory entry.
   Sometimes, such as on a directory being destroyed and the file not being
   able to be opened, the directory entry is the only event info. We can still
   get the rest of the path in those cases, however, by looking up the cached
   "upper" directory that event belongs to.
   Using the cache is helpful in other cases, too. This is an averaged bench of
   the three paths this function can go down:
     - Cache:            2427ns
     - Dir:              8905ns
     - Dir + Dir Entry:  31966ns */
inline auto lift_event_path(sys_resource_type& sr,
                            decltype(fanotify_event_metadata::mask) const& mask,
                            auto const& dfid_info,
                            std::filesystem::path const& watch_base_path,
                            event::callback const& callback) noexcept
    -> std::optional<std::filesystem::path>
{
  auto const& nip
      = [&]() -> std::optional<
                  std::pair<std::filesystem::path const, unsigned long const>> {
    /* The shenanigans we do here depend on this event being
       `FAN_EVENT_INFO_TYPE_DFID_NAME`. The kernel passes us
       some info about the directory and the directory entry
       (the filename) of this event that doesn't exist when
       other event types are reported. In particular, we need
       a file descriptor to the directory (which we use
       `readlink` on) and a character string representing the
       name of the directory entry.
       TLDR: We need information for the full path of the event,
       information which is only reported inside this `if`.

           [ The following is (mostly) quoting from the kernel ]
           Variable size struct for
           dir file handle + child file handle + name

           [ Omitting definition of `fanotify_info` here ]

           (struct fanotify_fh) dir_fh starts at
             buf[0]

           (optional) dir2_fh starts at
             buf[dir_fh_totlen]

           (optional) file_fh starts at
             buf[dir_fh_totlen + dir2_fh_totlen]

           name starts at
             buf[dir_fh_totlen + dir2_fh_totlen + file_fh_totlen]
           ...

           [ end quote ]


       The kernel guarentees that there is a null-terminated
       character string to the event's directory entry
       after the file handle to the directory.
       Confusing, right?
    */

    auto const path_accum_append
        = [](auto& path_accum, auto const& dfid_info, auto const& dir_fh,
             auto const& dirname_len) -> void {
      char* name_info = (char*)(dfid_info + 1);
      char* filename
          = (char*)(name_info + sizeof(struct file_handle)
                    + sizeof(dir_fh->f_handle) + sizeof(dir_fh->handle_bytes)
                    + sizeof(dir_fh->handle_type));
      if (filename != nullptr && strcmp(filename, ".") != 0)
        snprintf(path_accum + dirname_len, sizeof(path_accum) - dirname_len,
                 "/%s", filename);
    };

    auto const path_accum_front = [](auto& path_accum, auto const& dfid_info,
                                     auto const& dir_fh) -> void {
      char* name_info = (char*)(dfid_info + 1);
      char* filename
          = (char*)(name_info + sizeof(struct file_handle)
                    + sizeof(dir_fh->f_handle) + sizeof(dir_fh->handle_bytes)
                    + sizeof(dir_fh->handle_type));
      if (filename != nullptr && strcmp(filename, ".") != 0)
        snprintf(path_accum, sizeof(path_accum), "/%s", filename);
    };

    auto const dir_fh = (struct file_handle*)dfid_info->handle;

    unsigned long dir_id{(unsigned long)(std::abs(dir_fh->handle_type))};

    for (unsigned i = 0; i < dir_fh->handle_bytes; i++)
      dir_id += dir_fh->f_handle[i];

    auto const dit = sr.dir_name_container.find(dir_id);

    if (dit != sr.dir_name_container.end()) {
      /* We already have a path name, use it */
      char path_accum[PATH_MAX];
      auto dirname = dit->second.c_str();
      auto dirname_len = dit->second.string().length();
      snprintf(path_accum, sizeof(path_accum), "%s", dirname);
      path_accum_append(path_accum, dfid_info, dir_fh, dirname_len);

      return std::make_pair(std::filesystem::path(path_accum), dir_id);

    } else {
      /* We can get a path name, so get that and use it */
      char path_accum[PATH_MAX];
      int fd = open_by_handle_at(AT_FDCWD, dir_fh,
                                 O_RDONLY | O_CLOEXEC | O_PATH | O_NONBLOCK);
      if (fd > 0) {
        char procpath[128];
        snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", fd);
        auto const dirname_len
            = readlink(procpath, path_accum, sizeof(path_accum) - sizeof('\0'));
        close(fd);

        if (dirname_len > 0) {
          path_accum[dirname_len] = '\0';
          /* Put the directory name in the path accumulator.
             Then, if a directory entry for this event exists
             And it's not the directory itself
             Put it in the path accumulator. */
          path_accum_append(path_accum, dfid_info, dir_fh, dirname_len);

          return std::make_pair(std::filesystem::path(path_accum), dir_id);
        }

        do_warning("w/sys/readlink", watch_base_path, callback);
        return std::nullopt;
      } else {
        path_accum_front(path_accum, dfid_info, dir_fh);

        return std::make_pair(std::filesystem::path(path_accum), dir_id);
      }
    }
  }();

  if (nip.has_value()) {
    auto& [full_path, dir_id] = nip.value();
    if (mask & FAN_ONDIR) {
      if (mask & FAN_CREATE) {
        auto const& at = sr.dir_name_container.find(dir_id);
        if (at == sr.dir_name_container.end()) {
          sr.dir_name_container.emplace(dir_id, full_path.parent_path());
        }
        do_path_mark_add(full_path, sr.watch_fd, sr.path_mark_container);
      } else if (mask & FAN_DELETE) {
        do_path_mark_remove(full_path, sr.watch_fd, sr.path_mark_container);
        auto const& at = sr.dir_name_container.find(dir_id);
        if (at != sr.dir_name_container.end()) sr.dir_name_container.erase(at);
      }
    }
    return full_path;
  }
  return std::nullopt;
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/event_send
   This is the heart of the adapter.
   Everything flows from here. */
inline auto event_send(decltype(fanotify_event_metadata::mask) const& mask,
                       std::optional<std::filesystem::path> const& path,
                       event::callback const& callback) noexcept -> void
{
  using namespace wtr::watcher::event;

  if (path.has_value()) {
    auto const& p = path.value();
    if (mask & FAN_ONDIR) {
      if (mask & FAN_CREATE)
        callback({p, what::create, kind::dir});
      else if (mask & FAN_MODIFY)
        callback({p, what::modify, kind::dir});
      else if (mask & FAN_DELETE)
        callback({p, what::destroy, kind::dir});
      else if (mask & FAN_MOVED_FROM)
        callback({p, what::rename, kind::dir});
      else if (mask & FAN_MOVED_TO)
        callback({p, what::rename, kind::dir});
      else
        callback({p, what::other, kind::dir});

    } else {
      if (mask & FAN_CREATE)
        callback({p, what::create, kind::file});
      else if (mask & FAN_MODIFY)
        callback({p, what::modify, kind::file});
      else if (mask & FAN_DELETE)
        callback({p, what::destroy, kind::file});
      else if (mask & FAN_MOVED_FROM)
        callback({p, what::rename, kind::file});
      else if (mask & FAN_MOVED_TO)
        callback({p, what::rename, kind::file});
      else
        callback({p, what::other, kind::file});
    }
  } else {
    do_warning("w/self/no_path", "", callback);
  }
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_event_recv
   Reads through available (fanotify) filesystem events.
   Discerns their path and type.
   Calls the callback.
   Returns false on eventful errors.
   @note
   The `metadata->fd` field contains either a file
   descriptor or the value `FAN_NOFD`. File descriptors
   are always greater than 0. `FAN_NOFD` represents an
   event queue overflow for `fanotify` listeners which
   are _not_ monitoring file handles, such as mount
   monitors. The file handle is in the metadata when an
   `fanotify` listener is monitoring events by their
   file handles.
   The `metadata->vers` field may differ between kernel
   versions, so we check it against what we have been
   compiled with. */
inline auto do_event_recv(sys_resource_type& sr,
                          std::filesystem::path const& watch_base_path,
                          event::callback const& callback) noexcept -> bool
{
  /* Read some events. */
  alignas(struct fanotify_event_metadata) char event_buf[event_buf_len];
  auto event_read = read(sr.watch_fd, event_buf, sizeof(event_buf));

  /* Error */
  if (event_read < 0 && errno != EAGAIN)
    return do_error("e/sys/read", watch_base_path, callback);

  /* Eventful */
  else if (event_read > 0)
    /* Loop over everything in the event buffer. */
    for (auto* metadata = (struct fanotify_event_metadata const*)event_buf;
         FAN_EVENT_OK(metadata, event_read);
         metadata = FAN_EVENT_NEXT(metadata, event_read))
      if (metadata->fd == FAN_NOFD)
        if (metadata->vers == FANOTIFY_METADATA_VERSION)
          if (!(metadata->mask & FAN_Q_OVERFLOW))
            if (((fanotify_event_info_fid*)(metadata + 1))->hdr.info_type
                == FAN_EVENT_INFO_TYPE_DFID_NAME)

              /* This is the important part:
                 Send the events we recieve.
                 Everything before here here
                 is a layer of translation
                 between us and the kernel. */
              event_send(
                  metadata->mask,
                  lift_event_path(sr, metadata->mask,
                                  ((fanotify_event_info_fid*)(metadata + 1)),
                                  watch_base_path, callback),
                  callback);

            else
              return do_warning("w/self/event_info", watch_base_path, callback);
          else
            return do_error("e/sys/overflow", watch_base_path, callback);
        else
          return do_error("e/sys/kernel_version", watch_base_path, callback);
      else
        return do_error("e/sys/wrong_event_fd", watch_base_path, callback);

  /* Eventless */
  else
    return true;

  /* Value after looping */
  return true;
};

} /* namespace */

/*
  @brief wtr/watcher/<d>/adapter/watch
  Monitors `path` for changes.
  Invokes `callback` with an `event` when they happen.
  `watch` stops when asked to or irrecoverable errors occur.
  All events, including errors, are passed to `callback`.

  @param path
    A filesystem path to watch for events.

  @param callback
    A function to invoke with an `event` object
    when the files being watched change.

  @param is_living
    A function to decide whether we're dead.
*/
inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  auto const& is_living) noexcept
{
  /* Gather these resources:
       - system resources
           For fanotify and epoll
       - event recieve list
           For receiving epoll events */

  struct sys_resource_type sr = do_sys_resource_create(path, callback);

  if (sr.valid) {
    struct epoll_event event_recv_list[event_wait_queue_max];

    /* Do this work until dead:
        - Await filesystem events
        - Invoke `callback` on errors and events */

    while (is_living()) {
      int event_count = epoll_wait(sr.event_fd, event_recv_list,
                                   event_wait_queue_max, delay_ms);
      if (event_count < 0)
        return do_sys_resource_destroy(sr, path, callback)
               && do_error("e/sys/epoll_wait", path, callback);
      else if (event_count > 0)
        for (int n = 0; n < event_count; n++)
          if (event_recv_list[n].data.fd == sr.watch_fd)
            if (is_living())
              if (!do_event_recv(sr, path, callback))
                return do_sys_resource_destroy(sr, path, callback)
                       && do_error("e/self/event_recv", path, callback);
    }
    return do_sys_resource_destroy(sr, path, callback);
  } else {
    return do_sys_resource_destroy(sr, path, callback)
           && do_error("e/self/sys_resource", path, callback);
  }
}

} /* namespace fanotify */
} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
          || defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
#endif /* if !defined(WATER_WATCHER_USE_WARTHOG) */

/*
  @brief wtr/watcher/<d>/adapter/linux/inotify

  The Linux `inotify` adapter.
*/


#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
    || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <thread>
#include <tuple>
#include <unordered_map>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace inotify {

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>
   Anonymous namespace for "private" things. */
namespace {

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/constants
   - delay
       The delay, in milliseconds, while `epoll_wait` will
       'sleep' for until we are woken up. We usually check
       if we're still alive at that point.
   - event_wait_queue_max
       Number of events allowed to be given to do_event_recv
       (returned by `epoll_wait`). Any number between 1
       and some large number should be fine. We don't
       lose events if we 'miss' them, the events are
       still waiting in the next call to `epoll_wait`.
   - event_buf_len:
       For our event buffer, 4096 is a typical page size
       and sufficiently large to hold a great many events.
       That's a good thumb-rule.
   - in_init_opt
       Use non-blocking IO.
   - in_watch_opt
       Everything we can get.
   @todo
   - Measure perf of IN_ALL_EVENTS
   - Handle move events properly.
     - Use IN_MOVED_TO
     - Use event::<something> */
inline constexpr auto delay_ms = 16;
inline constexpr auto event_wait_queue_max = 1;
inline constexpr auto event_buf_len = 4096;
inline constexpr auto in_init_opt = IN_NONBLOCK;
inline constexpr auto in_watch_opt
    = IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_Q_OVERFLOW;

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/types
   - path_map_type
       An alias for a map of file descriptors to paths.
   - sys_resource_type
       An object representing an inotify file descriptor,
       an epoll file descriptor, an epoll configuration,
       and whether or not these resources are valid. */
using path_map_type = std::unordered_map<int, std::filesystem::path>;
struct sys_resource_type
{
  bool valid;
  int watch_fd;
  int event_fd;
  struct epoll_event event_conf;
};

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns
   Functions:

     do_path_map_create
       -> path_map_type

     do_sys_resource_create
       -> sys_resource_type

     do_resource_release
       -> bool

     do_event_recv
       -> bool
*/

inline auto do_path_map_create(int const watch_fd,
                               std::filesystem::path const& watch_base_path,
                               event::callback const& callback) noexcept
    -> path_map_type;
inline auto do_sys_resource_create(event::callback const& callback) noexcept
    -> sys_resource_type;
inline auto do_resource_release(int watch_fd, int event_fd,
                                std::filesystem::path const& watch_base_path,
                                event::callback const& callback) noexcept
    -> bool;
inline auto do_event_recv(int watch_fd, path_map_type& path_map,
                          std::filesystem::path const& watch_base_path,
                          event::callback const& callback) noexcept -> bool;

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_path_map_create
   If the path given is a directory
     - find all directories above the base path given.
     - ignore nonexistent directories.
     - return a map of watch descriptors -> directories.
   If `path` is a file
     - return it as the only value in a map.
     - the watch descriptor key should always be 1. */
inline auto do_path_map_create(int const watch_fd,
                               std::filesystem::path const& watch_base_path,
                               event::callback const& callback) noexcept
    -> path_map_type
{
  using rdir_iterator = std::filesystem::recursive_directory_iterator;
  /* Follow symlinks, ignore paths which we don't have permissions for. */
  static constexpr auto dir_opt
      = std::filesystem::directory_options::skip_permission_denied
        & std::filesystem::directory_options::follow_directory_symlink;

  static constexpr auto path_map_reserve_count = 256;

  auto dir_ec = std::error_code{};
  path_map_type path_map;
  path_map.reserve(path_map_reserve_count);

  auto do_mark = [&](auto& dir) {
    int wd = inotify_add_watch(watch_fd, dir.c_str(), in_watch_opt);
    return wd > 0 ? path_map.emplace(wd, dir).first != path_map.end() : false;
  };

  if (!do_mark(watch_base_path))
    return path_map_type{};
  else if (std::filesystem::is_directory(watch_base_path, dir_ec))
    /* @todo @note
       Should we bail from within this loop if `do_mark` fails? */
    for (auto const& dir : rdir_iterator(watch_base_path, dir_opt, dir_ec))
      if (!dir_ec)
        if (std::filesystem::is_directory(dir, dir_ec))
          if (!dir_ec)
            if (!do_mark(dir.path()))
              callback({"w/sys/path_unwatched@" / dir.path(),
                        event::what::other, event::kind::watcher});
  return path_map;
};

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_sys_resource_create
   Produces a `sys_resource_type` with the file descriptors from
   `inotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto do_sys_resource_create(event::callback const& callback) noexcept
    -> sys_resource_type
{
  auto const do_error
      = [&callback](auto const& msg, int watch_fd, int event_fd = -1) {
          callback({msg, event::what::other, event::kind::watcher});
          return sys_resource_type{
              .valid = false,
              .watch_fd = watch_fd,
              .event_fd = event_fd,
              .event_conf = {.events = 0, .data = {.fd = watch_fd}}};
        };

  int watch_fd
#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
      = inotify_init();
#elif defined(WATER_WATCHER_PLATFORM_LINUX_ANY)
      = inotify_init1(in_init_opt);
#endif

  if (watch_fd >= 0) {
    struct epoll_event event_conf
    {
      .events = EPOLLIN, .data { .fd = watch_fd }
    };

    int event_fd
#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
        = epoll_create(event_wait_queue_max);
#elif defined(WATER_WATCHER_PLATFORM_LINUX_ANY)
        = epoll_create1(EPOLL_CLOEXEC);
#endif

    if (event_fd >= 0)
      if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) >= 0)
        return sys_resource_type{.valid = true,
                                 .watch_fd = watch_fd,
                                 .event_fd = event_fd,
                                 .event_conf = event_conf};
      else
        return do_error("e/sys/epoll_ctl", watch_fd, event_fd);
    else
      return do_error("e/sys/epoll_create", watch_fd, event_fd);
  } else {
    return do_error("e/sys/inotify_init", watch_fd);
  }
}

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_resource_release
   Close the file descriptors `watch_fd` and `event_fd`.
   Invoke `callback` on errors. */
inline auto do_resource_release(int watch_fd, int event_fd,
                                std::filesystem::path const& watch_base_path,
                                event::callback const& callback) noexcept
    -> bool
{
  auto const watch_fd_close_ok = close(watch_fd) == 0;
  auto const event_fd_close_ok = close(event_fd) == 0;
  if (!watch_fd_close_ok)
    callback({"e/sys/close/watch_fd@" / watch_base_path, event::what::other,
              event::kind::watcher});
  if (!event_fd_close_ok)
    callback({"e/sys/close/event_fd@" / watch_base_path, event::what::other,
              event::kind::watcher});
  return watch_fd_close_ok && event_fd_close_ok;
}

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_event_recv
   Reads through available (inotify) filesystem events.
   Discerns their path and type.
   Calls the callback.
   Returns false on eventful errors.

   @todo
   Return new directories when they appear,
   Consider running and returning `find_dirs` from here.
   Remove destroyed watches. */
inline auto do_event_recv(int watch_fd, path_map_type& path_map,
                          std::filesystem::path const& watch_base_path,
                          event::callback const& callback) noexcept -> bool
{
  alignas(struct inotify_event) char buf[event_buf_len];

  enum class event_recv_state { eventful, eventless, error };

  auto const lift_this_event = [](int fd, char* buf) {
    /* Read some events. */
    ssize_t len = read(fd, buf, event_buf_len);

    /* EAGAIN means no events were found.
       We return `eventless` in that case. */
    if (len < 0 && errno != EAGAIN)
      return std::make_pair(event_recv_state::error, len);
    else if (len <= 0)
      return std::make_pair(event_recv_state::eventless, len);
    else
      return std::make_pair(event_recv_state::eventful, len);
  };

  /* Loop while events can be read from the inotify file descriptor. */
  while (true) {
    /* Read events */
    auto [status, len] = lift_this_event(watch_fd, buf);
    /* Handle the errored, eventless and eventful reads */
    switch (status) {
      case event_recv_state::eventful:
        /* Loop over all events in the buffer. */
        const struct inotify_event* this_event;
        for (char* ptr = buf; ptr < buf + len;
             ptr += sizeof(struct inotify_event) + this_event->len)
        {
          this_event = (const struct inotify_event*)ptr;

          /* @todo
             Consider using std::filesystem here. */
          auto const path_kind = this_event->mask & IN_ISDIR
                                     ? event::kind::dir
                                     : event::kind::file;
          int path_wd = this_event->wd;
          auto event_dir_path = path_map.find(path_wd)->second;
          auto event_base_name = std::filesystem::path(this_event->name);
          auto event_path = event_dir_path / event_base_name;

          if (this_event->mask & IN_Q_OVERFLOW) {
            callback({"e/self/overflow@" / watch_base_path, event::what::other,
                      event::kind::watcher});
          } else if (this_event->mask & IN_CREATE) {
            callback({event_path, event::what::create, path_kind});
            if (path_kind == event::kind::dir) {
              int new_watch_fd = inotify_add_watch(watch_fd, event_path.c_str(),
                                                   in_watch_opt);
              path_map[new_watch_fd] = event_path;
            }
          } else if (this_event->mask & IN_DELETE) {
            callback({event_path, event::what::destroy, path_kind});
            /* @todo rm watch, rm path map entry */
          } else if (this_event->mask & IN_MOVE) {
            callback({event_path, event::what::rename, path_kind});
          } else if (this_event->mask & IN_MODIFY) {
            callback({event_path, event::what::modify, path_kind});
          } else {
            callback({event_path, event::what::other, path_kind});
          }
        }
        /* We don't want to return here. We run until `eventless`. */
        break;
      case event_recv_state::error:
        callback({"e/sys/read@" / watch_base_path, event::what::other,
                  event::kind::watcher});
        return false;
        break;
      case event_recv_state::eventless: return true; break;
    }
  }
}

} /* namespace */

/*
  @brief wtr/watcher/<d>/adapter/watch
  Monitors `path` for changes.
  Invokes `callback` with an `event` when they happen.
  `watch` stops when asked to or irrecoverable errors occur.
  All events, including errors, are passed to `callback`.

  @param path
    A filesystem path to watch for events.

  @param callback
    A function to invoke with an `event` object
    when the files being watched change.

  @param is_living
    A function to decide whether we're dead.
*/
inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  auto const& is_living) noexcept
{
  auto const do_error = [&callback](auto const& msg) -> bool {
    callback({msg, event::what::other, event::kind::watcher});
    return false;
  };

  /* Gather these resources:
       - system resources
           For inotify and epoll
       - event recieve list
           For receiving epoll events
       - path map
           For event to path lookups */

  struct sys_resource_type sr = do_sys_resource_create(callback);
  if (sr.valid) {
    auto path_map = do_path_map_create(sr.watch_fd, path, callback);
    if (path_map.size() > 0) {
      struct epoll_event event_recv_list[event_wait_queue_max];

      /* Do this work until dead:
          - Await filesystem events
          - Invoke `callback` on errors and events */

      while (is_living()) {
        int event_count = epoll_wait(sr.event_fd, event_recv_list,
                                     event_wait_queue_max, delay_ms);
        if (event_count < 0)
          return do_resource_release(sr.watch_fd, sr.event_fd, path, callback)
                 && do_error("e/sys/epoll_wait@" / path);
        else if (event_count > 0)
          for (int n = 0; n < event_count; n++)
            if (event_recv_list[n].data.fd == sr.watch_fd)
              if (is_living())
                if (!do_event_recv(sr.watch_fd, path_map, path, callback))
                  return do_resource_release(sr.watch_fd, sr.event_fd, path,
                                             callback)
                         && do_error("e/self/event_recv@" / path);
      }
      return do_resource_release(sr.watch_fd, sr.event_fd, path, callback);
    } else {
      return do_resource_release(sr.watch_fd, sr.event_fd, path, callback)
             && do_error("e/self/path_map@" / path);
    }
  } else {
    return do_resource_release(sr.watch_fd, sr.event_fd, path, callback)
           && do_error("e/self/sys_resource@" / path);
  }
}

} /* namespace inotify */
} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) \
          || defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */
#endif /* if !defined(WATER_WATCHER_USE_WARTHOG) */
#endif /* W973564ED9F278A21F3E12037288412FBAF175F889 */
