#pragma once

/* REQUIRE */
#include <catch2/catch_test_macros.hpp>
/* vector */
#include <vector>
/* abs */
#include <cmath>
/* iota */
#include <numeric>
/* cout,
   boolalpha,
   endl */
#include <iostream>
/* string,
   to_string */
#include <string>
/* strcmp */
#include <cstring>
/* path, exists */
#include <filesystem>
/* regular_file_store_path */
#include <test_watcher/constant.hpp>
/* watch,
   event */
#include <watcher/watcher.hpp>

namespace wtr {
namespace test_watcher {

bool str_eq(auto a, auto b)
{
  return std::strcmp(a, b) == 0;
}

static void show_event_stream_preamble()
{
  std::cout << "{\"wtr.test_watcher\":"
            << "\n{\"stream\":\n{" << std::endl;
}

static void show_event_stream_postamble(auto alive_for_ms, bool is_watch_dead)
{
  std::cout << "}\n,\"milliseconds\":" << alive_for_ms
            << "\n,\"dead\":" << std::boolalpha << is_watch_dead << "}}"
            << std::endl;
}

static void show_strange_event(auto& title,
                               wtr::watcher::event::event const& ev)
{
  std::cout << "warning in " << title << ":"
            << "\n strange event at: " << ev.where << "\n json: {" << ev
            << "\n\n";
}

static void test_regular_file_event_handling(
    wtr::watcher::event::event const& this_event)
{
  using namespace wtr::watcher;

  /* Print first */
  this_event.kind != event::kind::watcher
      ? std::cout << this_event << "," << std::endl
      : std::cout << this_event << std::endl;

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
    // REQUIRE(std::string(this_event.where).starts_with("s/"));
  }
}

static void test_directory_event_handling(
    wtr::watcher::event::event const& this_event)
{
  using namespace wtr::watcher;

  /* Print first */
  this_event.kind != event::kind::watcher
      ? std::cout << this_event << "," << std::endl
      : std::cout << this_event << std::endl;

  if (this_event.kind != event::kind::watcher) {
    if (this_event.kind != event::kind::dir) {
      show_strange_event(__FUNCTION__, this_event);

      // REQUIRE(str_eq(std::filesystem::path(this_event.where).filename().c_str(),
      //                dir_store_path.filename().c_str()));
    } else {
      REQUIRE(this_event.kind == event::kind::dir);
    }
  } else {
    /* - "s/" means "success"
       - "e/" means "error" */
    // REQUIRE(std::string(this_event.where).starts_with("s/"));
  }
}

/* Make Events
   Mirror what the Watcher should see
   Half are creation events
   Half are destruction */
auto mk_events(auto watch_path, auto path_count, auto& event_list,
               auto should_reverse) -> void
{
  using namespace wtr::watcher;
  using std::vector, std::iota, std::abs, std::filesystem::exists,
      std::ofstream, std::to_string, std::cout, std::endl;

  /* - The path count must be even
     - The path count must be greater than 1 */
  REQUIRE(path_count % 2 == 0);
  REQUIRE(path_count > 1);

  vector<int> path_indices(path_count);
  if (should_reverse)
    iota(path_indices.rbegin(), path_indices.rend(), -path_count / 2);
  else
    iota(path_indices.begin(), path_indices.end(), -path_count / 2);

  for (auto& i : path_indices) {
    if (should_reverse ? i >= 0 : i < 0) {
      auto const path = watch_path + "/" + to_string(abs(i) - 0);
      cout << "creating " << path << endl;
      event_list.emplace_back(
          event::event{path, event::what::create, event::kind::file});
      ofstream{path};
      REQUIRE(exists(path));
    } else {
      auto const path = watch_path + "/" + to_string(abs(i) - 1);
      cout << "destroying " << path << endl;
      event_list.emplace_back(
          event::event{path, event::what::destroy, event::kind::file});
      remove(path.c_str());
      REQUIRE(!exists(path));
    }
  }
}

inline auto mk_events(auto watch_path, auto path_count, auto& event_list)
{
  return mk_events(watch_path, path_count, event_list, true);
}

inline auto mk_revents(auto watch_path, auto path_count, auto& event_list)
{
  return mk_events(watch_path, path_count, event_list, false);
}

} /* namespace test_watcher */
} /* namespace wtr */
