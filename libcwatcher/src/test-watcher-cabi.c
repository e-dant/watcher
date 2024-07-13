#include "wtr/watcher-cabi.h"
#include <stdio.h>
#include <string.h>

void callback(struct wtr_watcher_event event, void* data)
{
  int* count = (int*)data;
  *count += 1;
  printf(
#ifdef JSON
    "{"
    "\"count\":%d,"
    "\"path_name\":\"%s\","
    "\"effect_type\":%d,"
    "\"path_type\":%d,"
    "\"effect_time\":%lld,"
    "\"associated_path_name\":\"%s\""
    "}\n",
#else
    "count: %d, "
    "path name: %s, "
    "effect type: %d "
    "path type: %d, "
    "effect time: %lld, "
    "associated path name: %s\n",
#endif
    *count,
    event.path_name,
    event.effect_type,
    event.path_type,
    event.effect_time,
    event.associated_path_name ? event.associated_path_name : "");
}

int main(int argc, char** argv)
{
  char path[4096];
  memset(path, 0, sizeof(path));
  if (argc > 1) strncpy(path, argv[1], sizeof(path) - 1);
  int count = 0;
  void* watcher = wtr_watcher_open(path, callback, &count);
  if (! watcher) return 1;
  getchar();
  if (! wtr_watcher_close(watcher)) return 1;
  return 0;
}
