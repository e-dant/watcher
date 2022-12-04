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

  static auto cout_mtx = std::mutex{};
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

  std::filesystem::create_directory(base_store_path);

  for (auto const& p : store_path_list) std::filesystem::create_directory(p);

  auto gather_watcher_futures = [&]() {
    std::vector<std::future<bool>> futures;

    futures.push_back(std::async(std::launch::async, [&base_store_path]() {
      auto const watch_ok = wtr::watcher::watch(
          base_store_path, [&](wtr::watcher::event::event const& ev) {
            auto _ = std::scoped_lock{cout_mtx, event_recv_list_mtx};

            std::cout << "test @ " << base_store_path << " @ live -> recv\n => "
                      << ev << "\n\n";

            auto ok_add = true;

            for (auto const& p : watch_path_list)
              if (ev.where == p) ok_add = false;

            if (ok_add) event_recv_list.push_back(ev);
          });
      return watch_ok;
    }));

    for (auto const& p : store_path_list) {
      futures.push_back(std::async(std::launch::async, [&p]() {
        auto const watch_ok
            = wtr::watcher::watch(p, [&](wtr::watcher::event::event const& ev) {
                auto _ = std::scoped_lock{cout_mtx, event_recv_list_mtx};

                std::cout << "test @ " << p << " @ live -> recv\n => " << ev
                          << "\n\n";

                auto ok_add = true;

                for (auto const& p : watch_path_list)
                  if (ev.where == p) ok_add = false;

                if (ok_add) event_recv_list.push_back(ev);
              });
        return watch_ok;
      }));
    }

    return futures;
  }();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  for (int i = 0; i < path_count; i++) {
    for (auto const& p : store_path_list) {
      auto const new_dir_path = p / ("dir" + std::to_string(i));
      std::filesystem::create_directory(new_dir_path);
      REQUIRE(std::filesystem::exists(new_dir_path));
      event_sent_list.push_back(wtr::watcher::event::event{
          new_dir_path, wtr::watcher::event::what::create,
          wtr::watcher::event::kind::dir});

      auto const new_file_path = new_dir_path / "file.txt";
      std::ofstream{new_file_path}; /* NOLINT */
      REQUIRE(std::filesystem::exists(new_file_path));
      event_sent_list.push_back(wtr::watcher::event::event{
          new_file_path, wtr::watcher::event::what::create,
          wtr::watcher::event::kind::file});
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  wtr::watcher::die(base_store_path,
                    [&base_store_path](wtr::watcher::event::event const& ev) {
                      auto _ = std::scoped_lock{cout_mtx, event_recv_list_mtx};

                      std::cout << "test @ " << base_store_path
                                << " @ die -> recv\n => " << ev << "\n\n";

                      event_recv_list.push_back(ev);
                    });
  for (auto const& p : store_path_list)
    wtr::watcher::die(p, [&p](wtr::watcher::event::event const& ev) {
      auto _ = std::scoped_lock{cout_mtx, event_recv_list_mtx};

      std::cout << "test @ " << p << " @ die -> recv\n => " << ev << "\n\n";

      event_recv_list.push_back(ev);
    });

  for (auto& f : gather_watcher_futures) REQUIRE(f.get());

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
