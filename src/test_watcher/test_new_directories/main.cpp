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
TEST_CASE("New Directories", "[new_directories]")
{
  using namespace wtr::watcher;

  static constexpr auto path_count = 10;

  static auto event_recv_list = std::vector<wtr::watcher::event::event>{};
  static auto event_recv_list_mtx = std::mutex{};
  static auto cout_mtx = std::mutex{};

  static auto event_list = std::vector<wtr::watcher::event::event>{};
  static auto watch_path_list = std::vector<std::string>{};

  auto const base_store_path = wtr::test_watcher::test_store_path;
  auto const store_path_first = base_store_path / "new_directories_first_store";
  auto const store_path_second
      = base_store_path / "new_directories_second_store";
  auto const store_path_list = std::vector<std::string>{
      base_store_path, store_path_first, store_path_second};

  std::filesystem::create_directory(base_store_path);

  std::thread([&]() {
    auto ok = (wtr::watcher::watch(
        base_store_path, [&](wtr::watcher::event::event const& ev) {
          cout_mtx.lock();
          std::cout << "test @ '" << base_store_path << "' @ live -> recv\n => "
                    << ev << "\n\n";
          cout_mtx.unlock();

          event_recv_list_mtx.lock();
          auto ok_add = true;
          for (auto const& p : watch_path_list)
            if (ev.where == p) ok_add = false;
          if (ok_add) event_recv_list.push_back(ev);
          event_recv_list_mtx.unlock();
        }));
    REQUIRE(ok);
  }).detach();

  for (auto const& p : store_path_list) std::filesystem::create_directory(p);

  for (auto const& p : store_path_list)
    std::thread([&]() {
      auto ok
          = (wtr::watcher::watch(p, [&](wtr::watcher::event::event const& ev) {
              cout_mtx.lock();
              std::cout << "test @ '" << p << "' @ live -> recv\n => " << ev
                        << "\n\n";
              cout_mtx.unlock();

              event_recv_list_mtx.lock();
              auto ok_add = true;
              for (auto const& p : watch_path_list)
                if (ev.where == p) ok_add = false;
              if (ok_add) event_recv_list.push_back(ev);
              event_recv_list_mtx.unlock();
            }));
      REQUIRE(ok);
    }).detach();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  for (int i = 0; i < path_count; ++i) {
    for (auto const& p : store_path_list) {
      auto const new_dir_path = p + "/dir" + std::to_string(i);
      std::filesystem::create_directory(new_dir_path);
      REQUIRE(std::filesystem::exists(new_dir_path));
      event_list.push_back(wtr::watcher::event::event{
          new_dir_path, wtr::watcher::event::what::create,
          wtr::watcher::event::kind::dir});

      auto const new_file_path = new_dir_path + "/file.txt";
      std::ofstream{new_file_path};
      REQUIRE(std::filesystem::exists(new_file_path));
      event_list.push_back(wtr::watcher::event::event{
          new_file_path, wtr::watcher::event::what::create,
          wtr::watcher::event::kind::file});
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  /* std::filesystem::remove_all(base_store_path); */

  /* std::this_thread::sleep_for(std::chrono::milliseconds(1000)); */

  for (auto const& p : store_path_list)
    wtr::watcher::die(p, [](wtr::watcher::event::event const& ev) {
      cout_mtx.lock();
      std::cout << "test @ die -> recv\n => " << ev << "\n\n";
      cout_mtx.unlock();

      event_recv_list_mtx.lock();
      event_recv_list.push_back(ev);
      event_recv_list_mtx.unlock();
    });

  /* auto [event_list_first, event_recv_list_second] =
   * wtr::test_watcher::watch_gather( */
  /*     "New Directories", store_path, path_count); */

  for (event::event const& ev : event_list) {
    std::cout << "event => " << ev.where << "\n";
  }
  for (event::event const& ev : event_recv_list) {
    std::cout << "event recv => " << ev.where << "\n";
  }

  REQUIRE(true);
};
