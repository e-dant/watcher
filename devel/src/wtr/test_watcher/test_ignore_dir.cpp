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

/* Test that should_skip ignores specified directories */

TEST_CASE("Ignored Paths", "[dir][ignore][filter]")
{
  namespace fs = std::filesystem;
  using namespace std::chrono_literals;
  using namespace wtr::watcher;
  using namespace wtr::test_watcher;

  auto const tmpdir = make_local_tmp_dir();
  auto const ignored_dir = tmpdir / "ignored";
  auto const watched_dir = tmpdir / "watched";
  auto event_recv_list = std::vector<event>{};
  auto event_recv_list_mtx = std::mutex{};
  auto event_sent_list = std::vector<event>{};

  REQUIRE(fs::exists(tmpdir) || fs::create_directory(tmpdir));
  REQUIRE(fs::create_directory(ignored_dir));
  REQUIRE(fs::create_directory(watched_dir));

  std::this_thread::sleep_for(10ms);

  event_sent_list.push_back(
    {std::string("s/self/live@").append(tmpdir.string()),
     event::effect_type::create,
     event::path_type::watcher});

  auto watcher = watch(
    tmpdir,
    [&event_recv_list_mtx, &event_recv_list](event const& ev) {
      auto _ = std::scoped_lock{event_recv_list_mtx};
      event_recv_list.push_back(ev);
    },
    {ignored_dir.string()});

  std::this_thread::sleep_for(100ms);

  // Create a file in the ignored directory
  auto ignored_file = ignored_dir / "file.txt";
  std::ofstream(ignored_file).close();
  REQUIRE(fs::is_regular_file(ignored_file));

  // Create a file in the watched directory
  auto watched_file = watched_dir / "file.txt";
  std::ofstream(watched_file).close();
  REQUIRE(fs::is_regular_file(watched_file));

  event_sent_list.push_back(
    {watched_file, event::effect_type::create, event::path_type::file});

  // Wait for events
  for (int i = 0;; i++) {
    std::this_thread::sleep_for(10ms);
    auto _ = std::scoped_lock<std::mutex>{event_recv_list_mtx};
    if (event_recv_list.size() >= event_sent_list.size()) break;
    if (i > 1000) REQUIRE(! "Timeout: Waited more than one second for results");
  }

  event_sent_list.push_back(
    {std::string("s/self/die@").append(tmpdir.string()),
     event::effect_type::destroy,
     event::path_type::watcher});

  REQUIRE(watcher.close() == true);
  REQUIRE(! fs::exists(tmpdir) || fs::remove_all(tmpdir));

  // Check that no event for the ignored file is present
  for (auto const& ev : event_recv_list) {
    REQUIRE(ev.path_name != ignored_file);
    REQUIRE(ev.path_name != ignored_dir);
  }

  // Only events for watched_file and watcher status should be present
  check_event_lists_set_eq(event_sent_list, event_recv_list);
}

