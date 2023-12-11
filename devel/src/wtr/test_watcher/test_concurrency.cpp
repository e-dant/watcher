#include "snitch/snitch.hpp"
#include "test_watcher/event.hpp"
#include "test_watcher/test_watcher.hpp"
#include "wtr/watcher.hpp"
#include <iostream>
#include <tuple>
#include <utility>

/* Test that files are scanned */
TEST_CASE(
  "Concurrency",
  "[test][concurrent][file][dir][accuracy][not-perf]")
{
  using namespace wtr::watcher;
  using namespace wtr::test_watcher;

  check_event_lists_set_eq(watch_gather(
    "Concurrency",
    10,
    32));
};
