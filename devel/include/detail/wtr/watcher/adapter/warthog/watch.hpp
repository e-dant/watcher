#pragma once

/* WATER_WATCHER_PLATFORM_* */
#include <detail/wtr/watcher/platform.hpp>

#if defined(WATER_WATCHER_PLATFORM_UNKNOWN) \
|| defined(WATER_WATCHER_USE_WARTHOG)

/*
  @brief watcher/adapter/warthog

  A reasonably dumb adapter that works on any platform.

  This adapter beats `kqueue`, but it doesn't bean recieving
  filesystem events directly from the OS.

  This is the fallback adapter on platforms that either
    - Only support `kqueue` (`warthog` beats `kqueue`)
    - Only support the C++ standard library
*/

/* milliseconds */
#include <chrono>
/* string */
#include <string>
/* filesystem::* */
#include <filesystem>
/* function */
#include <functional>
/* error_code */
#include <system_error>
/* this_thread::sleep_for */
#include <thread>
/* unordered_map */
#include <unordered_map>
/* event
   callback */
#include <wtr/watcher.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
namespace {

/* clang-format off */

inline constexpr std::filesystem::directory_options
  scan_dir_options = 
    /* This is ridiculous */
    std::filesystem::directory_options::skip_permission_denied 
    & std::filesystem::directory_options::follow_directory_symlink;

using bucket_type = std::unordered_map<std::string, std::filesystem::file_time_type>;

/* clang-format on */

/*  @brief watcher/adapter/warthog/scan
    - Scans `path` for changes.
    - Updates our bucket to match the changes.
    - Calls `send_event` when changes happen.
    - Returns false if the file tree cannot be scanned. */
inline bool scan(std::filesystem::path const& path,
                 auto const& send_event,
                 bucket_type& bucket) noexcept {
  /* @brief watcher/adapter/warthog/scan_file
     - Scans a (single) file for changes.
     - Updates our bucket to match the changes.
     - Calls `send_event` when changes happen.
     - Returns false if the file cannot be scanned. */
  auto const& scan_file
  = [&](std::filesystem::path const& file, auto const& send_event) -> bool {
    using std::filesystem::exists, std::filesystem::is_regular_file,
    std::filesystem::last_write_time;
    if (exists(file) && is_regular_file(file)) {
      auto ec = std::error_code{};
      /* grabbing the file's last write time */
      auto const& timestamp = last_write_time(file, ec);
      if (ec) {
        /* the file changed while we were looking at it. so, we call the
         * closure, indicating destruction, and remove it from the bucket. */
        send_event(event::event{file, event::what::destroy, event::kind::file});
        if (bucket.contains(file)) bucket.erase(file);
      }
      /* if it's not in our bucket, */
      else if (! bucket.contains(file)) {
        /* we put it in there and call the closure, indicating creation. */
        bucket[file] = timestamp;
        send_event(event::event{file, event::what::create, event::kind::file});
      }
      /* otherwise, it is already in our bucket. */
      else {
        /* we update the file's last write time, */
        if (bucket[file] != timestamp) {
          bucket[file] = timestamp;
          /* and call the closure on them, indicating modification */
          send_event(
          event::event{file, event::what::modify, event::kind::file});
        }
      }
      return true;
    } /* if the path doesn't exist, we nudge the callee with `false` */
    else
      return false;
  };

  /* @brief watcher/adapter/warthog/scan_directory
     - Scans a (single) directory for changes.
     - Updates our bucket to match the changes.
     - Calls `send_event` when changes happen.
     - Returns false if the directory cannot be scanned. */
  auto const& scan_directory
  = [&](std::filesystem::path const& dir, auto const& send_event) -> bool {
    using std::filesystem::recursive_directory_iterator,
    std::filesystem::is_directory;
    /* if this thing is a directory */
    if (is_directory(dir)) {
      /* try to iterate through its contents */
      auto dir_it_ec = std::error_code{};
      for (auto const& file :
           recursive_directory_iterator(dir, scan_dir_options, dir_it_ec))
        /* while handling errors */
        if (dir_it_ec)
          return false;
        else
          scan_file(file.path(), send_event);
      return true;
    } else
      return false;
  };

  return scan_directory(path, send_event) ? true
       : scan_file(path, send_event)      ? true
                                          : false;
};

/* @brief wtr/watcher/warthog/tend_bucket
   If the bucket is empty, try to populate it.
   otherwise, prune it. */
inline bool tend_bucket(std::filesystem::path const& path,
                        auto const& send_event,
                        bucket_type& bucket) noexcept {
  /*  @brief watcher/adapter/warthog/populate
      @param path - path to monitor for
      Creates a file map, the "bucket", from `path`. */
  auto const& populate = [&](std::filesystem::path const& path) -> bool {
    using std::filesystem::exists, std::filesystem::is_directory,
    std::filesystem::recursive_directory_iterator,
    std::filesystem::last_write_time;
    /* this happens when a path was changed while we were reading it.
     there is nothing to do here; we prune later. */
    auto dir_it_ec = std::error_code{};
    auto lwt_ec = std::error_code{};
    if (exists(path)) {
      /* this is a directory */
      if (is_directory(path)) {
        for (auto const& file :
             recursive_directory_iterator(path, scan_dir_options, dir_it_ec)) {
          if (! dir_it_ec) {
            auto const& lwt = last_write_time(file, lwt_ec);
            if (! lwt_ec)
              bucket[file.path()] = lwt;
            else
              /* @todo use this practice elsewhere or make a fn for it
                 otherwise, this might be confusing and inconsistent. */
              bucket[file.path()] = last_write_time(path);
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

  /*  @brief watcher/adapter/warthog/prune
      Removes files which no longer exist from our bucket. */
  auto const& prune
  = [&](std::filesystem::path const& path, auto const& send_event) -> bool {
    using std::filesystem::exists, std::filesystem::is_regular_file,
    std::filesystem::is_directory, std::filesystem::is_symlink;
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
          send_event(event::event{bucket_it->first,
                                  event::what::destroy,
                                  is_regular_file(path) ? event::kind::file
                                  : is_directory(path)  ? event::kind::dir
                                    : is_symlink(path)  ? event::kind::sym_link
                                                        : event::kind::other});
          /* bucket, erase it! */
          bucket_it = bucket.erase(bucket_it);
        }();
    }
    return true;
  };

  return bucket.empty() ? populate(path)          ? true
                        : prune(path, send_event) ? true
                                                  : false
                        : true;
};

} /* namespace */

/* @pragma/tool/hone/insert namespace { */

/*
  @brief watcher/adapter/warthog/watch

  @param path:
   A path to watch for changes.

  @param callback:
   A callback to perform when the files
   being watched change.

  Monitors `path` for changes.

  Calls `callback` with an `event` when they happen.

  Unless it should stop, or errors present, `watch` recurses.
*/

inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  std::function<bool()> const& is_living) noexcept {
  using std::this_thread::sleep_for, std::chrono::milliseconds;
  /* Sleep for `delay_ms`.

     Then, keep running if
       - We are alive
       - The bucket is doing well
       - No errors occured while scanning

     Otherwise, stop and return false. */

  bucket_type bucket;

  static constexpr auto delay_ms = 16;

  while (is_living()) {
    if (! tend_bucket(path, callback, bucket)
        || ! scan(path, callback, bucket)) {
      callback(
      {"e/self/die/bad_fs@" + path.string(), evw::destroy, evk::watcher});

      return false;
    } else {
      if constexpr (delay_ms > 0) sleep_for(milliseconds(delay_ms));
    }
  }

  callback({"s/self/die@" + path.string(), evw::destroy, evk::watcher});

  return true;
}

/* @pragma/tool/hone/insert } */
} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

#endif /* defined(WATER_WATCHER_PLATFORM_UNKNOWN) \
          || defined(WATER_WATCHER_USE_WARTHOG) */
