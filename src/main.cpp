// clang-format off

#include <iostream>             // std::cout, std::endl
#include <thread>               // std::this_thread::sleep_for
#include <watcher/watcher.hpp>  // water::watcher::run, water::watcher::event

using
  water::watcher::event::what::path_destroy,
  water::watcher::event::what::path_create,
  water::watcher::event::what::path_modify,
  water::watcher::event,
  water::watcher::run;

const auto stutter_print = [](const event& ev) {

  const auto show_event = [ev](const auto& what)
  { std::cout << what << ": " << ev.where << std::endl; };

  switch (ev.what) {
    case path_create:  return show_event("created");
    case path_modify:  return show_event("modified");
    case path_destroy: return show_event("erased");
    default:           return show_event("unknown");
  }
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
      stutter_print);
}

// clang-format on