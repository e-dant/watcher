#pragma once

/* type: path */
#include <filesystem>
/* fn: watch_ctl */
#include <watcher/adapter/adapter.hpp>
/* type: callback */
#include <watcher/event.hpp>

namespace wtr {
namespace watcher {

/* @brief watcher/watch

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
     - The `die` function
     - The `watch` function
     - The `event` structure

   That's it.

   Happy hacking. */
inline bool watch(std::filesystem::path const& path, event::callback const& callback) noexcept
{
  return detail::adapter::watch_ctl(path, callback, true);
}

/* @brief watcher/die

   Stops the `watch`.
   Calls `callback`,
   then dies. */
inline bool die(
    std::filesystem::path const& path,
    event::callback const& callback = [](auto) -> void {}) noexcept
{
  return detail::adapter::watch_ctl(path, callback, false);
}

} /* namespace watcher */
} /* namespace wtr   */
