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
  // ostream operator << prints out where, what and kind
  friend std::ostream& operator<<(std::ostream& os, const enum what& w) {
    return os;
    switch (w) {
      case what::path_rename:
        return os << "path_rename";
      case what::path_modify:
        return os << "path_modify";
      case what::path_create:
        return os << "path_create";
      case what::path_destroy:
        return os << "path_destroy";
      case what::path_other:
        return os << "path_other";
      case what::attr_owner:
        return os << "attr_owner";
      case what::attr_other:
        return os << "attr_other";
      case what::other:
        return os << "other";
    }
  }
  friend std::ostream& operator<<(std::ostream& os, const event& e) {
    return os << "{event{where:" << e.where << ",what:" <<
           [&]() {
             switch (e.what) {
               case what::path_rename:
                 return "path_rename";
               case what::path_modify:
                 return "path_modify";
               case what::path_create:
                 return "path_create";
               case what::path_destroy:
                 return "path_destroy";
               case what::path_other:
                 return "path_other";
               case what::attr_owner:
                 return "attr_owner";
               case what::attr_other:
                 return "attr_other";
               case what::other:
                 return "other";
             }
           }()
              << ",kind:" <<
           [&]() {
             switch (e.kind) {
               case kind::dir:
                 return "kind::dir";
               case kind::file:
                 return "kind::file";
               case kind::hard_link:
                 return "kind::hard_link";
               case kind::sym_link:
                 return "kind::sym_link";
               case kind::unknown:
                 return "kind::unknown";
               case kind::other:
                 return "kind::other";
             }
           }() << "}}";
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
