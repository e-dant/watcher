#pragma once

#include <watcher/platform.hpp>
#if defined(PLATFORM_WINDOWS_ANY)

/*
  @brief watcher/adapter/windows

  The Windows adapter.
*/

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <watcher/event.hpp>

namespace water {
namespace watcher {
namespace detail {
namespace adapter {
namespace { /* anonymous namespace for "private" variables */
/*  shorthands for:
      - Path
      - Callback
      - directory_options
      - follow_directory_symlink
      - skip_permission_denied
      - bucket_t, a map type of string -> time */
/* clang-format off */
using
  std::filesystem::exists,
  std::filesystem::is_symlink,
  std::filesystem::is_directory,
  std::filesystem::is_directory,
  std::filesystem::is_regular_file,
  std::filesystem::last_write_time,
  std::filesystem::is_regular_file,
  std::filesystem::recursive_directory_iterator,
  std::filesystem::directory_options::follow_directory_symlink,
  std::filesystem::directory_options::skip_permission_denied;

/*  We need `bucket` to make caching state easier...
    which is what objects are for. Well, maybe this
    should be an object. I'm not sure. */
inline constexpr std::filesystem::directory_options
  dir_opt = skip_permission_denied & follow_directory_symlink;

static std::unordered_map<std::string, std::filesystem::file_time_type>
    bucket;  /* NOLINT */

/* clang-format on */

/*
  @brief watcher/adapter/windows/scan_file
  Scans a single `file` for changes. Updates our bucket. Calls `callback`.
*/
bool scan_file(const char* file, const auto& callback) {
  if (exists(file) && is_regular_file(file)) {
    auto ec = std::error_code{};
    /* grabbing the file's last write time */
    const auto timestamp = last_write_time(file, ec);
    if (ec) {
      /* the file changed while we were looking at it. so, we call the closure,
       * indicating destruction, and remove it from the bucket. */
      callback(event::event{file, event::what::destroy, event::kind::file});
      if (bucket.contains(file))
        bucket.erase(file);
    }
    /* if it's not in our bucket, */
    else if (!bucket.contains(file)) {
      /* we put it in there and call the closure, indicating creation. */
      bucket[file] = timestamp;
      callback(event::event{file, event::what::create, event::kind::file});
    }
    /* otherwise, it is already in our bucket. */
    else {
      /* we update the file's last write time, */
      if (bucket[file] != timestamp) {
        bucket[file] = timestamp;
        /* and call the closure on them, indicating modification */
        callback(event::event{file, event::what::modify, event::kind::file});
      }
    }
    return true;
  } /* if the path doesn't exist, we nudge the callee with `false` */
  else
    return false;
}

bool scan_directory(const char* dir, const auto& callback) {
  /* if this thing is a directory */
  if (is_directory(dir)) {
    /* try to iterate through its contents */
    auto dir_it_ec = std::error_code{};
    for (const auto& file :
         recursive_directory_iterator(dir, dir_opt, dir_it_ec))
      /* while handling errors */
      if (dir_it_ec)
        return false;
      else
        scan_file((const char*)(file.path().c_str()), callback);
    return true;
  } else
    return false;
}

} /* namespace */

/*
  @brief watcher/adapter/windows/watch

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
inline bool watch(const char* path, const auto& callback) {
  /* see note [alternative watch loop syntax] */
  using std::this_thread::sleep_for, std::chrono::milliseconds,
      std::filesystem::exists;
  /*  @brief watcher/adapter/windows/populate
      @param path - path to monitor for
      Creates a file map, the "bucket", from `path`. */
  const auto populate = [](const char* path) {
    /* this happens when a path was changed while we were reading it.
     there is nothing to do here; we prune later. */
    auto dir_it_ec = std::error_code{};
    auto lwt_ec = std::error_code{};
    if (exists(path)) {
      /* this is a directory */
      if (is_directory(path)) {
        for (const auto& file :
             recursive_directory_iterator(path, dir_opt, dir_it_ec)) {
          if (!dir_it_ec) {
            const auto lwt = last_write_time(file, lwt_ec);
            if (!lwt_ec)
              bucket[file.path().string()] = lwt;
            else
              /* @todo use this practice elsewhere or make a fn for it
                 otherwise, this might be confusing and inconsistent. */
              bucket[file.path().string()] = last_write_time(path);
          }
        }
      }
      /* this is a file */
      else {
        bucket[path] = last_write_time(path);
      }
    } else {
      return false;
    }
    return true;
  };

  /*  @brief watcher/adapter/windows/prune
      Removes files which no longer exist from our bucket. */
  const auto prune = [](const char* path, const auto& callback) {
    auto bucket_it = bucket.begin();
    /* while looking through the bucket's contents, */
    while (bucket_it != bucket.end()) {
      /* check if the stuff in our bucket exists anymore. */
      exists(bucket_it->first)
          /* if so, move on. */
          ? std::advance(bucket_it, 1)
          /* if not, call the closure, indicating destruction,
             and remove it from our bucket. */
          : [&]() {
              callback(event::event{bucket_it->first.c_str(),
                                    event::what::destroy,
                                    is_regular_file(path) ? event::kind::file
                                    : is_directory(path)  ? event::kind::dir
                                    : is_symlink(path) ? event::kind::sym_link
                                                       : event::kind::other});
              /* bucket, erase it! */
              bucket_it = bucket.erase(bucket_it);
            }();
    }
    return true;
  };

  if constexpr (delay_ms > 0)
    sleep_for(milliseconds(delay_ms));

  /* if the bucket is empty, try to populate it. otherwise, prune it. */
  bucket.empty() ? populate(path) : prune(path, callback);

  /* if no errors present, keep running. otherwise, leave. */
  return scan_directory(path, callback) ? adapter::watch<delay_ms>(path, callback)
         : scan_file(path, callback)    ? adapter::watch<delay_ms>(path, callback)
                                        : false;
}

/*
  # Notes

  ## Alternative `watch` loop syntax

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

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace water */

#endif /* if defined(PLATFORM_WINDOWS_ANY) */
