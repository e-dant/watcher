#pragma once

/* exists,
   remove_all,
   is_regular_file */
#include <filesystem>
/* ofstream */
#include <fstream>
/* string,
   to_string */
#include <string>
/* event */
#include <watcher/watcher.hpp>

namespace wtr {
namespace test_watcher {

inline auto create_regular_files(auto path, auto n) {
  for (int i = 0; i < n; i++)
    std::ofstream{path + std::to_string(i) + std::string(".txt")};
}

inline auto create_regular_files(std::filesystem::path path, auto n) {
  return create_regular_files(path.string(), n);
}

inline auto
create_regular_files(std::vector<wtr::watcher::event::event>& events) {
  for (auto& ev : events) std::ofstream{ev.where};
}

inline auto create_regular_files(auto& paths) {
  for (auto& path : paths) std::ofstream{path};
}

inline auto create_directories(auto path, auto n) {
  for (int i = 0; i < n; i++)
    std::filesystem::create_directory(path / std::to_string(i));
}

} /* namespace test_watcher */
} /* namespace wtr */