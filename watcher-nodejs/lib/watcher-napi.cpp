#define NAPI_VERSION 8
#include "wtr/watcher-c.h"
#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WITH_TSFN_NONBLOCKING 1

/*  Owns:
    - A "handle" to a watch object,
      which we can give back to the watcher
      when we want to close it.
    - A (thread-safe) function, storing the
      user's callback, which we can call when
      an event is ready.
    - A reference to the user's callback, which
      we release when the watcher is closed. */
struct WatcherWrapper {
  napi_ref js_callback_ref = NULL;
  napi_threadsafe_function tsfn = NULL;
  void* watcher = NULL;
};

static void effect_type_js_obj(napi_env env, napi_value* effect_obj)
{
  napi_create_object(env, effect_obj);
  napi_value rename, modify, create, destroy, owner, other;
  napi_create_int32(env, WTR_WATCHER_EVENT_RENAME, &rename);
  napi_create_int32(env, WTR_WATCHER_EVENT_MODIFY, &modify);
  napi_create_int32(env, WTR_WATCHER_EVENT_CREATE, &create);
  napi_create_int32(env, WTR_WATCHER_EVENT_DESTROY, &destroy);
  napi_create_int32(env, WTR_WATCHER_EVENT_OWNER, &owner);
  napi_create_int32(env, WTR_WATCHER_EVENT_OTHER, &other);
  napi_set_named_property(env, *effect_obj, "rename", rename);
  napi_set_named_property(env, *effect_obj, "modify", modify);
  napi_set_named_property(env, *effect_obj, "create", create);
  napi_set_named_property(env, *effect_obj, "destroy", destroy);
  napi_set_named_property(env, *effect_obj, "owner", owner);
  napi_set_named_property(env, *effect_obj, "other", other);
}

static void path_type_js_obj(napi_env env, napi_value* path_type_obj)
{
  napi_create_object(env, path_type_obj);
  napi_value dir, file, hard_link, sym_link, watcher, other;
  napi_create_int32(env, WTR_WATCHER_PATH_DIR, &dir);
  napi_create_int32(env, WTR_WATCHER_PATH_FILE, &file);
  napi_create_int32(env, WTR_WATCHER_PATH_HARD_LINK, &hard_link);
  napi_create_int32(env, WTR_WATCHER_PATH_SYM_LINK, &sym_link);
  napi_create_int32(env, WTR_WATCHER_PATH_WATCHER, &watcher);
  napi_create_int32(env, WTR_WATCHER_PATH_OTHER, &other);
  napi_set_named_property(env, *path_type_obj, "dir", dir);
  napi_set_named_property(env, *path_type_obj, "file", file);
  napi_set_named_property(env, *path_type_obj, "hardLink", hard_link);
  napi_set_named_property(env, *path_type_obj, "symLink", sym_link);
  napi_set_named_property(env, *path_type_obj, "watcher", watcher);
  napi_set_named_property(env, *path_type_obj, "other", other);
}

static napi_value event_to_js_obj(napi_env env, wtr_watcher_event* event)
{
  napi_value event_obj, path_name, effect_type, path_type, effect_time, associated_path_name;
  napi_create_object(env, &event_obj);
  napi_create_string_utf8(env, event->path_name, NAPI_AUTO_LENGTH, &path_name);
  napi_create_int32(env, event->effect_type, &effect_type);
  napi_create_int32(env, event->path_type, &path_type);
  napi_create_bigint_int64(env, event->effect_time, &effect_time);
  napi_set_named_property(env, event_obj, "pathName", path_name);
  napi_set_named_property(env, event_obj, "effectType", effect_type);
  napi_set_named_property(env, event_obj, "pathType", path_type);
  napi_set_named_property(env, event_obj, "effectTime", effect_time);
  if (event->associated_path_name) {
    napi_create_string_utf8(env, event->associated_path_name, NAPI_AUTO_LENGTH, &associated_path_name);
  } else {
    napi_get_null(env, &associated_path_name);
  }
  napi_set_named_property(env, event_obj, "associatedPathName", associated_path_name);
  return event_obj;
}

/*  Called by the callback bridge when an event is ready.
    Forwards event to js-land for the user.
    Expects an "owned" event, which will be freed in this scope. */
static void callback_js_receiver(napi_env env, napi_value js_callback, void* _unused, void* ctx)
{
  wtr_watcher_event* event = (wtr_watcher_event*)ctx;
  napi_value event_obj = event_to_js_obj(env, event);
  napi_value global;
  napi_get_global(env, &global);
  napi_value result;
  napi_call_function(env, global, js_callback, 1, &event_obj, &result);
#if WITH_TSFN_NONBLOCKING
  free((void*)event->path_name);
  free((void*)event->associated_path_name);
  free(event);
#endif
}

/*  Called by the watcher when an event is ready.
    Passes a newly allocated event to the wrapper's stored function.
    Expects the event it to be freed in the wrapper's stored function.  */
static void callback_bridge(struct wtr_watcher_event event_view, void* ctx)
{
  WatcherWrapper* wrapper = (WatcherWrapper*)ctx;
#if WITH_TSFN_NONBLOCKING
  wtr_watcher_event* event_owned = (wtr_watcher_event*)malloc(sizeof(wtr_watcher_event));
  event_owned->path_name = event_view.path_name ? strdup(event_view.path_name) : NULL;
  event_owned->effect_type = event_view.effect_type;
  event_owned->path_type = event_view.path_type;
  event_owned->effect_time = event_view.effect_time;
  event_owned->associated_path_name = event_view.associated_path_name ? strdup(event_view.associated_path_name) : NULL;
  if (wrapper->tsfn) {
    napi_call_threadsafe_function(wrapper->tsfn, event_owned, napi_tsfn_nonblocking);
#else
    napi_call_threadsafe_function(wrapper->tsfn, &event_view, napi_tsfn_blocking);
#endif
  }
}

/*  Should be called by the user when they are done with the watcher.
    Ends event processing and releases any owned resources. */
static napi_value close(napi_env env, napi_callback_info func_arg_info)
{
  void* ctx;
  napi_get_cb_info(env, func_arg_info, NULL, NULL, NULL, &ctx);
  napi_value result;
  napi_get_boolean(env, false, &result);
  if (! ctx) {
    return result;
  }
  WatcherWrapper* wrapper = (WatcherWrapper*)ctx;
  bool closed_ok = wtr_watcher_close(wrapper->watcher);
  napi_get_boolean(env, closed_ok, &result);
  if (wrapper->tsfn) {
    napi_release_threadsafe_function(wrapper->tsfn, napi_tsfn_release);
    wrapper->tsfn = NULL;
  } else {
    napi_get_boolean(env, false, &result);
  }
  if (wrapper->js_callback_ref) {
    napi_delete_reference(env, wrapper->js_callback_ref);
    wrapper->js_callback_ref = NULL;
  } else {
    napi_get_boolean(env, false, &result);
  }
  free(wrapper);
  wrapper = NULL;
  return result;
}

/*  Opens a watcher on a path (and any children).
    Calls the provided callback when events happen.
    Accepts two arguments, a path and a callback.
    Returns an object with a single method: close.
    Call `.close()` when you don't want to watch
    things anymore. */
static napi_value watch(napi_env env, napi_callback_info func_arg_info)
{
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, func_arg_info, &argc, args, NULL, NULL);
  if (argc != 2) {
    napi_throw_error(env, NULL, "Wrong number of arguments");
    return NULL;
  }
  char path[4096];
  size_t path_len;
  napi_get_value_string_utf8(env, args[0], path, sizeof(path), &path_len);
  WatcherWrapper* wrapper = (WatcherWrapper*)malloc(sizeof(WatcherWrapper));
  napi_value js_callback = args[1];
  napi_create_reference(env, js_callback, 1, &wrapper->js_callback_ref);
  napi_value work_name;
  napi_create_string_utf8(env, "Watcher", NAPI_AUTO_LENGTH, &work_name);
  size_t work_queue_size = 1024;
  napi_create_threadsafe_function(
    env,
    js_callback,
    NULL,
    work_name,
    work_queue_size,
    1,
    wrapper,
    NULL,
    NULL,
    callback_js_receiver,
    &wrapper->tsfn);
  wrapper->watcher = wtr_watcher_open(path, callback_bridge, wrapper);
  if (wrapper->watcher == NULL) {
    napi_throw_error(env, NULL, "Failed to open watcher");
    return NULL;
  }
  napi_value watcher_obj;
  napi_create_object(env, &watcher_obj);
  napi_value close_func;
  napi_create_function(env, "close", NAPI_AUTO_LENGTH, close, wrapper, &close_func);
  napi_set_named_property(env, watcher_obj, "close", close_func);
  napi_wrap(env, watcher_obj, wrapper, NULL, NULL, NULL);
  return watcher_obj;
}

/*  Module initialization */
static napi_value mod_init(napi_env env, napi_value exports)
{
  napi_value watch_func;
  napi_create_function(env, NULL, 0, watch, NULL, &watch_func);
  napi_set_named_property(env, exports, "watch", watch_func);
  napi_value effect_type_obj;
  effect_type_js_obj(env, &effect_type_obj);
  napi_set_named_property(env, exports, "EffectType", effect_type_obj);
  napi_value path_type_obj;
  path_type_js_obj(env, &path_type_obj);
  napi_set_named_property(env, exports, "PathType", path_type_obj);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, mod_init)
