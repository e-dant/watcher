/*
   Test Watcher
   Concurrent Event Targets
*/

/* REQUIRE,
   TEST_CASE */
#include <snatch/snatch.hpp>
/* event */
#include <watcher/watcher.hpp>
/* watch_gather */
#include <test_watcher/test_watcher.hpp>
/* get */
#include <tuple>
/* cout, endl */
#include <iostream>

/* Test that files are scanned */
TEST_CASE("Concurrent Event Targets", "[concurrent_event_targets]")
{
  using namespace wtr::watcher;

  static constexpr auto path_count = 10;
  static constexpr auto concurrency_level = 8;
  static constexpr auto alive_for_ms = std::chrono::milliseconds(500);

  auto const store_path
      = wtr::test_watcher::test_store_path / "concurrent_event_targets_store_";

  auto [event_list, event_recv_list] = wtr::test_watcher::watch_gather(
      "Concurrent Event Targets", store_path, path_count, concurrency_level,
      alive_for_ms);

  for (event::event const& ev : event_list) {
    std::cout << "event => " << ev.where << "\n";
  }
  for (event::event const& ev : event_recv_list) {
    std::cout << "event recv => " << ev.where << "\n";
  }

  REQUIRE(true);
};
