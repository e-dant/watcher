#pragma once

#include <catch2/catch_test_macros.hpp> /* REQUIRE */
#include <string>                       /* string, strcmp */
#include <watcher/watcher.hpp>          /* watch, event */

// #include <catch2/benchmark/catch_benchmark.hpp>
// #include <chrono>
// #include <cstdio>
// #include <cstring>
// #include <filesystem>
// #include <fstream>
// #include <iostream>
// #include <ratio>
// #include <thread>

bool str_eq(auto a, auto b)
{
  return std::strcmp(a, b) == 0;
}

void show_strange_event(auto& title, wtr::watcher::event::event const& ev)
{
  std::cout << "warning in " << title << ":"
            << "\n strange event at: " << ev.where << "\n json: {" << ev
            << "\n\n";
}

inline void test_regular_file_event_handling(
    wtr::watcher::event::event const& this_event)
{
  using namespace wtr::watcher;

  /* Print first */
  // this_event.kind != event::kind::watcher
  //     ? std::cout << this_event << "," << std::endl
  //     : std::cout << this_event << std::endl;

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
    REQUIRE(std::string(this_event.where).starts_with("s/"));
  }
}

inline void test_directory_event_handling(
    wtr::watcher::event::event const& this_event)
{
  using namespace wtr::watcher;

  /* Print first */
  // this_event.kind != event::kind::watcher
  //     ? std::cout << this_event << "," << std::endl
  //     : std::cout << this_event << std::endl;

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
    REQUIRE(std::string(this_event.where).starts_with("s/"));
  }
}
