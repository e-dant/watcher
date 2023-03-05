/*  cout,
    endl */
#include <iostream>
/*  vector */
#include <vector>
/*  REQUIRE,
    TEST_CASE */
#include <snitch/snitch.hpp>
/*  watch,
    event */
#include <wtr/watcher.hpp>

auto async_watch_test() -> bool
{
  // This decltype works for both oop and functional styles of the api
  using t = decltype(wtr::watch("", [](auto) {}));

  auto watchers = std::vector<t>{};

  auto ok = true;

  // Creation is blocking (until they're open)
  for (auto i = 0; i < 100; ++i)
    watchers.emplace_back(wtr::watch(".", [](auto) {}));

  // Non-blocking work would happen here

  // Destruction is blocking (until they're closed)
  for (auto& w : watchers)
    if (! w.close()) ok = false;

  return ok;
}

// oop-style    50 watchers ->  0.711812 s
// func-style   50 watchers ->  0.728276 s

// oop-style   100 watchers ->  1.210355 s
// func-style  100 watchers ->  1.163780 s

// oop-style   150 watchers ->  1.836673 s
// func-style  150 watchers ->  1.733675 s

// oop-style   500 watchers ->  5.549466 s
// func-style  500 watchers ->  5.325006 s

// oop-style   750 watchers ->  8.044522 s
// func-style  750 watchers ->  7.974121 s

// oop-style  1000 watchers -> 10.669242 s
// func-style 1000 watchers -> 10.643563 s
TEST_CASE("Bench Concurrent Watch Targets", "[bench_concurrent_watch_targets]")
{
  auto ok = async_watch_test();
  REQUIRE(ok);
};
