#pragma once

/* path */
#include <filesystem>
/* numeric_limits */
#include <limits>
/* mutex
   scoped_lock */
#include <mutex>
/* string */
#include <string>
/* unordered_map */
#include <unordered_map>
/* watch
   event
   callback */
#include <watcher/watcher.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {

namespace {
inline constexpr size_t size_t_max = std::numeric_limits<size_t>::max() - 1;
} /* namespace */

/* @brief wtr/watcher/detail/adapter/message
   We know two messages:
     - Tell a watcher to live
     - Tell a watcher to die */
enum class message { live, die };

/* @brief wtr/watcher/detail/adapter/adapter

   @param path:
     The root path to watch for filesystem events.

   @param callback (optional):
     Something (such as a closure) to be called when events
     occur in the path being watched.

   @param message:
     The message, either `live` or `die`, makes this
     function play a bit like member functions in a class.
     We want to hold (static) state in this function,
     so breaking `live` and `die` into two functions would
     be a bit ugly. */
inline bool adapter(std::filesystem::path const& path,
                    event::callback const& callback, message const msg) noexcept
{
  using evw = ::wtr::watcher::event::what;
  using evk = ::wtr::watcher::event::kind;

  /* @brief
     This (hash) map is used to give the watchers unique
     lifetimes on non-unique paths.

     The keys are (unique) paths.

     Paths are associated with the count of watchers alive
     on that path.

     The count is conceptually similar to a FIFO queue of
     living watchers.

     We can't have more than `numeric_limits<size_t>::max()`
     watchers on the same path. That number is large.
     18,446,744,073,709,551,615 on many platforms. */
  static auto lifetimes{std::unordered_map<std::string, size_t>{}};

  /* @brief
     A mutex to synchronize access to the container. */
  static auto lifetimes_mtx{std::mutex{}};

  /* @brief
     Some path as a string. We need the path as a string to
     store it as the key in a map because `path` doesn't
     know how to hash, but `string` does. */
  auto const& path_str = path.string();

  /* @brief
     If a path and a count exist for some path, then we
     increment the count.

     If a path and a count do not exist, we create them.

     A path cannot exist without a count, and all counts
     must be positive. `die()` must guarantee that.

     Returns a functor to check if we're still living.

     The functor is unique to every watcher on `path`. */
  auto const& live
      = [&path_str, &callback]() noexcept -> std::function<bool()> {
    auto _ = std::scoped_lock{lifetimes_mtx};

    auto const maybe_node = lifetimes.find(path_str);

    size_t const position
        = maybe_node != lifetimes.end() ? maybe_node->second + 1 : 1;

    if (position < size_t_max) [[likely]] {
      lifetimes.insert_or_assign(path_str, position);

      callback({"s/self/live@" + path_str, evw::create, evk::watcher});

      /* @brief
         Creates a predicate functor for a watcher to check
         whether or not it's alive. The functor returned is
         only valid for the most recently created watcher.

         The returned function evaluates to true if the count
         on a path is greater than or equal to our position
         in the count. We keep track of a watcher's position
         by copying the count into this function's state at
         time it's created. (That's why this is only valid
         for the most recently created watcher.)

         This only works if a position in the count uniquely
         represents a watcher's creation at a moment in time.
         Which it does: the count acts like a FIFO queue with
         no values, and the count is imbued in the predicate
         functor we return.

         The watchers should die safely and promptly the first
         time they get call this functor and it's false. */
      return [path_str, position]() noexcept -> bool {
        auto _ = std::scoped_lock{lifetimes_mtx};

        auto const maybe_node = lifetimes.find(path_str);

        return maybe_node != lifetimes.end() ? maybe_node->second >= position
                                             : false;
      };

    } else {
      callback({"e/self/live/too_many@" + path_str, evw::create, evk::watcher});

      return []() constexpr noexcept -> bool { return false; };
    }
  };

  /* @brief
     The watchers on a path are stopped in the same order
     that they were created.

     If a path has a watch count, we decrement it. The
     watchers should observe this and die if the count is
     less than what they were created with.

     If the count would be zero if we decremented it, then
     we remove the whole path. (`live()` expects there to
     be a path and a positive count or nothing.)

     Returns false if `path` is not being watched. */
  auto const& die = [&path_str, &callback]() noexcept -> bool {
    auto _ = std::scoped_lock{lifetimes_mtx};

    auto const maybe_node = lifetimes.find(path_str);

    if (maybe_node != lifetimes.end()) [[likely]] {
      if (maybe_node->second > 1)
        maybe_node->second -= 1;

      else
        lifetimes.erase(maybe_node);

      callback({"s/self/die@" + path_str, evw::destroy, evk::watcher});

      return true;

    } else {
      callback(
          {"e/self/die/not_alive@" + path_str, evw::destroy, evk::watcher});

      return false;
    }
  };

  switch (msg) {
    case message::live: return watch(path, callback, live());

    case message::die: return die();

    default: return false;
  }
}

} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */
