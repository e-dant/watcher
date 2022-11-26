#pragma once

/*
  @brief watcher/adapter/windows

  The Windows `ReadDirectoryChangesW` adapter.
*/

#include <watcher/platform.hpp>
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
namespace detail {
namespace adapter {
namespace {
namespace util {

inline std::string wstring_to_string(std::wstring const& in)
{
  size_t len = WideCharToMultiByte(CP_UTF8, 0, in.data(), in.size(), nullptr, 0,
                                   nullptr, nullptr);
  if (!len)
    return std::string{};
  else {
    std::unique_ptr<char> mem(new char[len]);
    if (!WideCharToMultiByte(CP_UTF8, 0, in.data(), in.size(), mem.get(), len,
                             nullptr, nullptr))
      return std::string{};
    else
      return std::string{mem.get(), len};
  }
}

inline std::wstring string_to_wstring(std::string const& in)
{
  size_t len = MultiByteToWideChar(CP_UTF8, 0, in.data(), in.size(), 0, 0);
  if (!len)
    return std::wstring{};
  else {
    std::unique_ptr<wchar_t> mem(new wchar_t[len]);
    if (!MultiByteToWideChar(CP_UTF8, 0, in.data(), in.size(), mem.get(), len))
      return std::wstring{};
    else
      return std::wstring{mem.get(), len};
  }
}

inline std::wstring cstring_to_wstring(char const* in)
{
  auto len = strlen(in);
  auto mem = std::vector<wchar_t>(len + 1);
  mbstowcs_s(nullptr, mem.data(), len + 1, in, len);
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

} /* namespace */

/* while living
   watch for events
   return when dead
   true if no errors */

inline bool watch(std::wstring const& path, event::callback const& callback,
                  auto const& is_living)
{
  using std::this_thread::sleep_for, std::chrono::milliseconds;

  /* @todo We need to find a better way of dealing with wide strings.
     This shouldn't affect us too much in the perf department because
     this is not the hot path. I wouldn't be surprised if the character
     strings lose information after all this back-and-forth. */
  auto path_str = util::wstring_to_string(path_wstr);

  do_scan_work_async(new watch_object{.callback = callback}, path_wstr.c_str(),
                     path_wstr.size() + 1, callback);

  while (is_living(path_str))
    if constexpr (delay_ms > 0) sleep_for(milliseconds(delay_ms));

  return true;
}

inline bool watch(wchar_t const* path, event::callback const& callback,
                  auto const& is_living)
{
  return watch(std::wstring{path, wcslen(path)}, callback, is_living);
}

inline bool watch(char const* path, event::callback const& callback,
                  auto const& is_living)
{
  return watch(util::cstring_to_wstring(path), callback, is_living);
}

inline bool watch(std::string const& path, event::callback const& callback,
                  auto const& is_living)
{
  return watch(util::string_to_wstring(path), callback, is_living);
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif
