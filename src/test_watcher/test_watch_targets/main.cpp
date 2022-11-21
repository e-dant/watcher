/*
   Test Watcher
   Event Targets
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
#include <vector>

static auto event_recv_list = std::vector<wtr::watcher::event::event>{};
static auto event_recv_list_mtx = std::mutex{};

/* Test that files are scanned */
TEST_CASE("Event Targets", "[event_targets]")
{
  /* Setup */
  using namespace wtr::watcher;
  using namespace wtr::test_watcher;
  using namespace std::chrono_literals;
  using namespace std::filesystem;
  using namespace std::this_thread;
  using std::thread, std::vector, std::string, std::cout, std::endl,
      std::chrono::milliseconds;

  auto event_list = vector<event::event>{};

  auto const watch_path = event_targets_store_path.string();

  show_conf("event_targets", test_store_path, watch_path);

  create_directory(test_store_path);
  create_directory(watch_path);

  sleep_for(prior_fs_events_clear_milliseconds);

  /* Start */
  show_event_stream_preamble();

  auto ms_begin = ms_now().count();

  /* Watch */
  thread([&]() {
    auto is_ok = watch(watch_path.c_str(), [](event::event const& ev) {
      cout << "(living) " << ev << endl;
      event_recv_list_mtx.lock();
      event_recv_list.push_back(event::event{ev});
      event_recv_list_mtx.unlock();
    });
    REQUIRE(is_ok);
  }).detach();

  event_recv_list_mtx.lock();
  mk_events(watch_path, path_count, event_list);
  event_recv_list_mtx.unlock();

  /* Wait */
  sleep_for(death_after_test_milliseconds);

  REQUIRE(detail::adapter::is_living(watch_path.c_str()));

  /* Stop Watch */
  event_list.emplace_back(
      event::event{"s/self/die", event::what::other, event::kind::watcher});
  bool const is_watch_dead = die(watch_path.c_str(), [](event::event const& ev) {
    cout << " (dying) " << ev << endl;
    event_recv_list_mtx.lock();
    event_recv_list.push_back(event::event{ev});
    event_recv_list_mtx.unlock();
  });

  REQUIRE(is_watch_dead);

  auto const alive_for_ms_actual_value = ms_duration(ms_begin);
  auto const alive_for_ms_target_value = death_after_test_milliseconds.count();

  show_event_stream_postamble(alive_for_ms_target_value, is_watch_dead);

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
