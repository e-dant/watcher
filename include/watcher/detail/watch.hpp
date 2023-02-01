#pragma once

/*  path */
#include <filesystem>
/*  async */
#include <future>
/*  function */
#include <functional>
/*  event
    callback
    adapter */
#include <watcher/watcher.hpp>

namespace wtr {
namespace watcher {

/* clang-format off */

/*  @brief wtr/watcher/watch

    @param path:
      The root path to watch for filesystem events.

    @param living_cb (optional):
      Something (such as a closure) to be called when events
      occur in the path being watched.

    This is an adaptor "switch" that chooses the ideal adaptor
    for the host platform.

    Every adapter monitors `path` for changes and invokes the
    `callback` with an `event` object when they occur.

    There are two things the user needs:
      - The `watch` function
      - The `event` object

    The watch function returns a function to stop its watcher.

    Typical use looks like this:
      auto lifetime = watch(".", [](event& e) {
        std::cout
          << "where: " << e.where << "\n"
          << "kind: "  << e.kind  << "\n"
          << "what: "  << e.what  << "\n"
          << "when: "  << e.when  << "\n"
          << std::endl;
      };

      auto dead = lifetime();

    That's it.

    Happy hacking. */
inline auto watch(std::filesystem::path const& path,
                  event::callback const& callback) noexcept
    -> std::function<bool()>
{
  using namespace ::wtr::watcher::detail::adapter;

  return

      [ =,
        /* Begin our lifetime in an asynchronous context */
        lifetime = std::async(std::launch::async, [=]() noexcept -> bool
                      { return adapter(path, callback, message::live); }).share()
      
      ]() noexcept -> bool {
        /* Return a function that will stop us when called */
        return adapter(path, callback, message::die)
            && lifetime.get();
      };
}

/* clang-format on */

} /* namespace watcher */
} /* namespace wtr   */
