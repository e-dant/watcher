#include <iostream>
#include "../../sinclude/watcher/watcher.hpp" /* Point this to wherever yours is */

int main(int argc, char** argv)
{
  using namespace wtr::watcher;
  return watch(argc > 1 ? argv[1] : ".", [](event::event const& this_event) {
    std::cout << this_event << ',' << std::endl;
  });
}
