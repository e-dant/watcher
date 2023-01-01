#pragma once

/* obj: path */
#include <filesystem>
/* obj: mutex */
#include <mutex>
/* obj: string */
#include <string>
/* obj: unordered_map */
#include <unordered_map>
/* fn: watch
   fn: die */
#include <watcher/event.hpp>

/* clang-format off */
#include <watcher/platform.hpp>
#include <watcher/adapter/windows/watch.hpp>
#include <watcher/adapter/darwin/watch.hpp>
#include <watcher/adapter/linux/watch.hpp>
#include <watcher/adapter/android/watch.hpp>
#include <watcher/adapter/warthog/watch.hpp>
/* clang-format on */

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

inline bool adapter(std::filesystem::path const& path,
                    event::callback const& callback, bool const& msg) noexcept
{
  /* @brief
     This container is used to count the number of watchers on some path.
     The key is a hash of a path.
     The value is the count of living watchers on a path. */
  static auto watcher_container{std::unordered_map<size_t, size_t>{}};

  /* @brief
     A primitive thread synchrony tool, a mutex, for the watcher container. */
  static auto watcher_mtx{std::mutex{}};

  /* @brief
     Some path's hash. We need this to interpret the `path`, which is a
     `std::filesystem::path`, as a `std::string`. `std::filesystem::path`
     doesn't know how to hash, but `std::string` does. */
  auto const& path_id
      = [&path]() { return std::hash<std::string>{}(path.string()); }();

  /* @brief
     Increment the watch count on a unique path.
     If the count is 0, insert the path into the set
     of living watchers.

     Always returns true. */
  auto const& live = [&path_id](std::filesystem::path const& path,
                                event::callback const& callback) -> bool {
    auto _ = std::scoped_lock{watcher_mtx};

    if (watcher_container.contains(path_id))
      watcher_container.at(path_id) += 1;

    else
      watcher_container[path_id] = 1;

    callback({"s/self/live@" + path.string(), event::what::create,
              event::kind::watcher});

    return true;
  };

  /* @brief
     Decrement the watch count on a unique path.
     If the count is 0, erase the path from the set
     of living watchers.

     When a path's count is 0, the watchers on that path die.

     Returns false if the `path` is not being watched. */
  auto const& die = [&path_id](std::filesystem::path const& path,
                               event::callback const& callback) -> bool {
    auto _ = std::scoped_lock{watcher_mtx};

    bool dead = true;

    if (watcher_container.contains(path_id)) {
      auto& at = watcher_container.at(path_id);

      if (at > 1)
        at -= 1;

      else
        watcher_container.erase(path_id);

    } else
      dead = false;

    callback({(dead ? "s/self/die@" : "e/self/die@") + path.string(),
              event::what::destroy, event::kind::watcher});

    return dead;
  };

  /* @brief
     A predicate given to the watchers.
     True if living, false if dead.

     The watchers are expected to die promptly,
     but safely, when this returns false. */
  auto const& is_living = [&path_id]() -> bool {
    auto _ = std::scoped_lock{watcher_mtx};

    return watcher_container.contains(path_id);
  };

  /* @brief
     We know two messages:
       - Tell a watcher to live
       - Tell a watcher to die
     There may be more messages in the future. */
  if (msg)
    return live(path, callback) && watch(path, callback, is_living);

  else
    return die(path, callback);
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */
