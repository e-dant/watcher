#pragma once

/*  path */
#include <filesystem>
/*  function */
#include <functional>
/*  is_*,
    invoke_result */
#include <type_traits>
/*  event
    callback
    adapter */
#include <wtr/watcher.hpp>

namespace wtr {
inline namespace watcher {

/*  Contains a way to stop an instance of `watch()`.
    This is the structure that we return from there.
    It is intended to allow the `watch(args).close()`
    syntax as well as an anonymous `watch(args)()`.
    We define it up here so that editors can suggest
    and complete the `.close()` function. Also because
    we can't template a type inside a function. */

template<class Fn>
requires(std::is_nothrow_invocable_v<Fn>
         and std::is_same_v<std::invoke_result_t<Fn>, bool>)
struct _ {
  Fn const close{};

  inline constexpr auto operator()() const noexcept -> bool { return this->close(); };

  inline constexpr _(Fn&& fn) noexcept
      : close{std::forward<Fn>(fn)} {};

  inline constexpr ~_() = default;
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

[[nodiscard("Returns a way to stop this watcher, for example: "
            "auto w = watch(p, cb) ; w.close() // or w();")]]

inline auto
watch(std::filesystem::path const& path,
      event::callback const& callback) noexcept
{
  using namespace ::detail::wtr::watcher::adapter;

  return _{[adapter{open(path, callback)}]() noexcept -> bool
           { return close(adapter); }};
};

} /* namespace watcher */
} /* namespace wtr   */
