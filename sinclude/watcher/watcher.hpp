#ifndef W973564ED9F278A21F3E12037288412FBAF175F889
#define W973564ED9F278A21F3E12037288412FBAF175F889

namespace wtr {
namespace watcher {
namespace detail {

enum class platform_type {
  /* Linux */
  linux_kernel,
  linux_kernel_unknown,

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

inline constexpr platform_type platform

/* linux */
# if defined(__linux__) && !defined(__ANDROID_API__)
#  define WATER_WATCHER_PLATFORM_LINUX_KERNEL_ANY TRUE

/* LINUX_VERSION_CODE
   KERNEL_VERSION */
#  include <linux/version.h>

/* linux >= 5.9.0 */
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_5_9_0
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_UNKNOWN FALSE
#  endif
/* linux >= 2.7.0 */
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 7, 0)
    = platform_type::linux_kernel;
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_2_7_0
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_UNKNOWN FALSE
/* linux unknown */
#  else
    = platform_type::linux_kernel_unknown;
#   define WATER_WATCHER_PLATFORM_LINUX_KERNEL_UNKNOWN TRUE
#  endif

/* android */
# elif defined(__ANDROID_API__)
    = platform_type::android;
#  define WATER_WATCHER_PLATFORM_ANDROID_ANY TRUE
#  define WATER_WATCHER_PLATFORM_ANDROID_UNKNOWN TRUE

/* apple */
# elif defined(__APPLE__)
#  define WATER_WATCHER_PLATFORM_MAC_ANY TRUE

/* TARGET_OS_* */
#  include <TargetConditionals.h>

/* apple mac catalyst */
#  if defined(TARGET_OS_MACCATALYST)
    = platform_type::mac_catalyst;
#  define WATER_WATCHER_PLATFORM_MAC_CATALYST TRUE
/* apple macos, osx */
#  elif defined(TARGET_OS_MAC)
    = platform_type::mac_os;
#  define WATER_WATCHER_PLATFORM_MAC_OS TRUE
/* apple ios */
#  elif defined(TARGET_OS_IOS)
    = platform_type::mac_ios;
#  define WATER_WATCHER_PLATFORM_MAC_IOS TRUE
/* apple unknown */
#  else
    = platform_type::mac_unknown;
#  define WATER_WATCHER_PLATFORM_MAC_UNKNOWN TRUE
#  endif /* apple target */

/* windows */
# elif defined(WIN32) || defined(_WIN32)
    = platform_type::windows;
#  define WATER_WATCHER_PLATFORM_WINDOWS_ANY TRUE
#  define WATER_WATCHER_PLATFORM_WINDOWS_UNKNOWN TRUE

/* unknown */
# else
# warning "host platform is unknown"
    = platform_type::unknown;
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

/* std::ostream */
#include <ostream>

/* std::chrono::system_clock::now
   std::chrono::duration_cast
   std::chrono::system_clock
   std::chrono::nanoseconds
   std::chrono::time_point */
#include <chrono>

/* std::filesystem::path */
#include <filesystem>

/* std::function */
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
  std::filesystem::path const where;
  enum what const what;
  enum kind const kind;
  long long const when{
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
/* event
   callback */

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

        auto what = [&buf]() noexcept -> event::what {
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

    callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});

    return true;
  } else {
    callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});
    return false;
  }
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* defined(WATER_WATCHER_PLATFORM_WINDOWS_ANY) */

/* WATER_WATCHER_PLATFORM_* */
#include <cstring>

#if defined(WATER_WATCHER_PLATFORM_MAC_ANY)

/*
  @brief watcher/adapter/darwin

  The Darwin `FSEvent` adapter.
*/

/* kFS*
   FS*
   CF*
   dispatch_queue* */
#include <CoreServices/CoreServices.h>
/* milliseconds */
#include <chrono>
/* function */
#include <functional>
/* path */
#include <filesystem>
/* numeric_limits */
#include <limits>
/* mt19937
   random_device
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
/* snprintf */
#include <cstdio>
/* unordered_set */
#include <unordered_set>
/* event
   callback */

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

struct argptr_type
{
  event::callback const& callback;
  std::unordered_set<std::string>* seen_created_paths;
};

inline constexpr auto delay_ms = std::chrono::milliseconds(16);
inline constexpr auto delay_s
    = std::chrono::duration_cast<std::chrono::seconds>(delay_ms);
inline constexpr auto has_delay = delay_ms.count() > 0;

inline constexpr auto time_flag = kFSEventStreamEventIdSinceNow;

/* We could OR `event_stream_flags` with `kFSEventStreamCreateFlagNoDefer` if we
   want less "sleepy" time after a period of no filesystem events. But we're
   talking about saving a maximum latency of `delay_ms` after some period of
   inactivity -- very small. (Not sure what the inactivity period is.) */
inline constexpr auto event_stream_flags
    = kFSEventStreamCreateFlagFileEvents
      | kFSEventStreamCreateFlagUseExtendedData
      | kFSEventStreamCreateFlagUseCFTypes;

inline std::tuple<FSEventStreamRef, dispatch_queue_t> event_stream_open(
    std::filesystem::path const& path, FSEventStreamCallback funcptr,
    argptr_type const& funcptr_args, std::function<void()> lifetime_fn) noexcept
{
  static constexpr CFIndex path_array_size{1};
  static constexpr auto queue_priority = -10;

  auto funcptr_context = FSEventStreamContext{0, (void*)&funcptr_args, nullptr,
                                              nullptr, nullptr};
  /* Creating this untyped array of strings is unavoidable.
     `path_cfstring` and `path_cfarray_cfstring` must be temporaries because
     `CFArrayCreate` takes the address of a string and `FSEventStreamCreate` the
     address of an array (of strings). There might be some UB around here. */
  void const* path_cfstring
      = CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
  CFArrayRef path_array = CFArrayCreate(
      nullptr, &path_cfstring, path_array_size, &kCFTypeArrayCallBacks);

  /* The event queue name doesn't seem to need to be unique.
     We try to make a unique name anyway, just in case.
     The event queue name will be:
       = "wtr" + [0, 28) character number
     And will always be a string between 5 and 32-characters long:
       = 3 (prefix) + [1, 28] (digits) + 1 (null char from snprintf) */
  char queue_name[3 + 28 + 1]{};
  std::mt19937 gen(std::random_device{}());
  std::snprintf(queue_name, sizeof(queue_name), "wtr%zu",
                std::uniform_int_distribution<size_t>(
                    0, std::numeric_limits<size_t>::max())(gen));

  /* Request a file event stream for `path` from the kernel
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

  /* Request a (very) high priority queue. */
  dispatch_queue_t queue = dispatch_queue_create(
      queue_name,
      dispatch_queue_attr_make_with_qos_class(
          DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, queue_priority));

  FSEventStreamSetDispatchQueue(stream, queue);

  FSEventStreamStart(stream);

  lifetime_fn();

  return std::make_tuple(stream, queue);
}

/* @note
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
  }
  return false;
}

inline std::filesystem::path path_from_event_at(void* event_recv_paths,
                                                unsigned long i) noexcept
{
  /* We make a path from a C string...
     In an array, in a dictionary...
     Without type safety...
     Because most of darwin's api's are `void*`-typed.

     We are *should be* guarenteed at least:
       1. Every target type's alignment:
          Although the aliases are untyped,
          the function names are.
       2. Non-null aliases:
          The dictionary and array data
          structures that we're working with
          are immutable and refcounted.

     IOW we can't guarentee type safety through types,
     but this is how Darwin's API is intended to be used. */
  return {CFStringGetCStringPtr(
      static_cast<CFStringRef>(CFDictionaryGetValue(
          static_cast<CFDictionaryRef>(
              CFArrayGetValueAtIndex(static_cast<CFArrayRef>(event_recv_paths),
                                     static_cast<CFIndex>(i))),
          kFSEventStreamEventExtendedDataPathKey)),
      kCFStringEncodingUTF8)};
}

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
inline void event_recv(ConstFSEventStreamRef,    /* `ConstFS..` is important */
                       void* arg_ptr,            /* Arguments passed to us */
                       unsigned long recv_count, /* Event count */
                       void* recv_paths,         /* Paths with events */
                       unsigned int const* recv_flags, /* Event flags */
                       FSEventStreamEventId const*     /* event stream id */
                       ) noexcept
{
  using evk = wtr::watcher::event::kind;
  using evw = wtr::watcher::event::what;

  auto [callback, seen_created] = *static_cast<argptr_type*>(arg_ptr);

  for (unsigned long i = 0; i < recv_count; i++) {
    auto path = path_from_event_at(recv_paths, i);
    /* `path` has no hash function, so we use a string. */
    auto path_str = path.string();

    decltype(*recv_flags) flag = recv_flags[i];

    /* A single path won't have different "kinds". */
    auto k = flag & kFSEventStreamEventFlagItemIsFile      ? evk::file
             : flag & kFSEventStreamEventFlagItemIsDir     ? evk::dir
             : flag & kFSEventStreamEventFlagItemIsSymlink ? evk::sym_link
             : flag
                     & (kFSEventStreamEventFlagItemIsHardlink
                        | kFSEventStreamEventFlagItemIsLastHardlink)
                 ? evk::hard_link
                 : evk::other;

    /* More than one thing might have happened to the same path.
       (Which is why we use non-exclusive `if`s.) */
    if (flag & kFSEventStreamEventFlagItemCreated) {
      if (seen_created->find(path_str) == seen_created->end()) {
        seen_created->emplace(path_str);
        callback(event::event{path, evw::create, k});
      }
    }
    if (flag & kFSEventStreamEventFlagItemRemoved) {
      auto const& seen_created_at = seen_created->find(path_str);
      if (seen_created_at != seen_created->end()) {
        seen_created->erase(seen_created_at);
        callback(event::event{path, evw::destroy, k});
      }
    }
    if (flag & kFSEventStreamEventFlagItemModified) {
      callback(event::event{path, evw::modify, k});
    }
    if (flag & kFSEventStreamEventFlagItemRenamed) {
      callback(event::event{path, evw::rename, k});
    }
  }
}

} /* namespace */

inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  using evk = ::wtr::watcher::event::kind;
  using evw = ::wtr::watcher::event::what;
  using std::this_thread::sleep_for;

  auto seen_created_paths = std::unordered_set<std::string>{};
  auto event_recv_argptr = argptr_type{callback, &seen_created_paths};

  auto ok = event_stream_close(
      event_stream_open(path, event_recv, event_recv_argptr, [&is_living]() {
        while (is_living())
          if constexpr (has_delay) sleep_for(delay_ms);
      }));

  if (ok)
    callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});

  else
    callback({"e/self/die@" + path.string(), evw::destroy, evk::watcher});

  return ok;
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr   */

#endif /* if defined(WATER_WATCHER_PLATFORM_MAC_ANY) */

/*
  @brief wtr/watcher/<d>/adapter/linux/fanotify

  The Linux `fanotify` adapter.
*/

/* WATER_WATCHER_PLATFORM_* */

#if defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_5_9_0) \
    && !defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

#define WATER_WATCHER_ADAPTER_LINUX_FANOTIFY

/* O_* */
#include <fcntl.h>
/* EPOLL*
   epoll_ctl
   epoll_wait
   epoll_event
   epoll_create1 */
#include <sys/epoll.h>
/* FAN_*
   fanotify_mark
   fanotify_init
   fanotify_event_metadata */
#include <sys/fanotify.h>
/* open
   close
   readlink */
#include <unistd.h>
/* errno */
#include <cerrno>
/* PATH_MAX */
#include <climits>
/* snprintf */
#include <cstdio>
/* strerror */
#include <cstring>
/* path
   is_directory
   directory_options
   recursive_directory_iterator */
#include <filesystem>
/* function */
#include <functional>
/* optional */
#include <optional>
/* unordered_map */
#include <unordered_map>
/* unordered_set */
#include <unordered_set>
/* tuple
   make_tuple */
#include <tuple>
/* event
   callback */

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace fanotify {

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>
   Anonymous namespace for "private" things. */
namespace {

/*  @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/constants
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
            * (3 * sizeof(fanotify_event_metadata)));
      But that's a lot of flourish for 72 bytes that won't
      be meaningful.

    - fan_init_flags:
      Post-event reporting, non-blocking IO and unlimited
      marks. We need sudo mode for the unlimited marks.
      If we were making a filesystem auditor, we might use:
          FAN_CLASS_PRE_CONTENT
          | FAN_UNLIMITED_QUEUE
          | FAN_UNLIMITED_MARKS

    - fan_init_opt_flags:
      Read-only, non-blocking, and close-on-exec. */
inline constexpr auto delay_ms = 16;
inline constexpr auto event_wait_queue_max = 1;
inline constexpr auto event_buf_len = PATH_MAX;
inline constexpr auto fan_init_flags = FAN_CLASS_NOTIF | FAN_REPORT_DFID_NAME
                                       | FAN_UNLIMITED_QUEUE
                                       | FAN_UNLIMITED_MARKS;
inline constexpr auto fan_init_opt_flags = O_RDONLY | O_NONBLOCK | O_CLOEXEC;

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/types
   - sys_resource_type
       An object holding:
         - An fanotify file descriptor
         - An epoll file descriptor
         - An epoll configuration
         - A set of watch marks (as returned by fanotify_mark)
         - A map of (sub)path handles to filesystem paths (names)
         - A boolean representing the validity of these resources */
using mark_set_type = std::unordered_set<int>;
using dir_map_type = std::unordered_map<unsigned long, std::filesystem::path>;

struct sys_resource_type
{
  bool valid;
  int watch_fd;
  int event_fd;
  epoll_event event_conf;
  mark_set_type mark_set;
  dir_map_type dir_map;
};

inline auto mark(std::filesystem::path const& full_path, int watch_fd,
                 mark_set_type& pmc) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd, FAN_MARK_ADD,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                             | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD, full_path.c_str());
  if (wd >= 0) {
    pmc.insert(wd);
    return true;

  } else
    return false;
};

inline auto mark(std::filesystem::path const& full_path, sys_resource_type& sr,
                 unsigned long dir_hash) noexcept -> bool
{
  if (sr.dir_map.find(dir_hash) == sr.dir_map.end())
    return mark(full_path, sr.watch_fd, sr.mark_set)
           && sr.dir_map.emplace(dir_hash, full_path.parent_path()).second;

  else
    return mark(full_path, sr.watch_fd, sr.mark_set);
}

inline auto unmark(std::filesystem::path const& full_path, int watch_fd,
                   mark_set_type& mark_set) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd, FAN_MARK_REMOVE,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                             | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD, full_path.c_str());
  auto const& at = mark_set.find(wd);

  if (wd >= 0 && at != mark_set.end()) {
    mark_set.erase(at);
    return true;

  } else
    return false;
};

inline auto unmark(std::filesystem::path const& full_path,
                   sys_resource_type& sr, unsigned long dir_hash) noexcept
    -> bool
{
  auto const& at = sr.dir_map.find(dir_hash);

  if (at != sr.dir_map.end()) sr.dir_map.erase(at);

  return unmark(full_path, sr.watch_fd, sr.mark_set);
}

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_sys_resource_open
   Produces a `sys_resource_type` with the file descriptors from
   `fanotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto do_sys_resource_open(std::filesystem::path const& path,
                                 event::callback const& callback) noexcept
    -> sys_resource_type
{
  namespace fs = ::std::filesystem;
  using evk = ::wtr::watcher::event::kind;
  using evw = ::wtr::watcher::event::what;

  auto const& do_error
      = [&callback](auto const& error, auto const& path, int watch_fd,
                    int event_fd = -1) noexcept -> sys_resource_type {
    auto msg = std::string(error)
                   .append("(")
                   .append(std::strerror(errno))
                   .append(")@")
                   .append(path);
    callback({msg, evw::other, evk::watcher});
    return sys_resource_type{
        .valid = false,
        .watch_fd = watch_fd,
        .event_fd = event_fd,
        .event_conf = {.events = 0, .data = {.fd = watch_fd}},
        .mark_set = {},
        .dir_map = {},
    };
  };
  auto do_path_map_container_create
      = [](int const watch_fd, fs::path const& base_path,
           event::callback const& callback) -> mark_set_type {
    using diter = fs::recursive_directory_iterator;

    /* Follow symlinks, ignore paths which we don't have permissions for. */
    static constexpr auto dopt
        = fs::directory_options::skip_permission_denied
          & fs::directory_options::follow_directory_symlink;

    static constexpr auto rsrv_count = 1024;

    mark_set_type pmc;
    pmc.reserve(rsrv_count);

    /* The filesystem library throws here even if we use the error code
       overloads. (Exceptions seem to be the only way to handle errors.) */

    if (mark(base_path, watch_fd, pmc))
      if (fs::is_directory(base_path)) try
        {
          for (auto& dir : diter(base_path, dopt))
            if (fs::is_directory(dir))
              if (!mark(dir.path(), watch_fd, pmc))
                callback({"w/sys/not_watched@" / base_path / "@" / dir.path(),
                          evw::other, evk::watcher});
        } catch (...) {
          callback(
              {"w/sys/not_watched@" / base_path, evw::other, evk::watcher});
        }

    return pmc;
  };

  int watch_fd = fanotify_init(fan_init_flags, fan_init_opt_flags);
  if (watch_fd >= 0) {
    auto pmc = do_path_map_container_create(watch_fd, path, callback);
    if (!pmc.empty()) {
      epoll_event event_conf{.events = EPOLLIN, .data{.fd = watch_fd}};

      int event_fd = epoll_create1(EPOLL_CLOEXEC);

      /* @note We could make the epoll and fanotify file descriptors
         non-blocking with `fcntl`. It's not clear if we can do this
         from their `*_init` calls. */
      /* fcntl(watch_fd, F_SETFL, O_NONBLOCK); */
      /* fcntl(event_fd, F_SETFL, O_NONBLOCK); */

      if (event_fd >= 0)
        if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) >= 0)
          return sys_resource_type{
              .valid = true,
              .watch_fd = watch_fd,
              .event_fd = event_fd,
              .event_conf = event_conf,
              .mark_set = std::move(pmc),
              .dir_map = {},
          };
        else
          return do_error("e/sys/epoll_ctl", path, watch_fd, event_fd);
      else
        return do_error("e/sys/epoll_create", path, watch_fd, event_fd);
    } else
      return do_error("e/sys/fanotify_mark", path, watch_fd);
  } else
    return do_error("e/sys/fanotify_init", path, watch_fd);
}

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_sys_resource_close
   Close the file descriptors `watch_fd` and `event_fd`. */
inline auto do_sys_resource_close(sys_resource_type& sr) noexcept -> bool
{
  return !(close(sr.watch_fd) && close(sr.event_fd));
}

/*  @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/lift_event
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
      - Dir + Dir Entry:  31966ns

    The shenanigans we do here depend on this event being
    `FAN_EVENT_INFO_TYPE_DFID_NAME`. The kernel passes us
    some info about the directory and the directory entry
    (the filename) of this event that doesn't exist when
    other event types are reported. In particular, we need
    a file descriptor to the directory (which we use
    `readlink` on) and a character string representing the
    name of the directory entry.
    TLDR: We need information for the full path of the event,
    information which is only reported inside this `if`.

    From the kernel:
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

    The kernel guarentees that there is a null-terminated
    character string to the event's directory entry
    after the file handle to the directory.
    Confusing, right? */
inline auto lift_event(sys_resource_type& sr,
                       fanotify_event_info_fid const* dir_fid_info,
                       std::filesystem::path const& base_path,
                       event::callback const& callback) noexcept
    -> std::tuple<std::filesystem::path, unsigned long>
{
  namespace fs = ::std::filesystem;

  auto path_imbue
      = [](char* path_accum, fanotify_event_info_fid const* dfid_info,
           file_handle* dir_fh, ssize_t dir_name_len = 0) noexcept -> void {
    char* name_info = (char*)(dfid_info + 1);
    char* file_name = static_cast<char*>(
        name_info + sizeof(file_handle) + sizeof(dir_fh->f_handle)
        + sizeof(dir_fh->handle_bytes) + sizeof(dir_fh->handle_type));

    if (file_name && std::strcmp(file_name, ".") != 0)
      std::snprintf(path_accum + dir_name_len, PATH_MAX - dir_name_len, "/%s",
                    file_name);
  };

  auto* dir_fh = (file_handle*)(dir_fid_info->handle);

  /* The sum of the handle's bytes. A low-quality hash.
     Unreliable after the directory's inode is recycled. */
  unsigned long dir_hash = std::abs(dir_fh->handle_type);
  for (unsigned char i = 0; i < dir_fh->handle_bytes; i++)
    dir_hash += *(dir_fh->f_handle + i);

  auto const& cache = sr.dir_map.find(dir_hash);

  if (cache != sr.dir_map.end()) {
    /* We already have a path name, use it */
    char path_buf[PATH_MAX];
    auto dir_name = cache->second.c_str();
    auto dir_name_len = cache->second.string().length();

    /* std::snprintf(path_buf, sizeof(path_buf), "%s", dir_name); */
    std::memcpy(path_buf, dir_name, dir_name_len);
    path_imbue(path_buf, dir_fid_info, dir_fh, dir_name_len);

    return std::make_tuple(fs::path{std::move(path_buf)}, dir_hash);

  } else {
    /* We can get a path name, so get that and use it */
    char path_buf[PATH_MAX];
    int fd = open_by_handle_at(AT_FDCWD, dir_fh,
                               O_RDONLY | O_CLOEXEC | O_PATH | O_NONBLOCK);
    if (fd > 0) {
      char procpath[128];
      std::snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", fd);
      ssize_t dirname_len
          = readlink(procpath, path_buf, sizeof(path_buf) - sizeof('\0'));
      close(fd);

      if (dirname_len > 0) {
        /* Put the directory name in the path accumulator.
           Passing `dirname_len` has the effect of putting
           the event's filename in the path buffer as well. */
        path_buf[dirname_len] = '\0';
        path_imbue(path_buf, dir_fid_info, dir_fh, dirname_len);

        return std::make_tuple(fs::path{std::move(path_buf)}, dir_hash);
      } else {
        callback({"w/sys/readlink@" / base_path, event::what::other,
                  event::kind::watcher});

        return std::make_tuple(fs::path{}, 0);
      }
    } else {
      path_imbue(path_buf, dir_fid_info, dir_fh);

      return std::make_tuple(fs::path{std::move(path_buf)}, dir_hash);
    }
  }
};

/* @brief wtr/watcher/<d>/adapter/linux/fanotify/<a>/fns/do_event_send
   Send events to the user. */
inline auto do_event_send(std::filesystem::path const& base_path,
                          event::callback const& callback,
                          sys_resource_type& sr,
                          fanotify_event_metadata const* metadata) noexcept
    -> bool
{
  using namespace ::wtr::watcher::event;

  auto [path, hash]
      = lift_event(sr, ((fanotify_event_info_fid const*)(metadata + 1)),
                   base_path, callback);

  auto m = metadata->mask;

  auto w = m & FAN_CREATE   ? what::create
           : m & FAN_DELETE ? what::destroy
           : m & FAN_MODIFY ? what::modify
           : m & FAN_MOVE   ? what::rename
                            : what::other;

  auto k = m & FAN_ONDIR ? kind::dir : kind::file;

  callback({path, w, k});

  return

      hash ? k == kind::dir ? w == what::create    ? mark(path, sr, hash)
                              : w == what::destroy ? unmark(path, sr, hash)
                                                   : true
                            : true
           : false;
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
                          std::filesystem::path const& base_path,
                          event::callback const& callback) noexcept -> bool
{
  enum class state { ok, none, err };

  auto do_error = [&base_path, &callback](char const* msg) noexcept -> bool {
    callback({msg / base_path, event::what::other, event::kind::watcher});
    return false;
  };

  /* Read some events. */
  alignas(fanotify_event_metadata) char event_buf[event_buf_len];
  auto event_read = read(sr.watch_fd, event_buf, sizeof(event_buf));

  switch (event_read > 0    ? state::ok
          : event_read == 0 ? state::none
          : errno == EAGAIN ? state::none
                            : state::err)
  {
    case state::ok: {
      /* Loop over everything in the event buffer. */
      for (auto* metadata = (fanotify_event_metadata const*)event_buf;
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
                return do_event_send(base_path, callback, sr, metadata);

              else
                return !do_error("w/self/event_info");
            else
              return do_error("e/sys/overflow");
          else
            return do_error("e/sys/kernel_version");
        else
          return do_error("e/sys/wrong_event_fd");
    } break;

    case state::none: return true; break;

    case state::err: return do_error("e/sys/read"); break;
  }

  /* Unreachable */
  return false;
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
                  std::function<bool()> const& is_living) noexcept
{
  auto do_error
      = [&path, &callback](sys_resource_type& sr, char const* msg) -> bool {
    using evk = ::wtr::watcher::event::kind;
    using evw = ::wtr::watcher::event::what;

    callback({msg / path, evw::other, evk::watcher});

    if (do_sys_resource_close(sr))
      callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});

    else
      callback({"e/self/die@" + path.string(), evw::other, evk::watcher});

    return false;
  };

  /* We have:
       - system resources
           For fanotify and epoll
       - event recieve list
           For receiving epoll events */

  sys_resource_type sr = do_sys_resource_open(path, callback);

  if (sr.valid) {
    epoll_event event_recv_list[event_wait_queue_max];

    /* While living:
        - Await filesystem events
        - Invoke `callback` on errors and events */

    while (is_living()) {
      int event_count = epoll_wait(sr.event_fd, event_recv_list,
                                   event_wait_queue_max, delay_ms);
      if (event_count < 0)
        return do_error(sr, "e/sys/epoll_wait");

      else if (event_count > 0) [[likely]]
        for (int n = 0; n < event_count; n++)
          if (event_recv_list[n].data.fd == sr.watch_fd) [[likely]]
            if (is_living()) [[likely]]
              if (!do_event_recv(sr, path, callback)) [[unlikely]]
                return do_error(sr, "e/self/event_recv");
    }

    callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});
    return do_sys_resource_close(sr);

  } else
    return do_error(sr, "e/self/sys_resource");
}

} /* namespace fanotify */
} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* !defined(WATER_WATCHER_USE_WARTHOG) */
#endif /* defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_5_9_0) \
          && !defined(WATER_WATCHER_PLATFORM_ANDROID_ANY) */

/*
  @brief wtr/watcher/<d>/adapter/linux/inotify

  The Linux `inotify` adapter.
*/

/* WATER_WATCHER_PLATFORM_* */

#if defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_2_7_0) \
    || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

#define WATER_WATCHER_ADAPTER_LINUX_INOTIFY

/* EPOLL*
   epoll_ctl
   epoll_wait
   epoll_event
   epoll_create
   epoll_create1 */
#include <sys/epoll.h>
/* IN_*
   inotify_init
   inotify_init1
   inotify_event
   inotify_add_watch */
#include <sys/inotify.h>
/* open
   read
   close */
#include <unistd.h>
/* path
   is_directory
   directory_options
   recursive_directory_iterator */
#include <filesystem>
/* function */
#include <functional>
/* tuple
   make_tuple */
#include <tuple>
/* unordered_map */
#include <unordered_map>
/* memcpy */
#include <cstring>
/* event
   callback */

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
  epoll_event event_conf;
};

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_path_map_create
   If the path given is a directory
     - find all directories above the base path given.
     - ignore nonexistent directories.
     - return a map of watch descriptors -> directories.
   If `path` is a file
     - return it as the only value in a map.
     - the watch descriptor key should always be 1. */
inline auto do_path_map_create(int const watch_fd,
                               std::filesystem::path const& base_path,
                               event::callback const& callback) noexcept
    -> path_map_type
{
  namespace fs = ::std::filesystem;
  using diter = fs::recursive_directory_iterator;
  using dopt = fs::directory_options;

  /* Follow symlinks, ignore paths which we don't have permissions for. */
  static constexpr auto fs_dir_opt
      = dopt::skip_permission_denied & dopt::follow_directory_symlink;

  static constexpr auto path_map_reserve_count = 256;

  auto dir_ec = std::error_code{};
  auto path_map = path_map_type{};
  path_map.reserve(path_map_reserve_count);

  auto do_mark = [&](fs::path const& d) noexcept -> bool {
    int wd = inotify_add_watch(watch_fd, d.c_str(), in_watch_opt);
    return wd > 0 ? path_map.emplace(wd, d).first != path_map.end() : false;
  };

  if (do_mark(base_path))
    if (fs::is_directory(base_path, dir_ec))
      for (auto dir : diter(base_path, fs_dir_opt, dir_ec))
        if (!dir_ec)
          if (fs::is_directory(dir, dir_ec))
            if (!dir_ec)
              if (!do_mark(dir.path()))
                callback({"w/sys/path_unwatched@" / dir.path(),
                          event::what::other, event::kind::watcher});

  return path_map;
};

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_sys_resource_open
   Produces a `sys_resource_type` with the file descriptors from
   `inotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto do_sys_resource_open(event::callback const& callback) noexcept
    -> sys_resource_type
{
  auto do_error = [&callback](auto msg, int watch_fd,
                              int event_fd = -1) noexcept -> sys_resource_type {
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
#elif defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_ANY)
      = inotify_init1(in_init_opt);
#endif

  if (watch_fd >= 0) {
    epoll_event event_conf{.events = EPOLLIN, .data{.fd = watch_fd}};

    int event_fd
#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
        = epoll_create(event_wait_queue_max);
#elif defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_ANY)
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
  } else
    return do_error("e/sys/inotify_init", watch_fd);
}

/* @brief wtr/watcher/<d>/adapter/linux/inotify/<a>/fns/do_sys_resource_close
   Close the file descriptors `watch_fd` and `event_fd`. */
inline auto do_sys_resource_close(sys_resource_type& sr) noexcept -> bool
{
  return !(close(sr.watch_fd) && close(sr.event_fd));
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
                          std::filesystem::path const& base_path,
                          event::callback const& callback) noexcept -> bool
{
  namespace fs = ::std::filesystem;
  using evk = ::wtr::watcher::event::kind;
  using evw = ::wtr::watcher::event::what;

  alignas(inotify_event) char buf[event_buf_len];

  enum class state { eventful, eventless, error };

  /* While inotify has events pending, read them.
     There might be several events from a single read.

     Three possible states:
      - eventful: there are events to read
      - eventless: there are no events to read
      - error: there was an error reading events

     The EAGAIN "error" means there is nothing
     to read. We count that as 'eventless'.

     Forward events and errors to the user.

     Return when eventless. */

recurse:

  ssize_t read_len = read(watch_fd, buf, event_buf_len);

  switch (read_len > 0      ? state::eventful
          : read_len == 0   ? state::eventless
          : errno == EAGAIN ? state::eventless
                            : state::error)
  {
    case state::eventful:
      /* Loop over all events in the buffer. */
      for (auto this_event = (inotify_event*)buf;
           this_event < (inotify_event*)(buf + read_len);
           this_event += this_event->len)
      {
        if (!(this_event->mask & IN_Q_OVERFLOW)) [[likely]] {
          auto path = path_map.find(this_event->wd)->second
                      / fs::path(this_event->name);

          auto kind = this_event->mask & IN_ISDIR ? evk::dir : evk::file;

          auto what = this_event->mask & IN_CREATE   ? evw::create
                      : this_event->mask & IN_DELETE ? evw::destroy
                      : this_event->mask & IN_MOVE   ? evw::rename
                      : this_event->mask & IN_MODIFY ? evw::modify
                                                     : evw::other;

          callback({path, what, kind});

          if (kind == evk::dir && what == evw::create)
            path_map[inotify_add_watch(watch_fd, path.c_str(), in_watch_opt)]
                = path;

          else if (kind == evk::dir && what == evw::destroy) {
            inotify_rm_watch(watch_fd, this_event->wd);
            path_map.erase(this_event->wd);
          }

        } else
          callback({"e/self/overflow@" / base_path, evw::other, evk::watcher});
      }
      /* Same as `return do_event_recv(..., buf)`.
         Our stopping condition is `eventless` or `error`. */
      goto recurse;

    case state::error:
      callback({"e/sys/read@" / base_path, evw::other, evk::watcher});
      return false;

    case state::eventless: return true;
  }

  /* Unreachable */
  return false;
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
                  std::function<bool()> const& is_living) noexcept
{
  auto do_error
      = [&path, &callback](sys_resource_type& sr, char const* msg) -> bool {
    using evk = ::wtr::watcher::event::kind;
    using evw = ::wtr::watcher::event::what;

    callback({msg / path, evw::other, evk::watcher});

    if (do_sys_resource_close(sr))
      callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});

    else
      callback({"e/self/die@" + path.string(), evw::other, evk::watcher});

    return false;
  };

  /* We have:
       - system resources
           For inotify and epoll
       - event recieve list
           For receiving epoll events
       - path map
           For event to path lookups */

  sys_resource_type sr = do_sys_resource_open(callback);
  if (sr.valid) {
    auto path_map = do_path_map_create(sr.watch_fd, path, callback);

    if (path_map.size() > 0) {
      epoll_event event_recv_list[event_wait_queue_max];

      /* While living:
          - Await filesystem events
          - Invoke `callback` on errors and events */

      while (is_living()) {
        int event_count = epoll_wait(sr.event_fd, event_recv_list,
                                     event_wait_queue_max, delay_ms);

        if (event_count < 0)
          return do_error(sr, "e/sys/epoll_wait@");

        else if (event_count > 0) [[likely]]
          for (int n = 0; n < event_count; n++)
            if (event_recv_list[n].data.fd == sr.watch_fd) [[likely]]
              if (!do_event_recv(sr.watch_fd, path_map, path, callback))
                  [[unlikely]]
                return do_error(sr, "e/self/event_recv@");
      }

      callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});
      return do_sys_resource_close(sr);

    } else
      return do_error(sr, "e/self/path_map@");

  } else
    return do_error(sr, "e/self/sys_resource@");
}

} /* namespace inotify */
} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* !defined(WATER_WATCHER_USE_WARTHOG) */
#endif /* defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_2_7_0) \
          || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY) */

/*
  @brief wtr/watcher/detail/adapter/linux

  The Linux adapters.
*/

/* WATER_WATCHER_PLATFORM_* */

#if defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_2_7_0) \
    || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#if !defined(WATER_WATCHER_USE_WARTHOG)

/* function */
#include <functional>
/* geteuid */
#include <unistd.h>
/* event
   callback
   inotify::watch
   fanotify::watch */

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

/*
  @brief watcher/detail/adapter/watch

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

  @note
  If we have a kernel that can use either `fanotify` or
  `inotify`, then we will use `fanotify` if the user is
  (effectively) root.

  If we can only use `fanotify` or `inotify`, then we'll
  use them. We only use `inotify` on Android.

  There should never be a system that can use `fanotify`,
  but not `inotify`. It's just here for completeness.
*/

inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  return

#if defined(WATER_WATCHER_ADAPTER_LINUX_FANOTIFY) \
    && defined(WATER_WATCHER_ADAPTER_LINUX_INOTIFY)

      geteuid() == 0 ? fanotify::watch(path, callback, is_living)
                     : inotify::watch(path, callback, is_living);

#elif defined(WATER_WATCHER_ADAPTER_LINUX_FANOTIFY)

      fanotify::watch(path, callback, is_living);

#elif defined(WATER_WATCHER_ADAPTER_LINUX_INOTIFY)

      inotify::watch(path, callback, is_living);

#else

#error "Define 'WATER_WATCHER_USE_WARTHOG' on kernel versions < 2.7"

#endif
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* defined(WATER_WATCHER_PLATFORM_LINUX_KERNEL_GTE_2_7_0) \
          || defined(WATER_WATCHER_PLATFORM_ANDROID_ANY) */
#endif /* !defined(WATER_WATCHER_USE_WARTHOG) */

/*
  @brief watcher/adapter/android

  The Android (Linux) `inotify` adapter.
*/

/* WATER_WATCHER_PLATFORM_* */

#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#endif

/* WATER_WATCHER_PLATFORM_* */

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

/* milliseconds */
#include <chrono>
/* string */
#include <string>
/* filesystem::* */
#include <filesystem>
/* function */
#include <functional>
/* error_code */
#include <system_error>
/* this_thread::sleep_for */
#include <thread>
/* unordered_map */
#include <unordered_map>
/* event
   callback */

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
  auto const& scan_file
      = [&](std::filesystem::path const& file, auto const& send_event) -> bool {
    using std::filesystem::exists, std::filesystem::is_regular_file,
        std::filesystem::last_write_time;
    if (exists(file) && is_regular_file(file)) {
      auto ec = std::error_code{};
      /* grabbing the file's last write time */
      auto const& timestamp = last_write_time(file, ec);
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
  auto const& scan_directory
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
  auto const& populate = [&](std::filesystem::path const& path) -> bool {
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
            auto const& lwt = last_write_time(file, lwt_ec);
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
  auto const& prune
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
                  std::function<bool()> const& is_living) noexcept
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

  while (is_living()) {
    if (!tend_bucket(path, callback, bucket) || !scan(path, callback, bucket)) {
      callback(
          {"e/self/die/bad_fs@" + path.string(), evw::destroy, evk::watcher});

      return false;
    } else {
      if constexpr (delay_ms > 0) sleep_for(milliseconds(delay_ms));
    }
  }

  callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});

  return true;
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* defined(WATER_WATCHER_PLATFORM_UNKNOWN) \
          || defined(WATER_WATCHER_USE_WARTHOG) */

/*  path */
#include <filesystem>
/*  shared_ptr */
#include <memory>
/*  mutex
    scoped_lock */
#include <mutex>
/*  mt19937
    random_device
    uniform_int_distribution */
#include <random>
/*  unordered_map */
#include <unordered_map>
/*  watch
    event
    callback */

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

enum class word { live, die };

struct message
{
  word word{word::live};
  size_t id{0};
};

inline message carry(std::shared_ptr<message> m) noexcept
{
  auto random_id = []() noexcept -> size_t {
    auto rng{std::mt19937{std::random_device{}()}};
    return std::uniform_int_distribution<size_t>{}(rng);
  };

  static auto mtx{std::mutex{}};

  auto _ = std::scoped_lock<std::mutex>{mtx};

  auto const w{m->word};

  m->word = word::die;

  m->id = m->id > 0 ? m->id : random_id();

  return message{w, m->id};
};

inline size_t adapter(std::filesystem::path const& path,
                      event::callback const& callback,
                      std::shared_ptr<message> previous) noexcept
{
  using evw = ::wtr::watcher::event::what;
  using evk = ::wtr::watcher::event::kind;

  /*  This map associates watchers with (maybe not unique) paths. */
  static auto lifetimes{std::unordered_map<size_t, std::filesystem::path>{}};

  /*  A mutex to synchronize access to the container. */
  static auto lifetimes_mtx{std::mutex{}};

  auto const msg = carry(previous);

  /*  Returns a functor to check if we're still living.
      The functor is unique to every watcher. */
  auto const& live = [id = msg.id, &path, &callback]() -> bool {
    auto const& create_lifetime
        = [id, &path, &callback]() noexcept -> std::function<bool()> {
      auto _ = std::scoped_lock{lifetimes_mtx};

      auto const maybe_node = lifetimes.find(id);

      if (maybe_node == lifetimes.end()) [[likely]] {
        lifetimes[id] = path;

        callback({"s/self/live@" + path.string(), evw::create, evk::watcher});

        return [id]() noexcept -> bool {
          auto _ = std::scoped_lock{lifetimes_mtx};

          return lifetimes.find(id) != lifetimes.end();
        };

      } else {
        callback({"e/self/already_alive@" + path.string(), evw::create,
                  evk::watcher});

        return []() constexpr noexcept -> bool { return false; };
      }
    };

    return watch(path, callback, create_lifetime()) ? id : 0;
  };

  auto const& die = [id = msg.id]() noexcept -> size_t {
    auto _ = std::scoped_lock{lifetimes_mtx};

    auto const maybe_node = lifetimes.find(id);

    if (maybe_node != lifetimes.end()) [[likely]] {
      size_t id = maybe_node->first;

      lifetimes.erase(maybe_node);

      return id;

    } else
      return 0;
  };

  switch (msg.word) {
    case word::live: return live();

    case word::die: return die();

    default: return false;
  }
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

/*  path */
#include <filesystem>
/*  async */
#include <future>
/*  function */
#include <functional>
/*  shared_ptr */
#include <memory>
/*  event
    callback
    adapter */

namespace wtr {
namespace watcher {

/*  @brief wtr/watcher/watch

    @param path:
      The root path to watch for filesystem events.

    @param living_cb (optional):
      Something (such as a closure) to be called when events
      occur in the path being watched.

    This is an adaptor "switch" that chooses the ideal adaptor
    for the host platform.

    Every adapter monitors `path` for changes and invokes the
    `callback` with an `event` object when they occur.

    There are two things the user needs:
      - The `watch` function
      - The `event` object

    The watch function returns a function to stop its watcher.

    Typical use looks like this:
      auto lifetime = watch(".", [](event& e) {
        std::cout
          << "where: " << e.where << "\n"
          << "kind: "  << e.kind  << "\n"
          << "what: "  << e.what  << "\n"
          << "when: "  << e.when  << "\n"
          << std::endl;
      };

      auto dead = lifetime();

    That's it.

    Happy hacking. */

[[nodiscard("Returns a function used to stop this watcher")]]

inline auto
watch(std::filesystem::path const& path,
      event::callback const& callback) noexcept -> std::function<bool()>
{
  using namespace ::wtr::watcher::detail::adapter;

  auto msg = std::shared_ptr<message>{new message{}};

  auto lifetime = std::async(std::launch::async, [=]() noexcept -> bool {
                    return adapter(path, callback, msg);
                  }).share();

  return [=]() noexcept -> bool {
    return adapter(path, callback, msg) && lifetime.get();
  };
}

} /* namespace watcher */
} /* namespace wtr   */

/*
  @brief watcher/watcher

  This is the public interface.
  Include and use this file.
*/

/* clang-format off */
/* clang-format on */
#endif /* W973564ED9F278A21F3E12037288412FBAF175F889 */
