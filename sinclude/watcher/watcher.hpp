#pragma once

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

/* @brief watcher/event
   There are only three things the user needs:
    - The `die` function
    - The `watch` function
    - The `event` object

   The `event` object is used to pass information about
   filesystem events to the (user-supplied) callback
   given to `watch`.

   The `event` object will contain the:
     - Path -- Which is always relative.
     - Type -- one of:
       - dir
       - file
       - hard_link
       - sym_link
       - watcher
       - other
     - Event type -- one of:
       - rename
       - modify
       - create
       - destroy
       - owner
       - other
     - Event time -- In nanoseconds since epoch

   The `watcher` type is special.
   Events with this type will include messages from
   the watcher. You may recieve error messages or
   important status updates.

   Happy hacking. */

/* - std::ostream */
#include <ostream>

/* - std::string */
#include <string>

/* - std::chrono::system_clock::now,
   - std::chrono::duration_cast,
   - std::chrono::system_clock,
   - std::chrono::nanoseconds,
   - std::chrono::time_point */
#include <chrono>

namespace wtr {
namespace watcher {
namespace event {

namespace {
using std::string, std::chrono::duration_cast, std::chrono::nanoseconds,
    std::chrono::time_point, std::chrono::system_clock;
}

/* @brief watcher/event/types
   - wtr::watcher::event
   - wtr::watcher::event::kind
   - wtr::watcher::event::what
   - wtr::watcher::event::callback */

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

struct event
{
  /* I like these names. Very human.
     'what happen'
     'event kind' */
  const string where;
  const enum what what;
  const enum kind kind;
  const long long when{
      duration_cast<nanoseconds>(
          time_point<system_clock>{system_clock::now()}.time_since_epoch())
          .count()};

  event(const char* where, const enum what what, const enum kind kind)
      : where{string{where}}, what{what}, kind{kind}
  {}

  event(const string where, const enum what what, const enum kind kind)
      : where{where}, what{what}, kind{kind}
  {}

  ~event() noexcept = default;

  /* @brief wtr/watcher/event/<<
     prints out where, what and kind.
     formats the output as a json object. */
  friend std::ostream& operator<<(std::ostream& os, const event& e)
  {
    /* clang-format off */
    auto const what_repr = [&]() {
      switch (e.what) {
        case what::rename:  return "rename";
        case what::modify:  return "modify";
        case what::create:  return "create";
        case what::destroy: return "destroy";
        case what::owner:   return "owner";
        case what::other:   return "other";
        default:            return "other";
      }
    }();

    auto const kind_repr = [&]() {
      switch (e.kind) {
        case kind::dir:       return "dir";
        case kind::file:      return "file";
        case kind::hard_link: return "hard_link";
        case kind::sym_link:  return "sym_link";
        case kind::watcher:   return "watcher";
        case kind::other:     return "other";
        default:              return "other";
      }
    }();

    return os << R"(")" << e.when << R"(":)"
              << "{"
                  << R"("where":")" << e.where   << R"(",)"
                  << R"("what":")"  << what_repr << R"(",)"
                  << R"("kind":")"  << kind_repr << R"(")"
              << "}";
    /* clang-format on */
  }
};

/* @brief watcher/event/callback
   Ensure the adapters can recieve events
   and will return nothing. */
using callback = void (*)(const event&);

} /* namespace event */
} /* namespace watcher */
} /* namespace wtr   */

#include <mutex>
#include <string>
#include <unordered_set>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

namespace {

using std::string, std::mutex, std::unordered_set;

static auto watcher_alive = unordered_set<string>{}; /* NOLINT */
static auto watcher_alive_mtx = mutex{};             /* NOLINT */

} /* namespace */

inline constexpr auto delay_ms = 16;

/*
  @brief watcher/adapter/watch
  Monitors `path` for changes.
  Invokes `callback` with an `event` when they happen.
  `watch` stops when asked to or irrecoverable errors occur.
  All events, including errors, are passed to `callback`.

  @param path:
   A `path` to watch for filesystem events.

  @param callback:
   A `callback` to invoke with an `event` object
   when the files being watched change.

  Other platforms might define more overloads, such as
  the wide `std::wstring` and `wchar_t const*` variants.
*/

inline bool watch(auto const& path, event::callback const& callback);
inline bool watch(char const* path, event::callback const& callback);

/*
  @brief watcher/adapter/is_living

  Used to determine whether `watch` should die.

  Likely may be overloaded by the user in the future.
*/

inline bool is_living(char const* path)
{
  watcher_alive_mtx.lock();
  bool _ = watcher_alive.contains(path);
  watcher_alive_mtx.unlock();
  return _;
}

inline bool is_living(auto const& path) { return is_living(path.c_str()); }

/*
  @brief watcher/adapter/can_watch

  Call this before `watch` to ensure only one `watch` exists.

  It might do other things or be removed at some point.
*/

inline bool make_living(char const* path)
{
  bool ok = true;
  watcher_alive_mtx.lock();

  if (watcher_alive.contains(path))
    ok = false;

  else
    watcher_alive.insert(path);

  watcher_alive_mtx.unlock();
  return ok;
}

inline bool make_living(auto const& path) { return make_living(path.c_str()); }

/*
  @brief watcher/adapter/die

  Invokes `callback` immediately before destroying itself.
*/

inline bool die(char const* path, event::callback const& callback)
{
  bool ok = true;

  {
    watcher_alive_mtx.lock();

    if (watcher_alive.contains(path))
      watcher_alive.erase(path);

    else
      ok = false;

    watcher_alive_mtx.unlock();
  }

  if (ok)
    callback(
        event::event{"s/self/die", event::what::destroy, event::kind::watcher});

  else
    callback(
        event::event{"e/self/die", event::what::destroy, event::kind::watcher});

  return ok;
}

inline bool die(auto const& path, event::callback const& callback)
{
  return die(path.c_str(), callback);
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

/* clang-format off */

/* clang-format on */

/*
  @brief watcher/adapter/windows

  The Windows `ReadDirectoryChangesW` adapter.
*/

#if defined(WATER_WATCHER_PLATFORM_WINDOWS_ANY)

#include <stdio.h>
#include <windows.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace wtr {
namespace watcher {

/* See the note in watcher/detail/adapter */
inline bool watch(wchar_t const* path, event::callback const& callback);

namespace detail {
namespace adapter {
namespace {
namespace util {

static std::string wstring_to_string(std::wstring const& in)
{
  using std::string;
  size_t len = WideCharToMultiByte(CP_UTF8, 0, in.data(), in.size(), nullptr, 0,
                                   nullptr, nullptr);
  if (!len)
    return string{};
  else {
    std::unique_ptr<char> mem(new char[len]);
    if (!WideCharToMultiByte(CP_UTF8, 0, in.data(), in.size(), mem.get(), len,
                             nullptr, nullptr))
      return string{};
    else
      return string{mem.get(), len};
  }
}

static std::wstring string_to_wstring(std::string const& in)
{
  using std::wstring;
  size_t len = MultiByteToWideChar(CP_UTF8, 0, in.data(), in.size(), 0, 0);
  if (!len)
    return wstring{};
  else {
    std::unique_ptr<wchar_t> mem(new wchar_t[len]);
    if (!MultiByteToWideChar(CP_UTF8, 0, in.data(), in.size(), mem.get(), len))
      return wstring{};
    else
      return wstring{mem.get(), len};
  }
}

static std::wstring cstring_to_wstring(char const* cstr)
{
  auto len = strlen(cstr);
  auto mem = std::vector<wchar_t>(len + 1);
  mbstowcs_s(nullptr, mem.data(), len + 1, cstr, len);
  return std::wstring(mem.data());
}

} /* namespace util */

/* The watch event overlap holds an event buffer */
struct watch_event_overlap
{
  OVERLAPPED o;

  FILE_NOTIFY_INFORMATION* buffer;

  unsigned buffersize;
};

/* The watch object holds a watch event overlap */
struct watch_object
{
  wtr::watcher::event::callback const& callback;

  HANDLE event_completion_token;

  HANDLE hdirectory;

  HANDLE hthreads;

  HANDLE event_token;

  watch_event_overlap* wolap;

  int working;

  WCHAR directoryname[256];
};

FILE_NOTIFY_INFORMATION* do_event_recv(watch_event_overlap* wolap,
                                       FILE_NOTIFY_INFORMATION* buffer,
                                       unsigned* bufferlength,
                                       unsigned buffersize, HANDLE hdirectory,
                                       event::callback const& callback)
{
  using namespace wtr::watcher;

  *bufferlength = 0;
  DWORD bytes_returned = 0;
  OVERLAPPED* po;
  memset(&wolap->o, 0, sizeof(OVERLAPPED));
  po = &wolap->o;

  if (ReadDirectoryChangesW(
          hdirectory, buffer, buffersize, true,
          FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION
              | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE
              | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES
              | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME,
          &bytes_returned, po, nullptr))
  {
    *bufferlength = bytes_returned > 0 ? bytes_returned : 0;
    buffer = bytes_returned > 0 ? buffer : nullptr;
  } else {
    switch (GetLastError()) {
      case ERROR_IO_PENDING:
        *bufferlength = 0;
        buffer = nullptr;
        callback(event::event{"e/watcher/rdc/io_pending", event::what::other,
                              event::kind::watcher});
        break;
      default:
        callback(event::event{"e/watcher/rdc", event::what::other,
                              event::kind::watcher});
        break;
    }
  }

  return buffer;
}

unsigned do_event_parse(FILE_NOTIFY_INFORMATION* pfni, unsigned bufferlength,
                        wchar_t const* dirname_wc,
                        event::callback const& callback)
{
  unsigned result = 0;

  while (pfni + sizeof(FILE_NOTIFY_INFORMATION) <= pfni + bufferlength) {
    if (pfni->FileNameLength % 2 == 0) {
      auto path = util::wstring_to_string(
          std::wstring{dirname_wc, wcslen(dirname_wc)} + std::wstring{L"\\"}
          + std::wstring{pfni->FileName, pfni->FileNameLength / 2});

      switch (pfni->Action) {
        case FILE_ACTION_MODIFIED:
          callback(event::event{path, event::what::modify, event::kind::file});
          break;
        case FILE_ACTION_ADDED:
          callback(event::event{path, event::what::create, event::kind::file});
          break;
        case FILE_ACTION_REMOVED:
          callback(event::event{path, event::what::destroy, event::kind::file});
          break;
        case FILE_ACTION_RENAMED_OLD_NAME:
          callback(event::event{path, event::what::rename, event::kind::file});
          break;
        case FILE_ACTION_RENAMED_NEW_NAME:
          callback(event::event{path, event::what::rename, event::kind::file});
          break;
        default: break;
      }
      result++;

      if (pfni->NextEntryOffset == 0)
        break;
      else
        pfni = (FILE_NOTIFY_INFORMATION*)((uint8_t*)pfni
                                          + pfni->NextEntryOffset);
    }
  }

  return result;
}

inline bool do_scan_work_async(watch_object* wobj, const wchar_t* directoryname,
                               size_t dname_len,
                               event::callback const& callback)
{
  static constexpr unsigned buffersize = 65536;

  wobj->event_token = CreateEventW(nullptr, true, false, nullptr);
  if (wobj->event_token) {
    wobj->hthreads = (HANDLE*)HeapAlloc(GetProcessHeap(), 0, sizeof(HANDLE));
    wobj->wolap = (watch_event_overlap*)HeapAlloc(GetProcessHeap(), 0,
                                                  sizeof(watch_event_overlap));
    wobj->working = true;
    wobj->event_completion_token
        = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    wcscpy_s(wobj->directoryname + 1, dname_len, directoryname);
    wobj->directoryname[0] = static_cast<WCHAR>(wcslen(directoryname));
    wobj->hdirectory = CreateFileW(
        directoryname, FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    CreateIoCompletionPort(wobj->hdirectory, wobj->event_completion_token,
                           (ULONG_PTR)wobj->hdirectory, 1);

    FILE_NOTIFY_INFORMATION* buffer;
    unsigned bufferlength;

    wobj->wolap->buffersize = buffersize;
    wobj->wolap->buffer = (FILE_NOTIFY_INFORMATION*)HeapAlloc(
        GetProcessHeap(), 0, wobj->wolap->buffersize);
    wobj->hthreads = nullptr;
    buffer = wobj->wolap->buffer;
    buffer = do_event_recv(wobj->wolap, buffer, &bufferlength, buffersize,
                           wobj->hdirectory, callback);
    while (buffer != nullptr && bufferlength != 0)
      do_event_parse(buffer, bufferlength, wobj->directoryname, callback);
    ResetEvent(wobj->event_token);
    wobj->hthreads = CreateThread(
        nullptr, 0,
        [](void* parameter) -> DWORD {
          struct watch_object* wobj = (struct watch_object*)parameter;

          ULONG_PTR completionkey;
          DWORD numberofbytes;
          BOOL flag;

          SetEvent(wobj->event_token);

          SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

          while (wobj->working) {
            OVERLAPPED* po;

            flag = GetQueuedCompletionStatus(wobj->event_completion_token,
                                             &numberofbytes, &completionkey,
                                             &po, INFINITE);
            if (po) {
              watch_event_overlap* wolap
                  = (watch_event_overlap*)CONTAINING_RECORD(
                      po, watch_event_overlap, o);

              if (numberofbytes) {
                auto* buffer = (FILE_NOTIFY_INFORMATION*)wolap->buffer;
                unsigned bufferlength = numberofbytes;

                while (wobj->working && buffer && bufferlength) {
                  do_event_parse(buffer, bufferlength, wobj->directoryname,
                                 wobj->callback);

                  buffer = do_event_recv(wolap, buffer, &bufferlength,
                                         wolap->buffersize, wobj->hdirectory,
                                         wobj->callback);
                }
              }
            }
          }

          return 0;
        },
        (void*)wobj, 0, nullptr);
    if (wobj->hthreads) WaitForSingleObject(wobj->event_token, INFINITE);
  }
  CloseHandle(wobj->event_token);

  return 0;
}

static bool scan(std::wstring& path, event::callback const& callback)
{
  using std::this_thread::sleep_for, std::chrono::milliseconds;

  auto path_str = util::wstring_to_string(path);

  do_scan_work_async(new watch_object{.callback = callback}, path.c_str(),
                     path.size() + 1, callback);

  while (true)

    if (!is_living(path_str.c_str()))
      break;

    else if constexpr (delay_ms > 0)
      sleep_for(milliseconds(delay_ms));

  return true;
}

} /* namespace */

/* check if living
   scan if so
   return if not living
   true if no errors */

inline bool watch(wchar_t const* path, event::callback const& callback)
{
  return scan(std::wstring{path, wcslen(path)}, callback);
}

inline bool watch(char const* path, event::callback const& callback)
{
  return scan(util::cstring_to_wstring(path), callback);
}

inline bool watch(std::string const& path, event::callback const& callback)
{
  return scan(path, callback);
}

inline bool watch(std::wstring const& path, event::callback const& callback)
{
  return scan(path, callback);
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

#include <CoreServices/CoreServices.h>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

using flag_pair = std::pair<FSEventStreamEventFlags, event::what>;

/* clang-format off */
inline constexpr auto flag_pair_count = 4;
inline constexpr std::array<flag_pair, flag_pair_count> flag_pair_container
  {
    /* basic information about what happened to some path.
       this group is the important one.
       See note [Extra Event Flags] */
    flag_pair(kFSEventStreamEventFlagItemCreated,        event::what::create),
    flag_pair(kFSEventStreamEventFlagItemModified,       event::what::modify),
    flag_pair(kFSEventStreamEventFlagItemRemoved,        event::what::destroy),
    flag_pair(kFSEventStreamEventFlagItemRenamed,        event::what::rename),
};
/* clang-format on */

auto mk_event_stream(char const* path, auto const& callback)
{
  /*  the contortions here are to please darwin.
      importantly, `path_as_refref` and its underlying types
      *are* const qualified. using void** is not ok. but it's also ok. */
  void const* path_cfstring
      = CFStringCreateWithCString(nullptr, path, kCFStringEncodingUTF8);

  /* We pass along the path we were asked to watch, */
  auto const translated_path
      = CFArrayCreate(nullptr,               /* not sure */
                      &path_cfstring,        /* path string(s) */
                      1,                     /* number of paths */
                      &kCFTypeArrayCallBacks /* callback */
      );

  /* the time point from which we want to monitor events (which is now), */
  auto const time_flag = kFSEventStreamEventIdSinceNow;

  /* the delay, in seconds */
  static constexpr auto delay_s = delay_ms > 0 ? delay_ms / 1000.0 : 0.0;

  /* and the event stream flags */
  auto const event_stream_flags = kFSEventStreamCreateFlagFileEvents
                                  | kFSEventStreamCreateFlagUseExtendedData
                                  | kFSEventStreamCreateFlagUseCFTypes
                                  | kFSEventStreamCreateFlagNoDefer;
  /* to the OS, requesting a file event stream which uses our callback. */
  return FSEventStreamCreate(
      nullptr,           /* allocator */
      callback,          /* callback; what to do */
      nullptr,           /* context (see note [event stream context]) */
      translated_path,   /* where to watch */
      time_flag,         /* since when (we choose since now) */
      delay_s,           /* time between fs event scans */
      event_stream_flags /* what data to gather and how */
  );
}

} /* namespace */

inline bool watch(auto const& path, event::callback const& callback)
{
  return watch(path.c_str(), callback);
}

inline bool watch(char const* path, event::callback const& callback)
{
  using std::chrono::seconds, std::chrono::milliseconds, std::to_string,
      std::string, std::this_thread::sleep_for,
      std::filesystem::is_regular_file, std::filesystem::is_directory,
      std::filesystem::is_symlink, std::filesystem::exists;

  static auto callback_hook = callback;
  auto const callback_adapter
      = [](ConstFSEventStreamRef const,   /* stream_ref */
           auto*,                         /* callback_info */
           size_t const event_recv_count, /* event count */
           auto* event_recv_paths,        /* paths with events */
           FSEventStreamEventFlags const* event_recv_what, /* event flags */
           FSEventStreamEventId const*                     /* event stream id */
        ) {
          auto decode_flags = [](FSEventStreamEventFlags const& flag_recv) {
            std::vector<event::what> translation;
            /* @todo this is a slow, dumb search. fix it. */
            for (flag_pair const& it : flag_pair_container)
              if (flag_recv & it.first) translation.push_back(it.second);
            return translation;
          };

          for (size_t i = 0; i < event_recv_count; i++) {
            char const* event_path = [&]() {
              auto const&& event_path_from_cfdict = [&]() {
                auto const&& event_path_from_cfarray
                    = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(
                        static_cast<CFArrayRef>(event_recv_paths),
                        static_cast<CFIndex>(i)));
                return static_cast<CFStringRef>(CFDictionaryGetValue(
                    event_path_from_cfarray,
                    kFSEventStreamEventExtendedDataPathKey));
              }();
              return CFStringGetCStringPtr(event_path_from_cfdict,
                                           kCFStringEncodingUTF8);
            }();
            /* see note [inode and time]
               for some extra stuff that can be done here. */
            auto const lift_event_kind = [](auto const& path) {
              return exists(path) ? is_regular_file(path) ? event::kind::file
                                    : is_directory(path)  ? event::kind::dir
                                    : is_symlink(path) ? event::kind::sym_link
                                                       : event::kind::other
                                  : event::kind::other;
            };
            for (auto const& what_it : decode_flags(event_recv_what[i]))
              if (event_path != nullptr)
                callback_hook(wtr::watcher::event::event{
                    event_path, what_it, lift_event_kind(event_path)});
          }
        };

  auto const do_make_event_handler_alive
      = [](FSEventStreamRef const& event_stream,
           dispatch_queue_t const& event_queue) -> bool {
    if (!event_stream || !event_queue) return false;
    FSEventStreamSetDispatchQueue(event_stream, event_queue);
    FSEventStreamStart(event_stream);
    return event_queue ? true : false;
  };

  auto const do_make_event_handler_dead
      = [](FSEventStreamRef const& event_stream,
           dispatch_queue_t const& event_queue) {
          FSEventStreamStop(event_stream);
          FSEventStreamInvalidate(event_stream);
          FSEventStreamRelease(event_stream);
          dispatch_release(event_queue);
          /* Assuming macOS > 10.8 or iOS > 6.0,
             we don't need to check for null on the
             dispatch queue (our `event_queue`).
             https://developer.apple.com/documentation
             /dispatch/1496328-dispatch_release */
        };

  auto const event_stream = mk_event_stream(path, callback_adapter);

  /* this should be the mersenne twister */
  auto const event_queue_name
      = ("wtr.watcher.event_queue." + to_string(std::rand()));

  /* request a high priority queue */
  dispatch_queue_t event_queue = dispatch_queue_create(
      event_queue_name.c_str(),
      dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL,
                                              QOS_CLASS_USER_INITIATED, -10));

  if (do_make_event_handler_alive(event_stream, event_queue))
    while (is_living(path))
      if constexpr (delay_ms > 0) sleep_for(milliseconds(delay_ms));

  do_make_event_handler_dead(event_stream, event_queue);

  return is_living(path) ? false : true;
}

/*
# Notes

## Event Stream Context

To set up a context with some parameters, something like this, from the
`fswatch` project repo, could be used:

  ```cpp
  std::unique_ptr<FSEventStreamContext> context(
      new FSEventStreamContext());
  context->version         = 0;
  context->info            = nullptr;
  context->retain          = nullptr;
  context->release         = nullptr;
  context->copyDescription = nullptr;
  ```

## Inode and Time

To grab the inode and time information about an event, something like this, also
from `fswatch`, could be used:

  ```cpp
  time_t curr_time;
  time(&curr_time);
  auto cf_inode = static_cast<CFNumberRef>(CFDictionaryGetValue(
      _path_info_dict, kFSEventStreamEventExtendedFileIDKey));
  unsigned long inode;
  CFNumberGetValue(cf_inode, kCFNumberLongType, &inode);
  std::cout << "_path_cfstring "
            << std::string(CFStringGetCStringPtr(_path_cfstring,
            kCFStringEncodingUTF8))
            << " (time/inode " << curr_time << "/" << inode << ")"
            << std::endl;
  ```

## Extra Event Flags

```
    // path information, i.e. whether the path is a file, directory, etc.
    // we can get this info much more easily later on in `wtr/watcher/event`.

    // flag_pair(kFSEventStreamEventFlagItemIsDir,          event::what::dir),
    // flag_pair(kFSEventStreamEventFlagItemIsFile,         event::what::file),
    // flag_pair(kFSEventStreamEventFlagItemIsSymlink, event::what::sym_link),
    // flag_pair(kFSEventStreamEventFlagItemIsHardlink, event::what::hard_link),
    // flag_pair(kFSEventStreamEventFlagItemIsLastHardlink,
event::what::hard_link),

    // path attribute events, such as the owner and some xattr data.
    // will be worthwhile soon to implement these.
    // @todo this.
    // flag_pair(kFSEventStreamEventFlagItemXattrMod,       event::what::other),
    // flag_pair(kFSEventStreamEventFlagOwnEvent,           event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemInodeMetaMod,   event::what::other),

    // some edge-cases which may be interesting later on.
    // flag_pair(kFSEventStreamEventFlagNone,               event::what::other),
    // flag_pair(kFSEventStreamEventFlagMustScanSubDirs,    event::what::other),
    // flag_pair(kFSEventStreamEventFlagUserDropped,        event::what::other),
    // flag_pair(kFSEventStreamEventFlagKernelDropped,      event::what::other),
    // flag_pair(kFSEventStreamEventFlagEventIdsWrapped,    event::what::other),
    // flag_pair(kFSEventStreamEventFlagHistoryDone,        event::what::other),
    // flag_pair(kFSEventStreamEventFlagRootChanged,        event::what::other),
    // flag_pair(kFSEventStreamEventFlagMount,              event::what::other),
    // flag_pair(kFSEventStreamEventFlagUnmount,            event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemFinderInfoMod,  event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemIsLastHardlink, event::what::other),
    // flag_pair(kFSEventStreamEventFlagItemCloned,         event::what::other),
```

*/

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr   */

#endif /* if defined(WATER_WATCHER_PLATFORM_MAC_ANY) */

/*
  @brief wtr/watcher/detail/adapter/linux

  The Linux `inotify` adapter.
*/

#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY)

#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {
/* @brief wtr/watcher/detail/adapter/linux/<a>
   Anonymous namespace for "private" things. */

/* @brief wtr/watcher/detail/adapter/linux/<a>/types
   Types:
     - dir_opt_type
     - path_container_type */
using dir_opt_type = std::filesystem::directory_options;
using path_container_type = std::unordered_map<int, std::string>;

/* @brief wtr/watcher/detail/adapter/linux/<a>/constants
   Constants:
     - event_max_count
     - in_init_opt
     - in_watch_opt
     - dir_opt */
inline constexpr auto event_max_count = 1;
inline constexpr auto path_container_reserve_count = 256;
inline constexpr auto in_init_opt = IN_NONBLOCK;
/* @todo
   Measure perf of IN_ALL_EVENTS */
/* @todo
   Handle move events properly.
     - Use IN_MOVED_TO
     - Use event::<something> */
inline constexpr auto in_watch_opt
    = IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_Q_OVERFLOW;
inline constexpr dir_opt_type dir_opt
    = std::filesystem::directory_options::skip_permission_denied
      & std::filesystem::directory_options::follow_directory_symlink;

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns
   Functions:

     do_path_container_create
       -> optional < path_container_type >

     do_event_resource_create
       -> optional < tuple < epoll_event, epoll_event*, int > >

     do_watch_fd_create
       -> optional < int >

     do_watch_fd_release
       -> bool

     do_event_wait_recv
       -> bool

     do_scan
       -> bool
*/

auto do_path_container_create(char const* base_path_c_str, int const watch_fd,
                              event::callback const& callback)
    -> std::optional<path_container_type>;
auto do_event_resource_create(int const watch_fd,
                              event::callback const& callback)
    -> std::optional<std::tuple<epoll_event, epoll_event*, int>>;
auto do_watch_fd_create(event::callback const& callback) -> std::optional<int>;
auto do_watch_fd_release(int watch_fd, event::callback const& callback) -> bool;
auto do_event_wait_recv(int watch_fd, int event_fd, epoll_event* event_list,
                        path_container_type& path_container,
                        event::callback const& callback) -> bool;
auto do_scan(int fd, path_container_type& path_container,
             event::callback const& callback) -> bool;

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_path_container_create
   If the path given is a directory
     - find all directories above the base path given.
     - ignore nonexistent directories.
     - return a map of watch descriptors -> directories.
   If `path` is a file
     - return it as the only value in a map.
     - the watch descriptor key should always be 1. */
auto do_path_container_create(char const* base_path_c_str, /* NOLINT */
                              int const watch_fd,
                              event::callback const& callback)
    -> std::optional<path_container_type>
{
  using rdir_iterator = std::filesystem::recursive_directory_iterator;

  auto dir_ec = std::error_code{};
  auto base_path = std::string{base_path_c_str};
  path_container_type path_map;
  path_map.reserve(path_container_reserve_count);

  auto do_mark = [&](auto& dir) {
    int wd = inotify_add_watch(watch_fd, dir.c_str(), in_watch_opt);
    return wd < 0
        ? [&](){
          callback(event::event{"e/sys/inotify_add_watch",
                   event::what::other, event::kind::watcher});
          return false; }()
        : [&](){
          path_map[wd] = dir;
          return true;  }();
  };

  if (!do_mark(base_path))
    return std::nullopt;
  else if (std::filesystem::is_directory(base_path, dir_ec))
    /* @todo @note
       Should we bail from within this loop if `do_mark` fails? */
    for (auto const& dir : rdir_iterator(base_path, dir_opt, dir_ec))
      if (!dir_ec)
        if (std::filesystem::is_directory(dir, dir_ec))
          if (!dir_ec) do_mark(dir.path());
  return std::move(path_map);
};

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_event_resource_create
   Return and initializes epoll events and file descriptors,
   which are the resources needed for an epoll_wait loop.
   Or return nothing. */
auto do_event_resource_create(int const watch_fd, /* NOLINT */
                              event::callback const& callback)
    -> std::optional<std::tuple<epoll_event, epoll_event*, int>>
{
  struct epoll_event event_conf
  {
    .events = EPOLLIN, .data { .fd = watch_fd }
  };
  struct epoll_event event_list[event_max_count];
  int event_fd = epoll_create1(EPOLL_CLOEXEC);

  if (epoll_ctl(event_fd, EPOLL_CTL_ADD, watch_fd, &event_conf) < 0) {
    callback(event::event{"e/sys/epoll_create1", event::what::other,
                          event::kind::watcher});
    return std::nullopt;
  } else
    return std::move(std::make_tuple(event_conf, event_list, event_fd));
};

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_watch_fd_create
   Return an (optional) file descriptor
   which may access the inotify api. */
auto do_watch_fd_create(event::callback const& callback) /* NOLINT */
    -> std::optional<int>
{
  int watch_fd
#if defined(WATER_WATCHER_PLATFORM_LINUX_ANY)
      = inotify_init1(in_init_opt);
#elif defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
      = inotify_init();
#endif

  if (watch_fd < 0) {
    callback(event::event{"e/sys/inotify_init1?", event::what::other,
                          event::kind::watcher});
    return std::nullopt;
  } else
    return watch_fd;
};

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_watch_fd_release
   Close the file descriptor `fd_watch`,
   Invoke `callback` on errors. */
auto do_watch_fd_release(int watch_fd, /* NOLINT */
                         event::callback const& callback) -> bool
{
  if (close(watch_fd) < 0) {
    callback(
        event::event{"e/sys/close", event::what::other, event::kind::watcher});
    return false;
  } else
    return true;
}

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_event_wait_recv
   - Await filesystem events.
   - When available, scan them.
   - Invoke the callback on errors. */
auto do_event_wait_recv(/* NOLINT */
                        int watch_fd,
                        int event_fd, /* Note the separate file descriptors for
                                         inotify and epoll */
                        epoll_event* event_list,
                        path_container_type& path_container,
                        event::callback const& callback) -> bool
{
  /* The more time asleep, the better,
     as long as we don't sleep forever,
     because we may need to die. */
  int const event_count
      = epoll_wait(event_fd, event_list, event_max_count, delay_ms);

  auto const do_event_dispatch = [&]() {
    for (int n = 0; n < event_count; n++)
      if (event_list[n].data.fd == watch_fd)
        if (!do_scan(watch_fd, path_container, callback)) return false;
    /* We return true on eventless invocations. */
    return true;
  };

  auto const do_event_error = [&]() {
    perror("epoll_wait");
    callback(event::event{"e/sys/epoll_wait", event::what::other,
                          event::kind::watcher});
    /* We always return false on errors. */
    return false;
  };

  auto is_ok = event_count < 0 ? do_event_error() : do_event_dispatch();

  if (is_ok)
    return do_event_wait_recv(watch_fd, event_fd, event_list, path_container,
                              callback);
  else
    return false;
}

/* @brief wtr/watcher/detail/adapter/linux/<a>/fns/do_scan
   Reads through available (inotify) filesystem events.
   Discerns their path and type.
   Calls the callback.
   Returns false on eventful errors.

   @todo
   Return new directories when they appear,
   Consider running and returning `find_dirs` from here.
   Remove destroyed watches. */
auto do_scan(int watch_fd, /* NOLINT */
             path_container_type& path_container,
             event::callback const& callback) -> bool
{
  /* 4096 is a typical page size. */
  static constexpr auto buf_len = 4096;
  alignas(struct inotify_event) char buf[buf_len];

  enum class event_recv_status { eventful, eventless, error };

  auto const lift_event_recv = [](int fd, char* buf) {
    /* Read some events. */
    ssize_t len = read(fd, buf, buf_len);

    /* EAGAIN means no events were found.
       We return `eventless` in this case. */
    if (len < 0 && errno != EAGAIN)
      return std::make_pair(event_recv_status::error, len);
    else if (len <= 0)
      return std::make_pair(event_recv_status::eventless, len);
    else
      return std::make_pair(event_recv_status::eventful, len);
  };

  /* Loop while events can be read from the inotify file descriptor. */
  while (true) {
    /* Read events */
    auto [status, len] = lift_event_recv(watch_fd, buf);
    /* Handle the errored, eventless and eventful reads */
    switch (status) {
      case event_recv_status::eventful:
        /* Loop over all events in the buffer. */
        const struct inotify_event* event_recv;
        for (char* ptr = buf; ptr < buf + len;
             ptr += sizeof(struct inotify_event) + event_recv->len)
        {
          event_recv = (const struct inotify_event*)ptr;

          /* @todo
             Consider using std::filesystem here. */
          auto const path_kind = event_recv->mask & IN_ISDIR
                                     ? event::kind::dir
                                     : event::kind::file;
          int path_wd = event_recv->wd;
          auto event_base_path = path_container.find(path_wd)->second;
          auto event_path
              = std::string(event_base_path + "/" + event_recv->name);

          if (event_recv->mask & IN_Q_OVERFLOW)
            callback(event::event{"e/self/overflow", event::what::other,
                                  event::kind::watcher});
          else if (event_recv->mask & IN_CREATE)
            callback(event::event{event_path.c_str(), event::what::create,
                                  path_kind});
          else if (event_recv->mask & IN_DELETE)
            callback(event::event{event_path.c_str(), event::what::destroy,
                                  path_kind});
          else if (event_recv->mask & IN_MOVE)
            callback(event::event{event_path.c_str(), event::what::rename,
                                  path_kind});
          else if (event_recv->mask & IN_MODIFY)
            callback(event::event{event_path.c_str(), event::what::modify,
                                  path_kind});
          else
            callback(event::event{event_path.c_str(), event::what::other,
                                  path_kind});
        }
        /* We don't want to return here. We run until `eventless`. */
        break;
      case event_recv_status::error:
        callback(event::event{"e/sys/read", event::what::other,
                              event::kind::watcher});
        return false;
        break;
      case event_recv_status::eventless: return true; break;
    }
  }
}

} /* namespace */

/* @brief wtr/watcher/detail/adapter/linux/fns/watch
   Monitors `base_path` for changes.
   Invokes `callback` with an `event` when they happen.

   @param base_path
   The path to watch for filesystem events.

   @param callback
   A callback to perform when the files
   being watched change. */
inline bool watch(auto const& path, event::callback const& callback)
{
  return watch(path.c_str(), callback);
}

inline bool watch(char const* path, event::callback const& callback)
{
  /*
     Values
   */

  auto watch_fd_optional = do_watch_fd_create(callback);

  if (watch_fd_optional.has_value()) {
    auto watch_fd = watch_fd_optional.value();

    auto&& path_container_optional
        = do_path_container_create(path, watch_fd, callback);

    if (path_container_optional.has_value()) {
      /* Find all directories above the base path given.
         Make a map of watch descriptors -> paths. */
      path_container_type&& path_container
          = std::move(path_container_optional.value());

      auto&& event_tuple = do_event_resource_create(watch_fd, callback);

      if (event_tuple.has_value()) {
        /* auto&& event_conf = std::get<0>(event_tuple.value()); */
        auto event_list = std::get<1>(event_tuple.value());
        auto event_fd = std::get<2>(event_tuple.value());

        /*
           Work
         */

        /* Loop until dead. */
        while (is_living(path)
               && do_event_wait_recv(watch_fd, event_fd, event_list,
                                     path_container, callback))
          continue;

        return do_watch_fd_release(watch_fd, callback);
      } else
        return do_watch_fd_release(watch_fd, callback);
    } else
      return do_watch_fd_release(watch_fd, callback);
  } else
    return false;
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_LINUX_ANY) */

/*
  @brief watcher/adapter/android

  The Android (Linux) `inotify` adapter.
*/

#if defined(WATER_WATCHER_PLATFORM_ANDROID_ANY)
#define WATER_WATCHER_USE_WARTHOG
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

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace { /* anonymous namespace for "private" things */
/* clang-format off */

inline constexpr std::filesystem::directory_options
  dir_opt = 
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
static bool scan(const char* path, auto const& send_event, bucket_type& bucket)
{
  /* @brief watcher/adapter/warthog/scan_file
     - Scans a (single) file for changes.
     - Updates our bucket to match the changes.
     - Calls `send_event` when changes happen.
     - Returns false if the file cannot be scanned. */
  auto const scan_file = [&](const char* file, auto const& send_event) -> bool {
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
      = [&](const char* dir, auto const& send_event) -> bool {
    using std::filesystem::recursive_directory_iterator,
        std::filesystem::is_directory;
    /* if this thing is a directory */
    if (is_directory(dir)) {
      /* try to iterate through its contents */
      auto dir_it_ec = std::error_code{};
      for (auto const& file :
           recursive_directory_iterator(dir, dir_opt, dir_it_ec))
        /* while handling errors */
        if (dir_it_ec)
          return false;
        else
          /* msvc complains without this cast */
          scan_file((const char*)(file.path().c_str()), send_event);
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
static bool tend_bucket(const char* path, auto const& send_event,
                        bucket_type& bucket)
{
  /*  @brief watcher/adapter/warthog/populate
      @param path - path to monitor for
      Creates a file map, the "bucket", from `path`. */
  auto const populate = [&](const char* path) -> bool {
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
             recursive_directory_iterator(path, dir_opt, dir_it_ec))
        {
          if (!dir_it_ec) {
            auto const lwt = last_write_time(file, lwt_ec);
            if (!lwt_ec)
              bucket[file.path().string()] = lwt;
            else
              /* @todo use this practice elsewhere or make a fn for it
                 otherwise, this might be confusing and inconsistent. */
              bucket[file.path().string()] = last_write_time(path);
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
  auto const prune = [&](const char* path, auto const& send_event) -> bool {
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
              send_event(event::event{bucket_it->first.c_str(),
                                      event::what::destroy,
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

inline bool watch(auto const& path, event::callback const& callback)
{
  return watch(path.c_str(), callback);
}

inline bool watch(char const* path, event::callback const& callback)
{
  using std::this_thread::sleep_for, std::chrono::milliseconds;
  /* Sleep for `delay_ms`.

     Then, keep running if
       - We are alive
       - The bucket is doing well
       - No errors occured while scanning

     Otherwise, stop and return false. */

  static bucket_type bucket;

  if constexpr (delay_ms > 0) sleep_for(milliseconds(delay_ms));

  return is_living(path) ? tend_bucket(path, callback, bucket)
                               ? scan(path, callback, bucket)
                                     ? watch(path, callback)
                                     : false
                               : false
                         : true;
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* if defined(WATER_WATCHER_PLATFORM_UNKNOWN) */

#include <string>

namespace wtr {
namespace watcher {

/* @brief watcher/watch

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
     - The `die` function
     - The `watch` function
     - The `event` structure

   That's it.

   Happy hacking. */
inline bool watch(auto const& path, event::callback const& callback)
{
  return detail::adapter::make_living(path)
             ? detail::adapter::watch(path, callback)
             : false;
}

/* @brief watcher/die

   Stops the `watch`.
   Calls `callback`,
   then dies. */
inline bool die(
    auto const& path, event::callback const& callback = [](auto) -> void {})
{
  return detail::adapter::die(path, callback);
}

} /* namespace watcher */
} /* namespace wtr   */
