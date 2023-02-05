/*
   Test Watcher
   Simple
*/

/* REQUIRE,
   TEST_CASE */
#include <snitch/snitch.hpp>
/* event */
#include <wtr/watcher.hpp>
/* watch_gather */
#include <test_watcher/test_watcher.hpp>
/* get */
#include <tuple>
/* cout, endl */
#include <iostream>
/* async,
   future,
   promise */
#include <future>
/* vector */
#include <vector>
/* string */
#include <string>
/* milliseconds */
#include <chrono>
/* mutex */
#include <mutex>
/* path,
   create */
#include <filesystem>

/* Test that files are scanned */
TEST_CASE("Simple", "[simple]") {
  using namespace wtr::watcher;

  static constexpr auto path_count = 3;

  static auto event_recv_list = std::vector<event::event>{};
  static auto event_recv_list_mtx = std::mutex{};

  static auto event_sent_list = std::vector<event::event>{};

  static auto watch_path_list = std::vector<std::string>{};

  auto const base_store_path = wtr::test_watcher::test_store_path;
  static auto const store_path = base_store_path / "simple_store";

  static constexpr auto title = "Simple";

  std::cout << title << std::endl;

  std::filesystem::create_directories(store_path);
  REQUIRE(std::filesystem::exists(base_store_path)
          && std::filesystem::exists(store_path));

  /* @todo
     This sleep is hiding a bug on darwin which picks
     up events slightly before we start watching. I'm
     ok with that bit of wiggle-room. */
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  event_sent_list.push_back(
  {std::string("s/self/live@").append(store_path.string()),
   event::what::create,
   event::kind::watcher});

  auto lifetime = wtr::watcher::watch(store_path, [](event::event const& ev) {
    auto _ = std::scoped_lock{event_recv_list_mtx};
    std::cout << ev << std::endl;
    event_recv_list.push_back(ev);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  for (int i = 0; i < path_count; ++i) {
    auto const new_dir_path = store_path / ("new_dir" + std::to_string(i));
    std::filesystem::create_directory(new_dir_path);

    REQUIRE(std::filesystem::is_directory(new_dir_path));

    event_sent_list.push_back(
    {new_dir_path, event::what::create, event::kind::dir});

    auto const new_file_path =
    store_path / ("new_file" + std::to_string(i) + ".txt");
    std::ofstream(new_file_path).close();

    REQUIRE(std::filesystem::is_regular_file(new_file_path));

    event_sent_list.push_back(
    {new_file_path, event::what::create, event::kind::file});

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  event_sent_list.push_back(
  {std::string("s/self/die@").append(store_path.string()),
   event::what::destroy,
   event::kind::watcher});

  auto dead = lifetime();

  REQUIRE(dead);

  std::filesystem::remove_all(base_store_path);
  REQUIRE(! std::filesystem::exists(base_store_path));

  auto const max_i = event_sent_list.size() > event_recv_list.size()
                   ? event_recv_list.size()
                   : event_sent_list.size();
  for (size_t i = 0; i < max_i; ++i) {
    if (event_sent_list[i].kind != wtr::watcher::event::kind::watcher) {
      if (event_sent_list[i].where != event_recv_list[i].where)
        std::cout << "[ where ] [ " << i << " ] sent "
                  << event_sent_list[i].where << ", but received "
                  << event_recv_list[i].where << "\n";
      if (event_sent_list[i].what != event_recv_list[i].what)
        std::cout << "[ what ] [ " << i << " ] sent " << event_sent_list[i].what
                  << ", but received " << event_recv_list[i].what << "\n";
      if (event_sent_list[i].kind != event_recv_list[i].kind)
        std::cout << "[ kind ] [ " << i << " ] sent " << event_sent_list[i].kind
                  << ", but received " << event_recv_list[i].kind << "\n";
      REQUIRE(event_sent_list[i].where == event_recv_list[i].where);
      REQUIRE(event_sent_list[i].what == event_recv_list[i].what);
      REQUIRE(event_sent_list[i].kind == event_recv_list[i].kind);
    }
  }

  REQUIRE(event_sent_list.size() == event_recv_list.size());
};
