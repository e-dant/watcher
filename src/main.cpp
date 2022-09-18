#include <chrono>
#include <iostream>
#include <thread>
#include <type_traits>
#include <watcher/concepts.hpp>
#include <watcher/watcher.hpp>

using namespace water;
using namespace watcher;
using namespace concepts;

// clang-format off
const auto stutter_print
    = [](const watcher::event& ev) {
  using
    watcher::event::what::path_create,
    watcher::event::what::path_modify,
    watcher::event::what::path_destroy,
    std::endl,
    std::cout;

  const auto show_event = [ev](const auto& s) {
    cout << s << ": " << ev.where << endl;
  };

  switch (ev.what) {
    case path_create:
      show_event("created");
      break;
    case path_modify:
      show_event("modified");
      break;
    case path_destroy:
      show_event("erased");
      break;
    default:
      show_event("unknown");
  }
};
// clang-format on

int main(int argc, char** argv) {
  const auto path = argc > 1
                        // we have been given a path,
                        // and we will use it
                        ? argv[1]
                        // otherwise, default to the
                        // current directory
                        : ".";
  return run(
      // scan the path, forever...
      path,
      // printing what we find,
      // every 16 milliseconds.
      stutter_print);
}
