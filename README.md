# Watcher

![CodeQL](https://github.com/e-dant/watcher/actions/workflows/codeql.yml/badge.svg)
![Linux](https://github.com/e-dant/watcher/actions/workflows/ubuntu.yml/badge.svg)
![MacOS](https://github.com/e-dant/watcher/actions/workflows/macos.yml/badge.svg)
![Windows](https://github.com/e-dant/watcher/actions/workflows/windows.yml/badge.svg)

## Quick Start

Write:

```cpp
/* tiny-main.cpp */
#include <iostream>
#include "../sinclude/watcher/watcher.hpp" /* Point this to wherever yours is */
int main(int argc, char** argv) {
  using water::watcher::event::event, water::watcher::watch, std::cout, std::endl;
  cout << R"({"water.watcher.stream":{)" << endl;

  /* This is the important line. */
  auto const is_watch_ok = watch<16>(
      argc > 1 ? argv[1] : ".",
      [](const event& this_event) { cout << this_event << ',' << endl; });

  cout << '}' << endl << '}' << endl;
  return is_watch_ok;
}
```

Compile & Run:

```sh
EXTRAS=$(test "$(uname)" = Darwin && echo '-framework CoreFoundation -framework CoreServices')

c++ -std=c++2a -O3 src/tiny-main.cpp -o watcher $EXTRAS

./watcher
```

Enjoy!

## Tell Me More

An arbitrary filesystem event watcher which is:

- simple
- efficient
- dependency free
- runnable anywhere
- header only

*Watcher* is extremely efficient. In most cases,
even when scanning millions of paths, this library
uses a near-zero amount of resources. *[1]*

If you don't want to use it in another project,
don't worry. It comes with one. Just build and
run and you've got yourself a filesystem watcher
which prints filesystem events as JSON, which is
pretty cool.

You could, for example, run this program, await events,
and filter through the noise:

```bash
# get
git clone https://github.com/e-dant/watcher.git && cd watcher

# build
cmake -S build/in -B build/out && cmake --build build/out --config Release
# or, from with the "water" project: `tell build`

# use
build/out/water.watcher | grep -oE 'needle-in-a-haystack/.+"'
```

*Watcher* is trivially easy to use, include,
and run. (I hope. If not, let me know.)

## Usage

> *Note*: If anything is unclear, make an issue about it!
For deeper information, please see the headers. They are
well-documented and are intended to be approachable.

### Important Files

- `watcher.hpp`:
    Public interface. Someday, this will be a module.
- `main.cpp`:
    Build this to use it as a CLI program.

### The Library

Copy the `include` or `sinclude` (for the single header)
into your project. Include as:

```cpp
#include <water/watcher.hpp>
```

After that, there are two things the user needs:
  - The `watch` function
  - The `event` structure

`watch` takes a path, which is a string-like thing, and a
callback, with is a function-like thing.

`event` is an object used to pass information about
filesystem events to `watch`.

The `event` object will contain the:
  - Path -- Which is always relative.
  - Path type -- one of:
    - File
    - Directory
    - Symbolic Link
    - Hard Link
    - Unknown
  - Event type -- one of:
    - Create
    - Modify
    - Destroy
    - OS-Specific Events
    - Unknown
  - Event time -- In nanoseconds since epoch

So, passing `watch` a string and a lambda would work well.

`watch` will happily continue watching until it is
asked to stop or it hits an unrecoverable error.

### Your Project

It is trivial to build programs that yield something useful.

Here is a snapshot of the output taken while preparing this
commit, right before writing this paragraph.

```json
"water.watcher.stream": {
    "1663556133054707000": {
      "where": "water/watcher/.git/objects/cc/tmp_obj_sfbyd6",
      "what": "destroy",
      "kind": "other"
    },
    "1663556133054710000": {
      "where": "water/watcher/.git/HEAD.lock",
      "what": "create",
      "kind": "other"
    },
    "1663556133054713000": {
      "where": "water/watcher/.git/refs/heads/next.lock",
      "what": "create",
      "kind": "other"
    },
    "1663556133054716000": {
      "where": "water/watcher/.git/refs/heads/next.lock",
      "what": "modify",
      "kind": "other"
    },
    "1663556133069940000": {
      "where": "water/watcher/.git/logs/HEAD",
      "what": "modify",
      "kind": "file"
    }
}
```

Which is pretty cool.

A `main` program suitable for this task:

```cpp
#include <iostream>            /* std::cout, std::endl */
#include <watcher/watcher.hpp> /* water::watcher::watch, water::watcher::event */

/* Watch a path, forever.
   Stream what happens.
   Print every 16ms. */
int main(int argc, char** argv) {
  using water::watcher::event::event, water::watcher::watch, std::cout, std::endl;
  cout << R"({"water.watcher.stream":{)" << endl;

  /* Use the path we were given
     or the current directory. */
  auto const path = argc > 1 ? argv[1] : ".";

  /* Show what happens.
     Format as json.
     Use event's stream operator. */
  auto const show_event_json = [](const event& this_event) {
    cout << this_event << ',' << endl;
  };

  /* Tick every 16ms. */
  static constexpr auto delay_ms = 16;

  /* Run forever. */
  auto const is_watch_ok = watch<delay_ms>(path, show_event_json);

  cout << '}' << endl << '}' << endl;
  return is_watch_ok;
}
```

## Consume

### In Your Project

Download this project. Include `watcher.hpp`. That's all.

This is a `FetchContent`-able project, if you're into that
kind of thing.

### As A CLI Program

#### Tell

```sh
cd water/water/watcher
`tell build`
# watches the current directory
`tell run`
# watches some path
# `tell run` your/favorite/path
# or, to do all of the above:
# `tell bun` your/favorite/path
```

This will take care of:
  - Building a compiler if one is not found
  - Linking the `compile_commands.json` file
    to this project's root
  - Actually building the project

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

## Notes

### [1] Exceptions to Efficient Scanning

There are two cases where *Watcher*'s efficiency takes a hit:

1. When the platform-independant `warthog` adapter is used.
This happens on platforms which lack better alternatives,
such as BSD and Solaris (`warthog` beats `kqueue`).
2. On embedded systems (where resources matter regardless).

*Watcher* is still efficient in these cases. However, depending
on your hardware and whether you need to watch 10-million paths
or not, a longer `delay_ms` (such as in the seconds-range) might
be necessary to prevent your machine from entering-the-atmosphere
temperature.

In all cases, this is a best-in-class filesystem watcher.
The `warthog` is a capable watcher.
