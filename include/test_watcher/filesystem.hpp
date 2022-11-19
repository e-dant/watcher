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

inline auto create_regular_files(auto path, auto n)
{
  using namespace std::filesystem;
  using std::ofstream, std::to_string, std::string;

  for (int i = 0; i < n; i++) {
    ofstream{path + to_string(i) + string(".txt")};
    // REQUIRE(is_regular_file(item));
  }
}

inline auto create_regular_files(std::filesystem::path path, auto n)
{
  using std::string;
  return create_regular_files(path.string(), n);
}

inline auto create_regular_files(
    std::vector<wtr::watcher::event::event>& events)
{
  for (auto& ev : events) std::ofstream{ev.where};
}

inline auto create_regular_files(auto& paths)
{
  for (auto& path : paths) std::ofstream{path};
}

inline auto create_directories(auto path, auto n)
{
  using namespace std::filesystem;

  for (int i = 0; i < n; i++) {
    auto item = path / std::to_string(i);
    create_directory(item);
    REQUIRE(is_directory(item));
  }
}

} /* namespace test_watcher */
} /* namespace wtr */