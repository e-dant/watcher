// clang-format off

#include <iostream>             // std::cout, std::endl
#include <thread>               // std::this_thread::sleep_for
#include <watcher/watcher.hpp>  // water::watcher::run, water::watcher::event

using namespace water::watcher::literal;

const auto show_event = [](const event& ev) {

  /* The event's << operator will print as json. */
  std::cout << ev << std::endl;

  /*
    // Or, parse manually like this:
    
    const auto do_show = [ev](const auto& what, const auto& kind)
    { std::cout << what << kind << ": " << ev.where << std::endl; };

    // See water/watcher/event.hpp for more documentation on these
    // and all other values.

    switch (ev.what) {
      case what::path_create:  return do_show("created");
      case what::path_modify:  return do_show("modified");
      case what::path_destroy: return do_show("erased");
      default:                 return do_show("unknown");
    }

    // Manual parsing is useful for further event filtering.
    // You could, for example, send `create` events to some
    // notification service and send `modify` events to some
    // database.
  */
};

int main(int argc, char** argv) {
  static constexpr auto delay_ms = 16;
  const auto path = argc > 1
                        // we have been given a path,
                        // and we will use it
                        ? argv[1]
                        // otherwise, default to the
                        // current directory
                        : ".";
  return run<delay_ms>(
      // scan the path, forever...
      path,
      // printing what we find,
      // every 16 milliseconds.
      show_event);
}

// clang-format on