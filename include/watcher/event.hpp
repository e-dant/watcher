#pragma once

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
  path_rename,
  path_modify,
  path_create,
  path_destroy,
  path_other,

  /* extended happenings:
     path attributes */
  attr_owner,
  attr_other,

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
  unknown,

  /* catch-all */
  other,
};

struct event {
  const char* where;
  const enum what what;
  const enum kind kind;
  event(const char* where, const enum what happen) noexcept
      : where{where}, what{happen}, kind{[&]() {
          using std::filesystem::is_regular_file, std::filesystem::is_directory,
              std::filesystem::is_symlink, std::filesystem::exists;
          return
              // exists?
              exists(where)
                  // file?
                  ? is_regular_file(where)
                        // -> kind/file
                        ? kind::file
                        // dir?
                        : is_directory(where)
                              // -> kind/dir
                              ? kind::dir
                              // sym link?
                              : is_symlink(where)
                                    // kind/sym_link
                                    ? kind::sym_link
                                    // default -> kind/unknown
                                    : kind::unknown
                  : kind::unknown;
        }()} {}
  ~event() noexcept = default;
};

namespace literal {
using                              // NOLINT
    water::watcher::event::what,   // NOLINT
    water::watcher::event::event;  // NOLINT
}

}  // namespace event
}  // namespace watcher
}  // namespace water
