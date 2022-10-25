#pragma once

/*
  @brief watcher/event

  There are two things the user needs:
    - The `watch` function
    - The `event` object

  The `event` object is used to pass information about
  filesystem events to the (user-supplied) callback
  given to `watch`.

  The `event` object will contain the:
    - Path -- Which is always relative.
    - Path type -- one of:
      - File
      - Directory
      - Symbolic Link
      - Hard Link
      - Unknown
    - Event type -- one of:
      - Create
      - Modify
      - Destroy
      - OS-Specific Events
      - Unknown
    - Event time -- In nanoseconds since epoch

  Happy hacking.
*/

/*
  @brief watcher/event/types
  - water::watcher::event::kind
  - water::watcher::event::what
  - water::watcher::event::event
  - water::watcher::event::callback
*/

/* std::ostream */
#include <ostream>

/* std::chrono::system_clock::now,
   std::chrono::duration_cast,
   std::chrono::system_clock,
   std::chrono::nanoseconds,
   std::chrono::time_point */
#include <chrono>

namespace water {
namespace watcher {
namespace event {

/*
  @brief water/watcher/event/what

  A structure intended to represent
  what has happened to some path
  at the moment of some affecting event.
*/

enum class what {
  /* the essential happenings */
  rename,
  modify,
  create,
  destroy,

  /* extended happenings:
     path attributes */
  owner,

  /* catch-all */
  other,
};

/*
  @brief water/watcher/event/kind

  The essential path types
*/
enum class kind {
  /* the essential path types */
  dir,
  file,
  hard_link,
  sym_link,

  /* the special path types */
  watcher,

  /* catch-all */
  other,
};

struct event
{
  /*
    I like these names. Very human.
    'what happen'
    'event kind'
  */
  const char* where;
  const enum what what;
  const enum kind kind;
  const long long when;

  event(const char* where, const enum what happen, const enum kind kind)
      : where{where},

        what{happen},

        kind{kind},

        /* wow! thanks chrono! */
        when{std::chrono::duration_cast<std::chrono::nanoseconds>(
                 std::chrono::time_point<std::chrono::system_clock>{
                     std::chrono::system_clock::now()}
                     .time_since_epoch())
                 .count()}
  {}

  ~event() noexcept = default;

  /* @brief water/watcher/event/<<

     prints out where, what and kind.
     formats the output as a json object. */
  friend std::ostream& operator<<(std::ostream& os, const event& e)
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

/*
  @brief watcher/event/callback

  Ensure the adapters recieve events and return nothing.

  Maybe this should be inferred. I'm not sure.
*/
using callback = void (*)(const event&);

} /* namespace event */
} /* namespace watcher */
} /* namespace water   */
