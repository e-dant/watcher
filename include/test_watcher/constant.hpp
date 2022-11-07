#pragma once

#include <chrono>     /* milliseconds */
#include <filesystem> /* current_path, operator/ */

namespace wtr {
namespace test_watcher {

auto constexpr static time_until_prior_fs_events_clear
    = std::chrono::milliseconds(10);
auto constexpr static time_until_death_after_test
    = std::chrono::milliseconds(10);
auto constexpr static path_count = 100000;
auto const test_store_path
    = std::filesystem::current_path() / "tmp_test_watcher";
auto const regular_file_store_path = test_store_path / "regular_file_store";
auto const dir_store_path = test_store_path / "dir_store";

} /* namespace test_watcher */
} /* namespace wtr */