#pragma once

/*  path */
#include <filesystem>
/*  async */
#include <future>
/*  function */
#include <functional>
/*  shared_ptr */
#include <memory>
/*  event
    callback
    adapter */
#include <watcher/watcher.hpp>

namespace wtr {
namespace watcher {

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

  auto msg = std::shared_ptr<message>{new message{}};

  auto lifetime = std::async(std::launch::async, [=]() noexcept -> bool {
                    return adapter(path, callback, msg);
                  }).share();

  return [=]() noexcept -> bool {
    return adapter(path, callback, msg) && lifetime.get();
  };
}

} /* namespace watcher */
} /* namespace wtr   */
