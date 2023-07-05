#pragma once

#include "snitch/snitch.hpp"
#include "test_watcher/constant.hpp"
#include "test_watcher/event.hpp"
#include "wtr/watcher.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace wtr {
namespace test_watcher {

auto watch_gather(auto const& title = "test",
                  auto const& store_path = test_store_path / "tmp_store",
                  int const path_count = 10,
                  int const concurrency_level = 1)
{
  namespace fs = ::std::filesystem;
  using namespace ::std::chrono_literals;

  auto event_recv_list = std::vector<wtr::watcher::event>{};
  auto event_recv_list_mtx = std::mutex{};

  auto event_sent_list = std::vector<wtr::watcher::event>{};
  auto watch_path_list = std::vector<fs::path>{};

  std::cerr << title << std::endl;

  {
    auto lifetimes = std::vector<std::unique_ptr<wtr::watcher::watch>>{};

    /* Setup the test directory */
    {
      // Base test directory, the same for the whole test suite
      REQUIRE(fs::create_directory(test_store_path));
      for (auto i = 0; i < concurrency_level; i++)
        watch_path_list.emplace_back(store_path / std::to_string(i));
      // Test case directory, usually unique to each test case
      REQUIRE(fs::create_directory(store_path));

      for (auto const& p : watch_path_list) REQUIRE(fs::create_directory(p));
    }

#ifdef __APPLE__
    /*  Darwin might pick up events beforehand if we don't sleep this off.
        I think this is technically a bug, but a good kind of bug.
        At least, not detrimental to the user. */
    std::this_thread::sleep_for(100ms);
#endif

    /* Watch Paths */
    {
      for (auto const& p : watch_path_list) {
        assert(fs::exists(p));

        lifetimes.emplace_back(std::make_unique<wtr::watch>(
          p,
          [&](wtr::watcher::event const& ev)
          {
            auto _ = std::scoped_lock{event_recv_list_mtx};
            std::cerr << ev << std::endl;
            auto ok_add = true;
            for (auto const& p : watch_path_list)
              if (ev.where == p) ok_add = false;
            if (ok_add) event_recv_list.emplace_back(ev);
          }));
      }
    }

    std::this_thread::sleep_for(10ms);

    /* Create Filesystem Events */
    {
      for (auto const& p : watch_path_list)
        event_sent_list.emplace_back(
          wtr::event{std::string("s/self/live@").append(p.string()),
                     wtr::event::what::create,
                     wtr::event::kind::watcher});
      for (auto const& p : watch_path_list)
        mk_events(p, path_count, &event_sent_list);
      for (auto const& p : watch_path_list)
        if (! fs::exists(p)) assert(fs::exists(p));
      for (auto const& p : watch_path_list)
        event_sent_list.emplace_back(
          wtr::event{std::string("s/self/die@").append(p.string()),
                     wtr::event::what::destroy,
                     wtr::event::kind::watcher});
    }

    std::this_thread::sleep_for(10ms);
  }

  /* Clean */
  REQUIRE(fs::remove_all(store_path));
  REQUIRE(fs::remove_all(test_store_path));

  return std::make_pair(std::move(event_sent_list), std::move(event_recv_list));
};

} /* namespace test_watcher */
} /* namespace wtr */
