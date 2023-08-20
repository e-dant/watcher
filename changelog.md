# Changelog

## 0.9.0


### Cli (0.9.0)

The watcher now runs forever when no timeout is passed in, just as it says.
(That's a bugfix.)

### API (0.9.0)

The API is now RAII-safe.
This hadn't been part of the API for a long time because I had been exploring
bindings to other languages. I had some concerns about an instance of the
watcher being destroyed across an FFI boundary. If those bindings are made, we
could write a separate C-style API for them to use. Manually closing the watcher
is still just fine, which is how it had been done before.

The `wtr::watch` API should still be intact for construction. There will be some
differences in how the objects are stored:

Before we, might spell "some watchers, in a vector" like this:

```cpp
auto lifetimes = std::vector<std::function<bool()>>{};
// Or `invoke_result_t<wtr::watch("", [](auto){});`
```

We can spell it like this now:

```cpp
auto lifetimes = std::vector<std::unique_ptr<wtr::watch>>{};
// ^ More normal
```

Destroying the watchers with the associated `.close()` function is still valid,
but is not typically necessary.

The `event` structure's field names were changed up a bit:

| old   | new         |
|-------|-------------|
| where | path_name   |
| kind  | path_type   |
| when  | effect_time |
| what  | effect_type |

### Etc (0.9.0)

This library (and all the programs in `src/*`) work with C++17 and up.

The Nix package was renamed from `wtr-watcher` to `watcher`. We can namespace
things differently in Nix if we want to.

The clang-format file avoids some oddities around namespaces and line breaks.

Removed `platform.hpp` as not needed. Platform definitions aren't complicated
enough to have a separate header for.

Removed `adapter.hpp` as not needed. The (minor) functionality it provided
was moved into `watch.hpp`.

The benchmarking programs are more clear about what they're benchmarking, and how.
They're still not *really* a benchmark suite, more of a performance regression test.

The concurrent watcher tests compare the *set* of events they receive against
the *set* of events we expect. Comparing the *order* of the events in those
tests was always a bit wrong, and the tests occasionally told us something was amiss.

Made the comment style more consistent. Removed the `@brief` comment sections
and normalized most comments to look like this:

```
/*  This is a
    multi-line
    comment. */
/*  This is a single-line comment. */
```

### Build (0.9.0)

The relative source and module paths seem to behave oddly on Windows and Nix
(flakes). The CMake files used to live around the `build/in` and
`build/in/cmake` directories. For now, it seems easiest to leave them in the
project's root directory.

If you had been building like this:

`cmake -S build/in -B build/out ...`

You should build like this now:

`cmake -S . -B build/out ...`

The sanitizer build don't live in subdirectories. They live alongside the other
binaries, affixed with the sanitizer name.

The unit test and bench targets aren't broken up by their test name. All the unit
tests have a single target. The benches have another target.

A build directory which previously looked like this:

```
build/out/this:
    nosan:
        wtr.watcher
        wtr.test_watcher.<test name>
        wtr.bench_watcher.<bench name>
        <other binaries ...>
    asan:
        wtr.watcher
        <other binaries ...>
    <other sanitizers ...>
```

Will now look like this:

```
build/out/this:
    wtr.watcher
    wtr.test_watcher
    wtr.bench_watcher
    wtr.watcher.<sanitizer name>
    wtr.test_watcher.<sanitizer name>
    wtr.bench_watcher.<sanitizer name>
```

Multi-configuration build systems will still break up all of the targets into
subdirectories by their build-configuration name. Those build systems are typical
of IDEs. The configuration names are typically `Debug`, `Release`, `MinSizeRel`
and `RelWithDebInfo`. (That behavior hasn't changed.)

Otherwise, the Nix flake was brushed up a bit to better support CMake's
`FetchContent` feature and Darwin's system libraries, and the `tool/build` script
checks for a `build` directory before (re)generating CMake files in it.

## 0.8.8

Fixed some build errors on g++ 10.2.1 around `using shorthand = big_long::enum_name;` syntax.
Made amalgamation (`tool/hone --header-content-amalgam`) much faster by skipping the formatting pass.

## 0.8.7

Removed the directory cache from the `fanotify` adapter. There were some cases where this cache produced misleading results.

## 0.8.6

Added *0.8.[5, 4]* to the changelog.

## 0.8.5

Added a blocking operation to the `tiny_watcher` example where the user's work would go. (In this case, reading something from `stdin`.)

## 0.8.4

Fixed a compilation error on MSVC. Evidently, MSVC has some difficulty knowing the difference between an `enum` and an `enum class` depending on its use, regardless of its declared type.

## 0.8.3

`std::filesystem` throws exceptions *even when we use the `std::error_code`* interfaces. Unfortunately, this library needs to build with exceptions for now as a workaround. The alternative is a crash. This was a regression and could happen when watching not-really-a-filesystem parts of the filesystem, like `/proc`, or just everything, `/`.

In the future, we could use a POSIX API to avoid `std::filesystem`'s reliance on exceptions.

## 0.8.2

Removed a stray `printf()` from the linux/inotify adapter.

## 0.8.1

Fixed a for loop with bad pointer math on the linux/inotify adapter.

## 0.8.0

The Darwin adapter is more skeptical of the kernel. We check for null on the context passed to us in `event_recv`, even though it shouldn't be possible for it to be null.

The `operator<<`s for `event` are more user-customizable. Their signatures look like this now:

```cpp
template<class Char, class CharTraits>
inline std::ostream& operator<<(std::basic_ostream<Char, CharTraits>& os, enum kind const& k) noexcept;
```

Users should be able to work off of `std::basic_ostream<Char, CharTraits>` if they want to send it to something other than the standard streams.

This is a new minor release because I'm not sure if the `basic_ostream` changes are ABI-compatible with `0.7.0`.

## 0.7.0

CLI code is a bit simpler. Usage is more helpful. Plans for filters.

The type of the `fanotify` adapter's loop variable for handle hashes is a `decltype` of the bytes. Not a char. This is correct.

This release should have been `0.6.1`.

## 0.6.0

### 0.6.0 API

`wtr::watcher::die` doesn't exist. Instead, closing the watcher is dependant on there *being* a watch: `watch()` returns a *unique* way for you to close it.

```cpp
// Object-like style

auto lifetime = wtr::watch(".", [](auto e){cout << e;});

lifetime.close();

// Function-like style

auto die = wtr::watch(".", [](auto e){cout << e;});

die();
```

`watch()` isn't an object. It's a function that returns a (structure containing a) named function, `close()`. `()` also resolve to `close()`, so it can be anonymous, too.

Instead of `#include <watcher/watcher.hpp>`, now you include `<wtr/watcher.hpp>`. This lines up with the namespace.

The namespace `wtr::watcher` is inlined, so calling `wtr::watcher::watch(...)` is the same as `wtr::watch(...)`. Inline namespaces are generally used for versioning, not simplifying. I still think it's a worthwhile change for the user.

### 0.6.0 Internal Behavior

Every watcher is unique. (Before, we tried to do fancy stuff by counting watchers on each path. I thought that would be useful someday to coalesce events. The watchers are so efficient that this doesn't seem to matter. If the OS wants to batch event reporting across "listeners", which it probably already does, then that's fine.)

## 0.5.6

### 0.5.6 Housekeeping

Everything we use from every file we include is documented.

The header slugs in (this) changelog don't conflict.

## 0.5.5

### 0.5.5 Behavior

When two or more watches are created on the same path,
they are closed in the order they are created.

### 0.5.5 Housekeeping

A few spare exceptionless functions were marked noexcept.

The delay between setting up test directories, used in the unit tests, is lower (10ms).

We need that delay because the kernel APIs, other than `inotify` and `fanotify`, don't tell us
when they're *acutally* ready to watch. And, when they are ready to watch, sometimes they (because
of batching) pick up filesystem events slightly before we *asked* them to watch.

(That's perfectly fine when the user is in control, but the unit tests currently expect events to
flow in a predetermined order.)

The `wtr::watcher::detail` namespace mirrors a real path: `include/watcher/detail`.

The regular expressions checking whether to build no targets or some targets actually work.

Shellcheck warnings were cleaned up on `build/build`.

## 0.5.4

The Linux adapter checks the kernel version (at compile-time) to select an API that actually exists.

## 0.5.3

The Linux `fanotify` adapter for linux is implemented.
It is selected if the user is (effectively) root and not on Android.
The `inotify` adapter is used in those cases.

The Darwin `FSEvents` adapter has independent resources per each (user) invocation of `watcher::watch`.

Documentation for `watcher::die` is more clear.

More than half (66%) of the codebase (3429 lines) is documentation (39%) and unit tests (27%).
The significant lines of code and documentation are current in the readme.

`tiny-main` is now `tiny_main`. We may rename this to `tiny_watcher` in the future.

The `tool/hone` amalgamation uses a valid identifier for an `ifdef` guard.

### 0.5.3 Internal

The `watcher::detail::adapter::watch_ctl` function is now `watcher::detail::adapter::adapter`.
(This is the only function in `watcher/adapter/adapter.hpp`.)

`watcher/adapter/adapter.hpp` is documented.

`watcher::detail::adapter::adapter::path_id` is a (constant) variable, not a function.

`watcher::detail::adapter::adapter::living_container` uses `std::string` as the key type, not an integral type.

The Windows adapter is given some much needed love.

Heavier reliance on using `watcher/watcher.hpp` internally.

## 0.5.2

The changelog for `0.5.0` is updated. Again.

## 0.5.1

The changelog for `0.5.0` is updated.

## 0.5.0

Gives support for several watchers. When watchers exist for the same path,
*Watcher* thinks of its resources on that path like a shared pointer.

A warning from the thread sanitizer is gone.

*Watcher* and its unit tests pass all sanitizers and unit tests without warnings.

The readme is more readable. More notes exist, and links to them are used.

A ConanCenter badge is beside the other badges in the readme.

Instead of overloads and templates for path types, `std::filesystem::path` is used.

Define a project roadmap for release `1.0.0`.

Unit tests are more robust. A "Simple" unit test is added.

## 0.4.3

Single-include amalgamation.

Consider automatic amalgamation.

## 0.4.2

Public functions defined in headers prefer `inline` over `static`.

Threads may not race when `epoll_wait` resumes on long wait periods despite `is_living()` being false.

No warnings in `tsan/wtr.test_watcher.test_event_targets`.

## 0.4.1

Clean misplaced file

## 0.4.0

Linux inotify adapter does not loop away from epoll wait.

The unit tests are more robust.

The namespace `water` is now `wtr`.

`wtr::watcher::event` holds a `string`, not `char*`, in `event.where`.

The `build/build` program is more intuitive. See `build/build --help`.

CI runners, especially those for macOS and Linux, have become more efficient and thorough. The use of `build/build` allows many CI runners to build and test across build types and sanitizers more easily than otherwsie. Some platform specifics for the Android and Windows are being sorted out. In the meantime, Android and Windows are using the "old" runner jobs.

## 0.3.3

The Android adapter is identical to the Linux `inotify`-based adapter except for its handling of `inotify_init` and the associated watch options.

Android was given a CI job.

## 0.3.2

Toge's portability PR was merged.

## 0.3.1

The `inotify`-based adapter for Linux is refined.

## 0.3.0

The `inotify`-based adapter for Linux is introduced. Concerns about its accuracy are carefully considered.

Niall's suggestions prompted future work being planned for improvements on the design of all adapters.

## 0.2.9

The `warthog` adapter is fixed on Windows. The Windows CI runner is nudged.

## 0.2.8

Tooling in `tool/release` is refined to match the typical commit syntax. This currently applies to the release hooks in `.version` and `build/in/CMakeLists.txt`.

## 0.2.7

`tool/release` writes to `.version` and `tool/hone` prepends `#pragma once` to the output from `--header-content-amalgam`.

The Windows CI runner is nudged.

## 0.2.6

Directory tree around `watch` is refined.

`tool/sloc` is introduced (to count the significant lines of codes).

`src/tiny-main.cpp` is introduced.

## 0.2.5

CI is implemented through GitHub Actions.

The MIT license is chosen.

## 0.2.4

A single-header artifact is created.

`tool/hone` is introduced to generate a single-header amalgamation of this project.

Documentation in the `README.md` is heavily updated.

## 0.2.3

`tool/release` is made more clear and the EBNF is made valid (a comma was missing).

## 0.2.2

`tool/release` is usable and fully-featured.

`tool/release` may show its usage as EBNF.

C++20 Concepts are removed for being very interesting but not beneficial enough to an API with a very small surface.

Comments are refactoring according to the MISRA guidelines.

## 0.2.1

`tell dbun` is refactored.

The `FSEvent` adapter is made more safe.

## 0.2.0

The first public release includes:

- The `event` object and the `watch` function
- Stream operator overloads for JSON output
- Event parsing control
- The CLI program
- Documentation throughout
