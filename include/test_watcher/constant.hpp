#pragma once

#include <chrono>     /* milliseconds */
#include <filesystem> /* current_path, operator/ */

namespace wtr {
namespace test_watcher {

inline constexpr unsigned long mk_events_options = 0x00000000;
inline constexpr unsigned long mk_events_die_before = 0x00000001;
inline constexpr unsigned long mk_events_die_after = 0x00000010;
inline constexpr unsigned long mk_events_reverse = 0x00000100;

inline constexpr auto concurrent_event_targets_concurrency_level = 2;

inline auto const test_store_path
    = std::filesystem::current_path() / "tmp_test_watcher";

} /* namespace test_watcher */
} /* namespace wtr */
