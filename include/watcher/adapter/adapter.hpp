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
  static auto living_container{std::unordered_map<std::string, size_t>{}};

  /* @brief
     A primitive thread synchrony tool, a mutex, for the watcher container. */
  static auto living_container_mtx{std::mutex{}};

  /* @brief
     Some path as a string. We need the path as a string to store it as the
     key in a map because `std::filesystem::path` doesn't know how to hash,
     but `std::string` does. */
  auto const& path_str = path.string();

  /* @brief
     A predicate given to the watchers.
     True if living, false if dead.

     The watchers are expected to die promptly,
     but safely, when this returns false. */
  auto const& is_living = [&path_str]() -> bool {
    auto _ = std::scoped_lock{living_container_mtx};

    return living_container.contains(path_str);
  };

  /* @brief
     Increment the watch count on a unique path.
     If the count is 0, insert the path into the set
     of living watchers.

     Always returns true. */
  auto const& live = [&path_str, &callback]() -> bool {
    auto _ = std::scoped_lock{living_container_mtx};

    living_container[path_str] = living_container.contains(path_str)
                                     ? living_container.at(path_str) + 1
                                     : 1;

    callback(
        {"s/self/live@" + path_str, event::what::create, event::kind::watcher});

    return true;
  };

  /* @brief
     Decrement the watch count on a unique path.
     If the count is 0, erase the path from the set
     of living watchers.

     When a path's count is 0, the watchers on that path die.

     Returns false if the `path` is not being watched. */
  auto const& die = [&path_str, &callback]() -> bool {
    auto _ = std::scoped_lock{living_container_mtx};

    bool dead = true;

    if (living_container.contains(path_str)) {
      auto& at = living_container.at(path_str);

      if (at > 1)
        at -= 1;

      else
        living_container.erase(path_str);

    } else
      dead = false;

    callback({(dead ? "s/self/die@" : "e/self/die@") + path_str,
              event::what::destroy, event::kind::watcher});

    return dead;
  };

  /* @brief
     We know two messages:
       - Tell a watcher to live
       - Tell a watcher to die
     There may be more messages in the future. */
  if (msg)
    return live() && watch(path, callback, is_living);

  else
    return die();
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */
