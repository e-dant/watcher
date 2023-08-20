#include "../../../include/wtr/watcher.hpp"  // Or wherever yours is
#include <iostream>

// This is the entire API.
int main()
{
  // The watcher will call this function on every event.
  // This function can block (depending on what you do in it).
  auto cb = [](wtr::event const& ev)
  {
    std::cout << "{\"" << ev.effect_time << "\":[" << ev.effect_type << ","
              << ev.path_type << "," << ev.path_name << "]}," << std::endl;
    // If you don't need special formatting, this works:
    // std::cout << ev << "," << std::endl;
    // Wide-strings works as well.
  };

  // Watch the current directory asynchronously.
  auto watcher = wtr::watch(".", cb);

  // Do some work. (We'll just wait for a newline.)
  std::cin.get();

  // The watcher closes itself around here.
}
