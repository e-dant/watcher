/*
   Test Watcher
   Concurrent Event Targets
*/

/* REQUIRE,
   TEST_CASE */
#include <snitch/snitch.hpp>
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
  static constexpr auto concurrency_level = 32;
  static constexpr auto alive_for_ms = std::chrono::milliseconds(500);

  auto const store_path
      = wtr::test_watcher::test_store_path / "concurrent_event_targets_store_";

  auto [event_sent_list, event_recv_list] = wtr::test_watcher::watch_gather(
      "Concurrent Event Targets", store_path, path_count, concurrency_level,
      alive_for_ms);

  auto const max_i = event_sent_list.size() > event_recv_list.size()
                         ? event_recv_list.size()
                         : event_sent_list.size();
  for (size_t i = 0; i < max_i; ++i) {
    if (event_sent_list[i].kind != wtr::watcher::event::kind::watcher) {
      if (event_sent_list[i].where != event_recv_list[i].where)
        std::cout << "[ where ] [ " << i << " ] sent "
                  << event_sent_list[i].where << ", but received "
                  << event_recv_list[i].where << "\n";
      if (event_sent_list[i].what != event_recv_list[i].what)
        std::cout << "[ what ] [ " << i << " ] sent " << event_sent_list[i].what
                  << ", but received " << event_recv_list[i].what << "\n";
      if (event_sent_list[i].kind != event_recv_list[i].kind)
        std::cout << "[ kind ] [ " << i << " ] sent " << event_sent_list[i].kind
                  << ", but received " << event_recv_list[i].kind << "\n";
      REQUIRE(event_sent_list[i].where == event_recv_list[i].where);
      REQUIRE(event_sent_list[i].what == event_recv_list[i].what);
      REQUIRE(event_sent_list[i].kind == event_recv_list[i].kind);
    }
  }

  REQUIRE(event_sent_list.size() == event_recv_list.size());
};
