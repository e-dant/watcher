#pragma once

#include <filesystem>          /* exists, remove_all */
#include <fstream>             /* ofstream */
#include <string>              /* string, to_string */
#include <watcher/watcher.hpp> /* event */
// #include <catch2/catch_test_macros.hpp> /* REQUIRE */

namespace wtr {
namespace test_watcher {

inline auto create_regular_files(auto path, auto n)
{
  using namespace std::filesystem;
  using std::ofstream, std::to_string, std::string;

  for (int i = 0; i < n; i++) {
    ofstream(path + to_string(i) + string(".txt"));
    // REQUIRE(is_regular_file(item));
  }
}

inline auto create_regular_files(std::filesystem::path path, auto n)
{
  using std::string;
  return create_regular_files(string(path), n);
}

inline auto create_regular_files(std::vector<wtr::watcher::event::event>& events)
{
  using std::ofstream;
  for (auto& ev : events) ofstream{ev.where};
}

inline auto create_regular_files(auto& paths)
{
  using std::ofstream;
  for (auto& path : paths) ofstream{path};
}

inline auto create_directories(auto path, auto n)
{
  using namespace std::filesystem;
  using std::to_string;

  for (int i = 0; i < n; i++) {
    auto item = to_string(i);

    create_directory(path / item);

    // REQUIRE(is_directory(item));
  }
}

} /* namespace test_watcher */
} /* namespace wtr */