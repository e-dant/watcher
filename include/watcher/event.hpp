#pragma once

namespace water {

namespace watcher {

// enum class status { created, modified, erased };

struct event {
  const char* where{};
  const enum class what {
    other,
    attr_owner,
    attr_other,
    path_rename,
    path_modify,
    path_create,
    path_destroy,
    path_other,
    path_is_dir,
    path_is_file,
    path_is_hard_link,
    path_is_sym_link,
  } what{};
  event(const char* path, const enum what happen) noexcept
      : where{path}, what{happen} {}
  ~event() noexcept = default;
};

}  // namespace watcher

}  // namespace water
