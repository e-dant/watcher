/* std::boolalpha,
   std::cout,
   std::endl */
#include <iostream>
/* std::stoul,
   std::string */
#include <string>
/* std::thread */
#include <thread>
/* std::make_tuple,
   std::tuple */
#include <tuple>
/* water::watcher::event::event,
   water::watcher::event::what,
   water::watcher::event::kind,
   water::watcher::watch,
   water::watcher::die */
#include <watcher/watcher.hpp>

namespace helpful_literals {
using std::this_thread::sleep_for, std::chrono::milliseconds,
    std::chrono::seconds, std::chrono::minutes, std::chrono::hours,
    std::chrono::days, std::boolalpha, std::stoull, std::thread, std::cout,
    std::endl;
using namespace water;                 /* watch, die */
using namespace water::watcher::event; /* event, what, kind */
} /* namespace helpful_literals */

/* Watch a path for some time.
   Stream what happens. */
int main(int argc, char** argv) {
  using namespace helpful_literals;

  /* Lift the user's choices from the command line.
     The options may be:
     1. Path to watch (optional)
     2. Time unit (optional, defaults to milliseconds)
     3. Time until death (optional)

     If the path to watch is unspecified,
     we use the user's current directory.

     If we aren't told when to die,
     we never do. */
  auto const [path_to_watch, time_until_death] = [](int argc, char** argv) {
    auto const lift_path_to_watch = [&]() { return argc > 1 ? argv[1] : "."; };
    auto const lift_time_until_death = [&]() {
      auto time_val = [&time_val_str = argv[3]]() {
        return stoull(time_val_str);
      };
      auto unit_is = [&tspec = argv[2]](const char* a) -> bool {
        return std::strcmp(a, tspec) == 0;
      };
      return argc > 3 ? unit_is("ms")  ? milliseconds(time_val())
                        : unit_is("s") ? seconds(time_val())
                        : unit_is("m") ? minutes(time_val())
                        : unit_is("h") ? hours(time_val())
                        : unit_is("d") ? days(time_val())
                        : argc > 2     ? milliseconds(time_val())
                                       : milliseconds(0)
                      : milliseconds(0);
    };
    return std::make_tuple(lift_path_to_watch(), lift_time_until_death());
  }(argc, argv);

  /* Show what happens. Format as json. Use event's stream operator. */
  auto const show_event_json = [](const event& this_event) {
    /* See note [Manual Parsing] */
    this_event.kind != kind::watcher ? cout << this_event << "," << endl
                                     : cout << this_event << endl;
  };

  auto const watch_expire = [&path_to_watch = path_to_watch, &show_event_json,
                             &time_until_death = time_until_death]() -> bool {
    cout << R"({"water.watcher":{"stream":{)" << endl;

    /* Watch on some other thread */
    thread([&]() { watcher::watch(path_to_watch, show_event_json); }).detach();

    /* Until our time expires */
    sleep_for(time_until_death);

    /* Then die */
    const bool is_watch_dead = watcher::die(show_event_json);

    /* It's also ok to die like this
       const bool is_watch_dead = watcher::die(); */

    /* And say so */
    cout << "}" << endl
         << R"(,"milliseconds":)" << time_until_death.count() << endl
         << R"(,"expired":)" << std::boolalpha << is_watch_dead
         << "}"
            "}"
         << endl;

    return is_watch_dead;
  };

  return time_until_death > milliseconds(0)
             /* Either watch for some time */
             ? watch_expire() ? 0 : 1
             /* Or run forever */
             : watcher::watch(path_to_watch, show_event_json);
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
  auto const do_show = [&ev](auto const& str)
  { std::cout << str << " " << ev.kind << " " << ev.where << std::endl; };

  switch (ev.what) {
    case what::create:  return do_show("created");
    case what::modify:  return do_show("modified");
    case what::destroy: return do_show("erased");
    default:            return do_show("unknown");
  }

  ```
*/
