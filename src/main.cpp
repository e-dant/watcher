#include <iostream>            /* std::cout, std::endl */
#include <watcher/watcher.hpp> /* water::watcher::watch, water::watcher::event */

/* Watch a path, forever.
   Stream what happens.
   Print every 16ms. */
int main(int argc, char** argv) {
  using water::watcher::event::event, water::watcher::watch, std::cout, std::endl;
  cout << R"({"water.watcher.stream":{)" << endl;

  /* Use the path we were given
     or the current directory. */
  const auto path = argc > 1 ? argv[1] : ".";

  /* Show what happens.
     Format as json.
     Use event's stream operator. */
  const auto show_event_json = [](const event& this_event) {
    cout << this_event << ',' << endl;
  };

  /* Tick every 16ms. */
  static constexpr auto delay_ms = 16;

  /* Run forever. */
  const auto is_watch_ok = watch<delay_ms>(path, show_event_json);

  cout << '}' << endl << '}' << endl;
  return is_watch_ok;
}

/*

# Notes

## Manual Parsing

  Manual parsing is useful for further event filtering.
  You could, for example, send `create` events to some
  notification service and send `modify` events to some
  database.

  To parse out meaning without an output stream:

  ```cpp
  const auto do_show = [ev](const auto& what, const auto& kind)
  { std::cout << what << kind << ": " << ev.where << std::endl; };

  switch (ev.what) {
    case what::create:  return do_show("created");
    case what::modify:  return do_show("modified");
    case what::destroy: return do_show("erased");
    default:                 return do_show("unknown");
  }

  ```
*/
