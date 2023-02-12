#include "../../include/wtr/watcher.hpp"  // Point this to wherever yours is
#include <iostream>

int main(int argc, char** argv)
{
  auto cb = [](wtr::event::event const& ev)
  {
    std::cout << ev << "," << std::endl;
    // Or, manually:
    // std::cout << "event at path: " << ev.where << "\n"
    //           << "kind of path: " << ev.kind << "\n"
    //           << "event type: " << ev.what << "\n"
    //           << "nanoseconds since epoch: " << ev.when
    //           << std::endl;
  };

  // This is the entire API.
  auto watcher = wtr::watch(argc > 1 ? argv[1] : ".", cb);

  // Close whenever you're done.
  // watcher.close();
}
