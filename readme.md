# Watcher

[![CodeQL](https://github.com/e-dant/watcher/actions/workflows/codeql.yml/badge.svg)](https://github.com/e-dant/watcher/actions/workflows/codeql.yml)
[![Linux](https://github.com/e-dant/watcher/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/e-dant/watcher/actions/workflows/ubuntu.yml)
[![macOS](https://github.com/e-dant/watcher/actions/workflows/macos.yml/badge.svg)](https://github.com/e-dant/watcher/actions/workflows/macos.yml)
[![Android](https://github.com/e-dant/watcher/actions/workflows/android.yml/badge.svg)](https://github.com/e-dant/watcher/actions/workflows/android.yml)
[![Windows](https://github.com/e-dant/watcher/actions/workflows/windows.yml/badge.svg)](https://github.com/e-dant/watcher/actions/workflows/windows.yml)
[![ConanCenter](https://repology.org/badge/version-for-repo/conancenter/watcher.svg)](https://conan.io/center/watcher)

## Quick Start

```cpp
/* tiny_watcher/main.cpp */
#include <iostream>
#include "../../sinclude/watcher/watcher.hpp" /* Point this to wherever yours is */

int main(int argc, char** argv) {
  using namespace wtr::watcher;
  return watch(argc > 1 ? argv[1] : ".", [](event::event const& this_event) {
    std::cout << this_event << ',' << std::endl;
  });
}
```

```sh
# Sigh
PLATFORM_EXTRAS=$(test "$(uname)" = Darwin && echo '-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -framework CoreFoundation -framework CoreServices')
# Build
eval c++ -std=c++2a -O3 src/tiny_watcher/main.cpp -o watcher $PLATFORM_EXTRAS
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
> These [3576](https://github.com/e-dant/watcher/blob/release/tool/sl)
lines, of which about half is documentation and tests, were written to be
[read](https://github.com/e-dant/watcher/blob/release/include/watcher/watch.hpp)
as easily as the API is used:
```cpp
watch(path, [](auto const& ev){cout << ev;});
```
```sh
wtr.watcher ~
```

2. Modular
> *Watcher* may be used as **a library, a program, or both**.
If you aren't looking to create something with the library, no worries.
Just use ours and you've got yourself a filesystem watcher which prints
filesystem events as JSON. Neat. Here's how:
```bash
git clone https://github.com/e-dant/watcher.git && cd watcher # The main branch is the (latest) release branch.
build/build this --no-build-debug --no-build-test --no-run-test # Build the release version for the host platform.
build/out/this/release/wtr.watcher | grep -oE 'needle-in-a-haystack/.+"' # Use it, pipe it, whatever. (This is a .exe on Windows.)
```

3. Efficient
> In [almost all cases](https://github.com/e-dant/watcher/tree/release#exception-to-efficient-scanning),
*Watcher* uses a near-zero amount of resources and makes
[efficienct use of the cache](https://github.com/e-dant/watcher/tree/release#cache-efficiency).

4. Safe
> We run this project through
[unit tests against all available sanitiziers](https://github.com/e-dant/watcher/actions).
The code is safe (with reasonably high certainty),
simple and clean. (This includes thread safety.)

5. Dependency free
> *Watcher* depends on the C++ Standard Library. For efficiency,
we use [System APIs](https://github.com/e-dant/watcher/tree/release#os-apis-used)
when possible on Linux, Darwin and Windows. For testing and
debugging, we use [Snitch](https://github.com/cschreib/snitch) and
[Sanitizers](https://clang.llvm.org/docs/index.html).

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

The [event](https://github.com/e-dant/watcher/blob/release/include/watcher/event.hpp)
and [watch](https://github.com/e-dant/watcher/blob/release/include/watcher/watch.hpp) headers
are short and approachable. (You only ever need to include `watcher/watcher.hpp`.)

After that, there are two things the user needs:
  - The `watch` function
  - The `die` function
  - The `event` object

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
  - Path, which is always absolute.
  - Type, one of:
    - dir
    - file
    - hard_link
    - sym_link
    - watcher
    - other
  - Event type, one of:
    - rename
    - modify
    - create
    - destroy
    - owner
    - other
  - Event time in nanoseconds since epoch

The `watcher` type is special.

Events with this type will include messages from
the watcher. You may recieve error messages or
important status updates.

Happy hacking.

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

A capable program is [here](https://github.com/e-dant/watcher/blob/release/src/watcher/main.cpp).

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

### Limitations

- Ready State
> There is no reliable way to communicate when a watcher is
ready to send events to the callback. For a few thousand paths,
this may take a few milliseconds. For a few million, consider
waiting a second or so.

### Exception to Efficient Scanning

Efficiency takes a hit when we bring out the `warthog`,
our platform-independent adapter. This adapter is used
on platforms that lack better alternatives, such as (not
Darwin) BSD and Solaris (because `warthog` beats `kqueue`).

*Watcher* is still relatively efficient when it has no
alternative better than `warthog`. As a thumb-rule,
scanning more than one-hundred-thousand paths with `warthog`
might stutter.

I'll keep my eyes open for better kernel APIs on BSD.

### OS APIs Used

Linux
- `inotify`
- `fanotify`
- `epoll`

Darwin
- `FSEvents`
- `dispatch_queue`

Windows
- `ReadDirectoryChangesW`
- `IoCompletionPort`

### Cache Efficiency

```sh
$ tool/test/dir &
$ tool/test/file &
$ valgrind --tool=cachegrind wtr.watcher ~ -s 30
```

```txt
I   refs:      797,368,564
I1  misses:          6,807
LLi misses:          2,799
I1  miss rate:        0.00%
LLi miss rate:        0.00%

D   refs:      338,544,669  (224,680,988 rd   + 113,863,681 wr)
D1  misses:         35,331  (     24,823 rd   +      10,508 wr)
LLd misses:         11,884  (      8,121 rd   +       3,763 wr)
D1  miss rate:         0.0% (        0.0%     +         0.0%  )

LLd miss rate:         0.0% (        0.0%     +         0.0%  )
LL refs:            42,138  (     31,630 rd   +      10,508 wr)
LL misses:          14,683  (     10,920 rd   +       3,763 wr)
LL miss rate:          0.0% (        0.0%     +         0.0%  )
```

### Directory Tree

> You can run [`tool/tree`](https://github.com/e-dant/watcher/blob/release/tool/tree) to view this tree locally.

```
watcher
├── src
│  ├── watcher
│  │  └── main.cpp
│  └── tiny_watcher
│     └── main.cpp
├── sinclude
│  └── watcher
│     └── watcher.hpp
├── include
│  └── watcher
│     ├── watcher.hpp
│     └── detail
│        ├── watch.hpp
│        ├── platform.hpp
│        ├── event.hpp
│        └── adapter
│           ├── adapter.hpp
│           ├── windows
│           │  └── watch.hpp
│           ├── warthog
│           │  └── watch.hpp
│           ├── linux
│           │  ├── watch.hpp
│           │  ├── inotify
│           │  │  └── watch.hpp
│           │  └── fanotify
│           │     └── watch.hpp
│           ├── darwin
│           │  └── watch.hpp
│           └── android
│              └── watch.hpp
└── build
   ├── build
   ├── out
   └── in

```
