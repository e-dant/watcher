#pragma once

#include <chrono>
#include <filesystem>

/*
  @brief watcher/event

  There are two things the user needs:
    - The `run` function
    - The `event` structure

  The `event` structure is used for passing information
  about filesystem events to the (user-supplied) callback
  available to `run`.

  Happy hacking.
*/

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

  /* catch-all */
  other,
};

struct event {
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
                 .count()} {}

  ~event() noexcept = default;

  /* @brief water/watcher/event/<<

     prints out where, what and kind.
     formats the output as a json object. */

  /* @note water/watcher/event/<<

     the only way to get this object is through one of the `run`s.
     If that were not the case, the time would not be correct,
     and this would need to change. */
  friend std::ostream& operator<<(std::ostream& os, const event& e) {
    /* clang-format off */
    const auto what_repr = [&]() {
      switch (e.what) {
        case what::rename:  return "rename";
        case what::modify:  return "modify";
        case what::create:  return "create";
        case what::destroy: return "destroy";
        case what::owner:   return "owner";
        case what::other:   return "other";
      }
    }();

    const auto kind_repr = [&]() {
      switch (e.kind) {
        case kind::dir:       return "dir";
        case kind::file:      return "file";
        case kind::hard_link: return "hard_link";
        case kind::sym_link:  return "sym_link";
        case kind::other:     return "other";
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

} /* namespace event */
} /* namespace watcher */
} /* namespace water   */
