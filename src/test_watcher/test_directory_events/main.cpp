/*
   Watcher
   Test Directory Events
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
TEST_CASE("Watch Directories", "[watch_directories]")
{
  /* Setup */
  using namespace wtr::watcher;
  using namespace std::chrono_literals;
  using namespace std::filesystem;
  using std::chrono::milliseconds;

  auto const test_store_path
      = std::filesystem::current_path() / "tmp_test_watcher";
  auto const regular_file_store_path = test_store_path / "regular_file_store";
  auto const dir_store_path = test_store_path / "dir_store";

  create_directory(test_store_path);
  create_directory(dir_store_path);
  std::this_thread::sleep_for(time_until_prior_fs_events_clear);

  /* Start */
  // std::cout << R"({"test.wtr.watcher":{"stream":{)" << std::endl;

#ifdef WATER_WATCHER_PLATFORM_WINDOWS_ANY
  auto const regular_file_store_path_str = regular_file_store_path.string();
  std::thread([&]() {
    watch(regular_file_store_path_str.c_str(), test_directory_event_handling);
  }).detach();

#else
  std::thread([&]() {
    watch(regular_file_store_path.c_str(), test_directory_event_handling);
  }).detach();
#endif

  // std::this_thread::sleep_for(1s);

  create_directories(dir_store_path, path_count);

  std::this_thread::sleep_for(time_until_death_after_test);

  bool const is_watch_dead = die(test_directory_event_handling);

  // std::cout << "}" << std::endl
  //           << R"(,"milliseconds":)" <<
  //           milliseconds(time_until_death_after_test).count()
  //           << std::endl
  //           << R"(,"dead":)" << std::boolalpha << is_watch_dead
  //           << "}"
  //              "}"
  //           << std::endl;

  /* Stop */
  remove_directories(dir_store_path, path_count);
  remove(dir_store_path);
  remove(test_store_path);
}
