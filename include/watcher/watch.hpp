#pragma once

#include <watcher/adapter/adapter.hpp>
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
static bool watch(const char* path, event::callback const& living_cb)
{
  return detail::adapter::make_living()
             ? detail::adapter::watch(path, living_cb)
             : false;
}

/* @brief watcher/die

   Stops the `watch`.
   Destroys itself. */
static bool die()
{
  using whatever = const event::event&;
  return detail::adapter::die([](whatever) -> void {});
}

/* @brief watcher/die

   Stops the `watch`.
   Calls `callback`,
   then destroys itself. */
static bool die(event::callback const& callback)
{
  return detail::adapter::die(callback);
}

} /* namespace watcher */
} /* namespace wtr   */
