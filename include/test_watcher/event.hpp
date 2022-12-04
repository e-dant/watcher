#pragma once

/* assert */
#include <cassert>
/* vector */
#include <vector>
/* abs */
#include <cmath>
/* iota */
#include <numeric>
/* ofstream */
#include <fstream>
/* string,
   to_string */
#include <string>
/* strcmp */
#include <cstring>
/* path,
   remove,
   exists */
#include <filesystem>
/* mk_events_options */
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

/* Make Events
   Mirror what the Watcher should see
   Half are creation events
   Half are destruction */
auto mk_events(std::filesystem::path const& base_path, auto const& path_count,
               std::vector<wtr::watcher::event::event>& event_list,
               unsigned long options = mk_events_options) -> void
{
  using namespace wtr::watcher;
  using std::iota, std::abs, std::filesystem::exists;

  /* - The path count must be even
     - The path count must be greater than 1 */
  assert(path_count % 2 == 0);
  assert(path_count > 1);

  std::vector<int> path_indices(path_count);

  if (options & mk_events_reverse)
    iota(path_indices.rbegin(), path_indices.rend(), -path_count / 2);
  else
    iota(path_indices.begin(), path_indices.end(), -path_count / 2);

  auto ev = event::event{"s/self/live@" / base_path, event::what::destroy,
                         event::kind::watcher};
  event_list.push_back(ev);
  auto has = false;
  for (auto& i : event_list)
    if (i == ev) has = true;
  assert(has);

  if (options & mk_events_die_before) {
    auto ev = event::event{"s/self/die@" / base_path, event::what::destroy,
                           event::kind::watcher};
    event_list.push_back(ev);
    auto has = false;
    for (auto& i : event_list)
      if (i == ev) has = true;
    assert(has);
  }

  for (auto& i : path_indices) {
    auto const path = base_path / std::to_string(i < 0 ? abs(i) - 1 : abs(i));
    if ((options & mk_events_reverse) ? i >= 0 : i < 0) {
      auto ev = event::event{path, event::what::create, event::kind::file};
      event_list.push_back(ev);
      std::ofstream{path}; /* NOLINT */
      auto has = false;
      for (auto& i : event_list)
        if (i == ev) has = true;
      assert(has);
      assert(std::filesystem::exists(path));
    } else {
      auto ev = event::event{path, event::what::destroy, event::kind::file};
      event_list.push_back(ev);
      std::filesystem::remove(path);
      auto has = false;
      for (auto& i : event_list)
        if (i == ev) has = true;
      assert(has);
      assert(!std::filesystem::exists(path));
    }
  }

  if (options & mk_events_die_after) {
    auto ev = event::event{"s/self/die@" / base_path, event::what::destroy,
                           event::kind::watcher};
    event_list.push_back(ev);
    auto has = false;
    for (auto& i : event_list)
      if (i == ev) has = true;
    assert(has);
  }
}

inline auto mk_revents(auto const& watch_path, auto const& path_count,
                       auto const& event_list,
                       unsigned long options = mk_events_options)
{
  return mk_events(watch_path, path_count, event_list,
                   options |= mk_events_reverse);
}

} /* namespace test_watcher */
} /* namespace wtr */
