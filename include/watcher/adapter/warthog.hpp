#pragma once

/*
  @brief watcher/adapter/warthog

  A reasonably dumb adapter that works on any platform.

  This adapter beats `kqueue`, but it doesn't bean recieving
  filesystem events directly from the OS.

  This is the fallback adapter on platforms that either
    - Only support `kqueue`
    - Only support the C++ standard library
*/

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <watcher/concepts.hpp>
#include <watcher/event.hpp>

namespace water {
namespace watcher {
namespace adapter {
namespace warthog {
namespace { /* anonymous namespace for "private" variables */
/*  shorthands for:
      - Path
      - Callback
      - directory_options
      - follow_directory_symlink
      - skip_permission_denied
      - bucket_t, a map type of string -> time */
// clang-format off
using
  concepts::Path,
  concepts::Callback,
  std::filesystem::exists,
  std::filesystem::is_directory,
  std::filesystem::last_write_time,
  std::filesystem::is_regular_file,
  std::filesystem::directory_options,
  std::filesystem::recursive_directory_iterator,
  std::filesystem::directory_options::follow_directory_symlink,
  std::filesystem::directory_options::skip_permission_denied;

/*  We need `bucket` to make caching state easier...
    which is what objects are for. Well, maybe this
    should be an object. I'm not sure. */
inline constexpr std::filesystem::directory_options
  dir_opt = skip_permission_denied & follow_directory_symlink;

static std::unordered_map<std::string, std::filesystem::file_time_type>
    bucket;  // NOLINT

// clang-format on

/*
  @brief watcher/constructor
  @param path - path to monitor for
  @see watcher::status
  Creates a file map, the "bucket", from `path`.
*/
auto populate(const Path auto& path = {"."}) {
  if (exists(path))
    if (is_directory(path))
      try {
        for (const auto& file : recursive_directory_iterator(path, dir_opt))
          if (exists(file))
            bucket[file.path().string()] = last_write_time(file);
      } catch (const std::exception& e) {
        /* this happens when a path was changed while we were reading it.
           there is nothing to do here; we prune later. */
      }
    else
      bucket[path] = last_write_time(path);
  else
    return false;
  return true;
}

/*
  @brief prune
  Removes files which no longer exist from our bucket.
*/
auto prune(const Path auto& path, const Callback auto& callback) {
  using std::filesystem::exists;
  const auto do_prune = [](const auto& callback) {
    auto file = bucket.begin();
    /* while look through the bucket's contents, */
    while (file != bucket.end()) {
      /* check if the stuff in our bucket exists anymore. */
      exists(file->first)
          /* if so, move on. */
          ? std::advance(file, 1)
          /* if not, call the closure, indicating destruction,
             and remove it from our bucket. */
          : [&]() {
              callback(event::event(file->first.c_str(), event::what::destroy));
              /* bucket, erase it! */
              file = bucket.erase(file);
            }();
    }
    return true;
  };

  /* if the bucket is empty, try to populate it. otherwise, prune it. */
  return bucket.empty() ? populate(path) : do_prune(callback);
}

/*
  @brief scan_file
  Scans a single `file` for changes. Updates our bucket. Calls `callback`.
*/
bool scan_file(const Path auto& file, const Callback auto& callback) {
  if (exists(file) && is_regular_file(file)) {
    auto ec = std::error_code{};
    /* grabbing the file's last write time */
    const auto timestamp = last_write_time(file, ec);
    if (ec) {
      /* the file changed while we were looking at it. so, we call the closure,
       * indicating destruction, and remove it from the bucket. */
      callback(event::event(file, event::what::destroy));
      if (bucket.contains(file))
        bucket.erase(file);
    }
    /* if it's not in our bucket, */
    else if (!bucket.contains(file)) {
      /* we put it in there and call the closure, indicating creation. */
      bucket[file] = timestamp;
      callback(event::event(file, event::what::create));
    }
    /* otherwise, it is already in our bucket. */
    else {
      /* we update the file's last write time, */
      if (bucket[file] != timestamp) {
        bucket[file] = timestamp;
        /* and call the closure on them, indicating modification */
        callback(event::event(file, event::what::modify));
      }
    }
    return true;
  } /* if the path doesn't exist, we nudge the callee with `false` */
  else
    return false;
}

bool scan_directory(const Path auto& dir, const Callback auto& callback) {
  /* if this thing is a directory */
  if (is_directory(dir)) {
    /* try to iterate through its contents */
    auto ec = std::error_code{};
    for (const auto& file : recursive_directory_iterator(dir, dir_opt, ec))
      /* while handling errors */
      if (ec)
        return false;
      else
        scan_file(file.path().c_str(), callback);
    return true;
  } else
    return false;
}

}  // namespace

/*
  @brief watcher/run

  @param closure (optional):
   A callback to perform when the files
   being watched change.
   @see Callback

  Monitors `path` for changes.

  Executes `callback` when they
  happen.

  Unless it should stop, or errors present,
  `run` recurses into itself.
*/
template <const auto delay_ms = 16>
inline bool run(const Path auto& path, const Callback auto& callback) {
  /* see note [alternative run loop syntax] */
  using std::this_thread::sleep_for, std::chrono::milliseconds,
      std::filesystem::exists;

  if constexpr (delay_ms > 0)
    sleep_for(milliseconds(delay_ms));
  /* if no errors present, keep running. otherwise, leave. */
  return

      prune(path, callback)

          ? scan_directory(path, callback)

                ? warthog::run<delay_ms>(path, callback)

                : scan_file(path, callback)

                      ? warthog::run<delay_ms>(path, callback)

                      : false

          : false;
}

/*
  # Notes

  ## Alternative `run` loop syntax

    The syntax currently being used is short, but somewhat irregular.
    An quivalent pattern is provided here, in case we want to change it.
    This may or may not be more clear. I'm not sure.

    ```cpp
    prune(path, callback);
    while (scan(path, callback))
      if constexpr (delay_ms > 0)
        sleep_for(milliseconds(delay_ms));
    return false;
    ```
*/

}  // namespace warthog
namespace literal {
using water::watcher::adapter::warthog::run;  // NOLINT
}
}  // namespace adapter
}  // namespace watcher
}  // namespace water
