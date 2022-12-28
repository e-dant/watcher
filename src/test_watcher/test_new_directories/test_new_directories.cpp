/*
   Test Watcher
   New Directories
*/

/* REQUIRE,
   TEST_CASE */
#include <snatch/snatch.hpp>
/* event */
#include <watcher/watcher.hpp>
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
/* this_thread */
#include <thread>
/* vector */
#include <vector>
/* string */
#include <string>
/* milliseconds */
#include <chrono>
/* mutex */
#include <mutex>
/* path,
   create,
   remove_all */
#include <filesystem>

/* Test that files are scanned */
TEST_CASE("New Directories", "[new_directories]")
{
  using namespace wtr::watcher;

  static constexpr auto path_count = 10;

  static auto event_recv_list = std::vector<wtr::watcher::event::event>{};
  static auto event_recv_list_mtx = std::mutex{};

  static auto event_sent_list = std::vector<wtr::watcher::event::event>{};
  static auto watch_path_list = std::vector<std::filesystem::path>{};

  auto const base_store_path = wtr::test_watcher::test_store_path;
  auto const store_path_first = base_store_path / "new_directories_first_store";
  auto const store_path_second
      = base_store_path / "new_directories_second_store";
  auto const store_path_list = std::vector<std::filesystem::path>{
      base_store_path, store_path_first, store_path_second};

  static constexpr auto title = "New Directories";

  std::cout << title << std::endl;

  std::filesystem::create_directory(base_store_path);

  event_sent_list.push_back(
      {std::string("s/self/live@").append(base_store_path),
       wtr::watcher::event::what::create, wtr::watcher::event::kind::watcher});

  auto watcher_future = (std::async(std::launch::async, [&base_store_path]() {
    auto const watch_ok = wtr::watcher::watch(
        base_store_path, [&](wtr::watcher::event::event const& ev) {
          auto _ = std::scoped_lock{event_recv_list_mtx};
          event_recv_list.push_back(ev);
        });
    return watch_ok;
  }));

  for (auto const& p : store_path_list) std::filesystem::create_directory(p);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (int i = 0; i < path_count; i++) {
    for (auto const& p : store_path_list) {
      auto const new_dir_path = p / ("dir" + std::to_string(i));
      std::filesystem::create_directory(new_dir_path);
      REQUIRE(std::filesystem::exists(new_dir_path));
      event_sent_list.push_back(wtr::watcher::event::event{
          new_dir_path, wtr::watcher::event::what::create,
          wtr::watcher::event::kind::dir});

      /* @todo
         This sleep is hiding a bug for the Linux adapters.
         It might on either our end -- something about how
         we're processing batches of events -- or it could
         be on the kernel's end... just dropping events.
         We should investigate and fix it if we can. */
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      auto const new_file_path = new_dir_path / "file.txt";
      std::ofstream{new_file_path}; /* NOLINT */
      REQUIRE(std::filesystem::exists(new_file_path));
      event_sent_list.push_back(wtr::watcher::event::event{
          new_file_path, wtr::watcher::event::what::create,
          wtr::watcher::event::kind::file});

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  event_sent_list.push_back({std::string("s/self/die@").append(base_store_path),
                             wtr::watcher::event::what::destroy,
                             wtr::watcher::event::kind::watcher});

  wtr::watcher::die(base_store_path,
                    [&base_store_path](wtr::watcher::event::event const& ev) {
                      auto _ = std::scoped_lock{event_recv_list_mtx};
                      event_recv_list.push_back(ev);
                    });

  REQUIRE(watcher_future.get());

  std::filesystem::remove_all(base_store_path);
  REQUIRE(!std::filesystem::exists(base_store_path));

  for (size_t i = 0;
       i < std::min(event_sent_list.size(), event_recv_list.size()); ++i)
  {
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
