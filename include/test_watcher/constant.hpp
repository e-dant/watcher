#pragma once

#include <chrono>     /* milliseconds */
#include <filesystem> /* current_path, operator/ */

namespace wtr {
namespace test_watcher {

unsigned long constexpr static mk_events_options = 0x00000000;
unsigned long constexpr static mk_events_die_before = 0x00000001;
unsigned long constexpr static mk_events_die_after = 0x00000010;
unsigned long constexpr static mk_events_reverse = 0x00000100;

auto constexpr static prior_fs_events_clear_milliseconds
    = std::chrono::milliseconds(10);
auto constexpr static death_after_test_milliseconds
    = std::chrono::milliseconds(1000);
auto constexpr static path_count = 10;

auto const concurrent_event_targets_concurrency_level = 2;

auto const test_store_path
    = std::filesystem::current_path() / "tmp_test_watcher";

auto const concurrent_event_targets_store_path
    = test_store_path / "concurrent_event_targets_store";
auto const event_targets_store_path = test_store_path / "event_targets_store";
auto const regular_file_store_path = test_store_path / "regular_file_store";
auto const dir_store_path = test_store_path / "dir_store";

} /* namespace test_watcher */
} /* namespace wtr */
