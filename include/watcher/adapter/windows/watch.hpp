#pragma once

/*
  @brief watcher/adapter/windows

  The Windows `ReadDirectoryChangesW` adapter.
*/

#include <watcher/platform.hpp>
#if defined(WATER_WATCHER_PLATFORM_WINDOWS_ANY)

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <stdio.h>
#include <windows.h>
#include <chrono>
#include <codecvt>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

using windows_flag_t = decltype(FILE_ACTION_ADDED);

using flag_pair = std::pair<windows_flag_t, enum event::what>;

// This should be able to be an inline constexpr array,
// but the windows cc disagrees.
const std::vector<flag_pair> flag_pair_container{
    flag_pair(FILE_ACTION_ADDED, event::what::create),
    flag_pair(FILE_ACTION_MODIFIED, event::what::modify),
    flag_pair(FILE_ACTION_REMOVED, event::what::destroy),
    flag_pair(FILE_ACTION_RENAMED_OLD_NAME, event::what::rename),
    flag_pair(FILE_ACTION_RENAMED_NEW_NAME, event::what::rename),
};

inline constexpr auto buf_len = sizeof(FILE_NOTIFY_INFORMATION) * 4096;

inline constexpr auto buf_len_dword = static_cast<DWORD>(buf_len);

static auto buf = calloc(buf_len, sizeof(buf_len));

typedef struct _WATCH_OVERLAPPED
{
  OVERLAPPED o;

  FILE_NOTIFY_INFORMATION* buffer;

  unsigned int buffersize;

} WATCH_OVERLAPPED, *PWATCH_OVERLAPPED;

struct windows_file_watcher
{
  wtr::watcher::event::callback const& callback;

  HANDLE hcompletion;

  HANDLE hdirectory;

  HANDLE* hthreads;

  WATCH_OVERLAPPED* wos;

  unsigned int count;

  int working;

  WCHAR directoryname[256];
};

struct file_watcher_thread_parameter
{
  struct windows_file_watcher* pwatcher;

  HANDLE hevent;
};

FILE_NOTIFY_INFORMATION* do_event_recv(PWATCH_OVERLAPPED pwo,
                                       FILE_NOTIFY_INFORMATION* buffer,
                                       unsigned int* bufferlength,
                                       unsigned int buffersize,
                                       HANDLE hdirectory,
                                       event::callback const& callback)
{
  using namespace wtr::watcher;

  *bufferlength = 0;

  DWORD bytes_returned = 0;
  OVERLAPPED* po;
  memset(&pwo->o, 0, sizeof(OVERLAPPED));
  po = &pwo->o;

  if (ReadDirectoryChangesW(
          hdirectory, buffer, buffersize, true,
          FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION
              | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE
              | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES
              | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME,
          &bytes_returned, po, nullptr))
  {
    // *bufferlength = bytes_returned > 0 ? bytes_returned : 0;
    // buffer = bytes_returned > 0 ? buffer : nullptr;
    if (bytes_returned) {
      *bufferlength = bytes_returned;
    } else {
      buffer = nullptr;
    }
  } else {
    auto ec = GetLastError();
            std::string where = "e/watcher/rdc";
    switch (ec) {
      case ERROR_IO_PENDING:
        *bufferlength = 0;
        buffer = nullptr;
        callback(event::event{where, event::what::other,
                              event::kind::watcher});
      default:
        callback(event::event{where, event::what::other,
                              event::kind::watcher});
        break;
    }
  }

  return buffer;
}

unsigned int do_event_parse(FILE_NOTIFY_INFORMATION* pfni,
                            unsigned int bufferlength,
                            const WCHAR* directoryname,
                            event::callback const& callback)
{
  unsigned int result = 0;

  while (pfni + sizeof(FILE_NOTIFY_INFORMATION) <= pfni + bufferlength) {
    auto filename_wc
        = std::wstring{directoryname, pfni->FileNameLength} + L"\n";

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
    std::string filename = convert.to_bytes(filename_wc);

    switch (pfni->Action) {
      case FILE_ACTION_MODIFIED:
        callback(event::event{filename.c_str(), event::what::modify,
                              event::kind::file});
        break;
      case FILE_ACTION_ADDED:
        callback(event::event{filename.c_str(), event::what::create,
                              event::kind::file});
        break;
      case FILE_ACTION_REMOVED:
        callback(event::event{filename.c_str(), event::what::destroy,
                              event::kind::file});
        break;
      case FILE_ACTION_RENAMED_OLD_NAME:
        callback(event::event{filename.c_str(), event::what::rename,
                              event::kind::file});
        break;
      case FILE_ACTION_RENAMED_NEW_NAME:
        callback(event::event{filename.c_str(), event::what::rename,
                              event::kind::file});
        break;
      default: break;
    }
    result++;

    if (pfni->NextEntryOffset == 0)
      break;
    else
      pfni = (FILE_NOTIFY_INFORMATION*)((uint8_t*)pfni + pfni->NextEntryOffset);
  }

  return (result);
}

inline bool do_scan_work_async(int count, const wchar_t* directoryname,
                               size_t dname_len,
                               event::callback const& callback)
{
  static constexpr unsigned buffersize = 65536;

  struct windows_file_watcher pwatcher = {.callback = callback};

  struct file_watcher_thread_parameter pfwtp[1];

  pfwtp->hevent = CreateEventW(nullptr, true, false, nullptr);
  if (pfwtp->hevent) {
    pwatcher.hthreads
        = (HANDLE*)HeapAlloc(GetProcessHeap(), 0, sizeof(HANDLE) * count);
    pwatcher.wos = (WATCH_OVERLAPPED*)HeapAlloc(
        GetProcessHeap(), 0, sizeof(WATCH_OVERLAPPED) * count);
    pwatcher.count = count;
    pwatcher.working = true;
    pwatcher.hcompletion
        = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    wcscpy_s(pwatcher.directoryname + 1, dname_len, directoryname);
    pwatcher.directoryname[0] = static_cast<WCHAR>(wcslen(directoryname));
    pwatcher.hdirectory = CreateFileW(
        directoryname, FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    CreateIoCompletionPort(pwatcher.hdirectory, pwatcher.hcompletion,
                           (ULONG_PTR)pwatcher.hdirectory, count);

    pfwtp->pwatcher = &pwatcher;

    FILE_NOTIFY_INFORMATION* buffer;
    unsigned bufferlength;

    for (auto i = 0; i < count; i++) {
      pwatcher.wos[i].buffersize = buffersize;
      pwatcher.wos[i].buffer = (FILE_NOTIFY_INFORMATION*)HeapAlloc(
          GetProcessHeap(), 0, pwatcher.wos[i].buffersize);
      pwatcher.hthreads[i] = nullptr;
      buffer = pwatcher.wos[i].buffer;
      buffer = do_event_recv(&pwatcher.wos[i], buffer, &bufferlength,
                             buffersize, pwatcher.hdirectory, callback);
      while (buffer != nullptr && bufferlength != 0)
        do_event_parse(buffer, bufferlength, pwatcher.directoryname, callback);
      ResetEvent(pfwtp->hevent);
      pwatcher.hthreads[i] = CreateThread(
          nullptr, 0,
          [](LPVOID parameter) -> DWORD {
            struct file_watcher_thread_parameter* pfwtp
                = (struct file_watcher_thread_parameter*)parameter;

            ULONG_PTR completionkey;
            DWORD numberofbytes;
            BOOL flag;

            SetEvent(pfwtp->hevent);

            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

            while (pfwtp->pwatcher->working) {
              OVERLAPPED* po;

              flag = GetQueuedCompletionStatus(pfwtp->pwatcher->hcompletion,
                                               &numberofbytes, &completionkey,
                                               &po, INFINITE);
              if (po) {
                PWATCH_OVERLAPPED pwo = (PWATCH_OVERLAPPED)CONTAINING_RECORD(
                    po, WATCH_OVERLAPPED, o);

                if (numberofbytes) {
                  auto* buffer = (FILE_NOTIFY_INFORMATION*)pwo->buffer;
                  unsigned int bufferlength = numberofbytes;

                  while (pfwtp->pwatcher->working && buffer && bufferlength) {
                    do_event_parse(buffer, bufferlength,
                                   pfwtp->pwatcher->directoryname,
                                   pfwtp->pwatcher->callback);

                    buffer = do_event_recv(
                        pwo, buffer, &bufferlength, pwo->buffersize,
                        pfwtp->pwatcher->hdirectory, pfwtp->pwatcher->callback);
                  }
                }
              }
            }

            return 0;
          },
          (LPVOID)pfwtp, 0, nullptr);
      if (pwatcher.hthreads[i]) WaitForSingleObject(pfwtp->hevent, INFINITE);
    }
    CloseHandle(pfwtp->hevent);
  }

  return (0);
}

static bool scan(std::wstring& path, event::callback const& callback)
{
  static constexpr auto thread_count = 1;

  do_scan_work_async(thread_count, path.c_str(), path.size() + 1, callback);

  while (true)
    if (!is_living()) break;

  return true;
}

namespace util {
inline std::wstring char_arr_to_wchar_str(char const* cstr)
{
  auto len = strlen(cstr);
  auto mem = std::vector<wchar_t>(len + 1);
  mbstowcs_s(nullptr, mem.data(), len + 1, cstr, len);
  return std::wstring{std::move(mem.data())};
}
}  // namespace util

}  // namespace

// check if living
// scan if so
// return if not living
// true if no errors

inline bool watch(wchar_t const* path_wchar_ptr,
                  event::callback const& callback)
{
  auto path = std::wstring{path_wchar_ptr, wcslen(path_wchar_ptr)};

  return scan(path, callback);
}

inline bool watch(char const* path_char_ptr, event::callback const& callback)
{
  auto path = util::char_arr_to_wchar_str(path_char_ptr);

  return scan(path, callback);
}

}  // namespace adapter
}  // namespace detail
}  // namespace watcher
}  // namespace wtr

#endif