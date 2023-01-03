#pragma once

/* path */
#include <filesystem>
/* callback
   event */
#include <watcher/event.hpp>
/* adapter */
#include <watcher/adapter/adapter.hpp>

namespace wtr {
namespace watcher {

/* @brief wtr/watcher/watch

   @param path:
     The root path to watch for filesystem events.

   @param living_cb (optional):
     Something (such as a closure) to be called when events
     occur in the path being watched.

   This is an adaptor "switch" that chooses the ideal adaptor
   for the host platform.

   Every adapter monitors `path` for changes and invokes the
   `callback` with an `event` object when they occur.

   There are three things the user needs:
     - The `die` function
     - The `watch` function
     - The `event` structure

   That's it.

   Happy hacking. */
inline bool watch(std::filesystem::path const& path,
                  event::callback const& callback) noexcept
{
  return detail::adapter::adapter(path, callback, true);
}

/* @brief wtr/watcher/die

   Stops a watcher at `path`.
   Calls `callback` with status messages.
   True if newly dead. */
inline bool die(
    std::filesystem::path const& path,
    event::callback const& callback = [](event::event) -> void {}) noexcept
{
  return detail::adapter::adapter(path, callback, false);
}

} /* namespace watcher */
} /* namespace wtr   */
