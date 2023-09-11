#pragma once

#include "wtr/watcher.hpp"
#include <atomic>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>

namespace wtr {
inline namespace watcher {

/*  An asynchronous filesystem watcher.

    Begins watching when constructed.

    Stops watching when the object's lifetime ends
    or when `.close()` is called.

    Closing the watcher is the only blocking operation.

    @param path:
      The root path to watch for filesystem events.

    @param living_cb (optional):
      Something (such as a closure) to be called when events
      occur in the path being watched.

    This is an adaptor "switch" that chooses the ideal
   adaptor for the host platform.

    Every adapter monitors `path` for changes and invokes
   the `callback` with an `event` object when they occur.

    There are two things the user needs: `watch` and
   `event`.

    Typical use looks something like this:

    auto w = watch(".", [](event const& e) {
      std::cout
        << "path_name:   " << e.path_name   << "\n"
        << "path_type:   " << e.path_type   << "\n"
        << "effect_type: " << e.effect_type << "\n"
        << "effect_time: " << e.effect_time << "\n"
        << std::endl;
    };

    That's it.

    Happy hacking. */
class watch {
private:
  std::filesystem::path const path{};
  event::callback const callback{};
  std::atomic<bool> is_open{true};
  std::future<bool> future{std::async(
    std::launch::async,
    [this]
    {
      this->callback(
        {"s/self/live@" + this->path.string(),
         event::effect_type::create,
         event::path_type::watcher});
      return ::detail::wtr::watcher::adapter::watch(
        this->path,
        this->callback,
        this->is_open);
    })};

public:
  inline watch(
    std::filesystem::path const& path,
    event::callback const& callback) noexcept
      : path{path}
      , callback{callback}
  {}

  inline auto close() noexcept -> bool
  {
    if (this->is_open) {
      this->is_open = false;
      auto ok = this->future.get();
      this->callback(
        {(ok ? "s/self/die@" : "e/self/die@") + this->path.string(),
         event::effect_type::destroy,
         event::path_type::watcher});
      return ok;
    }
    else
      return false;
  };

  inline ~watch() noexcept { this->close(); }
};

} /*  namespace watcher */
} /*  namespace wtr   */
