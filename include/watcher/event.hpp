#pragma once

namespace water {
namespace watcher {
namespace event {

enum class what {
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
};

struct event {
  const char* where{};
  const what what{};
  event(const char* path, const enum what happen) noexcept
      : where{path}, what{happen} {}
  ~event() noexcept = default;
};

}  // namespace event
}  // namespace watcher
}  // namespace water
