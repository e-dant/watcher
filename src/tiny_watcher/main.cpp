#include "../../include/wtr/watcher.hpp"  // Point this to wherever yours is
#include <iostream>

// This is the entire API.
int main()
{
  // The watcher will call this function on every event.
  auto cb = [](wtr::event const& e)
  {
    std::cout << "{\"" << e.when << "\":[" << e.where << "," << e.kind << ","
              << e.what << "]}," << std::endl;
    // You can also just stream like this:
    // std::cout << e << "," << std::endl;

    // And you can unfold the event like this:
    // auto [where, kind, what, when] = e;
  };

  // Watch the current directory asynchronously.
  auto watcher = wtr::watch(".", cb);

  // Do some work. (We'll just wait for a newline.)
  std::cin.get();

  // Close whenever you're done.
  // watcher.close();
}
