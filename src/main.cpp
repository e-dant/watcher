// clang-format off

#include <iostream>             // std::cout, std::endl
#include <thread>               // std::this_thread::sleep_for
#include <watcher/watcher.hpp>  // water::watcher::run, water::watcher::event

using
  water::watcher::event::what::path_destroy,
  water::watcher::event::what::path_create,
  water::watcher::event::what::path_modify,
  water::watcher::event::event,
  water::watcher::run;

const auto show_event = [](const event& ev) {

  const auto do_show = [ev](const auto& what)
  { std::cout << what << ": " << ev.where << std::endl; };

  switch (ev.what) {
    case path_create:  return do_show("created");
    case path_modify:  return do_show("modified");
    case path_destroy: return do_show("erased");
    default:           return do_show("unknown");
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
      show_event);
}

// clang-format on