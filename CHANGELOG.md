# Changelog

## 0.5.3

The Linux `fanotify` adapter for linux is implemented.
It is selected if the user is (effectively) root and not on Android.
The `inotify` adapter is used in those cases.

The Darwin `FSEvents` adapter has independent resources per each (user) invocation of `watcher::watch`.

Documentation for `watcher::die` is more clear.

More than half (66%) of the codebase (3422 lines) is documentation (39%) and unit tests (27%).
The significant lines of code and documentation are current in the readme.

`tiny-main` is now `tiny_main`. We may rename this to `tiny_watcher` in the future.

The `tool/hone` amalgamation uses a valid identifier for an `ifdef` guard.

### Internal

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
