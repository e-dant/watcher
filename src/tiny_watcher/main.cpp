#include "../../include/wtr/watcher.hpp" /* Point this to wherever yours is */
#include <iostream>

int main(int argc, char** argv) {
  using namespace wtr::watcher;
  return watch(argc > 1 ? argv[1] : ".", [](event::event const& ev) {
    std::cout << "event at path: " << ev.where << "\n"
              << "kind of path: " << ev.kind << "\n"
              << "event type: " << ev.what << "\n"
              << "at nanoseconds since epoch: " << ev.when << std::endl;
  })();
}
