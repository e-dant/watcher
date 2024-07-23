#include <cstdint>
#include <cstdio>
#include "wtr/watcher-c.h"
#include "wtr/watcher.hpp"

#ifdef _WIN32
#define PATH_BUF_LEN 4096
#include <windows.h>
#include <stringapiset.h>
#else
// We support a pipe-based API for non-Windows platforms.
#include <unistd.h>
#include <fcntl.h>
#include <string>
#endif

extern "C" {

#ifdef _WIN32
/*  - utf16_buf
      Must be null-terminated, which, with std::filesystem, it is.
    - utf8_buf
      Points to a null-terminated buffer, if conversion is successful, else null.
    - utf8_buf_len
      Just here conventionally. This buffer should be exactly 4096, also conventionally.
*/
static void utf16_to_utf8(wchar_t const* utf16_buf, char* utf8_buf, int utf8_buf_len) {
  if (!utf16_buf || !utf8_buf) return;
  static int const utf16_buf_len = -1;
  int wrote = WideCharToMultiByte(CP_UTF8, 0, utf16_buf, utf16_buf_len, utf8_buf, utf8_buf_len, NULL, NULL);
  if (wrote <= 0) {
    utf8_buf = nullptr;
  } else {
    utf8_buf[wrote] = 0;
  }
}
#endif

void* wtr_watcher_open(
  char const* const path,
  wtr_watcher_callback callback,
  void* context)
{
  auto wrapped_callback = [callback, context](wtr::watcher::event ev_owned)
  {
    wtr_watcher_event ev_view = {};
#ifdef _WIN32
    char path_name[PATH_BUF_LEN] = {0};
    char associated_path_name[PATH_BUF_LEN] = {0};
    utf16_to_utf8(ev_owned.path_name.c_str(), path_name, PATH_BUF_LEN);
    if (!path_name) return;
    ev_view.path_name = path_name;
    if (ev_owned.associated) {
      utf16_to_utf8(ev_owned.associated->path_name.c_str(), associated_path_name, PATH_BUF_LEN);
      if (!associated_path_name) return;
      ev_view.associated_path_name = associated_path_name;
    }
#else
    ev_view.path_name = ev_owned.path_name.c_str();
    if (ev_owned.associated) {
      ev_view.associated_path_name = ev_owned.associated->path_name.c_str();
    }
#endif
    ev_view.effect_type = (int8_t)ev_owned.effect_type;
    ev_view.path_type = (int8_t)ev_owned.path_type;
    ev_view.effect_time = ev_owned.effect_time;
    callback(ev_view, context);
  };
  return (void*)new wtr::watcher::watch(path, wrapped_callback);
}

bool wtr_watcher_close(void* watcher)
{
  if (! watcher) return false;
  if (! ((wtr::watcher::watch*)watcher)->close()) return false;
  delete (wtr::watcher::watch*)watcher;
  return true;
}

#ifdef _WIN32
void* wtr_watcher_open_pipe(char const* const path, int* read_fd)
{
  return NULL;
}

bool wtr_watcher_close_pipe(void* watcher, int read_fd)
{
  return false;
}
#else
void* wtr_watcher_open_pipe(char const* const path, int* read_fd, int* write_fd)
{
  if (! path) { fprintf(stderr, "Path is null.\n"); return NULL; }
  if (! read_fd) { fprintf(stderr, "Read fd is null.\n"); return NULL; }
  if (! write_fd) { fprintf(stderr, "Write fd is null.\n"); return NULL; }
  if (*read_fd > 0) { fprintf(stderr, "Read fd is already open.\n"); return NULL; }
  if (*write_fd > 0) { fprintf(stderr, "Write fd is already open.\n"); return NULL; }
  int pipe_fds[2];
  memset(pipe_fds, 0, sizeof(pipe_fds));
  if (pipe(pipe_fds) == -1) { perror("pipe"); return NULL; }
  if (pipe_fds[0] <= 0) { fprintf(stderr, "Read fd is invalid.\n"); return NULL; }
  if (pipe_fds[1] <= 0) { fprintf(stderr, "Write fd is invalid.\n"); return NULL; }
  fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK);
  fcntl(pipe_fds[1], F_SETFL, O_NONBLOCK);
  *read_fd = pipe_fds[0];
  *write_fd = pipe_fds[1];
  auto json_serialize_event_to_pipe = [pipe_fds](wtr::watcher::event event) {
    auto json = wtr::to<std::string>(event) + "\n";
    write(pipe_fds[1], json.c_str(), json.size());
  };
  auto w = new wtr::watcher::watch(path, json_serialize_event_to_pipe);
  if (! w) {
    perror("new wtr::watcher::watch");
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return NULL;
  }
  *read_fd = pipe_fds[0];
  return (void*)w;
}

bool wtr_watcher_close_pipe(void* watcher, int read_fd, int write_fd)
{
  bool ok = false;
  ok |= watcher && ((wtr::watcher::watch*)watcher)->close();
  ok |= write_fd > 0 && close(write_fd);
  ok |= read_fd > 0 && close(read_fd);
  if (watcher) delete (wtr::watcher::watch*)watcher;
  return ok;
}
#endif

}  // extern "C"
