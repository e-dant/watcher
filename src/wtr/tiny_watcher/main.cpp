// clang-format off
#include "../../../include/wtr/watcher.hpp"  // Or wherever yours is
#include <iostream>

using namespace std;
using namespace wtr;

// The event type, and every field within it, has
// string conversions and stream operators. All
// kinds of strings, the narrow, wide and odd ones.
// If we don't want particular formatting, we can
// json-serialize and show the event like this:
//   some_stream << event
// Here, we'll apply our own formatting.
auto show(event e)
{
  cout << to<string>(e.effect_type) + ' '
        + to<string>(e.path_type)   + ' '
        + to<string>(e.path_name)
        + (e.associated ? " -> " + to<string>(e.associated->path_name) : "")
       << endl;
}

int main()
{
  // Watch the current directory asynchronously,
  // calling the provided function for each event.
  auto watcher = watch(".", show);

  // Do some work. (We'll just wait for a newline.)
  cin.get();

  // The watcher would close itself around here,
  // though we can check and close it ourselves:
  return watcher.close() ? 0 : 1;
}
