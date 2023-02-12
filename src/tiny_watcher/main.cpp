#include "../../include/wtr/watcher.hpp"  // Point this to wherever yours is
#include <iostream>

// This is the entire API.
int main()
{
  // The watcher will call this function on every event.
  auto cb = [](wtr::event::event const& ev)
  {
    auto [where, kind, what, when] = ev;
    std::cout << "{\"" << when << "\":[" << where << "," << kind << "," << what << "," << "]}," << std::endl;
    // Or, simply:
    // std::cout << ev << "," << std::endl;
  };

  // Watch the current directory asynchronously.
  auto watcher = wtr::watch(".", cb);

  // Close whenever you're done.
  // watcher.close();
}
