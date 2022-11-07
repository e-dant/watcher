#pragma once

/* REQUIRE */
#include <catch2/catch_test_macros.hpp>
/* cout,
   boolalpha,
   endl */
#include <string>
/* string,
   strcmp */
#include <string>
/* path */
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
  std::cout << "{\"test.wtr.watcher\":"
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

static void thread_watch(std::filesystem::path watch_path, auto& event_handler)
{
  auto const p = watch_path.string();
  // #ifdef WATER_WATCHER_PLATFORM_WINDOWS_ANY
  //       = watch_path.string();
  // #else
  //       = watch_path;
  // #endif

  std::thread([&]() {
    wtr::watcher::watch(p.c_str(), event_handler);
  }).detach();
}

} /* namespace test_watcher */
} /* namespace wtr */