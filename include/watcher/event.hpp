#pragma once

#include <chrono>
#include <filesystem>

/*
  @brief watcher/event

  A structure for passing around event information.
  Intended to be passed to callback provided to `run`.
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

  event(const char* where, const enum what happen) noexcept
      : where{where}, what{happen}, kind{[&]() {
          using std::filesystem::is_regular_file, std::filesystem::is_directory,
              std::filesystem::is_symlink, std::filesystem::exists;
          /*
            There is only enough room in this world
            for two control flow operators:

            1. `if constexpr`
            2. `ternary if`

            I should write a proposal for the
            `constexpr ternary if`.
          */
          return

              exists(where)

                  ? is_regular_file(where)

                        ? kind::file

                        : is_directory(where)

                              ? kind::dir

                              : is_symlink(where)

                                    ? kind::sym_link

                                    : kind::other

                  : kind::other;
        }()} {}
  ~event() noexcept = default;

  /* @brief water/watcher/event/<<

     prints out where, what and kind.
     formats the output as a json object. */

  /* @note water/watcher/event/<<

     the only way to get this object is through one of the `run`s.
     If that were not the case, the time would not be correct,
     and this would need to change. */
  friend std::ostream& operator<<(std::ostream& os, const event& e) {
    /* wow! thanks chrono! */
    const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::time_point<std::chrono::system_clock>{
                             std::chrono::system_clock::now()}
                             .time_since_epoch())
                         .count();

    const auto w = [&]() {
      switch (e.what) {
        case what::rename:
          return "rename";
        case what::modify:
          return "modify";
        case what::create:
          return "create";
        case what::destroy:
          return "destroy";
        case what::owner:
          return "owner";
        case what::other:
          return "other";
      }
    }();

    const auto k = [&]() {
      switch (e.kind) {
        case kind::dir:
          return "dir";
        case kind::file:
          return "file";
        case kind::hard_link:
          return "hard_link";
        case kind::sym_link:
          return "sym_link";
        case kind::other:
          return "other";
      }
    }();

    return os

           << "\"event" << now

           << "\":{\"where\":\"" << e.where

           << "\",\"what\":\"" << w

           << "\",\"kind\":\"" << k

           << "\"}";
  }
};

}  // namespace event
namespace literal {
using                              // NOLINT
    water::watcher::event::kind,   // NOLINT
    water::watcher::event::what,   // NOLINT
    water::watcher::event::event;  // NOLINT
}  // namespace literal
}  // namespace watcher
}  // namespace water
