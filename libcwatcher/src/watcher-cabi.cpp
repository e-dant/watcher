#include "wtr/watcher-cabi.h"
#include "wtr/watcher.hpp"

extern "C" {

void* wtr_watcher_open(
  char const* const path,
  wtr_watcher_callback callback,
  void* context)
{
  auto wrapped_callback = [callback, context](wtr::watcher::event ev_owned)
  {
    wtr_watcher_event ev_view = {0};
    ev_view.path_name = ev_owned.path_name.c_str();
    ev_view.effect_type = (int8_t)ev_owned.effect_type;
    ev_view.path_type = (int8_t)ev_owned.path_type;
    ev_view.effect_time = ev_owned.effect_time;
    if (ev_owned.associated) {
      ev_view.associated_path_name = ev_owned.associated->path_name.c_str();
    }
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

}  // extern "C"
