/*
   Test Watcher
   Concurrent Event Targets
*/

/* REQUIRE,
   TEST_CASE */
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
#include <unordered_map>
#include <vector>

static auto event_recv_list = std::vector<wtr::watcher::event::event>{};
static auto event_recv_list_mtx = std::mutex{};

/* Test that files are scanned */
TEST_CASE("Concurrent Event Targets", "[concurrent_event_targets]")
{
  /* Setup */
  using namespace wtr::watcher;
  using namespace wtr::test_watcher;
  using namespace std::chrono_literals;
  using namespace std::filesystem;
  using namespace std::this_thread;
  using std::thread, std::vector, std::string, std::to_string, std::cout,
      std::endl, std::chrono::milliseconds;

  auto event_list = vector<event::event>{};

  auto watch_path_list = std::unordered_map<std::string, bool>{};

  for (auto i = 0; i < concurrent_event_targets_concurrency_level; i++) {
    auto _ = concurrent_event_targets_store_path.string() + to_string(i);
    watch_path_list[_] = detail::adapter::is_living(_);
  }

  for (auto const& p : watch_path_list)
    show_conf("concurrent_event_targets", test_store_path, p.first);

  create_directory(test_store_path);
  for (auto const& p : watch_path_list) create_directory(p.first);

  sleep_for(prior_fs_events_clear_milliseconds);

  /* Start */
  // show_event_stream_preamble();

  auto ms_begin = ms_now().count();

  /* Watch */
  for (auto const& p : watch_path_list)
    thread([&]() {
      REQUIRE(detail::adapter::is_living(p.first) == false);
      auto is_ok = watch(p.first, [](event::event const& ev) {
        // cout << "(living) " << ev << endl;
        event_recv_list_mtx.lock();
        event_recv_list.push_back(event::event{ev});
        event_recv_list_mtx.unlock();
      });
      REQUIRE(is_ok);
    }).detach();

  event_recv_list_mtx.lock();
  for (auto const& p : watch_path_list)
    mk_events(p.first, path_count, event_list,
              mk_events_options | mk_events_die_after);
  event_recv_list_mtx.unlock();

  /* Wait */
  sleep_for(death_after_test_milliseconds);

  for (auto const& p : watch_path_list)
    REQUIRE(detail::adapter::is_living(p.first));

  /* Stop Watch */
  for (auto i = 0; i < concurrent_event_targets_concurrency_level; i++) {
    auto _ = concurrent_event_targets_store_path.string() + to_string(i);
    watch_path_list.at(_) = die(_, [](event::event const& ev) {
      // cout << " (dying) " << ev << endl;
      event_recv_list_mtx.lock();
      event_recv_list.push_back(event::event{ev});
      event_recv_list_mtx.unlock();
    });
  }

  for (auto const& p : watch_path_list)
    REQUIRE(detail::adapter::is_living(p.first) == false);

  auto const alive_for_ms_actual_value = ms_duration(ms_begin);
  auto const alive_for_ms_target_value = death_after_test_milliseconds.count();

  // for (auto const& p : watch_path_list)
  //   show_event_stream_postamble(alive_for_ms_target_value, p.second);

  cout << "Alive for:" << endl
       << " Expected: " << alive_for_ms_target_value << "ms" << endl
       << " Actual: " << alive_for_ms_actual_value << "ms" << endl;

  cout << "have events:\n";
  for (auto& it : event_recv_list) cout << it << "\n";

  cout << "expected events:\n";
  for (auto& it : event_list) cout << it << "\n";

  /* Clean */
  remove_all(test_store_path);
};
