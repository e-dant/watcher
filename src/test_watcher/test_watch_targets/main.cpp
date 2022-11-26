/*
   Test Watcher
   Event Targets
*/

/* REQUIRE,
   TEST_CASE */
#include <filesystem>
#include <snatch/snatch.hpp>
/* milliseconds */
#include <chrono>
/* create_directory,
   create_regular_files,
   remove_regular_files,
   test_store_path,
   regular_file_store_path,
   dir_store_path */
#include <test_watcher/test_watcher.hpp>
/* thread,
   sleep_for */
#include <thread>
/* watch,
   event,
   die */
#include <watcher/watcher.hpp>
/* etc */
#include <iostream>
#include <mutex>
#include <unordered_set>
#include <vector>

static auto event_recv_list = std::vector<wtr::watcher::event::event>{};
static auto event_recv_list_mtx = std::mutex{};
static auto event_list = std::vector<wtr::watcher::event::event>{};
static auto watch_path_list = std::vector<std::string>{};

/* Test that files are scanned */
TEST_CASE("Event Targets", "[event_targets]")
{
  /* Setup */
  using namespace wtr::watcher;
  using namespace wtr::test_watcher;
  using namespace std::chrono_literals;
  using namespace std::filesystem;
  using namespace std::this_thread;
  using std::thread, std::string, std::to_string, std::cout, std::endl,
      std::chrono::milliseconds;

  for (auto i = 0; i < 1; i++) {
    auto _ = event_targets_store_path.string() + to_string(i);
    watch_path_list.push_back(_);
  }

  {
    auto wps = string{};
    for (auto const& p : watch_path_list)
      wps += '"' + p + "\",\n  ";

    show_conf("event_targets", test_store_path, wps);
  }

  create_directory(test_store_path);
  for (auto const& p : watch_path_list) create_directory(p);

  /* Start */
  for (auto const& p : watch_path_list)
    show_event_stream_preamble();

  auto ms_begin = (ms_now() - prior_fs_events_clear_milliseconds).count();

  /* Watch */
  for (auto const& p : watch_path_list)
    thread([&]() {
      auto is_ok = (watch(p, [](event::event const& ev) {
        cout << "test_watch_targets @ alive =>\n  " << ev << endl;
        event_recv_list_mtx.lock();
        event_recv_list.push_back(ev);
        event_recv_list_mtx.unlock();
      }));
      std::cout << "test_watch_targets @ dead -> ok =>\n  " << (is_ok ? "true" : "false") << std::endl;
      REQUIRE(is_ok);
    }).detach();

  sleep_for(prior_fs_events_clear_milliseconds);

  event_recv_list_mtx.lock();
  for (auto const& p : watch_path_list) {
    mk_events(p, path_count, event_list,
              mk_events_options | mk_events_die_after);
    REQUIRE(std::filesystem::exists(p));
  }
  event_recv_list_mtx.unlock();

  /* Stop Watch */
  for (auto i = 0; i < 1; i++) {
    auto _ = event_targets_store_path.string() + to_string(i);
    auto ok = die(_, [](event::event const& ev) {
      cout << "test_watch_targets -> die =>\n  " << ev << endl;
      event_recv_list_mtx.lock();
      event_recv_list.push_back(ev);
      event_recv_list_mtx.unlock();
    });
    REQUIRE(ok);
  }

  auto const alive_for_ms_actual_value = ms_duration(ms_begin);
  auto const alive_for_ms_target_value = death_after_test_milliseconds.count();

  for (auto const& p : watch_path_list)
    show_event_stream_postamble(alive_for_ms_target_value, true);

  cout << "test_watch_targets -> alive_for @ ms @ expected =>\n  " << alive_for_ms_target_value << "ms\n"
       << "test_watch_targets -> alive_for @ ms @ actual =>\n  " << alive_for_ms_actual_value << "ms\n";

  cout << "(test/events_expected)\n";
  for (auto& it : event_list) cout << "  " << it << "\n";

  cout << "(test/events_actual)\n";
  for (auto& it : event_recv_list) cout << "  " << it << "\n";

  /* Clean */
  remove_all(test_store_path);
};
