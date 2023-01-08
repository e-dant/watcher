#pragma once

/* milliseconds */
#include <chrono>
/* assert */
#include <cassert>
/* this_thread::sleep_for */
#include <thread>
/* async,
   future,
   promise */
#include <future>
/* watch,
   event,
   die */
#include <watcher/watcher.hpp>
/* mutex */
#include <mutex>
/* ms_now,
   ms_duration */
#include <test_watcher/chrono.hpp>
/* mk_events */
#include <test_watcher/event.hpp>
/* vector */
#include <vector>
/* test_store_path */
#include <test_watcher/constant.hpp>
/* pair */
#include <tuple>
/* move */
#include <utility>
/* etc */
#include <filesystem>
#include <iostream>

namespace wtr {
namespace test_watcher {

/* @brief
     - Create a watcher for each `concurrency_level`
     - For each watcher:
       - Create event(s)
       - Kill watcher(s)

   @return
     pair of expected and recieved events

   @todo
     Make printing an option */
auto watch_gather(auto const& /* Title */
                      title
                  = "test",
                  auto const& /* Store Path */
                      store_path
                  = wtr::test_watcher::test_store_path / "tmp_store",
                  int const /* Path Event Count */
                      path_count
                  = 10,
                  int const /* Concurrent Watcher Count */
                      concurrency_level
                  = 1,
                  std::chrono::milliseconds const& /* Simulated Lifetime */
                      alive_for_ms_target
                  = std::chrono::milliseconds(1000))
{
  static constexpr auto pre_create_event_delay = std::chrono::milliseconds(10);
  static constexpr auto pre_stop_watch_delay = std::chrono::milliseconds(10);

  auto event_recv_list = std::vector<wtr::watcher::event::event>{};
  auto event_recv_list_mtx = std::mutex{};

  auto event_sent_list = std::vector<wtr::watcher::event::event>{};
  auto watch_path_list = std::vector<std::filesystem::path>{};

  auto futures = std::vector<std::future<bool>>{};

  /* Setup */
  {
    if (alive_for_ms_target.count() <= 0)
      assert(alive_for_ms_target.count() > 0);

    for (auto i = 0; i < concurrency_level; i++)
      watch_path_list.emplace_back(store_path / std::to_string(i));

    auto wps = std::string{};
    for (auto const& p : watch_path_list) wps += "\"" + p.string() + "\",\n  ";

    std::cout << title << std::endl;

    std::filesystem::create_directory(wtr::test_watcher::test_store_path);
    assert(std::filesystem::exists(wtr::test_watcher::test_store_path));

    std::filesystem::create_directory(store_path);
    assert(std::filesystem::exists(store_path));

    for (auto const& p : watch_path_list) {
      std::filesystem::create_directory(p);
      assert(std::filesystem::exists(p));
    }
  }

  /* Start Time */
  auto const ms_begin = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());

  if (ms_begin.count() <= 0)
    assert(ms_begin.count() > 0);

  /* Watch Paths */
  {
    for (auto const& p : watch_path_list) {
      assert(std::filesystem::exists(p));

      futures.emplace_back(std::async(std::launch::async, [&]() {
        auto const watch_ok = (wtr::watcher::watch(
            p, [&](wtr::watcher::event::event const& ev) {
              auto _ = std::scoped_lock{event_recv_list_mtx};
              std::cout << ev << std::endl;
              auto ok_add = true;
              for (auto const& p : watch_path_list)
                if (ev.where == p) ok_add = false;
              if (ok_add) event_recv_list.emplace_back(ev);
            }));
        return watch_ok;
      }));
    }
  }

  /* Wait */
  {
    std::this_thread::sleep_for(pre_create_event_delay);
  }

  /* Create Filesystem Events */
  {
    for (auto const& p : watch_path_list)
      event_sent_list.emplace_back(
          watcher::event::event{std::string("s/self/live@").append(p.string()),
                                wtr::watcher::event::what::create,
                                wtr::watcher::event::kind::watcher});
    for (auto const& p : watch_path_list)
      wtr::test_watcher::mk_events(p, path_count, &event_sent_list);
    for (auto const& p : watch_path_list)
      if (!std::filesystem::exists(p)) assert(std::filesystem::exists(p));
    for (auto const& p : watch_path_list)
      event_sent_list.emplace_back(
          watcher::event::event{std::string("s/self/die@").append(p.string()),
                                wtr::watcher::event::what::destroy,
                                wtr::watcher::event::kind::watcher});
  }

  /* Wait */
  {
    std::this_thread::sleep_for(pre_stop_watch_delay);
  }

  /* Stop Watchers */
  {
    for (auto i = 0; i < concurrency_level; i++) {
      auto const p = store_path / std::to_string(i);
      assert(std::filesystem::exists(p));
      auto dead
          = wtr::watcher::die(p, [&event_recv_list, &event_recv_list_mtx](
                                     wtr::watcher::event::event const& ev) {
              auto _ = std::scoped_lock{event_recv_list_mtx};
              event_recv_list.emplace_back(ev);
            });
      if (!dead)
        assert(dead);
    }
    for (auto& f : futures)
      if (!f.get()) assert(f.get());
  }

  /* Show Results */
  {
    /* @todo return time alive */
    auto const delayed_for = (std::chrono::milliseconds(16) * concurrency_level)
                             + pre_create_event_delay + pre_stop_watch_delay;
    auto const alive_for_ms_actual_value
        = std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
          - delayed_for;
    if (alive_for_ms_actual_value.count() <= 0) assert(alive_for_ms_actual_value.count() > 0);

    /* for (auto const& p : watch_path_list) */
    /*   wtr::test_watcher::show_event_stream_postamble( */
    /*       alive_for_ms_target.count(), true); */

    /* std::cout << "test @ target @ alive for @ ms => " */
    /*           << alive_for_ms_target.count() << "\n" */
    /*           << "test @ actual @ alive for @ ms => " */
    /*           << alive_for_ms_actual_value.count() << "\n"; */

    /* std::cout << "test @ target @ events =>\n"; */
    /* for (auto& it : event_list) std::cout << " " << it << "\n"; */

    /* std::cout << "test @ actual @ events =>\n"; */
    /* for (auto& it : event_recv_list) std::cout << " " << it << "\n"; */
  }

  /* Clean */
  {
    std::filesystem::remove_all(store_path);
    assert(!std::filesystem::exists(store_path));

    std::filesystem::remove_all(wtr::test_watcher::test_store_path);
    assert(!std::filesystem::exists(wtr::test_watcher::test_store_path));
  }

  /* The assertions would have already failed us
     if we were not ok */
  return std::make_pair(std::move(event_sent_list), std::move(event_recv_list));
};

} /* namespace test_watcher */
} /* namespace wtr */
