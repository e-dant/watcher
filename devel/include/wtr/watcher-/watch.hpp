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
#include <wtr/watcher.hpp>

namespace wtr {
namespace watcher {

/*  Contains a way to stop an instance of `watch()`.
    This is the structure that we return from there.
    It is intended to allow the `watch(args).close()`
    syntax as well as an anonymous `watch(args)()`.
    We define it up here so that editors can suggest
    and complete the `.close()` function. */
struct _ {
  std::function<bool()> close;

  bool operator()() const noexcept { return close(); }
};

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

    auto w = watch(".", [](event const& e) {
      std::cout
        << "where: " << e.where << "\n"
        << "kind: "  << e.kind  << "\n"
        << "what: "  << e.what  << "\n"
        << "when: "  << e.when  << "\n"
        << std::endl;
    };
    auto dead = w.close(); // w() also works

    That's it.

    Happy hacking. */

[[nodiscard("Returns a way to stop this watcher, for example: auto w = "
            "watch(p, cb) ; w.close() // or w();")]]

inline _
watch(std::filesystem::path const& path,
      event::callback const& callback) noexcept
{
  using namespace ::detail::wtr::watcher::adapter;

  /*  A message, unique to this watcher.
      Shared between this scope and the adapter.
      Think of it like a cookie. */
  auto msg = std::make_shared<message>();

  /*  Start and run the watcher asynchronously.
      Every watcher has a unique lifetime. */
  auto lifetime =
    std::async(std::launch::async,
               [=]() noexcept -> bool { return adapter(path, callback, msg); })
      .share();

  /*  Provides the user with a way to stop the watcher.
      The `close()` function is unique to every watcher.
      A watcher that doesn't exist or isn't "owned"
      can't be closed. That's important.
      The structure that we return is intended to allow
      the `watch(args).close()` syntax as well as an
      anonymous `watch(args)()`. Overloading the `()`
      operator allows the first syntax. */
  return _{.close = [=]() noexcept -> bool
           { return adapter(path, callback, msg) && lifetime.get(); }};
}

} /* namespace watcher */
} /* namespace wtr   */
