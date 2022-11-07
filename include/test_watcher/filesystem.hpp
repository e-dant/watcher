#pragma once

#include <filesystem> /* exists, remove_all */
#include <fstream>    /* ofstream */
#include <string>     /* string, to_string */
// #include <catch2/catch_test_macros.hpp> /* REQUIRE */

namespace wtr {
namespace test_watcher {

auto create_regular_files(auto path, auto n)
{
  using namespace std::filesystem;
  using std::ofstream;

  for (int i = 0; i < n; i++) {
    auto item = std::to_string(i) + std::string(".txt");

    ofstream{path / item};

    // REQUIRE(is_regular_file(item));
  }
}

auto remove_regular_files(auto path, auto n)
{
  using namespace std::filesystem;

  for (int i = 0; i < n; i++) {
    auto item = path / (std::to_string(i) + std::string(".txt"));

    if (exists(item)) remove_all(item);

    // REQUIRE(!exists(item));
  }
}

auto create_directories(auto path, auto n)
{
  using namespace std::filesystem;

  for (int i = 0; i < n; i++) {
    auto item = std::to_string(i);

    create_directory(path / item);

    // REQUIRE(is_directory(item));
  }
}

auto remove_directories(auto path, auto n)
{
  using namespace std::filesystem;

  for (int i = 0; i < n; i++) {
    auto item = path / std::to_string(i);

    if (exists(item)) remove_all(item);

    // REQUIRE(!exists(item));
  }
}

} /* namespace test_watcher */
} /* namespace wtr */