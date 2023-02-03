#pragma once

/*
  @brief watcher/adapter/windows

  The Windows `ReadDirectoryChangesW` adapter.
*/

#include <watcher/detail/platform.hpp>

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
#include <watcher/watcher.hpp>

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
