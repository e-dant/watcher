/*
   Test Watcher
   Directory Events
*/

/* REQUIRE,
   TEST_CASE */
#include <catch2/catch_test_macros.hpp>
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

/* Test that directories are scanned */
TEST_CASE("Directory Events", "[directory_events]")
{
  /* Setup */
  using namespace wtr::watcher;
  using namespace wtr::test_watcher;
  using namespace std::chrono_literals;
  using namespace std::filesystem;
  using std::chrono::milliseconds;

  auto const watch_path = dir_store_path;
  create_directory(test_store_path);
  create_directory(watch_path);

  std::this_thread::sleep_for(time_until_prior_fs_events_clear);

  /* Start */
  show_event_stream_preamble();

  /* Watch */
  thread_watch(watch_path, test_directory_event_handling);

  /* Create Events */
  create_directories(watch_path, path_count);

  /* Wait */
  std::this_thread::sleep_for(time_until_death_after_test);

  /* Stop Watch */
  bool const is_watch_dead = die(test_directory_event_handling);
  auto const alive_for_ms = milliseconds(time_until_death_after_test).count();
  show_event_stream_postamble(alive_for_ms, is_watch_dead);

  /* Clean */
  remove_directories(watch_path, path_count);
  remove(watch_path);
  remove(test_store_path);
}
