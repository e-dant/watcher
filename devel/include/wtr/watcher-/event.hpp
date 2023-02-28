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
     - Path, which is always absolute.
     - Type, one of:
       - dir
       - file
       - hard_link
       - sym_link
       - watcher
       - other
     - Event type, one of:
       - rename
       - modify
       - create
       - destroy
       - owner
       - other
     - Event time in nanoseconds since epoch

   The `watcher` type is special.
   Events with this type will include messages from
   the watcher. You may recieve error messages or
   important status updates.

   Happy hacking. */

/* std::basic_ostream */
#include <ios>

/* std::chrono::system_clock::now
   std::chrono::duration_cast
   std::chrono::system_clock
   std::chrono::nanoseconds
   std::chrono::time_point */
#include <chrono>

/* std::filesystem::path */
#include <filesystem>

/* std::function */
#include <functional>

namespace wtr {
inline namespace watcher {
namespace event {

namespace {
using std::function, std::chrono::duration_cast, std::chrono::nanoseconds,
  std::chrono::time_point, std::chrono::system_clock;
} /* namespace */

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

struct event {
  /* I like these names. Very human.
     'what happen'
     'event kind' */
  std::filesystem::path const where;
  enum what const what;
  enum kind const kind;
  long long const when{
    duration_cast<nanoseconds>(
      time_point<system_clock>{system_clock::now()}.time_since_epoch())
      .count()};

  event(std::filesystem::path const where,
        enum what const what,
        enum kind const kind) noexcept
      : where{where},
        what{what},
        kind{kind} {};

  ~event() noexcept = default;
};

/* @brief wtr/watcher/event/<<
   Streams out a `what` value. */
template<class Char, class CharTraits>
inline std::ostream& operator<<(std::basic_ostream<Char, CharTraits>& os,
                                enum what const& w) noexcept
{
  /* clang-format off */
  switch (w) {
    case what::rename  : return os << "\"rename\"";
    case what::modify  : return os << "\"modify\"";
    case what::create  : return os << "\"create\"";
    case what::destroy : return os << "\"destroy\"";
    case what::owner   : return os << "\"owner\"";
    case what::other   : return os << "\"other\"";
    default            : return os << "\"other\"";
  }
  /* clang-format on */
}

/* @brief wtr/watcher/event/<<
   Streams out a `kind` value. */
template<class Char, class CharTraits>
inline std::ostream& operator<<(std::basic_ostream<Char, CharTraits>& os,
                                enum kind const& k) noexcept
{
  /* clang-format off */
  switch (k) {
    case kind::dir       : return os << "\"dir\"";
    case kind::file      : return os << "\"file\"";
    case kind::hard_link : return os << "\"hard_link\"";
    case kind::sym_link  : return os << "\"sym_link\"";
    case kind::watcher   : return os << "\"watcher\"";
    case kind::other     : return os << "\"other\"";
    default              : return os << "\"other\"";
  }
  /* clang-format on */
}

/* @brief wtr/watcher/event/<<
   Streams out `where`, `what` and `kind`.
   Formats the stream as a json object. */
template<class Char, class CharTraits>
inline std::ostream& operator<<(std::basic_ostream<Char, CharTraits>& os,
                                event const& ev) noexcept
{
  /* clang-format off */
    return os << R"(")" << ev.when << R"(":)"
              << "{" << R"("where":)" << ev.where << R"(,)"
                     << R"("what":)"  << ev.what  << R"(,)"
                     << R"("kind":)"  << ev.kind  << "}";
  /* clang-format on */
}

/* @brief wtr/watcher/event/==
   Compares event objects for equivalent
   `where`, `what` and `kind` values. */
inline bool operator==(event const& lhs, event const& rhs) noexcept
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
inline bool operator!=(event const& lhs, event const& rhs) noexcept
{
  return ! (lhs == rhs);
};

/* @brief watcher/event/callback
   Ensure the adapters can recieve events
   and will return nothing. */
using callback = function<void(event const&)>;

} /* namespace event */
} /* namespace watcher */
} /* namespace wtr   */
