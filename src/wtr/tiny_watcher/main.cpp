// clang-format off
#include "wtr/watcher.hpp"  // Or wherever yours is
#include <iostream>

using namespace std;
using namespace wtr;

// The event type, and every field within it, has
// string conversions and stream operators. All
// kinds of strings -- The narrow, wide and weird.
// If we don't want particular formatting, we can
// json-serialize and show the event like this:
//   some_stream << event
// Here, we'll apply our own formatting.
auto smol(event e) -> string {
  return to<string>(e.effect_type) + ' '
       + to<string>(e.path_type)   + ' '
       + to<string>(e.path_name)
       + (! e.associated ? ""
          : " -> " + smol(*e.associated));
}

auto show(event e) { cout << smol(e) << endl; }

auto main() -> int {
  // Watch the current directory asynchronously,
  // calling the provided function for each event.
  auto watcher = watch(".", show);

  // Do some work. (We'll just wait for a newline.)
  cin.get();

  // The watcher would close itself around here,
  // though we can check and close it ourselves:
  return watcher.close() ? 0 : 1;
}
