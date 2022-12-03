#pragma once

//
//
//
#include <iostream>
//
//
//
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
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

  Other platforms might define more overloads, such as
  the wide `std::wstring` and `wchar_t const*` variants.
*/

inline bool watch(auto const& path, event::callback const& callback,
                  auto const& is_living) noexcept;

inline bool watch_ctl(auto const& path, event::callback const& callback,
                      bool const msg) noexcept
{
  auto wcont = std::unordered_set<std::string>{};
  auto wcont_mtx = std::mutex{};

  auto const& live
      = [&wcont, &wcont_mtx](std::string const& path, event::callback const& callback) -> bool {
    bool ok = true;
    wcont_mtx.lock();

    if (wcont.contains(path))
      ok = false;

    else
      wcont.insert(path);

    std::cout << "watch_ctl -> live -> '" << path << "' => "
              << (ok ? "true" : "false") << std::endl;

    wcont_mtx.unlock();

    callback(event::event{(ok ? "s/self/live@" : "e/self/live@") + path,
                          event::what::create, event::kind::watcher});
    return ok;
  };

  auto const& is_living = [&wcont, &wcont_mtx](std::string const& path) -> bool {
    wcont_mtx.lock();
    bool living = wcont.contains(path);
    std::cout << "watch_ctl -> is_living -> '" << path << "' => "
              << (living ? "true" : "false") << std::endl;
    wcont_mtx.unlock();
    return living;
  };

  auto const& die
      = [&wcont, &wcont_mtx](std::string const& path, event::callback const& callback) -> bool {
    bool ok = true;

    wcont_mtx.lock();

    if (wcont.contains(path))
      wcont.erase(path);

    else
      ok = false;

    std::cout << "watch_ctl -> die -> '" << path << "' => "
              << (ok ? "true" : "false") << std::endl;

    wcont_mtx.unlock();

    callback(event::event{(ok ? "s/self/die@" : "e/self/die@") + path,
                          event::what::destroy, event::kind::watcher});

    return ok;
  };

  if (msg) {
    auto ok = live(path, callback) ? watch(path, callback, is_living) : false;
    std::cout << "watch -> adapter -> watch_ctl -> msg -> live -> '" << path
              << "' => " << (ok ? "true" : "false") << std::endl;
    return ok;
  } else {
    auto ok = die(path, callback);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms + 1));
    std::cout << "watch -> adapter -> watch_ctl -> msg -> die -> '" << path
              << "' => " << (ok ? "true" : "false") << std::endl;
    return ok;
  }
}

inline bool watch_ctl(char const* path, event::callback const& callback,
                      bool const msg) noexcept
{
  return watch_ctl(std::string(path), callback, msg);
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */

/* We need these down here because we forward-declare them above.
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
