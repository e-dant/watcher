#pragma once

#include <chrono>     /* milliseconds */
#include <filesystem> /* current_path, operator/ */

namespace wtr {
namespace test_watcher {

auto constexpr static prior_fs_events_clear_milliseconds
    = std::chrono::milliseconds(10);
auto constexpr static death_after_test_milliseconds
    = std::chrono::milliseconds(1000);
auto constexpr static path_count = 10;
auto const test_store_path
    = std::filesystem::current_path() / "tmp_test_watcher";
auto const event_targets_store_path = test_store_path / "event_targets_store";
auto const regular_file_store_path = test_store_path / "regular_file_store";
auto const dir_store_path = test_store_path / "dir_store";

} /* namespace test_watcher */
} /* namespace wtr */
