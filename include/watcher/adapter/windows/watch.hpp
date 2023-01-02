#pragma once

/*
  @brief watcher/adapter/windows

  The Windows `ReadDirectoryChangesW` adapter.
*/

#include <watcher/platform.hpp>
#if defined(WATER_WATCHER_PLATFORM_WINDOWS_ANY)

#include <stdio.h>
#include <windows.h>
/* pair
   make_pair */
#include <tuple>
/* milliseconds */
#include <chrono>
/* mutex
   scoped_lock */
#include <mutex>
/* string */
#include <string>
/* path */
#include <filesystem>
/* this_thread::sleep_for */
#include <thread>
/* vector */
#include <vector>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

using event_thread_param_type
    = std::pair<class watch_event_proxy*, std::function<bool()> const&>;

inline constexpr auto delay_ms = std::chrono::milliseconds(16);

/* I think the default page size in Windows is 64kb,
   so 65536 might also work well. */
inline constexpr auto event_buf_len_max = 8192;

/* Hold resources necessary to recieve and send filesystem events. */
class watch_event_proxy
{
 public:
  bool valid{false};

  wtr::watcher::event::callback const& callback;

  HANDLE event_completion_token{nullptr};

  HANDLE event_thread_handle{nullptr};

  HANDLE event_token{CreateEventW(nullptr, true, false, nullptr)};

  OVERLAPPED event_overlap{};

  FILE_NOTIFY_INFORMATION event_buf[event_buf_len_max];

  DWORD event_buf_len_ready{0};

  wchar_t path_name[256]{L""};

  HANDLE path_handle{nullptr};

  std::mutex mtx{};

  watch_event_proxy(std::filesystem::path const& path,
                    event::callback const& callback) noexcept
      : callback{callback}
  {
    memcpy(path_name, path.c_str(), path.string().size());

    path_handle = CreateFileW(
        path.c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (path_handle) {
      event_completion_token
          = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

      if (event_completion_token) {
        CreateIoCompletionPort(path_handle, event_completion_token,
                               (ULONG_PTR)path_handle, 1);
        valid = true;
      }
    }
  }

  ~watch_event_proxy() noexcept
  {
    if (event_token) CloseHandle(event_token);
    if (event_completion_token) CloseHandle(event_completion_token);
    if (event_thread_handle) CloseHandle(event_thread_handle);
  }

  bool is_valid() const noexcept { return valid && event_buf != nullptr; }

  bool has_event() const noexcept { return event_buf_len_ready != 0; }
};

inline void do_event_recv(watch_event_proxy* w) noexcept
{
  using namespace wtr::watcher;

  auto _ = std::scoped_lock<std::mutex>{w->mtx};

  w->event_buf_len_ready = 0;
  DWORD bytes_returned = 0;
  memset(&w->event_overlap, 0, sizeof(OVERLAPPED));

  if (ReadDirectoryChangesW(
          w->path_handle, w->event_buf, event_buf_len_max, true,
          FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION
              | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE
              | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES
              | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME,
          &bytes_returned, &w->event_overlap, nullptr))
  {
    w->event_buf_len_ready = bytes_returned > 0 ? bytes_returned : 0;
    if (w->event_buf == nullptr) w->valid = false;
  } else {
    switch (GetLastError()) {
      case ERROR_IO_PENDING:
        w->event_buf_len_ready = 0;
        w->valid = false;
        w->callback(event::event{"e/watcher/rdc/io_pending", event::what::other,
                                 event::kind::watcher});
        break;
      default:
        w->callback(event::event{"e/watcher/rdc", event::what::other,
                                 event::kind::watcher});
        break;
    }
  }
}

inline void do_event_send(watch_event_proxy* w) noexcept
{
  auto&& wstring_to_string = [](std::wstring const& in) -> std::string {
    size_t in_len = WideCharToMultiByte(CP_UTF8, 0, in.data(),
                                        static_cast<int>(in.size()), nullptr, 0,
                                        nullptr, nullptr);
    if (in_len < 1)
      return std::string{};
    else {
      std::unique_ptr<char> out(new char[in_len]);
      size_t out_len = WideCharToMultiByte(
          CP_UTF8, 0, in.data(), static_cast<int>(in.size()), out.get(),
          static_cast<int>(in_len), nullptr, nullptr);
      if (out_len < 1)
        return std::string{};
      else
        return std::string{out.get(), in_len};
    }
  };

  auto _ = std::scoped_lock<std::mutex>{w->mtx};

  FILE_NOTIFY_INFORMATION* buf = w->event_buf;

  while (buf + sizeof(FILE_NOTIFY_INFORMATION) <= buf + w->event_buf_len_ready)
  {
    if (buf->FileNameLength % 2 == 0) {
      auto path = wstring_to_string(
          std::wstring{w->path_name, wcslen(w->path_name)} + std::wstring{L"\\"}
          + std::wstring{buf->FileName, buf->FileNameLength / 2});

      switch (buf->Action) {
        case FILE_ACTION_MODIFIED:
          w->callback({path, event::what::modify, event::kind::file});
          break;
        case FILE_ACTION_ADDED:
          w->callback({path, event::what::create, event::kind::file});
          break;
        case FILE_ACTION_REMOVED:
          w->callback({path, event::what::destroy, event::kind::file});
          break;
        case FILE_ACTION_RENAMED_OLD_NAME:
          w->callback({path, event::what::rename, event::kind::file});
          break;
        case FILE_ACTION_RENAMED_NEW_NAME:
          w->callback({path, event::what::rename, event::kind::file});
          break;
        default: break;
      }

      if (buf->NextEntryOffset == 0)
        break;
      else
        buf = (FILE_NOTIFY_INFORMATION*)((uint8_t*)buf + buf->NextEntryOffset);
    }
  }
}

inline bool do_scan_work_async(watch_event_proxy* w,
                               std::function<bool()> const& is_living) noexcept
{
  if (w->is_valid()) {
    do_event_recv(w);

    while (w->is_valid() && w->has_event()) {
      do_event_send(w);
    }

    ResetEvent(w->event_token);
    w->event_thread_handle = CreateThread(
        /* Child processes do not inherit this thread */
        nullptr,
        /* Give us the default stack size */
        0,
        /* A closure for work */
        [](void* event_thread_param) -> DWORD {
          auto [w, is_living] = *(event_thread_param_type*)event_thread_param;

          SetEvent(w->event_token);

          SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

          while (is_living()) {
            ULONG_PTR completion_key{};
            LPOVERLAPPED overlap{};

            GetQueuedCompletionStatus(w->event_completion_token,
                                      &w->event_buf_len_ready, &completion_key,
                                      &overlap, INFINITE);
            if (overlap) {
              while (w->is_valid() && w->has_event()) {
                do_event_send(w);
                do_event_recv(w);
              }
            }
          }

          return 0;
        },
        /* Pass `w` and `is_living` to the thread */
        (void*)new event_thread_param_type{w, is_living},
        /* Start running after creation */
        0,
        /* We don't need the thread id */
        nullptr);

    if (w->event_thread_handle) WaitForSingleObject(w->event_token, INFINITE);

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
  using std::this_thread::sleep_for, std::chrono::milliseconds;

  do_scan_work_async(new watch_event_proxy{path, callback}, is_living);

  while (is_living())
    if constexpr (delay_ms > milliseconds(0)) sleep_for(delay_ms);

  return true;
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif
