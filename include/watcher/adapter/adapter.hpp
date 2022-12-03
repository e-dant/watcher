#pragma once

//
//
//
#include <iostream>
//
//
//
/* obj: path */
#include <filesystem>
/* obj: mutex */
#include <mutex>
/* obj: unordered_set */
#include <unordered_set>
/* fn: watch
   fn: die */
#include <watcher/event.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

inline constexpr auto delay_ms = 16;

/*
  @brief watcher/adapter/watch
  Monitors `path` for changes.
  Invokes `callback` with an `event` when they happen.
  `watch` stops when asked to or irrecoverable errors occur.
  All events, including errors, are passed to `callback`.

  @param path:
   A `path` to watch for filesystem events.

  @param callback:
   A `callback` to invoke with an `event` object
   when the files being watched change.
*/

inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback,
                  auto const& is_living) noexcept;

inline bool watch_ctl(std::filesystem::path const& path,
                      event::callback const& callback, bool const msg) noexcept
{
  auto const path_id = [&path]() -> unsigned long {
    auto path_str = path.string();
    return std::hash<decltype(path_str)>{}(path_str);
  };

  static auto watcher_container = std::unordered_set<unsigned long>{};
  static auto watcher_container_mtx = std::mutex{};

  auto const& live = [&path_id](std::filesystem::path const& path,
                                event::callback const& callback) -> bool {
    auto const id = path_id();

    bool alive = true;

    watcher_container_mtx.lock();

    if (watcher_container.contains(id))
      alive = false;

    else
      watcher_container.insert(id);

    std::cout << "watch_ctl -> live -> '" << path << "' -> " << id << " => "
              << (alive ? "true" : "false") << std::endl;

    watcher_container_mtx.unlock();

    callback(event::event{(alive ? "s/self/live@" : "e/self/live@") / path,
                          event::what::create, event::kind::watcher});
    return alive;
  };

  auto const& is_living
      = [&path_id](std::filesystem::path const& path) -> bool {
    auto const id = path_id();

    watcher_container_mtx.lock();

    bool alive = watcher_container.contains(id);

    std::cout << "watch_ctl -> is_living -> '" << path << "' -> " << id
              << " => " << (alive ? "true" : "false") << std::endl;

    watcher_container_mtx.unlock();

    return alive;
  };

  auto const& die = [&path_id](std::filesystem::path const& path,
                               event::callback const& callback) -> bool {
    auto const id = path_id();

    bool dead = true;

    watcher_container_mtx.lock();

    if (watcher_container.contains(id))
      watcher_container.erase(id);

    else
      dead = false;

    std::cout << "watch_ctl -> die -> '" << path << "' -> " << id << " => "
              << (dead ? "true" : "false") << std::endl;

    watcher_container_mtx.unlock();

    callback(event::event{(dead ? "s/self/die@" : "e/self/die@") / path,
                          event::what::destroy, event::kind::watcher});

    return dead;
  };

  if (msg) {
    auto ok = live(path, callback) ? watch(path, callback, is_living) : false;
    std::cout << "watch -> adapter -> watch_ctl -> msg -> live -> '" << path
              << "' => " << (ok ? "true" : "false") << std::endl;
    return ok;
  } else {
    auto ok = die(path, callback);
    std::cout << "watch -> adapter -> watch_ctl -> msg -> die -> '" << path
              << "' => " << (ok ? "true" : "false") << std::endl;
    return ok;
  }
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

/* We need these headers at the bottom because the function they
   implement, `watch`, is forward-declared above.
   We need to include `<watcher/platform.hpp>` first because the
   platform definitions there decide which adapter is used.
   The `warthog` adapter is a fallback. */

/* clang-format off */

#include <watcher/platform.hpp>

#include <watcher/adapter/windows/watch.hpp>
#include <watcher/adapter/darwin/watch.hpp>
#include <watcher/adapter/linux/watch.hpp>
#include <watcher/adapter/android/watch.hpp>
#include <watcher/adapter/warthog/watch.hpp>

/* clang-format on */
