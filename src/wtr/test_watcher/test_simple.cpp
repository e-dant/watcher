#include "snitch/snitch.hpp"
#include "test_watcher/test_watcher.hpp"
#include "wtr/watcher.hpp"
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

/* Test that files are scanned */
TEST_CASE("Simple", "[test][dir][file][simple]")
{
  namespace fs = ::std::filesystem;
  using namespace ::std::chrono_literals;
  using namespace ::wtr::watcher;
  using namespace ::wtr::test_watcher;

  static constexpr auto path_count = 3;
  static constexpr auto title = "Simple";
  static auto event_recv_list = std::vector<event>{};
  static auto event_recv_list_mtx = std::mutex{};
  static auto event_sent_list = std::vector<event>{};
  static auto watch_path_list = std::vector<std::string>{};
  static auto const store_path = test_store_path / "simple_store";

  std::cout << title << std::endl;

  fs::create_directories(store_path);
  REQUIRE(fs::exists(store_path));

  /* @todo
     This sleep is hiding a bug on darwin which picks
     up events slightly before we start watching. I'm
     ok with that bit of wiggle-room. */
  std::this_thread::sleep_for(10ms);

  event_sent_list.push_back(
    {std::string("s/self/live@").append(store_path.string()),
     event::what::create,
     event::kind::watcher});

  auto watcher = watch(
    store_path,
    [](event const& ev)
    {
      auto _ = std::scoped_lock{event_recv_list_mtx};
      std::cout << ev << std::endl;
      event_recv_list.push_back(ev);
    });

  std::this_thread::sleep_for(10ms);

  for (int i = 0; i < path_count; ++i) {
    auto const new_dir_path = store_path / ("new_dir" + std::to_string(i));
    fs::create_directory(new_dir_path);

    REQUIRE(fs::is_directory(new_dir_path));

    event_sent_list.push_back(
      {new_dir_path, event::what::create, event::kind::dir});

    auto const new_file_path =
      store_path / ("new_file" + std::to_string(i) + ".txt");
    std::ofstream(new_file_path).close();

    REQUIRE(fs::is_regular_file(new_file_path));

    event_sent_list.push_back(
      {new_file_path, event::what::create, event::kind::file});

    std::this_thread::sleep_for(10ms);
  }

  event_sent_list.push_back(
    {std::string("s/self/die@").append(store_path.string()),
     event::what::destroy,
     event::kind::watcher});

  auto dead = watcher.close();

  REQUIRE(dead);

  fs::remove_all(test_store_path);
  REQUIRE(! fs::exists(test_store_path));

  check_event_lists_eq(event_sent_list, event_recv_list);
};
