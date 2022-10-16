# Watcher

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
*Watcher* strives to be friendly. For deeper information,
please see the headers -- they are well-documented and are
intended to be approachable.

There are two things the user needs:
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

`watch` will happily keep continue watching until it is
asked to stop or it hits an unrecoverable error.

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
#include <watcher/watcher.hpp> /* water::watcher::run, water::watcher::event */

/* Watch a path, forever.
   Stream what happens.
   Print every 16ms. */
int main(int argc, char** argv) {
  using water::watcher::event::event, water::watcher::run, std::cout, std::endl;
  cout << R"({"water.watcher.stream":{)" << endl;

  /* Use the path we were given
     or the current directory. */
  const auto path = argc > 1 ? argv[1] : ".";

  /* Show what happens.
     Format as json.
     Use event's stream operator. */
  const auto show_event_json = [](const event& this_event) {
    cout << this_event << ',' << endl;
  };

  /* Tick every 16ms. */
  static constexpr auto delay_ms = 16;

  /* Run forever. */
  const auto is_watch_ok = run<delay_ms>(path, show_event_json);

  cout << '}' << endl << '}' << endl;
  return is_watch_ok;
}
```

## Content

- `watcher.hpp`:
    Public interface. Someday, this will be a module.
- `main.cpp`:
    Build this to use it as a CLI program.

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
