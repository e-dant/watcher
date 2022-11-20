# Watcher

![CodeQL](https://github.com/e-dant/watcher/actions/workflows/codeql.yml/badge.svg)
![Linux](https://github.com/e-dant/watcher/actions/workflows/ubuntu.yml/badge.svg)
![macOS](https://github.com/e-dant/watcher/actions/workflows/macos.yml/badge.svg)
![Android](https://github.com/e-dant/watcher/actions/workflows/android.yml/badge.svg)
![Windows](https://github.com/e-dant/watcher/actions/workflows/windows.yml/badge.svg)
[![ConanCenter package](https://repology.org/badge/version-for-repo/conancenter/watcher.svg)](https://conan.io/center/watcher)

## Quick Start

```cpp
/* tiny-main.cpp */
#include <iostream>
#include "../../sinclude/watcher/watcher.hpp" /* Point this to wherever yours is */

int main(int argc, char** argv) {
  using namespace wtr::watcher;
  return watch(argc > 1 ? argv[1] : ".", [](const event::event& this_event) {
    std::cout << this_event << ',' << std::endl;
  });
}
```

```sh
# Platform specifics. What can you do.
PLATFORM_EXTRAS=$(test "$(uname)" = Darwin && echo '-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -framework CoreFoundation -framework CoreServices')
# Build
eval c++ -std=c++2a -O3 src/watcher/tiny-main.cpp -o watcher $PLATFORM_EXTRAS
# Run
./watcher
```

```json
"1666393024210001000": {"where": "./watcher/.git/logs/HEAD","what": "modify","kind": "file"},
"1666393024210026000": {"where": "./watcher/.git/logs/refs/heads/next","what": "modify","kind": "file"},
"1666393024210032000": {"where": "./watcher/.git/refs/heads/next.lock","what": "create","kind": "other"},
```

Enjoy!

---

## Tell Me More

An arbitrary filesystem event watcher which is

1. Simple
> *Watcher* is dead simple to use:
```cpp
watch(path, [](auto event::event){cout << event;});
```

2. Modular
> *Watcher* may be used as **a library, a program, or both**.
If you aren't looking to create something with the library, no worries.
Just build ours and run and you've got yourself a filesystem watcher
which prints filesystem events as JSON. Neat. Here's how:
```bash
git clone https://github.com/e-dant/watcher.git && cd watcher
build/build this --no-build-debug --no-build-test --no-run-test
build/out/this/release/wtr.watcher | grep -oE 'needle-in-a-haystack/.+"'
```

3. Efficient
> In [most cases](https://github.com/e-dant/watcher/tree/release#exceptions-to-efficient-scanning),
*Watcher* uses a near-zero amount of resources.

4. Safe
> We run this project through unit tests against all available
sanitiziers. The exception to this is Windows, which has one
unreliable sanitizer and almost no safety instrumentation beyond
compiler errors. **A user should be skeptical of the Windows
implementation and confident in all others.**

5. Dependency free
> The *Watcher* library depends on the C++ Standard Library. For
greater efficiency, we will use [System APIs](https://github.com/e-dant/watcher/tree/release#directory-tree)
when possible on Linux, Darwin and Windows. For testing and
debugging, we use plenty frameworks and instruments.

6. Runnable anywhere
> *Watcher* is runnable almost anywhere. The only requirement
is a filesystem.

---

## Usage

### Project Content

The important pieces are the (header-only) library and the (optional) CLI program.

- Library: `include/watcher/watcher.hpp`. Include this to use *Watcher* in your project.
- Program: `src/watcher/main.cpp`. Build this to use *Watcher* from the command line.

A directory tree is [in the notes below](https://github.com/e-dant/watcher/tree/release#directory-tree).

### The Library

Copy the `include` or `sinclude` (for the single header)
into your project. Include as:

```cpp
#include <watcher/watcher.hpp>
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
 - Path
 - Path type:
   - Directory
   - File
   - Hard Link
   - Symbolic Link
   - Watcher
   - Other
 - Event type:
   - Rename
   - Modify
   - Create
   - Destroy
   - Owner
   - Other
 - Event time as nanoseconds since epoch

The `watcher` type is special.

Events with this type will include important
messages from the watcher. You may recieve
notifications about the watcher stopping,
unwrapping a bad optional, and OS-level errors.

[The event header](https://github.com/e-dant/watcher/blob/release/include/watcher/event.hpp)
is short and approachable.

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
#include <ratio>
#include <string>
/* std::strcmp */
#include <cstring>
/* std::thread */
#include <thread>
/* std::make_tuple,
   std::tuple */
#include <tuple>
/* wtr::watcher::event::event,
   wtr::watcher::event::what,
   wtr::watcher::event::kind,
   wtr::watcher::watch,
   wtr::watcher::die */
#include <watcher/watcher.hpp>

namespace helpful_literals {
using std::this_thread::sleep_for, std::chrono::milliseconds,
    std::chrono::seconds, std::chrono::minutes, std::chrono::hours,
    std::chrono::days, std::boolalpha, std::strcmp, std::thread, std::cout,
    std::endl;
using namespace wtr;                 /* watch, die */
using namespace wtr::watcher::event; /* event, what, kind */
} /* namespace helpful_literals */

/* Watch a path for some time.
   Stream what happens. */
int main(int argc, char** argv)
{
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
      auto const lift_time = [&]() { return std::stoull(argv[3]); };
      auto const lift_typed_time = [&]() {
        auto const unit_is = [&](const char* a) { return !strcmp(a, argv[2]); };
        return unit_is("-ms") || unit_is("-milliseconds")
                   ? milliseconds(lift_time())
               : unit_is("-s") || unit_is("-seconds") ? seconds(lift_time())
               : unit_is("-m") || unit_is("-minutes") ? minutes(lift_time())
               : unit_is("-h") || unit_is("-hours")   ? hours(lift_time())
               : unit_is("-d") || unit_is("-days")    ? days(lift_time())
                                                      : milliseconds(0);
      };
      return argc == 4   ? lift_typed_time()
             : argc == 3 ? milliseconds(lift_time())
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
    cout << R"({"wtr":{"watcher":{"stream":{)" << endl;

    /* Watch on some other thread */
    thread([&]() { watcher::watch(path_to_watch, show_event_json); }).detach();

    /* Until our time expires */
    sleep_for(time_until_death);

    /* Then die */
    const bool is_watch_dead = watcher::die(show_event_json);

    /* It's also ok to die like this
       const bool is_watch_dead = watcher::die(); */

    /* And say so */
    cout << "}"
         << "\n,\"milliseconds\":" << time_until_death.count()
         << "\n,\"dead\":" << std::boolalpha << is_watch_dead
         << "\n}}}"
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

#### `build` Script

```sh
build/build
cd build/out/this/release

# watches the current directory forever
./wtr.watcher
# watches some path for 10 seconds
./wtr.watcher 'your/favorite/path' -s 10
```

This will take care of:
  - Building a compiler if one is not found
  - Linking the `compile_commands.json` file
    to this project's root
  - Building the project's debug and release variants

#### CMake

```sh
cmake -S build/in -B build/out
cmake --build build/out --config Release
cd build/out

# watches the current directory forever
./wtr.watcher
# watches some path for 10 seconds
./wtr.watcher 'your/favorite/path' -s 10
```

## Notes

### Exceptions to Efficient Scanning

There are two cases where *Watcher*'s efficiency takes a hit:

1. When the platform-independant `warthog` adapter is used.
This happens on platforms which lack better alternatives,
such as BSD and Solaris (`warthog` beats `kqueue`).
2. On embedded systems (where resources matter regardless).

*Watcher* is still relatively efficient in these cases, but
may use a non-negligible amount of CPU time. For a thumb-
rule, scanning more than one-hundred-thousand paths might
stutter on hardware from this, or the last, decade.

### System Libraries

Linux
- `inotify`
- `fanotify`
- `epoll`

Darwin
- `FSEvents`
- `dispatch_queue_attr_make_with_qos_class`

Windows
- `ReadDirectoryChangesW`
- `IoCompletionPort`

### Directory Tree

> You can generate one yourself, too, by running
[`tool/tree`](https://github.com/e-dant/watcher/blob/release/tool/tree)

```
watcher
├── src
│  └── watcher
│     ├── tiny-main.cpp
│     └── main.cpp
├── sinclude
│  └── watcher
│     └── watcher.hpp
├── include
│  └── watcher
│     ├── watcher.hpp
│     ├── watch.hpp
│     ├── platform.hpp
│     ├── event.hpp
│     └── adapter
│        ├── adapter.hpp
│        ├── windows
│        │  └── watch.hpp
│        ├── warthog
│        │  └── watch.hpp
│        ├── linux
│        │  └── watch.hpp
│        ├── darwin
│        │  └── watch.hpp
│        └── android
│           └── watch.hpp
└── build
   ├── build
   ├── out
   └── in
```