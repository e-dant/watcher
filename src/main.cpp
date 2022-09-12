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
    = [](const Path auto file, const status s) {
  using
    status::created,
    status::modified,
    status::erased,
    std::endl,
    std::cout;

  const auto pf = [&file](const auto s) {
    cout << s << ": " << file << endl;
  };

  switch (s) {
    case created:
      pf("created");
      break;
    case modified:
      pf("modified");
      break;
    case erased:
      pf("erased");
      break;
    default:
      pf("unknown");
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
