#include "snitch/snitch.hpp"
#include "test_watcher/test_watcher.hpp"
#include "wtr/watcher.hpp"
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

/* Test that files are scanned */
TEST_CASE("New Directories", "[test][dir][watch-target]")
{
  namespace fs = std::filesystem;
  using namespace std::chrono_literals;
  using namespace wtr::watcher;
  using namespace wtr::test_watcher;

  static constexpr auto path_count = 10;
  static constexpr auto title = "New Directories";
  static auto env_verbose = std::getenv("VERBOSE") != nullptr;
  auto const base_store_path = test_store_path;
  auto const store_path_first = base_store_path / "new_directories_first_store";
  auto const store_path_second =
    base_store_path / "new_directories_second_store";
  auto const store_path_list =
    std::vector<fs::path>{base_store_path, store_path_first, store_path_second};
  auto const new_store_path_list =
    std::vector<fs::path>{store_path_first, store_path_second};
  auto event_recv_list = std::vector<event>{};
  auto event_recv_list_mtx = std::mutex{};
  auto event_sent_list = std::vector<event>{};
  auto watch_path_list = std::vector<fs::path>{};

  std::cout << title << std::endl;

  fs::create_directory(base_store_path);

  /* @todo
     This sleep is hiding a bug on darwin which picks
     up events slightly before we start watching. I'm
     ok with that bit of wiggle-room. */
  std::this_thread::sleep_for(100ms);

  event_sent_list.push_back(
    {std::string("s/self/live@").append(base_store_path.string()),
     event::effect_type::create,
     event::path_type::watcher});

  auto lifetime = watch(
    base_store_path,
    [&](event const& ev)
    {
      auto _ = std::scoped_lock{event_recv_list_mtx};
      if (env_verbose) std::cout << ev << std::endl;
      event_recv_list.push_back(ev);
    });

  /* @todo
     This sleep is hiding a bug on Linux which begins
     watching very slightly after we ask it to. */
  std::this_thread::sleep_for(10ms);

  for (auto const& p : new_store_path_list) {
    fs::create_directory(p);
    event_sent_list.push_back(
      event{p, event::effect_type::create, event::path_type::dir});
  }

  std::this_thread::sleep_for(10ms);

  for (int i = 0; i < path_count; i++) {
    for (auto const& p : store_path_list) {
      auto const new_dir_path = p / ("dir" + std::to_string(i));
      fs::create_directory(new_dir_path);
      REQUIRE(fs::exists(new_dir_path));
      event_sent_list.push_back(
        event{new_dir_path, event::effect_type::create, event::path_type::dir});

      /* @todo
         This sleep is hiding a bug for the Linux adapters.
         It might on either our end -- something about how
         we're processing batches of events -- or it could
         be on the kernel's end... just dropping events.
         We should investigate and fix it if we can. */
      std::this_thread::sleep_for(10ms);

      auto const new_file_path = new_dir_path / "file.txt";
      std::ofstream{new_file_path}; /* NOLINT */
      REQUIRE(fs::exists(new_file_path));
      event_sent_list.push_back(event{
        new_file_path,
        event::effect_type::create,
        event::path_type::file});

      std::this_thread::sleep_for(10ms);
    }
  }

  std::this_thread::sleep_for(10ms);

  event_sent_list.push_back(
    {std::string("s/self/die@").append(base_store_path.string()),
     event::effect_type::destroy,
     event::path_type::watcher});

  auto dead = lifetime.close();

  REQUIRE(dead);

  fs::remove_all(base_store_path);
  REQUIRE(! fs::exists(base_store_path));

  check_event_lists_eq(event_sent_list, event_recv_list);
};
