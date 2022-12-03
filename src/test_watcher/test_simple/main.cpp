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
TEST_CASE("Simple", "[simple]")
{
  using namespace wtr::watcher;

  static constexpr auto path_count = 3;

  static auto event_recv_list = std::vector<event::event>{};
  static auto event_recv_list_mtx = std::mutex{};
  static auto cout_mtx = std::mutex{};

  static auto event_sent_list = std::vector<event::event>{};

  static auto watch_path_list = std::vector<std::string>{};

  auto const base_store_path = wtr::test_watcher::test_store_path;
  static auto const store_path = base_store_path / "simple_store";

  std::filesystem::create_directories(store_path);
  REQUIRE(std::filesystem::exists(base_store_path)
          && std::filesystem::exists(store_path));

  event_sent_list.push_back({"s/self/live@" + store_path.string(),
                             event::what::create, event::kind::watcher});

  auto watch_handle = std::async(std::launch::async, []() {
    auto const watch_ok
        = wtr::watcher::watch(store_path, [](event::event const& ev) {
            cout_mtx.lock();
            std::cout << "test @ '" << store_path << "' @ live -> recv\n => "
                      << ev << "\n\n";
            cout_mtx.unlock();

            event_recv_list_mtx.lock();
            event_recv_list.push_back(ev);
            event_recv_list_mtx.unlock();
          });
    return watch_ok;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  for (int i = 0; i < path_count; ++i) {
    auto const new_dir_path = store_path / ("new_dir" + std::to_string(i));
    std::filesystem::create_directory(new_dir_path);

    REQUIRE(std::filesystem::is_directory(new_dir_path));

    event_sent_list.push_back(
        {new_dir_path, event::what::create, event::kind::dir});

    auto const new_file_path
        = store_path / ("new_file" + std::to_string(i) + ".txt");
    std::ofstream(new_file_path).close();

    REQUIRE(std::filesystem::is_regular_file(new_file_path));

    event_sent_list.push_back(
        {new_file_path, event::what::create, event::kind::file});

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  event_sent_list.push_back({"s/self/live@" + store_path.string(),
                             event::what::create, event::kind::watcher});

  auto const die_ok
      = wtr::watcher::die(store_path, [&](event::event const& ev) {
          cout_mtx.lock();
          std::cout << "test @ '" << store_path << "' @ die -> recv\n => " << ev
                    << "\n\n";
          cout_mtx.unlock();

          event_recv_list_mtx.lock();
          event_recv_list.push_back(ev);
          event_recv_list_mtx.unlock();
        });

  REQUIRE(die_ok);
  REQUIRE(watch_handle.get());

  std::cout << "events sent =>\n";
  for (auto const& ev : event_sent_list) {
    std::cout << " " << ev.where << ",\n";
  }

  std::cout << "events recv =>\n";
  for (auto const& ev : event_recv_list) {
    std::cout << " " << ev.where << ",\n";
  }

  std::filesystem::remove_all(base_store_path);
  REQUIRE(!std::filesystem::exists(base_store_path));
};
