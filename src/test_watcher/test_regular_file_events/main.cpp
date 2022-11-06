#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ratio>
#include <string>
#include <thread>
#include <watcher/watcher.hpp>

static constexpr auto time_until_prior_fs_events_clear
    = std::chrono::milliseconds(10);
static constexpr auto time_until_death_after_test
    = std::chrono::milliseconds(10);
static constexpr auto path_count = 100000;

bool str_eq(auto a, auto b) { return std::strcmp(a, b) == 0; }

auto create_regular_files(auto path, auto n)
{
  using namespace std::filesystem;
  using std::ofstream;

  for (int i = 0; i < n; i++) {
    auto item = std::to_string(i) + std::string(".txt");

    ofstream{path / item};

    // REQUIRE(is_regular_file(item));
  }
}

auto remove_regular_files(auto path, auto n)
{
  using namespace std::filesystem;

  for (int i = 0; i < n; i++) {
    auto item
        = path / (std::to_string(i) + std::string(".txt"));

    if (exists(item)) remove_all(item);

    // REQUIRE(!exists(item));
  }
}

void show_strange_event(auto& title, wtr::watcher::event::event const& ev)
{
  // std::cout << "warning in " << title << ":"
  //           << "\n strange event at: " << ev.where << "\n json: {" << ev
  //           << "\n\n";
}

void test_regular_file_event_handling(
    wtr::watcher::event::event const& this_event)
{
  using namespace wtr::watcher;

  /* Print first */
  // this_event.kind != event::kind::watcher
  //     ? std::cout << this_event << "," << std::endl
  //     : std::cout << this_event << std::endl;

  if (this_event.kind != event::kind::watcher) {
    if (this_event.kind != event::kind::file) {
      show_strange_event(__FUNCTION__, this_event);

      // REQUIRE(str_eq(std::filesystem::path(this_event.where).filename().c_str(),
      //                regular_file_store_path.filename().c_str()));
    } else {
      REQUIRE(this_event.kind == event::kind::file);
    }
  } else {
    /* - "s/" means "success"
       - "e/" means "error" */
    REQUIRE(std::string(this_event.where).starts_with("s/"));
  }
}

/* Test that files are scanned */
TEST_CASE("Watch Regular Files", "[watch_regular_files]")
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
  create_directory(regular_file_store_path);
  std::this_thread::sleep_for(time_until_prior_fs_events_clear);

  /* Start */
  // std::cout << R"({"test.wtr.watcher":{"stream":{)" << std::endl;

#ifdef WATER_WATCHER_PLATFORM_WINDOWS_ANY
  auto const regular_file_store_path_str
      = regular_file_store_path.string();
  std::thread([&]() {
    watch(regular_file_store_path_str.c_str(), test_regular_file_event_handling);
  }).detach();

#else
  std::thread([&]() {
    watch(regular_file_store_path.c_str(), test_regular_file_event_handling);
  }).detach();
#endif

  // std::this_thread::sleep_for(1s);

  create_regular_files(test_store_path, path_count);

  std::this_thread::sleep_for(time_until_death_after_test);

  bool const is_watch_dead = die(test_regular_file_event_handling);

  // std::cout << "}" << std::endl
  //           << R"(,"milliseconds":)" <<
  //           milliseconds(time_until_death_after_test).count()
  //           << std::endl
  //           << R"(,"dead":)" << std::boolalpha << is_watch_dead
  //           << "}"
  //              "}"
  //           << std::endl;

  /* Stop */
  remove_regular_files(test_store_path, path_count);
  remove(regular_file_store_path);
  remove(test_store_path);
}
