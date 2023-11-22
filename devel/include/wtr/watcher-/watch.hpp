#pragma once

#include "wtr/watcher.hpp"
#include <atomic>
#include <filesystem>
#include <future>

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
  std::atomic<bool> is_living{true};
  std::future<bool> watching{};

public:
  inline watch(
    std::filesystem::path const& path,
    event::callback const& callback) noexcept
      : watching{std::async(
        std::launch::async,
        [this, path, callback]
        {
          auto ec = std::error_code{};
          auto abs_path = std::filesystem::absolute(path, ec);
          auto path_ok =
            ! ec && std::filesystem::is_directory(abs_path, ec) && ! ec;
          callback(
            {(path_ok ? "s/self/live@" : "e/self/live@") + abs_path.string(),
             event::effect_type::create,
             event::path_type::watcher});
          auto died_ok = ! path_ok ? true
                                   : ::detail::wtr::watcher::adapter::watch(
                                     abs_path,
                                     callback,
                                     this->is_living);
          callback(
            {(died_ok ? "s/self/die@" : "e/self/die@") + abs_path.string(),
             event::effect_type::destroy,
             event::path_type::watcher});
          return path_ok && died_ok;
        })}
  {}

  inline auto close() noexcept -> bool
  {
    this->is_living = false;
    return this->watching.valid() && this->watching.get();
  };

  inline ~watch() noexcept { this->close(); }
};

} /*  namespace watcher */
} /*  namespace wtr   */
