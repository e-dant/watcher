#pragma once

/* milliseconds */
#include <chrono>
/* assert */
#include <cassert>
/* thread,
   sleep_for */
#include <thread>
/* watch,
   event,
   die */
#include <watcher/watcher.hpp>
/* mutex */
#include <mutex>
/* ms_now,
   ms_duration */
#include <test_watcher/chrono.hpp>
/* show_conf */
#include <test_watcher/conf.hpp>
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
    - Create watcher(s)
    - Create event(s)
    - Kill watcher(s)
    - Return expected and recieved events
   @todo
    Make printing an option */
static auto watch_gather(
    auto const& title = "test", /* Title */
    auto const& store_path      /* Store Path */
    = wtr::test_watcher::test_store_path / "tmp_store",
    int const path_count = 10,       /* Path Event Count */
    int const concurrency_level = 1, /* Concurrent Watcher Count */
    std::chrono::milliseconds const  /* Simulated Lifetime */
        alive_for_ms_target
    = std::chrono::milliseconds(1000))
{
  static auto event_recv_list = std::vector<wtr::watcher::event::event>{};
  static auto event_recv_list_mtx = std::mutex{};

  static auto event_list = std::vector<wtr::watcher::event::event>{};
  static auto watch_path_list = std::vector<std::string>{};

  /* static auto cout_mtx = std::mutex{}; */

  static constexpr auto pre_create_event_delay = std::chrono::milliseconds(500);
  static constexpr auto pre_stop_watch_delay = std::chrono::milliseconds(500);

  /* Setup */
  {
    assert(alive_for_ms_target.count() > 0);

    for (auto i = 0; i < concurrency_level; i++) {
      auto _ = store_path.string() + std::to_string(i);
      watch_path_list.push_back(_);
    }

    auto wps = std::string{};
    for (auto const& p : watch_path_list) wps += '"' + p + "\",\n  ";

    wtr::test_watcher::show_conf(title, store_path, wps);

    std::filesystem::create_directory(wtr::test_watcher::test_store_path);
    assert(std::filesystem::exists(wtr::test_watcher::test_store_path));

    std::filesystem::create_directory(store_path);
    assert(std::filesystem::exists(store_path));

    for (auto const& p : watch_path_list) {
      std::filesystem::create_directory(p);
      assert(std::filesystem::exists(p));
    }
  }

  /* for (auto const& p : watch_path_list) */
  /*   wtr::test_watcher::show_event_stream_preamble(); */

  /* Start Time */
  auto ms_begin = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());

  assert(ms_begin.count() > 0);

  /* Watch Paths */
  {
    for (auto const& p : watch_path_list) {
      /* std::cout << "test @ live @ create @ '" << p << "'\n"; */
      assert(std::filesystem::exists(p));

      std::thread([&]() {
        auto ok = (wtr::watcher::watch(
            p, [&](wtr::watcher::event::event const& ev) {
              /* cout_mtx.lock(); */
              /* std::cout << "test @ live -> recv => " << ev << "\n"; */
              /* cout_mtx.unlock(); */

              event_recv_list_mtx.lock();
              auto ok_add = true;
              for (auto const& p : watch_path_list)
                if (ev.where == p) ok_add = false;
              if (ok_add) event_recv_list.push_back(ev);
              event_recv_list_mtx.unlock();
            }));

        /* cout_mtx.lock(); */
        /* std::cout << "test @ live @ result @ '" << p << "' => " */
        /*           << (ok ? "true\n" : "false\n"); */
        /* cout_mtx.unlock(); */

        assert(ok);
      }).detach();
    }
  }

  /* Wait */
  {
    std::this_thread::sleep_for(pre_create_event_delay);
  }

  /* Create Filesystem Events */
  {
    event_recv_list_mtx.lock();
    for (auto const& p : watch_path_list) {
      wtr::test_watcher::mk_events(
          p, path_count, event_list,
          wtr::test_watcher::mk_events_options
              | wtr::test_watcher::mk_events_die_after);
      assert(std::filesystem::exists(p));
    }
    event_recv_list_mtx.unlock();
  }

  /* Wait */
  {
    std::this_thread::sleep_for(pre_stop_watch_delay);
  }

  /* Stop Watchers */
  {
    for (auto i = 0; i < concurrency_level; i++) {
      auto p = store_path.string() + std::to_string(i);
      /* std::cout << "test @ die @ create @ '" << p << "'\n"; */
      assert(std::filesystem::exists(p));

      auto ok = wtr::watcher::die(p, [](wtr::watcher::event::event const& ev) {
        /* cout_mtx.lock(); */
        /* std::cout << "test @ die -> recv => " << ev << "\n"; */
        /* cout_mtx.unlock(); */

        event_recv_list_mtx.lock();
        event_recv_list.push_back(ev);
        event_recv_list_mtx.unlock();
      });

      /* cout_mtx.lock(); */
      /* std::cout << "test @ die @ result @ '" << p << "' => " */
      /*           << (ok ? "true\n" : "false\n"); */
      /* cout_mtx.unlock(); */

      assert(ok);
    }
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
    assert(alive_for_ms_actual_value.count() > 0);

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
  return std::make_pair(std::move(event_list), std::move(event_recv_list));
};

} /* namespace test_watcher */
} /* namespace wtr */
