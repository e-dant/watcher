/*
   Test Watcher
   Simple
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
/* thread */
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
   create */
#include <filesystem>

/* Test that files are scanned */
TEST_CASE("Simple", "[simple]")
{
  using namespace wtr::watcher;

  static constexpr auto path_count = 3;

  static auto event_recv_list = std::vector<wtr::watcher::event::event>{};
  static auto event_recv_list_mtx = std::mutex{};
  static auto cout_mtx = std::mutex{};

  static auto event_list = std::vector<wtr::watcher::event::event>{};
  static auto watch_path_list = std::vector<std::string>{};

  auto const base_store_path = wtr::test_watcher::test_store_path;
  auto const store_path = base_store_path / "simple_store";

  std::filesystem::create_directory(base_store_path / store_path);

  auto const watch_ok = wtr::watcher::watch(
      store_path, [&](wtr::watcher::event::event const& ev) {
        cout_mtx.lock();
        std::cout << "test @ '" << store_path << "' @ live -> recv\n => " << ev
                  << "\n\n";
        cout_mtx.unlock();

        event_recv_list_mtx.lock();
        event_recv_list.push_back(ev);
        event_recv_list_mtx.unlock();
      });

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  for (int i = 0; i < path_count; ++i) {
    auto const new_dir_path = store_path / ("dir" + std::to_string(i));
    std::filesystem::create_directory(new_dir_path);
    REQUIRE(std::filesystem::exists(new_dir_path));
    event_list.push_back(wtr::watcher::event::event{
        new_dir_path, wtr::watcher::event::what::create,
        wtr::watcher::event::kind::dir});

    auto const new_file_path
        = new_dir_path / "file" / (std::to_string(i) + ".txt");
    auto f = std::ofstream{new_file_path};
    f.close();
    REQUIRE(std::filesystem::exists(new_file_path));
    event_list.push_back(wtr::watcher::event::event{
        new_file_path, wtr::watcher::event::what::create,
        wtr::watcher::event::kind::file});

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::filesystem::remove_all(base_store_path);

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  auto const die_ok = wtr::watcher::die(
      store_path, [&](wtr::watcher::event::event const& ev) {
        cout_mtx.lock();
        std::cout << "test @ '" << store_path << "' @ die -> recv\n => " << ev
                  << "\n\n";
        cout_mtx.unlock();

        event_recv_list_mtx.lock();
        event_recv_list.push_back(ev);
        event_recv_list_mtx.unlock();
      });

  REQUIRE(watch_ok);
  REQUIRE(die_ok);

  for (event::event const& ev : event_list) {
    std::cout << "event => " << ev.where << "\n";
  }
  for (event::event const& ev : event_recv_list) {
    std::cout << "event recv => " << ev.where << "\n";
  }

  REQUIRE(true);
};
