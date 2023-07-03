#pragma once

#include <chrono>
#include <cassert>
#include <thread>
#include <functional>
#include <mutex>
#include <vector>
#include <tuple>
#include <utility>
#include <filesystem>
#include <iostream>

#include "wtr/watcher.hpp"
#include "test_watcher/chrono.hpp"
#include "test_watcher/event.hpp"
#include "test_watcher/constant.hpp"

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
                    title = "test",
                  auto const& /* Store Path */
                    store_path = wtr::test_watcher::test_store_path
                               / "tmp_store",
                  int const /* Path Event Count */
                    path_count = 10,
                  int const /* Concurrent Watcher Count */
                    concurrency_level = 1,
                  std::chrono::milliseconds const& /* Simulated Lifetime */
                    alive_for_ms_target = std::chrono::milliseconds(1000))
{
  static constexpr auto pre_create_event_delay = std::chrono::milliseconds(10);
  static constexpr auto pre_stop_watch_delay = std::chrono::milliseconds(10);

  auto event_recv_list = std::vector<wtr::watcher::event>{};
  auto event_recv_list_mtx = std::mutex{};

  auto event_sent_list = std::vector<wtr::watcher::event>{};
  auto watch_path_list = std::vector<std::filesystem::path>{};

  auto lifetimes = std::vector<std::function<bool()>>{};

  /* Setup */
  {
    if (alive_for_ms_target.count() <= 0)
      assert(alive_for_ms_target.count() > 0);

    for (auto i = 0; i < concurrency_level; i++)
      watch_path_list.emplace_back(store_path / std::to_string(i));

    auto wps = std::string{};
    for (auto const& p : watch_path_list) {
      // Doing this in a more "straightforward"
      // way has issues with some gcc's builtin memcpy
      // (within the iostream stdlib headers?)
      wps.append("\"");
      wps.append(p.string());
      wps.append("\",\n  ");
      // wps += "\"" + p.string() + "\",\n  ";
    }

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

  if (ms_begin.count() <= 0) assert(ms_begin.count() >= 0);

  /* Watch Paths */
  {
    for (auto const& p : watch_path_list) {
      assert(std::filesystem::exists(p));

      lifetimes.emplace_back(
        wtr::watcher::watch(p,
                            [&](wtr::watcher::event const& ev)
                            {
                              auto _ = std::scoped_lock{event_recv_list_mtx};
                              std::cout << ev << std::endl;
                              auto ok_add = true;
                              for (auto const& p : watch_path_list)
                                if (ev.where == p) ok_add = false;
                              if (ok_add) event_recv_list.emplace_back(ev);
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
        watcher::event{std::string("s/self/live@").append(p.string()),
                       wtr::watcher::event::what::create,
                       wtr::watcher::event::kind::watcher});
    for (auto const& p : watch_path_list)
      wtr::test_watcher::mk_events(p, path_count, &event_sent_list);
    for (auto const& p : watch_path_list)
      if (! std::filesystem::exists(p)) assert(std::filesystem::exists(p));
    for (auto const& p : watch_path_list)
      event_sent_list.emplace_back(
        watcher::event{std::string("s/self/die@").append(p.string()),
                       wtr::watcher::event::what::destroy,
                       wtr::watcher::event::kind::watcher});
  }

  /* Wait */
  {
    std::this_thread::sleep_for(pre_stop_watch_delay);
  }

  /* Stop Watchers */
  {
    for (auto& f : lifetimes)
      if (! f()) assert(false);
  }

  /* Show Results */
  {
    /* @todo return time alive */
    auto const delayed_for = (std::chrono::milliseconds(16) * concurrency_level)
                           + pre_create_event_delay + pre_stop_watch_delay;
    auto const alive_for_ms_actual_value =
      std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch())
      - delayed_for;
    if (alive_for_ms_actual_value.count() <= 0)
      assert(alive_for_ms_actual_value.count() >= 0);
  }

  /* Clean */
  {
    std::filesystem::remove_all(store_path);
    assert(! std::filesystem::exists(store_path));

    std::filesystem::remove_all(wtr::test_watcher::test_store_path);
    assert(! std::filesystem::exists(wtr::test_watcher::test_store_path));
  }

  /* The assertions would have already failed us
     if we were not ok */
  return std::make_pair(std::move(event_sent_list), std::move(event_recv_list));
};

} /* namespace test_watcher */
} /* namespace wtr */
