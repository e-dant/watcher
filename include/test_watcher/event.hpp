#pragma once

/* assert */
#include <cassert>
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

inline bool str_eq(char const* a, char const* b)
{
  return std::strcmp(a, b) == 0;
}

static void show_event_stream_preamble()
{
  std::cout << "\n{\"wtr\":"
               "\n{\"test_watcher\":"
               "\n{\"stream\":"
               "\n{\n";
}

static void show_event_stream_postamble(auto alive_for_ms, bool is_watch_dead)
{
  std::cout << "}"
            << "\n,\"milliseconds\":" << alive_for_ms
            << "\n,\"dead\":" << std::boolalpha << is_watch_dead << "\n}}}"
            << std::endl;
}

static void show_strange_event(auto& title,
                               wtr::watcher::event::event const& ev)
{
  std::cout << "warning in " << title << ":"
            << "\n strange event at: " << ev.where << "\n json: {" << ev
            << "\n\n";
}

/* Make Events
   Mirror what the Watcher should see
   Half are creation events
   Half are destruction */
static auto mk_events(auto watch_path, auto path_count,
                      std::vector<wtr::watcher::event::event>& event_list,
                      unsigned long options = mk_events_options) -> void
{
  using namespace wtr::watcher;
  using std::vector, std::iota, std::abs, std::filesystem::exists,
      std::ofstream, std::to_string;

  /* - The path count must be even
     - The path count must be greater than 1 */
  assert(path_count % 2 == 0);
  assert(path_count > 1);

  vector<int> path_indices(path_count);

  if (options & mk_events_reverse)
    iota(path_indices.rbegin(), path_indices.rend(), -path_count / 2);
  else
    iota(path_indices.begin(), path_indices.end(), -path_count / 2);

  auto ev = event::event{"s/self/live@" + watch_path, event::what::destroy,
                         event::kind::watcher};
  event_list.push_back(ev);
  auto has = false;
  for (auto& i : event_list)
    if (i == ev) has = true;
  assert(has);

  if (options & mk_events_die_before) {
    auto ev = event::event{"s/self/die@" + watch_path, event::what::destroy,
                           event::kind::watcher};
    event_list.push_back(ev);
    auto has = false;
    for (auto& i : event_list)
      if (i == ev) has = true;
    assert(has);
  }

  for (auto& i : path_indices) {
    auto const path = watch_path + "/" + to_string(i < 0 ? abs(i) - 1 : abs(i));
    if ((options & mk_events_reverse) ? i >= 0 : i < 0) {
      auto ev = event::event{path, event::what::create, event::kind::file};
      event_list.push_back(ev);
      ofstream{path};
      auto has = false;
      for (auto& i : event_list)
        if (i == ev) has = true;
      assert(has);
      assert(exists(path));
    } else {
      auto ev = event::event{path, event::what::destroy, event::kind::file};
      event_list.push_back(ev);
      remove(path.c_str());
      auto has = false;
      for (auto& i : event_list)
        if (i == ev) has = true;
      assert(has);
      assert(!exists(path));
    }
  }

  if (options & mk_events_die_after) {
    auto ev = event::event{"s/self/die@" + watch_path, event::what::destroy,
                           event::kind::watcher};
    event_list.push_back(ev);
    auto has = false;
    for (auto& i : event_list)
      if (i == ev) has = true;
    assert(has);
  }
}

inline auto mk_revents(auto watch_path, auto path_count, auto& event_list,
                       unsigned long options = mk_events_options)
{
  return mk_events(watch_path, path_count, event_list,
                   options |= mk_events_reverse);
}

} /* namespace test_watcher */
} /* namespace wtr */
