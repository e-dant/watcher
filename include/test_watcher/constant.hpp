#pragma once

#include <chrono>     /* milliseconds */
#include <filesystem> /* current_path, operator/ */

namespace wtr {
namespace test_watcher {

unsigned long constexpr static mk_events_options = 0x00000000;
unsigned long constexpr static mk_events_die_before = 0x00000001;
unsigned long constexpr static mk_events_die_after = 0x00000010;
unsigned long constexpr static mk_events_reverse = 0x00000100;

auto const concurrent_event_targets_concurrency_level = 2;

auto const test_store_path
    = std::filesystem::current_path() / "tmp_test_watcher";

} /* namespace test_watcher */
} /* namespace wtr */
