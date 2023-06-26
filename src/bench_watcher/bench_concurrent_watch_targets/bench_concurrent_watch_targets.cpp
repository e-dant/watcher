/*  nanoseconds,
    system_clock,
    duration_cast */
#include <algorithm>
#include <tuple>
#include <chrono>
/*  ofstream */
#include <fstream>
/*  printf */
#include <cstdio>
/*  unique_ptr */
#include <memory>
/*  array */
#include <array>
/*  vector */
#include <vector>
/*  REQUIRE,
    TEST_CASE */
#include <snitch/snitch.hpp>
/*  test_store_path */
#include <test_watcher/constant.hpp>
/*  watch,
    event */
#include <utility>
#include <wtr/watcher.hpp>

// clang-format off

namespace ti = ::std::chrono;

struct BenchCfg {
  int watcher_count;
  int event_count;
};

struct BenchResult {
  struct BenchCfg cfg;
  ti::nanoseconds time_taken;
  unsigned nth;
};

struct Range {
  int start{1};
  int stop{1};
  int step{1};
};

struct RangePair {
  Range watcher_range;
  Range event_range;
};

auto show_result(BenchResult const& res) -> void
{
  auto const s = ti::duration_cast<ti::seconds>(res.time_taken).count();
  auto const ms = ti::duration_cast<ti::milliseconds>(res.time_taken).count();
  auto const us = ti::duration_cast<ti::microseconds>(res.time_taken).count();
  auto const ns = res.time_taken.count();

  printf(
    "Watcher count|Event count|Time taken: %i|%i|",
    res.cfg.watcher_count, res.cfg.event_count);
  s > 0
    ? printf("%ld s\n", s)
    : ms > 0
      ? printf("%ld ms\n", ms)
      : us > 0
        ? printf("%ld us\n", us)
        : printf("%ld ns\n", ns);
};

template<BenchCfg cfg>
class Bench{
public:
  auto concurrent_watchers() -> BenchResult
  {
    namespace ww = ::wtr::watcher;
    namespace tw = ::wtr::test_watcher;
    namespace fs = ::std::filesystem;

    static unsigned nth = 0;

    if (! fs::exists(tw::test_store_path))
      fs::create_directory(tw::test_store_path);

    auto start = ti::system_clock{}.now();

    auto watchers = std::array<std::unique_ptr<ww::Watch>, cfg.watcher_count>{};

    for (int i = 0; i < cfg.watcher_count; ++i)
      watchers.at(i) = std::move(
          std::make_unique<ww::Watch>(
            tw::test_store_path,
              [](auto) {}
            ));

    for (int i = 0; i < cfg.event_count; ++i)
      std::ofstream{tw::test_store_path / std::to_string(i)};  // touch

    auto time_taken = duration_cast<ti::nanoseconds>(
        ti::system_clock{}.now() - start);

    if (std::filesystem::exists(tw::test_store_path))
      std::filesystem::remove_all(tw::test_store_path);

    return {cfg, time_taken, nth++};
  };
};

template<RangePair Rp>
constexpr auto bench_range() -> void
{
  // Until the end ...
  if constexpr (
      Rp.watcher_range.start + Rp.watcher_range.step <= Rp.watcher_range.stop
      && Rp.event_range.start + Rp.event_range.step <= Rp.event_range.stop)
  {
    // Run this ...
    auto res =
      Bench<BenchCfg{
        .watcher_count=Rp.watcher_range.start,
        .event_count=Rp.event_range.start}>{}
      .concurrent_watchers();
    show_result(res);

    // Then run the next ...
    return bench_range<
      RangePair{
        .watcher_range = Range{
          .start=Rp.watcher_range.start + Rp.watcher_range.step,
          .stop=Rp.watcher_range.stop,
          .step=Rp.watcher_range.step},
        .event_range = Range{
          .start=Rp.event_range.start + Rp.event_range.step,
          .stop=Rp.event_range.stop,
          .step=Rp.event_range.step}}>();
  }
};

// Beware of:
//   fatal error: template instantiation depth exceeds
//   maximum of <some number, usually around 1k>

TEST_CASE("Bench Concurrent Watch Targets", "[bench_concurrent_watch_targets]")
{
  bench_range<RangePair{
    .watcher_range={.start=1, .stop=1, .step=0},
    .event_range={.start=100, .stop=1000, .step=100}}>();
};

TEST_CASE("Bench Concurrent Watch Targets 2", "[bench_concurrent_watch_targets2]")
{
  bench_range<RangePair{
    .watcher_range={.start=1, .stop=30, .step=5},
    .event_range={.start=100, .stop=100, .step=0}}>();
};

TEST_CASE("Bench Concurrent Watch Targets 3", "[bench_concurrent_watch_targets3]")
{
  bench_range<RangePair{
    .watcher_range={.start=1, .stop=1, .step=0},
    .event_range={.start=100, .stop=10000, .step=1000}}>();
};

// clang-format on
