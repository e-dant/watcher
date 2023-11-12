#include "snitch/snitch.hpp"
#include "test_watcher/constant.hpp"
#include "wtr/watcher.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

// clang-format off

using namespace std;
using namespace wtr;

inline auto do_nothing() -> void {
#ifdef _WIN32
  unsigned volatile char _;
  _ = 0;
#else
  asm volatile ("nop");
#endif
};

struct BenchCfg {
  int watch_count;
  int event_count;
};

struct BenchResult {
  struct BenchCfg cfg;
  chrono::nanoseconds time_taken_total;
  chrono::nanoseconds time_taken_fsops;
  chrono::nanoseconds time_taken_watch;
  chrono::nanoseconds clock_overhead;
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

struct TimeParts {
  long long seconds;
  long long milliseconds;
  long long microseconds;
  long long nanoseconds;
};

auto time_parts(chrono::nanoseconds ns) -> TimeParts
{
  return {
    static_cast<long long>(chrono::duration_cast<chrono::seconds>(ns).count()),
    static_cast<long long>(chrono::duration_cast<chrono::milliseconds>(ns).count()),
    static_cast<long long>(chrono::duration_cast<chrono::microseconds>(ns).count()),
    static_cast<long long>(ns.count())
  };
};

auto ftime(char* buf, unsigned long len, chrono::nanoseconds tnanos) {
  auto [ss, ms, us, ns] = time_parts(tnanos);
  auto b = ss > 0
    ? snprintf(buf, len, "%lld s", ss)
    : ms > 0
      ? snprintf(buf, len, "%lld ms", ms)
      : us > 0
        ? snprintf(buf, len, "%lld us", us)
        : snprintf(buf, len, "%lld ns", ns);
  return b;
}

auto show_results(auto res) -> void
{
  char buf[2048 * 80]{0};
  char* p = buf;
  auto colw = 20;
  auto idx = [&](char* p)
  { return sizeof(buf) - (p - buf); };
  auto inc = [&](char* p, int n)
  { return p + n < buf + sizeof(buf) ? p + n : 0; };
  auto fill = [&](char* p, int n) {
    auto np = p;
    while (n --> 0)
      np = inc(np, snprintf(np, idx(np), " "));
    return np;
  };
  auto incnfill = [&](char* p, int n)
  { return fill(inc(p, n), colw - n > 0 ? colw - n : 0); };
  auto hdrs =
  { "Total Time", "Total Fs Ops Time", "Total Watch Time", "Fs Ops Time/Event", "Watch Time/Event", "Clock Overhead", "Watchers", "Events" };
  for (auto hdr : hdrs)
    p = incnfill(p, snprintf(p, idx(p), "%s", hdr));
  p = inc(p, snprintf(p, idx(p), "\n"));
  for (auto r : res) {
    p = incnfill(p, ftime(p, idx(p), r.time_taken_total));
    p = incnfill(p, ftime(p, idx(p), r.time_taken_fsops));
    p = incnfill(p, ftime(p, idx(p), r.time_taken_watch));
    p = incnfill(p, ftime(p, idx(p), r.time_taken_fsops / r.cfg.watch_count));
    p = incnfill(p, ftime(p, idx(p), r.time_taken_watch / r.cfg.event_count));
    p = incnfill(p, ftime(p, idx(p), r.clock_overhead));
    p = incnfill(p, snprintf(p, idx(p), "%i", r.cfg.watch_count));
    p = incnfill(p, snprintf(p, idx(p), "%i", r.cfg.event_count));
    p = inc(p, snprintf(p, idx(p), "\n"));
  }
  printf("%s", buf);
};

auto now() -> chrono::nanoseconds
{
  return chrono::duration_cast<chrono::nanoseconds>(
      chrono::system_clock{}.now().time_since_epoch());
};

auto since(chrono::nanoseconds const& start) -> chrono::nanoseconds
{
  return now() - start;
};

template<BenchCfg cfg>
class Bench{
public:
  auto concurrent_watchers() -> BenchResult
  {
    auto path = test_watcher::test_store_path;

    if (! filesystem::exists(path))
      filesystem::create_directory(path);

    /*  Try to approximate the overhead of
        observing the time and a bit of
        addition by doing what we do in
        the ofstream loop, but without the
        actual filesystem ops.
        The longer this loop, the more
        reliable this figure probably is. */
    auto clock_overhead = 0ns;
    {
      constexpr int n = 1000;
      for (int i = 0; i < n; ++i) {
        auto start = now();
        (volatile void)do_nothing();
        clock_overhead += since(start);
      }
      clock_overhead /= n;
    }

    /*  Forces entry into the closure when the
        watcher decides to call it. Compilers
        might otherwise optimize a function
        with an empty body away. We use this
        to measure the accumulated time taken
        by the watcher between witnessing an
        event and sending it through to us.
    */
    auto cb = [](auto) { (volatile void)do_nothing(); };

    /*  For now, we're not measuring the time
        taken for the de/construction of the
        watchers or the set-up/tear-down time
        of the directory tree.
        Open question if we should measure
        the time taken for de/construction
        of these watchers. If we do want that,
        we should take care not to measure the
        allocation overhead. */
    auto watchers = array<unique_ptr<watch>, cfg.watch_count>{};
    for (int i = 0; i < cfg.watch_count; ++i)
      watchers.at(i) = std::move(make_unique<watch>(path, cb));

    auto start = now();
    auto time_taken_fsops = chrono::nanoseconds{0};
    for (int i = 0; i < cfg.event_count; ++i) {
      auto start_fsops = now();
      ofstream{path / to_string(i)};  // touch
      time_taken_fsops += since(start_fsops);
    }
    auto time_taken_total = since(start);
    time_taken_fsops -= clock_overhead * cfg.event_count;
    auto time_taken_watch = time_taken_total - time_taken_fsops;

    if (filesystem::exists(path))
      filesystem::remove_all(path);

    return {
      cfg,
      time_taken_total,
      time_taken_fsops,
      time_taken_watch,
      clock_overhead,
      [] { static unsigned nth = 0; return nth++; }(),
    };
  };
};

template<RangePair Rp>
auto bench_range() -> vector<BenchResult>
{
  auto res = vector<BenchResult>{};

  // Until the end ...
  if constexpr (
      Rp.watcher_range.start + Rp.watcher_range.step <= Rp.watcher_range.stop
      && Rp.event_range.start + Rp.event_range.step <= Rp.event_range.stop)
  {
    // Run this ...
    auto head =
      Bench<BenchCfg{
        .watch_count=Rp.watcher_range.start,
        .event_count=Rp.event_range.start}>{}
      .concurrent_watchers();

    // And the rest ...
    auto tail =
      bench_range<
        RangePair{
          .watcher_range = Range{
            .start=Rp.watcher_range.start + Rp.watcher_range.step,
            .stop=Rp.watcher_range.stop,
            .step=Rp.watcher_range.step},
          .event_range = Range{
            .start=Rp.event_range.start + Rp.event_range.step,
            .stop=Rp.event_range.stop,
            .step=Rp.event_range.step}}
      >();

    res.push_back(head);
    res.insert(res.end(), tail.begin(), tail.end());
  }

  return res;
};

auto vec_cat(auto... vs) -> vector<BenchResult>
{
  auto res = vector<BenchResult>{};
  (res.insert(res.end(), vs.begin(), vs.end()), ...);
  return res;
};

/*  We test that the overhead of watching a filesystem is unencumbering.

    IOW, our results should be bound to how fast we can perform io on
    the filesystem, not by how fast the watcher can witness events.

    We want to offload most of the unrelated work onto compile-time
    computations so that we can accurately measure a "common" watcher
    path/callback setup. Things like:
      Allocating, storing and resizing vectors of watchers.
      Complicated iteration logic.
    We use some fancy templates for that, so beware of the (fatal)
    compiler error when testing many watchers or events:
      template instantiation depth exceeds <some number around 1k>
*/

TEST_CASE("Concurrent Watch Target Performance", "[perf][concurrent][file][watch-target]")
{
  //  Warming up the cache
  bench_range<RangePair{{1,3,1},{100,500,100}}>();

  auto res = vec_cat(
    bench_range<RangePair{
        .watcher_range=  { .start=1,    .stop=1,     .step=0    },
        .event_range=    { .start=100,  .stop=1000,  .step=100  }}>(),
    bench_range<RangePair{
        .watcher_range=  { .start=1,    .stop=1,     .step=0    },
        .event_range=    { .start=1000, .stop=10000, .step=1000 }}>(),
    bench_range<RangePair{
        .watcher_range=  { .start=5,    .stop=30,    .step=5    },
        .event_range=    { .start=1000, .stop=1000,  .step=0    }}>()
  );
  show_results(res);

  for (auto r : res)
    CHECK(r.time_taken_watch < (r.time_taken_fsops / 100));
};

// clang-format on
