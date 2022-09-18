# Watcher

This project is trivially easy to use, include,
and run. (I hope.) This is a single-header,
use-it-anywhere, pass-it-anything kind of library.

If you don't want to use it in another project,
don't worry, because it comes with one. Just build
and run and you've got yourself a filesystem
watcher, which is pretty cool.

## Summary

The source code and build system for *Watcher*,
an arbitrary path watcher.

## Usage

There are only two functions that are important:
  - `water::watcher::scan`
  - `water::watcher::run`

Each take a path, which is a string-like thing,
and a callback, with is a function-like thing.

So, passing them a string and a lambda would do
nicely.

The only difference between the `scan` and `run`
is that `scan` runs once. `run` keeps on going,
forever.

### Brief

```cpp
// at some point, create a 'run'
// with some millisecond delay,
// it's template parameter.
// the default is 16 ms.
water::watcher::run<16>(
  // providing it with some path
  // to scan, forever...
  ".",
  // providing it with a callback,
  // that does whatever you'd like...
  [](
    const water::concepts::Path auto& file,
    const water::watcher::status& s) {
      const auto pf = [&file](const auto& s) {
        std::cout << s << ": " << file << std::endl;
      };
      // such as printing what happened.
      switch (s) {
        case created:
          pf("created");
          break;
        case modified:
          pf("modified");
          break;
        case erased:
          pf("erased");
          break;
        default:
          pf("unknown");
      }
      });
```

### Long

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <type_traits>
#include <watcher.hpp>

using namespace water;
using namespace watcher;
using namespace concepts;

const auto stutter_print
    = [](const Path auto& file, const status& s)
  {

  using status::created, status::modified, status::erased,
      std::endl, std::cout;

  const auto pf = [&file](const auto& s) {
    cout << s << ": " << file << endl;
  };

  switch (s) {
    case created:
      pf("created");
      break;
    case modified:
      pf("modified");
      break;
    case erased:
      pf("erased");
      break;
    default:
      pf("unknown");
  }
};

int main(int argc, char** argv) {
  auto path = argc > 1
                  // we have been given a path,
                  // and we will use it
                  ? argv[1]
                  // otherwise, default to the
                  // current directory
                  : ".";
  return run(
      // scan the path, forever...
      path,
      // printing what we find,
      // every 16 milliseconds.
      stutter_print);
}
```

## Content

- `watcher.hpp`:
    Include this to use it elsewhere.
- `main.cpp`:
    Build this to use it as a CLI program.

## Consume

### In Your Project

Download the `watcher.hpp` file and include it in
your project. That's all.

### As A CLI Program

#### Tell

```sh
cd water/water/watcher
`tell build`
# watches the current directory
`tell run`
# watches some path
# `tell run` 'your/favorite/path'
```

This will take care of:
  - building a compiler if one is not found
  - linking the `compile_commands.json` file
    to this project's root.

#### CMake

```sh
cd water/water/watcher/build
cmake -S 'in' -B 'out' -G 'Ninja' -Wno-dev
cmake --build out --config Release
cd out
# watches the current directory
./water.watcher
# watches some path
# ./water.watcher 'your/favorite/path'
```