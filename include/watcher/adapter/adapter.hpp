#pragma once

/* obj: path */
#include <filesystem>
/* obj: mutex */
#include <mutex>
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
  auto const path_id = [&path]() -> unsigned long {
    auto path_str = path.string();
    return std::hash<decltype(path_str)>{}(path_str);
  };

  static auto watcher_container
      = std::unordered_map<unsigned long, unsigned long>{};
  static auto watcher_mtx = std::mutex{};

  auto const& live = [&path_id](std::filesystem::path const& path,
                                event::callback const& callback) -> bool {
    auto _ = std::scoped_lock{watcher_mtx};

    auto const id = path_id();

    bool alive = true;

    if (watcher_container.contains(id))
      watcher_container.at(id) += 1;

    else
      watcher_container[id] = 1;

    /* std::cout << "watch_ctl -> live -> '" << path << "' -> " << id << " => "
     */
    /*           << (alive ? "true" : "false") << std::endl; */

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

    /* std::cout << "watch_ctl -> die -> '" << path << "' -> " << id << " => "
     */
    /*           << (dead ? "true" : "false") << std::endl; */

    callback({(dead ? "s/self/die@" : "e/self/die@") + path.string(),
              event::what::destroy, event::kind::watcher});

    return dead;
  };

  auto const& is_living = [&path_id]() -> bool {
    auto _ = std::scoped_lock{watcher_mtx};

    auto const id = path_id();

    bool alive = watcher_container.contains(id);

    /* std::cout << "watch_ctl -> is_living -> '" << path << "' -> " << id */
    /*           << " => " << (alive ? "true" : "false") << std::endl; */

    return alive;
  };

  if (msg) {
    auto alive
        = live(path, callback) ? watch(path, callback, is_living) : false;
    /* std::cout << "watch -> adapter -> watch_ctl -> msg -> live -> '" << path
     */
    /*           << "' => " << (alive ? "true" : "false") << std::endl; */
    return alive;
  } else {
    auto dead = die(path, callback);
    /* std::cout << "watch -> adapter -> watch_ctl -> msg -> die -> '" << path
     */
    /*           << "' => " << (dead ? "true" : "false") << std::endl; */
    return dead;
  }
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */
