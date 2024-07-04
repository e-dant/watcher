#pragma once

#if defined(_WIN32)

#include "wtr/watcher.hpp"
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

    path_handle = CreateFileW(
      path.c_str(),
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
      is_valid = CreateIoCompletionPort(
                   path_handle,
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

inline auto is_valid(watch_event_proxy& w) noexcept -> bool
{
  return w.is_valid;
}

inline auto has_event(watch_event_proxy& w) noexcept -> bool
{
  return w.event_buf_len_ready != 0;
}

inline auto do_event_recv(
  watch_event_proxy& w,
  ::wtr::watcher::event::callback const& callback) noexcept -> bool
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

  if (read_ok) {
    w.event_buf_len_ready = bytes_returned > 0 ? bytes_returned : 0;
    return true;
  }
  else {
    switch (GetLastError()) {
      case ERROR_IO_PENDING :
        w.event_buf_len_ready = 0;
        w.is_valid = false;
        callback(
          {"e/sys/read/pending",
           ::wtr::event::effect_type::other,
           ::wtr::event::path_type::watcher});
        break;
      default :
        callback(
          {"e/sys/read",
           ::wtr::event::effect_type::other,
           ::wtr::event::path_type::watcher});
        break;
    }
    return false;
  }
}

inline auto do_event_send(
  watch_event_proxy& w,
  ::wtr::watcher::event::callback const& callback) noexcept -> bool
{
  using namespace ::wtr::watcher;

  FILE_NOTIFY_INFORMATION* buf = w.event_buf;

  struct RenameEventTracker
  {
    std::filesystem::path path_name;
    enum ::wtr::watcher::event::effect_type effect_type;
    enum ::wtr::watcher::event::path_type path_type;
    bool set = false;
  };
  // Rename events on Windows send two individual messages
  // that correspond with the old data and the new data
  // While it is believed that these are sent sequentially
  // with the old data first, there is no guarantee in the documentation.
  // These trackers are used to ensure all data is available for the callback
  // regardless of the order
  RenameEventTracker old_tracker;
  RenameEventTracker new_tracker;
  const auto trigger_rename_callback = [&]()
  {
      ::wtr::watcher::event old_event{old_tracker.path_name, old_tracker.effect_type, old_tracker.path_type};
      ::wtr::watcher::event new_event{new_tracker.path_name, new_tracker.effect_type, new_tracker.path_type};
      callback({old_event, std::move(new_event)});

      // Reset for the possibility of more events
      old_tracker = {};
      new_tracker = {};
  };

  if (is_valid(w)) {
    while (buf + sizeof(FILE_NOTIFY_INFORMATION)
           <= buf + w.event_buf_len_ready) {
      if (buf->FileNameLength % 2 == 0) {
        auto path_name =
          w.path / std::wstring{buf->FileName, buf->FileNameLength / 2};

        auto effect_type = [&buf]() noexcept
        {
          switch (buf->Action) {
            case FILE_ACTION_MODIFIED : return event::effect_type::modify;
            case FILE_ACTION_ADDED : return event::effect_type::create;
            case FILE_ACTION_REMOVED : return event::effect_type::destroy;
            case FILE_ACTION_RENAMED_OLD_NAME :
              return event::effect_type::rename;
            case FILE_ACTION_RENAMED_NEW_NAME :
              return event::effect_type::rename;
            default : return event::effect_type::other;
          }
        }();

        auto path_type = [&path_name]()
        {
          try {
            return std::filesystem::is_directory(path_name)
                   ? event::path_type::dir
                   : event::path_type::file;
          } catch (...) {
            return event::path_type::other;
          }
        }();

        if (buf->Action == FILE_ACTION_RENAMED_OLD_NAME)
        {
          old_tracker.path_name = path_name;
          old_tracker.effect_type = effect_type;
          old_tracker.path_type = path_type;
          old_tracker.set = true;

          if (new_tracker.set)
            trigger_rename_callback();
            
        }
        else if (buf->Action == FILE_ACTION_RENAMED_NEW_NAME)
        {
          new_tracker.path_name = path_name;
          new_tracker.effect_type = effect_type;
          new_tracker.path_type = path_type;
          new_tracker.set = true;

          if (old_tracker.set)
            trigger_rename_callback();
        }
        else
        {
          callback({path_name, effect_type, path_type});
        }

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
}

}  // namespace

/*  while living
    watch for events
    return when dead
    true if no errors */
inline auto watch(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback,
  semabin const& is_living) noexcept -> bool
{
  using namespace ::wtr::watcher;

  auto w = watch_event_proxy{path};

  if (is_valid(w)) {
    do_event_recv(w, callback);

    while (is_valid(w) && has_event(w)) { do_event_send(w, callback); }

    while (is_living.state() == semabin::state::pending) {
      ULONG_PTR completion_key{0};
      LPOVERLAPPED overlap{nullptr};

      bool complete = GetQueuedCompletionStatus(
        w.event_completion_token,
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

    return true;
  }
  else {
    return false;
  }
}

} /*  namespace adapter */
} /*  namespace watcher */
} /*  namespace wtr */
} /*  namespace detail */

#endif
