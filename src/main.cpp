// clang-format off

#include <iostream>             // std::cout, std::endl
#include <thread>               // std::this_thread::sleep_for
#include <watcher/watcher.hpp>  // water::watcher::run, water::watcher::event

using namespace water::watcher::literal;

const auto show_event = [](const event& ev) {

  std::cout << ev << std::endl;

  // const auto do_show = [ev](const auto& what, const auto& kind)
  // { std::cout << what << kind << ": " << ev.where << std::endl; };

  // switch (ev.what) {
  //   case what::path_create:  return do_show("created");
  //   case what::path_modify:  return do_show("modified");
  //   case what::path_destroy: return do_show("erased");
  //   default:                 return do_show("unknown");
  // }
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