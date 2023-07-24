#ifndef W973564ED9F278A21F3E12037288412FBAF175F889
#define W973564ED9F278A21F3E12037288412FBAF175F889

/* std::basic_ostream */
#include <ios>
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
inline namespace watcher {

/*  @brief watcher/event
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
    important status updates.  */

struct event {

private:
  /* I like these names. Very human.
     'what happen'
     'event kind' */
  using path_type = std::filesystem::path;
  using ns = std::chrono::nanoseconds;
  using clock = std::chrono::system_clock;
  using time_point = std::chrono::time_point<clock>;

public:
  /*  @brief watcher/event/callback
      Ensure the adapters can recieve events
      and will return nothing. */
  using callback = std::function<void(event const&)>;

  /*  @brief wtr/watcher/event/what
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

  /*  @brief wtr/watcher/event/kind
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

  path_type const where{};

  enum what const what {};

  enum kind const kind {};

  long long const when{
    duration_cast<ns>(time_point{clock::now()}.time_since_epoch()).count()};

  event(std::filesystem::path const& where,
        enum what const& what,
        enum kind const& kind) noexcept
      : where{where},
        what{what},
        kind{kind} {};

  ~event() noexcept = default;
};

/*  @brief wtr/watcher/event/<<
    Streams out a `what` value. */
template<class Char, class CharTraits>
inline constexpr auto operator<<(std::basic_ostream<Char, CharTraits>& os,
                                 enum event::what const& w) noexcept
  -> std::basic_ostream<Char, CharTraits>&
{
  /* clang-format off */
  switch (w) {
    case event::what::rename  : return os << "\"rename\"";
    case event::what::modify  : return os << "\"modify\"";
    case event::what::create  : return os << "\"create\"";
    case event::what::destroy : return os << "\"destroy\"";
    case event::what::owner   : return os << "\"owner\"";
    case event::what::other   : return os << "\"other\"";
    default                   : return os << "\"other\"";
  }
  /* clang-format on */
};

/*  @brief wtr/watcher/event/<<
    Streams out a `kind` value. */
template<class Char, class CharTraits>
inline constexpr auto operator<<(std::basic_ostream<Char, CharTraits>& os,
                                 enum event::kind const& k) noexcept
  -> std::basic_ostream<Char, CharTraits>&
{
  /* clang-format off */
  switch (k) {
    case event::kind::dir       : return os << "\"dir\"";
    case event::kind::file      : return os << "\"file\"";
    case event::kind::hard_link : return os << "\"hard_link\"";
    case event::kind::sym_link  : return os << "\"sym_link\"";
    case event::kind::watcher   : return os << "\"watcher\"";
    case event::kind::other     : return os << "\"other\"";
    default                     : return os << "\"other\"";
  }
  /* clang-format on */
};

/*  @brief wtr/watcher/event/<<
    Streams out `where`, `what` and `kind`.
    Formats the stream as a json object.
    Looks like this (without line breaks)
      "1678046920675963000":{
       "where":"/some_file.txt",
       "what":"create",
       "kind":"file"
      } */
template<class Char, class CharTraits>
inline constexpr auto operator<<(std::basic_ostream<Char, CharTraits>& os,
                                 event const& ev) noexcept
  -> std::basic_ostream<Char, CharTraits>&
{
  /* clang-format off */
    return os << '"' << ev.when
              << "\":{\"where\":" << ev.where
              <<    ",\"what\":"  << ev.what
              <<    ",\"kind\":"  << ev.kind  << '}';
  /* clang-format on */
};

/*  @brief wtr/watcher/event/==
    A "strict" comparison of an event's `when`,
    `where`, `what` and `kind` values.
    Keep in mind that this compares `when`,
    which might not be desireable. */
inline auto operator==(event const& l, event const& r) noexcept -> bool
{
  /* clang-format off */
  return l.where == r.where
      && l.what  == r.what
      && l.kind  == r.kind
      && l.when  == r.when;
  /* clang-format on */
};

/*  @brief wtr/watcher/event/!=
    Not == */
inline auto operator!=(event const& l, event const& r) noexcept -> bool
{
  return ! (l == r);
};

} /* namespace watcher */
} /* namespace wtr   */

#if defined(_WIN32)

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <windows.h>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace {

inline constexpr auto delay_ms = std::chrono::milliseconds(16);
inline constexpr auto delay_ms_dw = static_cast<DWORD>(delay_ms.count());
inline constexpr auto has_delay = delay_ms > std::chrono::milliseconds(0);
/*  I think the default page size in Windows is 64kb,
    so 65536 might also work well. */
inline constexpr auto event_buf_len_max = 8192;

/*  Hold resources necessary to recieve and send filesystem events. */
class watch_event_proxy {
public:
  bool is_valid{true};

  std::filesystem::path path{};

  wchar_t path_name[256]{L""};

  HANDLE path_handle{nullptr};

  HANDLE event_completion_token{nullptr};

  HANDLE event_token{CreateEventW(nullptr, true, false, nullptr)};

  OVERLAPPED event_overlap{};

  FILE_NOTIFY_INFORMATION event_buf[event_buf_len_max];

  DWORD event_buf_len_ready{0};

  watch_event_proxy(std::filesystem::path const& path) noexcept
      : path{path}
  {
    memcpy(path_name, path.c_str(), path.string().size());

    path_handle =
      CreateFileW(path.c_str(),
                  FILE_LIST_DIRECTORY,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr,
                  OPEN_EXISTING,
                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                  nullptr);

    if (path_handle)
      event_completion_token =
        CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

    if (event_completion_token)
      is_valid = CreateIoCompletionPort(path_handle,
                                        event_completion_token,
                                        (ULONG_PTR)path_handle,
                                        1)
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

inline bool
do_event_recv(watch_event_proxy& w,
              ::wtr::watcher::event::callback const& callback) noexcept
{
  using namespace ::wtr::watcher;

  w.event_buf_len_ready = 0;
  DWORD bytes_returned = 0;
  memset(&w.event_overlap, 0, sizeof(OVERLAPPED));

  auto read_ok = ReadDirectoryChangesW(
    w.path_handle,
    w.event_buf,
    event_buf_len_max,
    true,
    FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION
      | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE
      | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES
      | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME,
    &bytes_returned,
    &w.event_overlap,
    nullptr);

  if (w.event_buf && read_ok) {
    w.event_buf_len_ready = bytes_returned > 0 ? bytes_returned : 0;
    return true;
  }
  else {
    switch (GetLastError()) {
      case ERROR_IO_PENDING :
        w.event_buf_len_ready = 0;
        w.is_valid = false;
        callback({"e/sys/read/pending",
                  ::wtr::event::what::other,
                  ::wtr::event::kind::watcher});
        break;
      default :
        callback({"e/sys/read",
                  ::wtr::event::what::other,
                  ::wtr::event::kind::watcher});
        break;
    }
    return false;
  }
}

inline bool
do_event_send(watch_event_proxy& w,
              ::wtr::watcher::event::callback const& callback) noexcept
{
  using namespace ::wtr::watcher;

  FILE_NOTIFY_INFORMATION* buf = w.event_buf;

  if (is_valid(w)) {
    while (buf + sizeof(FILE_NOTIFY_INFORMATION)
           <= buf + w.event_buf_len_ready) {
      if (buf->FileNameLength % 2 == 0) {
        auto where =
          w.path / std::wstring{buf->FileName, buf->FileNameLength / 2};

        auto what = [&buf]() noexcept
        {
          switch (buf->Action) {
            case FILE_ACTION_MODIFIED : return event::what::modify;
            case FILE_ACTION_ADDED : return event::what::create;
            case FILE_ACTION_REMOVED : return event::what::destroy;
            case FILE_ACTION_RENAMED_OLD_NAME : return event::what::rename;
            case FILE_ACTION_RENAMED_NEW_NAME : return event::what::rename;
            default : return event::what::other;
          }
        }();

        auto kind = [&where]()
        {
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
          buf =
            (FILE_NOTIFY_INFORMATION*)((uint8_t*)buf + buf->NextEntryOffset);
      }
    }
    return true;
  }

  else {
    return false;
  }
}  // namespace

}  // namespace

/*  while living
    watch for events
    return when dead
    true if no errors */
inline bool watch(std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  using namespace ::wtr::watcher;

  auto w = watch_event_proxy{path};

  if (is_valid(w)) {
    do_event_recv(w, callback);

    while (is_valid(w) && has_event(w)) { do_event_send(w, callback); }

    while (is_living()) {
      ULONG_PTR completion_key{0};
      LPOVERLAPPED overlap{nullptr};

      bool complete = GetQueuedCompletionStatus(w.event_completion_token,
                                                &w.event_buf_len_ready,
                                                &completion_key,
                                                &overlap,
                                                delay_ms_dw);

      if (complete && overlap) {
        while (is_valid(w) && has_event(w)) {
          do_event_send(w, callback);
          do_event_recv(w, callback);
        }
      }
    }

    callback({"s/self/die@" + path.string(),
              ::wtr::event::what::destroy,
              ::wtr::event::kind::watcher});

    return true;
  }
  else {
    callback({"s/self/die@" + path.string(),
              ::wtr::event::what::destroy,
              ::wtr::event::kind::watcher});
    return false;
  }
}

} /* namespace adapter */
} /* namespace watcher */
} /* namespace wtr */
} /* namespace detail */

#endif

#if defined(__APPLE__)

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
  using pathset = ::std::unordered_set<::std::string>;
  ::wtr::watcher::event::callback const callback{};
  ::std::shared_ptr<pathset> seen_created_paths{new pathset{}};
};

struct sysres_type {
  FSEventStreamRef stream{};
  argptr_type argptr{};
};

inline auto path_from_event_at(void* event_recv_paths, unsigned long i) noexcept
  -> ::std::filesystem::path
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
inline auto event_recv(ConstFSEventStreamRef,    /*  `ConstFS..` is important */
                       void* argptr,             /*  Arguments passed to us */
                       unsigned long recv_count, /*  Event count */
                       void* recv_paths,         /*  Paths with events */
                       unsigned int const* recv_flags, /*  Event flags */
                       FSEventStreamEventId const*     /*  event stream id */
                       ) noexcept -> void
{
  using evk = enum ::wtr::watcher::event::kind;
  using evw = enum ::wtr::watcher::event::what;

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

        /*  A single path won't have different "kinds". */
        auto k = flag & kFSEventStreamEventFlagItemIsFile    ? evk::file
               : flag & kFSEventStreamEventFlagItemIsDir     ? evk::dir
               : flag & kFSEventStreamEventFlagItemIsSymlink ? evk::sym_link
               : flag & any_hard_link                        ? evk::hard_link
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

/*  Make sure that event_recv has the same type as, or is
    convertible to, an FSEventStreamCallback. We don't use
    `is_same_v()` here because `event_recv` is `noexcept`.
    Side note: Is an exception qualifier *really* part of
    the type? Or, is it a "kind"? Something else?
    We want this assertion for nice compiler errors. */
static_assert(FSEventStreamCallback{event_recv} == event_recv);

inline auto
open_event_stream(::std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback) noexcept
  -> std::shared_ptr<sysres_type>
{
  using namespace std::chrono_literals;
  using std::chrono::duration_cast, std::chrono::seconds;

  auto sysres =
    std::make_shared<sysres_type>(sysres_type{nullptr, argptr_type{callback}});

  auto context = FSEventStreamContext{
    /*  FSEvents.h: "Currently the only valid value is zero." */
    0,
    /*  The "actual" context; our "argument pointer". This must
        be alive until we clear the event stream. */
    static_cast<void*>(&sysres->argptr),
    /*  An optional "retention" callback. We don't need this
        because we manage `argptr`'s lifetime ourselves */
    nullptr,
    /*  An optional "release" callback, not needed for the
        same reasons as the retention callback */
    nullptr,
    /*  An optional string for debugging purposes */
    nullptr};

  void const* path_cfstring =
    CFStringCreateWithCString(nullptr, path.c_str(), kCFStringEncodingUTF8);
  CFArrayRef path_array = CFArrayCreate(
    /*  A custom allocator is optional */
    nullptr,
    /*  Data: A pointer-to-pointer of (in our case) strings */
    &path_cfstring,
    /*  We're just storing one path here */
    1,
    /*  A predefined structure which is "appropriate for use when
        the values in a CFArray are all CFTypes" (CFArray.h) */
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

  /*  Request a filesystem event stream for `path` from the kernel.
      The event stream will call `event_recv` with `context`
      and some details about each filesystem event the kernel sees
      for the paths in `path_array`. */
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
    event_stream_flags);

  FSEventStreamSetDispatchQueue(
    stream,
    /*  We don't need to retain, maintain or release this dispatch
        queue. It's a global system queue, and it outlives us. */
    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

  FSEventStreamStart(stream);

  sysres->stream = stream;

  return sysres;
}

inline auto
close_event_stream(std::shared_ptr<sysres_type> const& sysres) noexcept -> bool
{
  if (sysres->stream) {
    /*  `FSEventStreamInvalidate()` only needs to be called if
        we scheduled via `FSEventStreamScheduleWithRunLoop()`.
        That scheduling function is deprecated (as of macOS 13).
        Calling `FSEventStreamInvalidate()` fails an assertion
        and produces a warning in the console. However, calling
        `FSEventStreamRelease()` without first invalidating via
        `FSEventStreamInvalidate()` *also* fails an assertions,
        and produces a warning. I'm not sure what the right call
        to make here is. */
    FSEventStreamStop(sysres->stream);
    // ?? FSEventStreamInvalidate(sysres->stream);
    FSEventStreamRelease(sysres->stream);
    sysres->stream = nullptr;
    return true;
  }
  else
    return false;
}

/*  Keeping this here, away from the `while (is_living()) ...`
    loop, because I'm thinking about moving all the lifetime
    management up a layer or two. Maybe the user-facing `watch`
    class can take over the sleep timer, threading, and closing
    the system's resources. Maybe we don't even need an adapter
    layer... Just a way to expose a concistent and non-blocking
    kernel API.

    The sleep timer and threading are probably unnecessary,
    anyway. Maybe there's some stop token or something more
    asio-like that we can use instead of the sleep/`is_living()`
    loop. Instead of threading, we should just become part of an
    `io_context` and let `asio` handle the runtime.

    I'm also thinking of ways use `asio` in this project. The
    `awaitable` coroutines look like they might fit. Might need
    to rip out the `callback` param. This is a relatively small
    project, so there isn't *too* much work to do. (Last words?) */
inline auto open_watch(::std::filesystem::path const& path,
                       ::wtr::watcher::event::callback const& callback) noexcept
{
  return [sysres = open_event_stream(path, callback)]() noexcept -> bool
  { return close_event_stream(std::move(sysres)); };
}

} /* namespace */

inline auto watch(::std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback,
                  ::std::function<bool()> const& is_living) noexcept -> bool
{
  using namespace ::std::chrono_literals;
  using evk = enum ::wtr::watcher::event::kind;
  using evw = enum ::wtr::watcher::event::what;
  using ::std::this_thread::sleep_for;

  auto close_watch = open_watch(path, callback);
  while (is_living()) sleep_for(16ms);
  auto ok = close_watch();
  if (ok)
    callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});
  else
    callback({"e/self/die@" + path.string(), evw::destroy, evk::watcher});
  return ok;
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr   */
} /*  namespace detail */

#endif

#if (defined(__linux__) || defined(__ANDROID_API__)) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)) \
  && ! defined(__ANDROID_API__)

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <optional>
#include <sys/epoll.h>
#include <sys/fanotify.h>
#include <system_error>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace fanotify {

/*  - delay
      The delay, in milliseconds, while `epoll_wait` will
      'sleep' for until we are woken up. We usually check
      if we're still alive at that point.
    - event_wait_queue_max
      Number of events allowed to be given to recv
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

/*  - mark_set_type
        A set of file descriptors for fanotify resources.
    - system_resources
        An object holding:
          - An fanotify file descriptor
          - An epoll file descriptor
          - An epoll configuration
          - A set of watch marks (as returned by fanotify_mark)
          - A map of (sub)path handles to filesystem paths (names)
          - A boolean: whether or not the resources are valid */
using mark_set_type = std::unordered_set<int>;

struct system_resources {
  bool valid;
  int watch_fd;
  int event_fd;
  epoll_event event_conf;
  mark_set_type mark_set;
};

inline auto mark(std::filesystem::path const& full_path,
                 int watch_fd,
                 mark_set_type& ms) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd,
                         FAN_MARK_ADD,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                           | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD,
                         full_path.c_str());
  if (wd >= 0) {
    ms.insert(wd);
    return true;
  }
  else
    return false;
};

inline auto mark(std::filesystem::path const& full_path,
                 system_resources& sr) noexcept -> bool
{
  return mark(full_path, sr.watch_fd, sr.mark_set);
};

inline auto unmark(std::filesystem::path const& full_path,
                   int watch_fd,
                   mark_set_type& mark_set) noexcept -> bool
{
  int wd = fanotify_mark(watch_fd,
                         FAN_MARK_REMOVE,
                         FAN_ONDIR | FAN_CREATE | FAN_MODIFY | FAN_DELETE
                           | FAN_MOVE | FAN_DELETE_SELF | FAN_MOVE_SELF,
                         AT_FDCWD,
                         full_path.c_str());
  auto const& at = mark_set.find(wd);

  if (wd >= 0 && at != mark_set.end()) {
    mark_set.erase(at);
    return true;
  }
  else
    return false;
};

inline auto unmark(std::filesystem::path const& full_path,
                   system_resources& sr) noexcept -> bool
{
  return unmark(full_path, sr.watch_fd, sr.mark_set);
};

/*  Produces a `system_resources` with the file descriptors from
    `fanotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto
open_system_resources(std::filesystem::path const& path,
                      ::wtr::watcher::event::callback const& callback) noexcept
  -> system_resources
{
  namespace fs = ::std::filesystem;
  using ev = ::wtr::watcher::event;

  auto do_error = [&path,
                   &callback](char const* const msg,
                              int watch_fd,
                              int event_fd = -1) noexcept -> system_resources
  {
    callback(
      {std::string{msg} + "(" + std::strerror(errno) + ")@" + path.string(),
       ev::what::other,
       ev::kind::watcher});

    return system_resources{
      .valid = false,
      .watch_fd = watch_fd,
      .event_fd = event_fd,
      .event_conf = {.events = 0, .data = {.fd = watch_fd}},
      .mark_set = {},
    };
  };

  auto do_path_map_container_create =
    [](int const watch_fd,
       fs::path const& base_path,
       ::wtr::watcher::event::callback const& callback) -> mark_set_type
  {
    using diter = fs::recursive_directory_iterator;

    /*  Follow symlinks, ignore paths which we don't have permissions for. */
    static constexpr auto dopt =
      fs::directory_options::skip_permission_denied
      & fs::directory_options::follow_directory_symlink;

    static constexpr auto rsrv_count = 1024;

    auto pmc = mark_set_type{};
    pmc.reserve(rsrv_count);

    try {
      if (mark(base_path, watch_fd, pmc))
        if (fs::is_directory(base_path))
          for (auto& dir : diter(base_path, dopt))
            if (fs::is_directory(dir))
              if (! mark(dir.path(), watch_fd, pmc))
                callback({"w/sys/not_watched@" + base_path.string() + "@"
                            + dir.path().string(),
                          ev::what::other,
                          ev::kind::watcher});
    } catch (...) {}

    return pmc;
  };

  int watch_fd = fanotify_init(fan_init_flags, fan_init_opt_flags);
  if (watch_fd >= 0) {
    auto pmc = do_path_map_container_create(watch_fd, path, callback);
    if (! pmc.empty()) {
      epoll_event event_conf{.events = EPOLLIN, .data{.fd = watch_fd}};

      int event_fd = epoll_create1(EPOLL_CLOEXEC);

      /*  @note We could make the epoll and fanotify file descriptors
          non-blocking with `fcntl`. It's not clear if we can do this
          from their `*_init` calls.

          fcntl(watch_fd, F_SETFL, O_NONBLOCK);
          fcntl(event_fd, F_SETFL, O_NONBLOCK); */

      if (event_fd >= 0)
        if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) >= 0)
          return system_resources{
            .valid = true,
            .watch_fd = watch_fd,
            .event_fd = event_fd,
            .event_conf = event_conf,
            .mark_set = std::move(pmc),
          };
        else
          return do_error("e/sys/epoll_ctl", watch_fd, event_fd);
      else
        return do_error("e/sys/epoll_create", watch_fd, event_fd);
    }
    else
      return do_error("e/sys/fanotify_mark", watch_fd);
  }
  else
    return do_error("e/sys/fanotify_init", watch_fd);
};

inline auto close_system_resources(system_resources&& sr) noexcept -> bool
{
  return close(sr.watch_fd) == 0 && close(sr.event_fd) == 0;
};

/*  "Promotes" an event's metadata to a full path.
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
// clang-format off
// note at the end of file re. clang format
inline auto promote(fanotify_event_metadata const* mtd) noexcept
  -> std::tuple<bool,
                std::filesystem::path,
                enum ::wtr::watcher::event::what,
                enum ::wtr::watcher::event::kind>
{
  namespace fs = ::std::filesystem;
  using ev = ::wtr::watcher::event;

  auto path_imbue = [](char* path_accum,
                       fanotify_event_info_fid const* dfid_info,
                       file_handle* dir_fh,
                       ssize_t dir_name_len = 0) noexcept -> void
  {
    char* name_info = (char*)(dfid_info + 1);
    char* file_name = static_cast<char*>(
      name_info + sizeof(file_handle) + sizeof(dir_fh->f_handle)
      + sizeof(dir_fh->handle_bytes) + sizeof(dir_fh->handle_type));

    if (file_name && std::strcmp(file_name, ".") != 0)
      std::snprintf(path_accum + dir_name_len,
                    PATH_MAX - dir_name_len,
                    "/%s",
                    file_name);
  };

  auto dir_fid_info = ((fanotify_event_info_fid const*)(mtd + 1));

  auto dir_fh = (file_handle*)(dir_fid_info->handle);

  auto what = mtd->mask & FAN_CREATE ? ev::what::create
            : mtd->mask & FAN_DELETE ? ev::what::destroy
            : mtd->mask & FAN_MODIFY ? ev::what::modify
            : mtd->mask & FAN_MOVE   ? ev::what::rename
                                     : ev::what::other;

  auto kind = mtd->mask & FAN_ONDIR ? ev::kind::dir : ev::kind::file;

  /*  We can get a path name, so get that and use it */
  char path_buf[PATH_MAX];
  int fd = open_by_handle_at(AT_FDCWD,
                             dir_fh,
                             O_RDONLY | O_CLOEXEC | O_PATH | O_NONBLOCK);
  if (fd > 0) {
    char fs_proc_path[128];
    std::snprintf(fs_proc_path, sizeof(fs_proc_path), "/proc/self/fd/%d", fd);
    ssize_t dirname_len =
      readlink(fs_proc_path, path_buf, sizeof(path_buf) - sizeof('\0'));
    close(fd);

    if (dirname_len > 0) {
      /*  Put the directory name in the path accumulator.
          Passing `dirname_len` has the effect of putting
          the event's filename in the path buffer as well. */
      path_buf[dirname_len] = '\0';
      path_imbue(path_buf, dir_fid_info, dir_fh, dirname_len);

      return std::make_tuple(true,
                             fs::path{std::move(path_buf)},
                             what,
                             kind);
    }

    else
      return std::make_tuple(false, fs::path{}, what, kind);
  }
  else {
    path_imbue(path_buf, dir_fid_info, dir_fh);

    return std::make_tuple(true,
                           fs::path{std::move(path_buf)},
                           what,
                           kind);
  }
};

// clang-format on

inline auto
check_and_update(std::tuple<bool,
                            std::filesystem::path,
                            enum ::wtr::watcher::event::what,
                            enum ::wtr::watcher::event::kind> const& r,
                 system_resources& sr) noexcept
  -> std::tuple<bool,
                std::filesystem::path,
                enum ::wtr::watcher::event::what,
                enum ::wtr::watcher::event::kind> {
    using ev = ::wtr::watcher::event;

    auto [valid, path, what, kind] = r;

    return std::make_tuple(

      valid

        ? kind == ev::kind::dir

          ? what == ev::what::create  ? mark(path, sr)
          : what == ev::what::destroy ? unmark(path, sr)
                                      : true

          : true

        : false,

      path,

      what,

      kind);
  };

/*  Send events to the user.
    This is the important part.
    Most of the other code is
    a layer of translation
    between us and the kernel. */
inline auto
send(std::tuple<bool,
                std::filesystem::path,
                enum ::wtr::watcher::event::what,
                enum ::wtr::watcher::event::kind> const& from_kernel,
     ::wtr::watcher::event::callback const& callback) noexcept -> bool
{
  auto [ok, path, what, kind] = from_kernel;

  return ok ? (callback({path, what, kind}), ok) : ok;
};

/*  Reads through available (fanotify) filesystem events.
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
inline auto recv(system_resources& sr,
                 std::filesystem::path const& base_path,
                 ::wtr::watcher::event::callback const& callback) noexcept
  -> bool
{
  enum class state { ok, none, err };

  auto do_error = [&base_path,
                   &callback](char const* const msg) noexcept -> bool
  {
    return (callback({std::string{msg} + "@" + base_path.string(),
                      ::wtr::watcher::event::what::other,
                      ::wtr::watcher::event::kind::watcher}),
            false);
  };

  /*  Read some events. */
  alignas(fanotify_event_metadata) char event_buf[event_buf_len];
  auto event_read = read(sr.watch_fd, event_buf, sizeof(event_buf));

  switch (event_read > 0    ? state::ok
          : event_read == 0 ? state::none
          : errno == EAGAIN ? state::none
                            : state::err) {
    case state::ok : {
      /*  Loop over everything in the event buffer. */
      for (auto* mtd = (fanotify_event_metadata const*)event_buf;
           FAN_EVENT_OK(mtd, event_read);
           mtd = FAN_EVENT_NEXT(mtd, event_read))
        if (mtd->fd == FAN_NOFD) [[likely]]
          if (mtd->vers == FANOTIFY_METADATA_VERSION) [[likely]]
            if (! (mtd->mask & FAN_Q_OVERFLOW)) [[likely]]
              if (((fanotify_event_info_fid*)(mtd + 1))->hdr.info_type
                  == FAN_EVENT_INFO_TYPE_DFID_NAME) [[likely]]

                /*  Send the events we receive. */
                return send(check_and_update(promote(mtd), sr), callback);

              else
                return ! do_error("w/self/event_info");
            else
              return do_error("e/sys/overflow");
          else
            return do_error("e/sys/kernel_version");
        else
          return do_error("e/sys/wrong_event_fd");
    } break;

    case state::none : return true; break;

    case state::err : return do_error("e/sys/read"); break;
  }

  /*  Unreachable */
  return false;
};

inline bool watch(std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  using ev = ::wtr::watcher::event;

  auto done = [&path, &callback](system_resources&& sr) noexcept -> bool
  {
    return

      close_system_resources(std::move(sr))

        ? (callback({"s/self/die@" + path.string(),
                     ev::what::other,
                     ev::kind::watcher}),
           true)

        : (callback({"e/self/die@" + path.string(),
                     ev::what::other,
                     ev::kind::watcher}),
           false);
  };

  auto do_error = [&path, &callback, &done](system_resources&& sr,
                                            char const* const msg) -> bool
  {
    return (callback({std::string{msg} + "@" + path.string(),
                      ev::what::other,
                      ev::kind::watcher}),

            done(std::move(sr)),

            false);
  };

  /*  While:
      - System resources for fanotify and epoll
      - An event list for receiving epoll events
      - We're alive
      Do:
      - Await filesystem events
      - Invoke `callback` on errors and events */

  auto sr = open_system_resources(path, callback);

  epoll_event event_recv_list[event_wait_queue_max];

  if (sr.valid) [[likely]] {
    while (is_living()) [[likely]]

    {
      int event_count = epoll_wait(sr.event_fd,
                                   event_recv_list,
                                   event_wait_queue_max,
                                   delay_ms);
      if (event_count < 0)
        return do_error(std::move(sr), "e/sys/epoll_wait");

      else if (event_count > 0) [[likely]]
        for (int n = 0; n < event_count; n++)
          if (event_recv_list[n].data.fd == sr.watch_fd) [[likely]]
            if (is_living()) [[likely]]
              if (! recv(sr, path, callback)) [[unlikely]]
                return do_error(std::move(sr), "e/self/event_recv");
    }

    return done(std::move(sr));
  }

  else
    return do_error(std::move(sr), "e/self/sys_resource");
};

// clang-format off
// returning tuples is confusing clang format

}  /* namespace fanotify */
}  /* namespace adapter */
}  /* namespace watcher */
}  /* namespace wtr */
}  /* namespace detail */

// clang-format on

#endif
#endif

#if (defined(__linux__) || defined(__ANDROID_API__)) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 7, 0)) || defined(__ANDROID_API__)

#include <cstring>
#include <filesystem>
#include <functional>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <system_error>
#include <tuple>
#include <unistd.h>
#include <unordered_map>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace inotify {

/*  - delay
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
inline constexpr auto in_watch_opt =
  IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_Q_OVERFLOW;

/*  - path_map_type
        An alias for a map of file descriptors to paths.
    - sys_resource_type
        An object representing an inotify file descriptor,
        an epoll file descriptor, an epoll configuration,
        and whether or not the resources are valid */
using path_map_type = std::unordered_map<int, std::filesystem::path>;

struct sys_resource_type {
  bool valid;
  int watch_fd;
  int event_fd;
  epoll_event event_conf;
};

/*  If the path given is a directory
      - find all directories above the base path given.
      - ignore nonexistent directories.
      - return a map of watch descriptors -> directories.
    If `path` is a file
      - return it as the only value in a map.
      - the watch descriptor key should always be 1. */
inline auto path_map(std::filesystem::path const& base_path,
                     ::wtr::watcher::event::callback const& callback,
                     sys_resource_type const& sr) noexcept -> path_map_type
{
  namespace fs = ::std::filesystem;
  using ev = ::wtr::watcher::event;
  using diter = fs::recursive_directory_iterator;
  using dopt = fs::directory_options;

  /*  Follow symlinks, ignore paths which we don't have permissions for. */
  static constexpr auto fs_dir_opt =
    dopt::skip_permission_denied & dopt::follow_directory_symlink;

  static constexpr auto path_map_reserve_count = 256;

  auto pm = path_map_type{};
  pm.reserve(path_map_reserve_count);

  auto do_mark = [&](fs::path const& d) noexcept -> bool
  {
    int wd = inotify_add_watch(sr.watch_fd, d.c_str(), in_watch_opt);
    return wd > 0 ? pm.emplace(wd, d).first != pm.end() : false;
  };

  try {
    if (sr.valid)
      if (do_mark(base_path))
        if (fs::is_directory(base_path))
          for (auto dir : diter(base_path, fs_dir_opt))
            if (fs::is_directory(dir))
              if (! do_mark(dir.path()))
                callback({"w/sys/not_watched@" + base_path.string() + "@"
                            + dir.path().string(),
                          ev::what::other,
                          ev::kind::watcher});
  } catch (...) {}

  return pm;
};

/*  Produces a `sys_resource_type` with the file descriptors from
    `inotify_init` and `epoll_create`. Invokes `callback` on errors. */
inline auto
system_unfold(::wtr::watcher::event::callback const& callback) noexcept
  -> sys_resource_type
{
  auto do_error = [&callback](char const* const msg,
                              int watch_fd,
                              int event_fd = -1) noexcept -> sys_resource_type
  {
    callback({msg,
              ::wtr::watcher::event::what::other,
              ::wtr::watcher::event::kind::watcher});
    return sys_resource_type{
      .valid = false,
      .watch_fd = watch_fd,
      .event_fd = event_fd,
      .event_conf = {.events = 0, .data = {.fd = watch_fd}}
    };
  };

  int watch_fd
#if defined(__ANDROID_API__)
    = inotify_init();
#else
    = inotify_init1(in_init_opt);
#endif

  if (watch_fd >= 0) {
    epoll_event event_conf{.events = EPOLLIN, .data{.fd = watch_fd}};

    int event_fd
#if defined(__ANDROID_API__)
      = epoll_create(event_wait_queue_max);
#else
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
  }
  else
    return do_error("e/sys/inotify_init", watch_fd);
}

inline auto system_fold(sys_resource_type& sr) noexcept -> bool
{
  return ! (close(sr.watch_fd) && close(sr.event_fd));
}

/*  Reads through available (inotify) filesystem events.
    Discerns their path and type.
    Calls the callback.
    Returns false on eventful errors.
    @todo
    Return new directories when they appear,
    Consider running and returning `find_dirs` from here.
    Remove destroyed watches. */
inline auto
do_event_recv(int watch_fd,
              path_map_type& pm,
              std::filesystem::path const& base_path,
              ::wtr::watcher::event::callback const& callback) noexcept -> bool
{
  namespace fs = ::std::filesystem;

  alignas(inotify_event) char buf[event_buf_len];

  enum class state { eventful, eventless, error };

  /*  While inotify has events pending, read them.
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
                            : state::error) {
    case state::eventful : {
      /*  Loop over all events in the buffer. */
      auto this_event = (inotify_event*)buf;
      while (this_event < (inotify_event*)(buf + read_len)) {
        if (! (this_event->mask & IN_Q_OVERFLOW)) [[likely]] {
          auto path =
            pm.find(this_event->wd)->second / fs::path(this_event->name);

          auto kind = this_event->mask & IN_ISDIR
                      ? ::wtr::watcher::event::kind::dir
                      : ::wtr::watcher::event::kind::file;

          auto what =
            this_event->mask & IN_CREATE ? ::wtr::watcher::event::what::create
            : this_event->mask & IN_DELETE
              ? ::wtr::watcher::event::what::destroy
            : this_event->mask & IN_MOVE   ? ::wtr::watcher::event::what::rename
            : this_event->mask & IN_MODIFY ? ::wtr::watcher::event::what::modify
                                           : ::wtr::watcher::event::what::other;

          callback({path, what, kind});

          if (kind == ::wtr::watcher::event::kind::dir
              && what == ::wtr::watcher::event::what::create)
            pm[inotify_add_watch(watch_fd, path.c_str(), in_watch_opt)] = path;

          else if (kind == ::wtr::watcher::event::kind::dir
                   && what == ::wtr::watcher::event::what::destroy) {
            inotify_rm_watch(watch_fd, this_event->wd);
            pm.erase(this_event->wd);
          }
        }
        else
          callback({"e/self/overflow@" + base_path.string(),
                    ::wtr::watcher::event::what::other,
                    ::wtr::watcher::event::kind::watcher});

        this_event += sizeof(inotify_event);
      }
      /*  Same as `return do_event_recv(..., buf)`.
          Our stopping condition is `eventless` or `error`. */
      goto recurse;
    }

    case state::error :
      callback({"e/sys/read@" + base_path.string(),
                ::wtr::watcher::event::what::other,
                ::wtr::watcher::event::kind::watcher});
      return false;

    case state::eventless : return true;
  }

  /*  Unreachable */
  return false;
}

inline bool watch(std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  using ev = ::wtr::watcher::event;

  auto do_error = [&path, &callback](bool clean, std::string&& msg) -> bool
  {
    callback({msg + path.string(), ev::what::other, ev::kind::watcher});

    if (clean)
      callback(
        {"s/self/die@" + path.string(), ev::what::other, ev::kind::watcher});

    else
      callback(
        {"e/self/die@" + path.string(), ev::what::other, ev::kind::watcher});

    return false;
  };

  /*  While:
      - A lifetime the user hasn't ended
      - A historical map of watch descriptors
        to long paths (for event reporting)
      - System resources for inotify and epoll
      - An event buffer for events from epoll
      - We're alive
      Do:
      - Await filesystem events
      - Invoke `callback` on errors and events */

  sys_resource_type sr = system_unfold(callback);

  epoll_event event_recv_list[event_wait_queue_max];

  auto pm = path_map(path, callback, sr);

  if (sr.valid) [[likely]]

    if (pm.size() > 0) [[likely]] {
      while (is_living()) [[likely]]

      {
        int event_count = epoll_wait(sr.event_fd,
                                     event_recv_list,
                                     event_wait_queue_max,
                                     delay_ms);

        if (event_count < 0)
          return do_error(system_fold(sr), "e/sys/epoll_wait@");

        else if (event_count > 0) [[likely]]
          for (int n = 0; n < event_count; n++)
            if (event_recv_list[n].data.fd == sr.watch_fd) [[likely]]
              if (! do_event_recv(sr.watch_fd, pm, path, callback)) [[unlikely]]
                return do_error(system_fold(sr), "e/self/event_recv@");
      }

      callback(
        {"s/self/die@" + path.string(), ev::what::destroy, ev::kind::watcher});
      return system_fold(sr);
    }
    else
      return do_error(system_fold(sr), "e/self/path_map@");

  else
    return do_error(system_fold(sr), "e/self/sys_resource@");
}

} /* namespace inotify */
} /* namespace adapter */
} /* namespace watcher */
} /* namespace wtr */
} /* namespace detail */

#endif
#endif

#if (defined(__linux__) || defined(__ANDROID_API__)) \
  && ! defined(WATER_WATCHER_USE_WARTHOG)

#include <functional>
#include <linux/version.h>
#include <unistd.h>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {

/*  If we have a kernel that can use either `fanotify` or
    `inotify`, then we will use `fanotify` if the user is
    (effectively) root.
    We can use `fanotify` (and `inotify`, too) on a kernel
    version 5.9.0 or greater.
    If we can only use `inotify`, then we'll just use that.
    We only use `inotify` on Android and on kernel versions
    less than 5.9.0 and greater than/equal to 2.7.0.
    Below 2.7.0, you can use the `warthog` adapter by defining
    `WATER_WATCHER_USE_WARTHOG` at some point during the build
    or before including 'wtr/watcher.hpp'. */

inline bool watch(std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)) \
  && ! defined(__ANDROID_API__)
  return geteuid() == 0 ? fanotify::watch(path, callback, is_living)
                        : inotify::watch(path, callback, is_living);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 7, 0)) \
  || defined(__ANDROID_API__)
  return inotify::watch(path, callback, is_living);
#else
#error "Define 'WATER_WATCHER_USE_WARTHOG' on kernel versions < 2.7.0"
#endif
}

} /* namespace adapter */
} /* namespace watcher */
} /* namespace wtr */
} /* namespace detail */

#endif

/*  A sturdy, universal adapter.

    This is the fallback adapter on platforms that either
      - Only support `kqueue` (`warthog` beats `kqueue`)
      - Only support the C++ standard library */

#if ! defined(__linux__) && ! defined(__ANDROID_API__) && ! defined(__APPLE__) \
  && ! defined(_WIN32)

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {
namespace {

/* clang-format off */

inline constexpr std::filesystem::directory_options
  scan_dir_options = 
    std::filesystem::directory_options::skip_permission_denied 
    & std::filesystem::directory_options::follow_directory_symlink;

using bucket_type = std::unordered_map<std::string, std::filesystem::file_time_type>;

/* clang-format on */

/*  - Scans `path` for changes.
    - Updates our bucket to match the changes.
    - Calls `send_event` when changes happen.
    - Returns false if the file tree cannot be scanned. */
inline bool scan(std::filesystem::path const& path,
                 auto const& send_event,
                 bucket_type& bucket) noexcept
{
  /*  - Scans a (single) file for changes.
      - Updates our bucket to match the changes.
      - Calls `send_event` when changes happen.
      - Returns false if the file cannot be scanned. */
  auto const& scan_file = [&](std::filesystem::path const& file,
                              auto const& send_event) -> bool
  {
    using namespace ::wtr::watcher;
    using std::filesystem::exists, std::filesystem::is_regular_file,
      std::filesystem::last_write_time;

    if (exists(file) && is_regular_file(file)) {
      auto ec = std::error_code{};
      /*  grabbing the file's last write time */
      auto const& timestamp = last_write_time(file, ec);
      if (ec) {
        /*  the file changed while we were looking at it. so, we call the
            closure, indicating destruction, and remove it from the bucket. */
        send_event(event{file, event::what::destroy, event::kind::file});
        if (bucket.contains(file)) bucket.erase(file);
      }
      /*  if it's not in our bucket, */
      else if (! bucket.contains(file)) {
        /*  we put it in there and call the closure, indicating creation. */
        bucket[file] = timestamp;
        send_event(event{file, event::what::create, event::kind::file});
      }
      /*  otherwise, it is already in our bucket. */
      else {
        /*  we update the file's last write time, */
        if (bucket[file] != timestamp) {
          bucket[file] = timestamp;
          /*  and call the closure on them, indicating modification */
          send_event(event{file, event::what::modify, event::kind::file});
        }
      }
      return true;
    } /*  if the path doesn't exist, we nudge the callee with `false` */
    else
      return false;
  };

  /*  - Scans a (single) directory for changes.
      - Updates our bucket to match the changes.
      - Calls `send_event` when changes happen.
      - Returns false if the directory cannot be scanned. */
  auto const& scan_directory = [&](std::filesystem::path const& dir,
                                   auto const& send_event) -> bool
  {
    using std::filesystem::recursive_directory_iterator,
      std::filesystem::is_directory;
    /*  if this thing is a directory */
    if (is_directory(dir)) {
      /*  try to iterate through its contents */
      auto dir_it_ec = std::error_code{};
      for (auto const& file :
           recursive_directory_iterator(dir, scan_dir_options, dir_it_ec))
        /*  while handling errors */
        if (dir_it_ec)
          return false;
        else
          scan_file(file.path(), send_event);
      return true;
    }
    else
      return false;
  };

  return scan_directory(path, send_event) ? true
       : scan_file(path, send_event)      ? true
                                          : false;
};

/*  If the bucket is empty, try to populate it.
    otherwise, prune it. */
inline bool tend_bucket(std::filesystem::path const& path,
                        auto const& send_event,
                        bucket_type& bucket) noexcept
{
  /*  Creates a file map, the "bucket", from `path`. */
  auto const& populate = [&](std::filesystem::path const& path) -> bool
  {
    using std::filesystem::exists, std::filesystem::is_directory,
      std::filesystem::recursive_directory_iterator,
      std::filesystem::last_write_time;
    /*  this happens when a path was changed while we were reading it.
        there is nothing to do here; we prune later. */
    auto dir_it_ec = std::error_code{};
    auto lwt_ec = std::error_code{};
    if (exists(path)) {
      /*  this is a directory */
      if (is_directory(path)) {
        for (auto const& file :
             recursive_directory_iterator(path, scan_dir_options, dir_it_ec)) {
          if (! dir_it_ec) {
            auto const& lwt = last_write_time(file, lwt_ec);
            if (! lwt_ec)
              bucket[file.path()] = lwt;
            else
              bucket[file.path()] = last_write_time(path);
          }
        }
      }
      /*  this is a file */
      else {
        bucket[path] = last_write_time(path);
      }
    }
    else {
      return false;
    }
    return true;
  };

  /*  Removes files which no longer exist from our bucket. */
  auto const& prune = [&](std::filesystem::path const& path,
                          auto const& send_event) -> bool
  {
    using namespace ::wtr::watcher;
    using std::filesystem::exists, std::filesystem::is_regular_file,
      std::filesystem::is_directory, std::filesystem::is_symlink;

    auto bucket_it = bucket.begin();
    /*  while looking through the bucket's contents, */
    while (bucket_it != bucket.end()) {
      /*  check if the stuff in our bucket exists anymore. */
      exists(bucket_it->first)
        /*  if so, move on. */
        ? std::advance(bucket_it, 1)
        /*  if not, call the closure, indicating destruction,
            and remove it from our bucket. */
        : [&]()
      {
        send_event(event{bucket_it->first,
                         event::what::destroy,
                         is_regular_file(path) ? event::kind::file
                         : is_directory(path)  ? event::kind::dir
                         : is_symlink(path)    ? event::kind::sym_link
                                               : event::kind::other});
        /*  bucket, erase it! */
        bucket_it = bucket.erase(bucket_it);
      }();
    }
    return true;
  };

  return bucket.empty() ? populate(path)          ? true
                        : prune(path, send_event) ? true
                                                  : false
                        : true;
};

} /* namespace */

inline bool watch(std::filesystem::path const& path,
                  ::wtr::watcher::event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept
{
  using std::this_thread::sleep_for, std::chrono::milliseconds;

  /*  Sleep for `delay_ms`.

      Then, keep running if
        - We are alive
        - The bucket is doing well
        - No errors occured while scanning

      Otherwise, stop and return false. */

  bucket_type bucket;

  static constexpr auto delay_ms = 16;

  while (is_living()) {
    if (! tend_bucket(path, callback, bucket)
        || ! scan(path, callback, bucket)) {
      callback(
        {"e/self/die/bad_fs@" + path.string(), evw::destroy, evk::watcher});

      return false;
    }
    else {
      if constexpr (delay_ms > 0) sleep_for(milliseconds(delay_ms));
    }
  }

  callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});

  return true;
}

} /* namespace adapter */
} /* namespace watcher */
} /* namespace wtr */
} /* namespace detail */

#endif

#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {

struct future {
  using shared = std::shared_ptr<future>;

  mutable std::mutex lk{};
  std::future<bool> work{};
  bool closed{false};
};

inline auto open(std::filesystem::path const& path,
                 ::wtr::watcher::event::callback const& callback) noexcept
  -> future::shared
{
  auto fut = std::make_shared<future>();

  callback({"s/self/live@" + path.string(),
            ::wtr::watcher::event::what::create,
            ::wtr::watcher::event::kind::watcher});

  fut->work = std::async(std::launch::async,
                         [path, callback, fut]() noexcept -> bool
                         {
                           return watch(path,
                                        callback,
                                        [fut]() noexcept -> bool
                                        {
                                          auto _ = std::scoped_lock{fut->lk};
                                          return ! fut->closed;
                                        });
                         });

  return fut;
};

inline auto close(future::shared const& fut) noexcept -> bool
{
  if (! fut->closed) {
    {
      auto _ = std::scoped_lock{fut->lk};
      fut->closed = true;
    }
    return fut->work.get();
  }

  else
    return false;
};

}  // namespace adapter
}  // namespace watcher
}  // namespace wtr
}  // namespace detail

/*  path */
#include <filesystem>
/*  function */
#include <functional>
/*  is_nothrow_invocable */
#include <type_traits>
/*  convertible_to */
#include <concepts>
/*  event
    callback
    adapter */

namespace wtr {
inline namespace watcher {

/*  @brief wtr/watcher/watch

    An asynchronous filesystem watcher.

    Begins watching when constructed.

    Stops watching when the object's lifetime ends
    or when `.close()` is called.

    Closing the watcher is the only blocking operation.

    @param path:
      The root path to watch for filesystem events.

    @param living_cb (optional):
      Something (such as a closure) to be called when events
      occur in the path being watched.

    This is an adaptor "switch" that chooses the ideal adaptor
    for the host platform.

    Every adapter monitors `path` for changes and invokes the
    `callback` with an `event` object when they occur.

    There are two things the user needs: `watch` and `event`.

    Typical use looks something like this:

    auto w = watch(".", [](event const& e) {
      std::cout
        << "where: " << e.where << "\n"
        << "kind: "  << e.kind  << "\n"
        << "what: "  << e.what  << "\n"
        << "when: "  << e.when  << "\n"
        << std::endl;
    };

    That's it.

    Happy hacking. */
class watch {
private:
  using Callback = ::wtr::watcher::event::callback;
  using Path = ::std::filesystem::path;
  using Fut = ::detail::wtr::watcher::adapter::future::shared;
  Fut fut{};

public:
  inline auto close() const noexcept -> bool
  {
    return ::detail::wtr::watcher::adapter::close(this->fut);
  };

  inline watch(Path const& path, Callback const& callback) noexcept
      : fut{::detail::wtr::watcher::adapter::open(path, callback)}
  {}

  inline ~watch() noexcept { this->close(); }
};

inline namespace v0_8 {

/*  Contains a way to stop an instance of `watch()`.
    This is the structure that we return from there.
    It is intended to allow the `watch(args).close()`
    syntax as well as an anonymous `watch(args)()`.
    We define it up here so that editors can suggest
    and complete the `.close()` function. Also because
    we can't template a type inside a function.

    This thing is similar to an unnamed function object
    containing a named method. */

template<class Fn>
requires(std::is_nothrow_invocable_v<Fn>
         and std::is_same_v<std::invoke_result_t<Fn>, bool>)
struct _ {
  Fn const close{};

  inline constexpr auto operator()() const noexcept -> bool
  {
    return this->close();
  };

  inline constexpr _(Fn&& fn) noexcept
      : close{std::forward<Fn>(fn)} {};

  inline constexpr ~_() = default;
};

/*  @brief wtr/watcher/watch

    Returns an asyncronous filesystem watcher as a function
    object. Calling the function object with `()` or `.close()`
    will stop the watcher.

    Closing the watcher is the only blocking operation.

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

    auto w = watch(".", [](event const& e) {
      std::cout
        << "where: " << e.where << "\n"
        << "kind: "  << e.kind  << "\n"
        << "what: "  << e.what  << "\n"
        << "when: "  << e.when  << "\n"
        << std::endl;
    };
    auto dead = w.close(); // w() also works

    That's it.

    Happy hacking. */

[[nodiscard("Returns a way to stop this watcher, for example: "
            "auto w = watch(p, cb) ; w.close() // or w();")]]

inline auto
watch0(std::filesystem::path const& path,
       event::callback const& callback) noexcept
{
  using namespace ::detail::wtr::watcher::adapter;

  return _{[adapter{open(path, callback)}]() noexcept -> bool
           { return close(adapter); }};
};

} /* namespace v0_8 */
} /* namespace watcher */
} /* namespace wtr   */
#endif /* W973564ED9F278A21F3E12037288412FBAF175F889 */
