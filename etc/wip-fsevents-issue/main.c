#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <unistd.h>

bool path_from_event_at(
  char* into,
  unsigned len_max,
  void* event_recv_paths,
  unsigned long i)
{
  if (! event_recv_paths) return false;
  void const* from_arr =
    CFArrayGetValueAtIndex((CFArrayRef)event_recv_paths, (CFIndex)i);
  if (! from_arr) return false;
  void const* from_dict = CFDictionaryGetValue(
    (CFDictionaryRef)from_arr,
    kFSEventStreamEventExtendedDataPathKey);
  if (! from_dict) return false;
  char const* as_cstr =
    CFStringGetCStringPtr((CFStringRef)from_dict, kCFStringEncodingUTF8);
  if (! as_cstr) return false;
  unsigned len = strlen(as_cstr);
  if (len > len_max) return false;
  memcpy(into, as_cstr, len);
  into[len] = '\0';
  return true;
}

struct Ctx {
  char* memory;
  unsigned bytes_capacity;
  unsigned bytes_in_use;
};

void ctx_self_dbgprint(struct Ctx* ctx)
{
  fprintf(
    stderr,
    "memory @ %p\nbytes cap: %ubytes used: %u",
    ctx->memory,
    ctx->bytes_capacity,
    ctx->bytes_in_use);
}

char* ctx_insert(struct Ctx** octx, char const* path, unsigned path_len)
{
  if (! octx || ! *octx || ! path) return NULL;
  struct Ctx* ctx = *octx;
  if (ctx->bytes_in_use + path_len + 1 > ctx->bytes_capacity) {
    return NULL;
    unsigned new_cap = ctx->bytes_capacity * 2;
    void* new_memory = realloc(ctx->memory, new_cap);
    if (! new_memory) return NULL;
    ctx->memory = new_memory;
    ctx->bytes_capacity = new_cap;
  }
  char* at = (char*)ctx->memory + ctx->bytes_in_use;
  memset(at, 0, path_len + 1);
  memcpy(at, path, path_len);
  ctx->bytes_in_use += path_len + 1;
  return at;
}

void event_recv(
  ConstFSEventStreamRef _1,
  void* maybe_ctx,
  unsigned long count,
  void* paths,
  unsigned const* flags,
  FSEventStreamEventId const* _2)
{
  if (! maybe_ctx || ! paths || ! flags) return;
  struct Ctx* ctx = maybe_ctx;
  for (unsigned long i = 0; i < count; i++) {
    unsigned ev_path_buf_len = 1024;
    char ev_path_buf[1024];
    memset(ev_path_buf, 0, sizeof(ev_path_buf));
    path_from_event_at(ev_path_buf, ev_path_buf_len, paths, i);
    if (ctx->bytes_in_use + ev_path_buf_len + 1 > ctx->bytes_capacity) {
      unsigned new_cap = ctx->bytes_capacity * 2;
      void* new_memory = realloc(ctx->memory, new_cap);
      if (! new_memory) continue;
      ctx->memory = new_memory;
      ctx->bytes_capacity = new_cap;
    }
    char* insertion = ((char*)ctx->memory) + ctx->bytes_in_use;
    memset(insertion, 0, ev_path_buf_len);
    memcpy(insertion, ev_path_buf, ev_path_buf_len - 1);
    ctx->bytes_in_use += ev_path_buf_len;
    fprintf(stderr, "inserted: %s\n", insertion);
  }
}

FSEventStreamRef
open_event_stream(char const* path, dispatch_queue_t queue, void* ctx)
{
  FSEventStreamContext context = {
    .version = 0,
    .info = ctx,
    .retain = NULL,
    .release = NULL,
    .copyDescription = NULL,
  };

  void const* path_cfstring =
    CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
  CFArrayRef const path_array =
    CFArrayCreate(NULL, &path_cfstring, 1, &kCFTypeArrayCallBacks);

  unsigned long fsev_listen_since = kFSEventStreamEventIdSinceNow;
  unsigned fsev_listen_for = kFSEventStreamCreateFlagFileEvents
                           | kFSEventStreamCreateFlagUseExtendedData
                           | kFSEventStreamCreateFlagUseCFTypes;
  FSEventStreamRef stream = FSEventStreamCreate(
    NULL,
    &event_recv,
    &context,
    path_array,
    fsev_listen_since,
    0.016,
    fsev_listen_for);

  if (stream && queue) {
    FSEventStreamSetDispatchQueue(stream, queue);
    FSEventStreamStart(stream);
    return stream;
  }
  else
    return NULL;
}

bool close_event_stream(FSEventStreamRef s)
{
  if (s) {
    FSEventStreamFlushSync(s);
    FSEventStreamStop(s);
    FSEventStreamInvalidate(s);
    FSEventStreamRelease(s);
    s = NULL;
    return true;
  }
  else
    return false;
}

struct Watcher {
  struct Ctx* ctx;
  FSEventStreamRef stream;
};

#define n_watchers 5
#define event_count 500
#define ctx_default_capacity 2048

int main()
{
  return 0;

  dispatch_queue_t queue =
    dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

  struct Watcher* watchers[n_watchers];
  memset(watchers, 0, sizeof(watchers));

  // Some number of watchers
  for (int nth_watcher = 0; nth_watcher < n_watchers; ++nth_watcher) {
    struct Ctx* ctx = calloc(sizeof(struct Ctx), 1);
    ctx->memory = calloc(ctx_default_capacity, 1);
    ctx->bytes_capacity = ctx_default_capacity;
    struct Watcher* w = calloc(sizeof(struct Watcher), 1);
    w->ctx = ctx;
    w->stream = open_event_stream("/tmp", queue, ctx);
    watchers[nth_watcher] = w;
  }

  // Some events in the background
  for (int i = 0; i < event_count; i++) {
    char path_buf[256];
    memset(path_buf, 0, sizeof(path_buf));
    snprintf(path_buf, sizeof(path_buf), "/tmp/hi%d", i);
    FILE* f = fopen(path_buf, "w");
    fclose(f);
    unlink(path_buf);
  }

  // Clean up (we never get here during a crash)
  fprintf(stderr, "cleaning up\n");
  for (int nth_watcher = 0; nth_watcher < n_watchers; ++nth_watcher) {
    struct Watcher* w = watchers[nth_watcher];
    close_event_stream(w->stream);
    free(w->ctx->memory);
    free(w->ctx);
    free(w);
  }

  return 0;
}
