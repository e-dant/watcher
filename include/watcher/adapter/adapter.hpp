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

inline bool watch_ctl(std::filesystem::path const& path,
                      event::callback const& callback, bool const msg) noexcept
{
  auto const path_id
      = [&path]() { return std::hash<std::string>{}(path.string()); };

  static auto watcher_container{std::unordered_map<size_t, size_t>{}};

  static auto watcher_mtx{std::mutex{}};

  auto const& live = [&path_id](std::filesystem::path const& path,
                                event::callback const& callback) -> bool {
    auto _ = std::scoped_lock{watcher_mtx};

    auto const id = path_id();

    bool alive = true;

    if (watcher_container.contains(id))
      watcher_container.at(id) += 1;

    else
      watcher_container[id] = 1;

    callback({(alive ? "s/self/live@" : "e/self/live@") + path.string(),
              event::what::create, event::kind::watcher});

    return alive;
  };

  auto const& die = [&path_id](std::filesystem::path const& path,
                               event::callback const& callback) -> bool {
    auto _ = std::scoped_lock{watcher_mtx};

    auto const id = path_id();

    bool dead = true;

    if (watcher_container.contains(id))

      if (watcher_container.at(id) > 1)
        watcher_container.at(id) -= 1;

      else
        watcher_container.erase(id);

    else
      dead = false;

    callback({(dead ? "s/self/die@" : "e/self/die@") + path.string(),
              event::what::destroy, event::kind::watcher});

    return dead;
  };

  auto const& is_living = [&path_id]() -> bool {
    auto _ = std::scoped_lock{watcher_mtx};

    return watcher_container.contains(path_id());
  };

  if (msg)
    return live(path, callback) ? watch(path, callback, is_living) : false;

  else
    return die(path, callback);
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */
