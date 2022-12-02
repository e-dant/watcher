#pragma once

/* @brief watcher/event
   There are only three things the user needs:
    - The `die` function
    - The `watch` function
    - The `event` object

   The `event` object is used to pass information about
   filesystem events to the (user-supplied) callback
   given to `watch`.

   The `event` object will contain the:
     - Path -- Which is always relative.
     - Type -- one of:
       - dir
       - file
       - hard_link
       - sym_link
       - watcher
       - other
     - Event type -- one of:
       - rename
       - modify
       - create
       - destroy
       - owner
       - other
     - Event time -- In nanoseconds since epoch

   The `watcher` type is special.
   Events with this type will include messages from
   the watcher. You may recieve error messages or
   important status updates.

   Happy hacking. */

/* - std::ostream */
#include <ostream>

/* - std::string */
#include <string>

/* - std::chrono::system_clock::now,
   - std::chrono::duration_cast,
   - std::chrono::system_clock,
   - std::chrono::nanoseconds,
   - std::chrono::time_point */
#include <chrono>

/* - std::filesystem::path */
#include <filesystem>

/* - std::function */
#include <functional>

namespace wtr {
namespace watcher {
namespace event {

namespace {
using std::string, std::function, std::chrono::duration_cast,
    std::chrono::nanoseconds, std::chrono::time_point,
    std::chrono::system_clock;
}

/* @brief watcher/event/types
   - wtr::watcher::event
   - wtr::watcher::event::kind
   - wtr::watcher::event::what
   - wtr::watcher::event::callback */

/* @brief wtr/watcher/event/what
   A structure intended to represent
   what has happened to some path
   at the moment of some affecting event. */
enum class what {
  /* The essentials */
  rename,
  modify,
  create,
  destroy,

  /* The attributes */
  owner,

  /* Catch-all */
  other,
};

/* @brief wtr/watcher/event/kind
   The essential kinds of paths. */
enum class kind {
  /* The essentials */
  dir,
  file,
  hard_link,
  sym_link,

  /* The specials */
  watcher,

  /* Catch-all */
  other,
};

struct event
{
  /* I like these names. Very human.
     'what happen'
     'event kind' */
  const string where;
  const enum what what;
  const enum kind kind;
  const long long when{
      duration_cast<nanoseconds>(
          time_point<system_clock>{system_clock::now()}.time_since_epoch())
          .count()};

  event(char const* where, enum what const what, enum kind const kind) noexcept
      : where{string{where}}, what{what}, kind{kind} {};

  event(std::filesystem::path const where, enum what const what,
        enum kind const kind) noexcept
      : where{where.string()}, what{what}, kind{kind} {};

  event(string const where, enum what const what, enum kind const kind) noexcept
      : where{where}, what{what}, kind{kind} {};

  ~event() noexcept = default;

  /* @brief wtr/watcher/event/==
     Compares event objects for equivalent
     `where`, `what` and `kind` values. */
  friend bool operator==(event const& lhs, event const& rhs) noexcept
  {
    /* True if */
    return
        /* The path */
        lhs.where == rhs.where
        /* And what happened */
        && lhs.what == rhs.what
        /* And the kind of path */
        && lhs.kind == rhs.kind
        /* And the time */
        && lhs.when == rhs.when;
    /* Are the same. */
  };

  /* @brief wtr/watcher/event/!=
     Not == */
  friend bool operator!=(event const& lhs, event const& rhs) noexcept
  {
    return !(lhs == rhs);
  };

  /* @brief wtr/watcher/event/<<
     prints out where, what and kind.
     formats the output as a json object. */
  friend std::ostream& operator<<(std::ostream& os, const event& e) noexcept
  {
    /* clang-format off */
    auto const what_repr = [&]() {
      switch (e.what) {
        case what::rename:  return "rename";
        case what::modify:  return "modify";
        case what::create:  return "create";
        case what::destroy: return "destroy";
        case what::owner:   return "owner";
        case what::other:   return "other";
        default:            return "other";
      }
    }();

    auto const kind_repr = [&]() {
      switch (e.kind) {
        case kind::dir:       return "dir";
        case kind::file:      return "file";
        case kind::hard_link: return "hard_link";
        case kind::sym_link:  return "sym_link";
        case kind::watcher:   return "watcher";
        case kind::other:     return "other";
        default:              return "other";
      }
    }();

    return os << R"(")" << e.when << R"(":)"
              << "{"
                  << R"("where":")" << e.where   << R"(",)"
                  << R"("what":")"  << what_repr << R"(",)"
                  << R"("kind":")"  << kind_repr << R"(")"
              << "}";
    /* clang-format on */
  }
};

/* @brief watcher/event/callback
   Ensure the adapters can recieve events
   and will return nothing. */
using callback = function<void(event const&)>;

} /* namespace event */
} /* namespace watcher */
} /* namespace wtr   */
