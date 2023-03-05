/*  cout,
    endl */
#include <fstream>
#include <iostream>
/*  vector */
#include <vector>
/*  REQUIRE,
    TEST_CASE */
#include <snitch/snitch.hpp>
/*  test_store_path */
#include <test_watcher/constant.hpp>
/*  watch,
    event */
#include <wtr/watcher.hpp>

using namespace wtr::test_watcher;

auto async_watch_test() -> bool
{
  // This decltype works for both oop and functional styles of the api
  using t = decltype(wtr::watch("", [](auto) {}));

  auto watchers = std::vector<t>{};

  auto ok = true;

  // Creation is non-blocking
  for (auto i = 0; i < 100; ++i)
    watchers.emplace_back(wtr::watch(test_store_path, [](auto) {}));

  // Event handling is non-blocking
  for (auto i = 0; i < 100; ++i)
    std::ofstream{test_store_path / std::to_string(i)};  // touch

  // Destruction is blocking (until they're closed)
  for (auto& w : watchers)
    if (! w.close()) ok = false;

  return ok;
};

//  50 watchers ->  0.711812 s
//  100 watchers ->  1.210355 s
//  150 watchers ->  1.836673 s
//  500 watchers ->  5.549466 s
//  750 watchers ->  8.044522 s
//  1000 watchers -> 10.669242 s
TEST_CASE("Bench Concurrent Watch Targets", "[bench_concurrent_watch_targets]")
{
  if (! std::filesystem::exists(test_store_path))
    std::filesystem::create_directory(test_store_path);

  auto ok = async_watch_test();

  if (std::filesystem::exists(test_store_path))
    std::filesystem::remove_all(test_store_path);

  REQUIRE(ok);
};
