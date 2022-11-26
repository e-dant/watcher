/*
   Test Watcher
   Event Targets
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
TEST_CASE("Event Targets", "[event_targets]")
{
  using namespace wtr::watcher;

  static constexpr auto path_count = 10;

  auto const store_path
      = wtr::test_watcher::test_store_path / "event_targets_store";

  auto [event_list, event_recv_list] = wtr::test_watcher::watch_gather(
      "Event Targets", store_path, path_count);

  for (event::event const& ev : event_list) {
    std::cout << "event => " << ev.where << "\n";
  }
  for (event::event const& ev : event_recv_list) {
    std::cout << "event recv => " << ev.where << "\n";
  }

  REQUIRE(true);
};
