#include <iostream>
#include "../sinclude/watcher/watcher.hpp"
int main(int argc, char** argv) {
  using water::watcher::event::event, water::watcher::watch, std::cout,
      std::endl;
  cout << R"({"water.watcher.stream":{)" << endl;
  const bool is_watch_ok = watch<16>(
      argc > 1 ? argv[1] : ".",
      [](const event& this_event) { cout << this_event << ',' << endl; });
  cout << '}' << endl << '}' << endl;
  return is_watch_ok;
}
