#pragma once

#include "wtr/watcher.hpp"
#include <filesystem>
#include <fstream>
#include <string>

namespace wtr {
namespace test_watcher {

inline auto create_regular_files(auto const& path, auto n)
{
  for (int i = 0; i < n; i++)
    std::ofstream{path + std::to_string(i) + std::string(".txt")};
}

inline auto create_regular_files(std::filesystem::path const& path, auto n)
{
  return create_regular_files(path.string(), n);
}

inline auto create_regular_files(std::vector<wtr::event>& events)
{
  for (auto& ev : events) std::ofstream{ev.path_name};
}

inline auto create_regular_files(auto const& paths)
{
  for (auto const& path : paths) std::ofstream{path};
}

inline auto create_directories(auto const& path, auto n)
{
  for (decltype(n) i = 0; i < n; i++)
    std::filesystem::create_directory(path / std::to_string(i));
}

} /* namespace test_watcher */
} /* namespace wtr */
