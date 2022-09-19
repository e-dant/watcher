# Watcher

This project is trivially easy to use, include,
and run. (I hope.) This is a header-only,
use-it-anywhere, pass-it-anything, highly efficient,
easy to use kind of library.

*Watcher* is often extremely efficient. In most cases,
even when scanning millions of files and directories,
*Watcher* uses a near-zero amount of resources are used.
*[Notes:1]*

If you don't want to use it in another project,
don't worry, because it comes with one. Just build
and run and you've got yourself a filesystem
watcher, which is pretty cool.

## Summary

The source code and build system for *Watcher*,
an arbitrary path watcher.

## Usage

There is one one function that is important:
  - `water::watcher::run`

`run` takes a path, which is a string-like thing,
and a callback, with is a function-like thing.

So, passing `run` a string and a lambda would do
nicely.

`run` will happily keep continue watching, forever,
until it is asked to stop or it hits an unrecoverable
error.

Because *Watcher* has a stream operator, it is trivial
to quickly build programs that yield some useful information,
such as this:

```json
"water.watcher.stream":{
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

It is a snapshot of the output taken while preparing this commit,
right before writing this paragraph.

### Brief

```cpp
using water::watcher::literal;
/* at some point, create a 'run'
   with some millisecond delay.
   the default is 16 ms, */
run<16>(
  /* provide it with some path
     to scan, forever.
     the default is the current
     working directory. */
  ".",
  /* provide it with a callback,
     which may be passed an event,
     `water::watcher::event::event`,
     and that does whatever you'd like, */
  [](const event& ev) {
    const auto show_event = [ev](const auto& what)
    { std::cout << what << ": " << ev.where << std::endl; };

    /* such as printing what happened. */
    switch (ev.what) {
      case what::create:  return show_event("created");
      case what::modify:  return show_event("modified");
      case what::destroy: return show_event("erased");
      default:           return show_event("unknown");
    }
  });
```

### Detail

```cpp
#include <iostream>             // std::cout, std::endl
#include <thread>               // std::this_thread::sleep_for
#include <watcher/watcher.hpp>  // water::watcher::run, water::watcher::event

using namespace water::watcher::literal;

const auto show_event = [](const event& ev) {

  const auto do_show = [ev](const auto& what)
  { std::cout << what << ": " << ev.where << std::endl; };

  switch (ev.what) {
    case what::create:  return do_show("created");
    case what::modify:  return do_show("modified");
    case what::destroy: return do_show("erased");
    default:                 return do_show("unknown");
  }
};

int main(int argc, char** argv) {
  static constexpr auto delay_ms = 16;
  const auto path = argc > 1
                        // we have been given a path,
                        // and we will use it
                        ? argv[1]
                        // otherwise, default to the
                        // current directory
                        : ".";
  return run<delay_ms>(
      // scan the path, forever...
      path,
      // printing what we find,
      // every 16 milliseconds.
      show_event);
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

## Notes

### [1] Exceptions to Efficient Scanning

There are two cases where *Watcher*'s efficiency takes a hit:

1. On Solaris, where the slow adapter (`warthog`) will be used
   because no better alternative exists (`kqueue` is worse).
2. On embedded systems (where resources matter).

*Watcher* is still efficient in these cases, but a longer
`delay_ms` might be necessary.
