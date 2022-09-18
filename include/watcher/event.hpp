#pragma once

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

// /*
//   @brief water/watcher/event/kind
// 
//   The essential path types
// */
// enum class kind {
//   /* the essential path types */
//   path_is_dir,
//   path_is_file,
//   path_is_hard_link,
//   path_is_sym_link,
// 
//   /* catch-all */
//   other,
// };

struct event {
  const char* where{};
  const what what{};
  event(const char* where, const enum what happen) noexcept
      : where{where}, what{happen} {}
  ~event() noexcept = default;
};

}  // namespace event
}  // namespace watcher
}  // namespace water
