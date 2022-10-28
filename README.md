# Watcher

![CodeQL](https://github.com/e-dant/watcher/actions/workflows/codeql.yml/badge.svg)
![Linux](https://github.com/e-dant/watcher/actions/workflows/ubuntu.yml/badge.svg)
![macOS](https://github.com/e-dant/watcher/actions/workflows/macos.yml/badge.svg)
![Android](https://github.com/e-dant/watcher/actions/workflows/android.yml/badge.svg)
![Windows](https://github.com/e-dant/watcher/actions/workflows/windows.yml/badge.svg)

## Quick Start

Write:

```cpp
/* tiny-main.cpp */
#include <iostream>
#include "../sinclude/watcher/watcher.hpp" /* Point this to wherever yours is */

int main(int argc, char** argv) {
  using namespace water::watcher;
  return watch(argc > 1 ? argv[1] : ".", [](const event::event& this_event) {
    std::cout << this_event << ',' << std::endl;
  });
}
```

Compile & Run:

```sh
# Step 1: Big long path. What can you do.
PLATFORM_EXTRAS=$(test "$(uname)" = Darwin \
  && echo '-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -framework CoreFoundation -framework CoreServices')

# Step 2: Make the thing.
eval c++ -std=c++2a -O3 src/tiny-main.cpp -o watcher $PLATFORM_EXTRAS

# Step 3: Run the thing.
./watcher

# Alternatively: build/build this release tiny
```

Enjoy!

## Tell Me More

An arbitrary filesystem event watcher which is:

- simple
- efficient
- dependency free
- runnable anywhere
- header only

*Watcher* is extremely efficient. In many cases,
even when scanning thousands of paths, this library
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
  - The `die` function
  - The `event` structure

`watch` takes a path, which is a string-like thing, and a
callback, with is a function-like thing. Passing `watch`
a character array and a lambda would work well.

`watch` will happily continue watching until it is
asked to stop or it hits an unrecoverable error.

`event` is an object used to pass information about
filesystem events to `watch`.

`die` kills the `watch`. If a `callback` is given,
`die` will invoke it immediately before death.

The `event` object is used to pass information about
filesystem events to the (user-supplied) callback
given to `watch`.

The `event` object will contain the:
 - Path -- Which is always relative.
 - Type -- one of:
   - dir
   - file
   - hard_link
   - sym_link
   - watcher
   - other
 - Event type -- one of:
   - rename
   - modify
   - create
   - destroy
   - owner
   - other
 - Event time -- In nanoseconds since epoch

The `watcher` type is special.

Events with this type will include messages from
the watcher. You may recieve error messages or
important status updates.

### Your Project

It is trivial to build programs that yield something useful.

Here is a snapshot of the output taken while preparing this
commit, right before writing this paragraph.

```json
{
  "1666393024210001000": {
    "where": "./watcher/.git/logs/HEAD",
    "what": "modify",
    "kind": "file"
  },
  "1666393024210026000": {
    "where": "./watcher/.git/logs/refs/heads/next",
    "what": "modify",
    "kind": "file"
  },
  "1666393024210032000": {
    "where": "./watcher/.git/refs/heads/next.lock",
    "what": "create",
    "kind": "other"
  }
}
```

Which is pretty cool.

I loaded it up with a little help from `sed 's/},}/}}/g' | jq`.

A `main` program suitable for this task:

```cpp
/* std::boolalpha,
   std::cout,
   std::endl */
#include <iostream>
/* std::stoul,
   std::string */
#include <string>
/* std::thread */
#include <thread>
/* std::make_tuple,
   std::tuple */
#include <tuple>
/* water::watcher::event::event,
   water::watcher::event::what,
   water::watcher::event::kind,
   water::watcher::watch,
   water::watcher::die */
#include <watcher/watcher.hpp>

namespace helpful_literals {
using std::this_thread::sleep_for, std::chrono::milliseconds,
    std::chrono::seconds, std::chrono::minutes, std::chrono::hours,
    std::chrono::days, std::boolalpha, std::stoull, std::thread, std::cout,
    std::endl;
using namespace water;                 /* watch, die */
using namespace water::watcher::event; /* event, what, kind */
} /* namespace helpful_literals */

/* Watch a path for some time.
   Stream what happens. */
int main(int argc, char** argv) {
  using namespace helpful_literals;

  /* Lift the user's choices from the command line.
     The options may be:
     1. Path to watch (optional)
     2. Time unit (optional, defaults to milliseconds)
     3. Time until death (optional)

     If the path to watch is unspecified,
     we use the user's current directory.

     If we aren't told when to die,
     we never do. */
  auto const [path_to_watch, time_until_death] = [](int argc, char** argv) {
    auto const lift_path_to_watch = [&]() { return argc > 1 ? argv[1] : "."; };
    auto const lift_time_until_death = [&]() {
      auto time_val = [&time_val_str = argv[3]]() {
        return stoull(time_val_str);
      };
      auto unit_is = [&tspec = argv[2]](const char* a) -> bool {
        return std::strcmp(a, tspec) == 0;
      };
      return argc > 3 ? unit_is("ms")  ? milliseconds(time_val())
                        : unit_is("s") ? seconds(time_val())
                        : unit_is("m") ? minutes(time_val())
                        : unit_is("h") ? hours(time_val())
                        : unit_is("d") ? days(time_val())
                        : argc > 2     ? milliseconds(time_val())
                                       : milliseconds(0)
                      : milliseconds(0);
    };
    return std::make_tuple(lift_path_to_watch(), lift_time_until_death());
  }(argc, argv);

  /* Show what happens. Format as json. Use event's stream operator. */
  auto const show_event_json = [](const event& this_event) {
    /* See note [Manual Parsing] */
    this_event.kind != kind::watcher ? cout << this_event << "," << endl
                                     : cout << this_event << endl;
  };

  auto const watch_expire = [&path_to_watch = path_to_watch, &show_event_json,
                             &time_until_death = time_until_death]() -> bool {
    cout << R"({"water.watcher":{"stream":{)" << endl;

    /* Watch on some other thread */
    thread([&]() { watcher::watch(path_to_watch, show_event_json); }).detach();

    /* Until our time expires */
    sleep_for(time_until_death);

    /* Then die */
    const bool is_watch_dead = watcher::die(show_event_json);

    /* It's also ok to die like this
       const bool is_watch_dead = watcher::die(); */

    /* And say so */
    cout << "}" << endl
         << R"(,"milliseconds":)" << time_until_death.count() << endl
         << R"(,"expired":)" << std::boolalpha << is_watch_dead
         << "}"
            "}"
         << endl;

    return is_watch_dead;
  };

  return time_until_death > milliseconds(0)
             /* Either watch for some time */
             ? watch_expire() ? 0 : 1
             /* Or run forever */
             : watcher::watch(path_to_watch, show_event_json);
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
