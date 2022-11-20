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

static std::wstring char_arr_to_wstring(char const* cstr)
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

  HANDLE hcompletion;

  HANDLE hdirectory;

  HANDLE* hthreads;

  watch_event_overlap* wolap;

  unsigned count;

  int working;

  WCHAR directoryname[256];
};

/* The watch object thread holds a watch object */
struct watch_object_thread
{
  struct watch_object* wobj;

  HANDLE hevent;
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

inline bool do_scan_work_async(int count, const wchar_t* directoryname,
                               size_t dname_len,
                               event::callback const& callback)
{
  static constexpr unsigned buffersize = 65536;

  struct watch_object wobj = {.callback = callback};

  struct watch_object_thread wothr[1];

  wothr->hevent = CreateEventW(nullptr, true, false, nullptr);
  if (wothr->hevent) {
    wobj.hthreads
        = (HANDLE*)HeapAlloc(GetProcessHeap(), 0, sizeof(HANDLE) * count);
    wobj.wolap = (watch_event_overlap*)HeapAlloc(
        GetProcessHeap(), 0, sizeof(watch_event_overlap) * count);
    wobj.count = count;
    wobj.working = true;
    wobj.hcompletion
        = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    wcscpy_s(wobj.directoryname + 1, dname_len, directoryname);
    wobj.directoryname[0] = static_cast<WCHAR>(wcslen(directoryname));
    wobj.hdirectory = CreateFileW(
        directoryname, FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    CreateIoCompletionPort(wobj.hdirectory, wobj.hcompletion,
                           (ULONG_PTR)wobj.hdirectory, count);

    wothr->wobj = &wobj;

    FILE_NOTIFY_INFORMATION* buffer;
    unsigned bufferlength;

    for (auto i = 0; i < count; i++) {
      wobj.wolap[i].buffersize = buffersize;
      wobj.wolap[i].buffer = (FILE_NOTIFY_INFORMATION*)HeapAlloc(
          GetProcessHeap(), 0, wobj.wolap[i].buffersize);
      wobj.hthreads[i] = nullptr;
      buffer = wobj.wolap[i].buffer;
      buffer = do_event_recv(&wobj.wolap[i], buffer, &bufferlength, buffersize,
                             wobj.hdirectory, callback);
      while (buffer != nullptr && bufferlength != 0)
        do_event_parse(buffer, bufferlength, wobj.directoryname, callback);
      ResetEvent(wothr->hevent);
      wobj.hthreads[i] = CreateThread(
          nullptr, 0,
          [](LPVOID parameter) -> DWORD {
            struct watch_object_thread* wothr
                = (struct watch_object_thread*)parameter;

            ULONG_PTR completionkey;
            DWORD numberofbytes;
            BOOL flag;

            SetEvent(wothr->hevent);

            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

            while (wothr->wobj->working) {
              OVERLAPPED* po;

              flag = GetQueuedCompletionStatus(wothr->wobj->hcompletion,
                                               &numberofbytes, &completionkey,
                                               &po, INFINITE);
              if (po) {
                watch_event_overlap* wolap
                    = (watch_event_overlap*)CONTAINING_RECORD(
                        po, watch_event_overlap, o);

                if (numberofbytes) {
                  auto* buffer = (FILE_NOTIFY_INFORMATION*)wolap->buffer;
                  unsigned bufferlength = numberofbytes;

                  while (wothr->wobj->working && buffer && bufferlength) {
                    do_event_parse(buffer, bufferlength,
                                   wothr->wobj->directoryname,
                                   wothr->wobj->callback);

                    buffer = do_event_recv(
                        wolap, buffer, &bufferlength, wolap->buffersize,
                        wothr->wobj->hdirectory, wothr->wobj->callback);
                  }
                }
              }
            }

            return 0;
          },
          (LPVOID)wothr, 0, nullptr);
      if (wobj.hthreads[i]) WaitForSingleObject(wothr->hevent, INFINITE);
    }
    CloseHandle(wothr->hevent);
  }

  return 0;
}

static bool scan(std::wstring& path, event::callback const& callback)
{
  static constexpr auto thread_count = 1;

  do_scan_work_async(thread_count, path.c_str(), path.size() + 1, callback);

  while (true)
    if (!is_living()) break;

  return true;
}

} /* namespace */

/* check if living */
/* scan if so */
/* return if not living */
/* true if no errors */

inline bool watch(wchar_t const* path_wchar_ptr,
                  event::callback const& callback)
{
  auto path = std::wstring{path_wchar_ptr, wcslen(path_wchar_ptr)};

  return scan(path, callback);
}

inline bool watch(char const* path_char_ptr, event::callback const& callback)
{
  auto path = util::char_arr_to_wstring(path_char_ptr);

  return scan(path, callback);
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif
