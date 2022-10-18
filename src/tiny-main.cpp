/* tiny-main.cpp */
#include <iostream>
#include "../sinclude/watcher/watcher.hpp" /* Point this to wherever yours is */
int main(int argc, char** argv) {
  using water::watcher::event::event, water::watcher::watch, std::cout, std::endl;
  cout << R"({"water.watcher.stream":{)" << endl;

  /* This is the important line. */
  const auto is_watch_ok = watch<16>(
      argc > 1 ? argv[1] : ".",
      [](const event& this_event) { cout << this_event << ',' << endl; });

  cout << '}' << endl << '}' << endl;
  return is_watch_ok;
}
