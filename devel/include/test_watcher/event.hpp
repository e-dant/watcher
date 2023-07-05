#pragma once

#include "snitch/snitch.hpp"
#include "test_watcher/constant.hpp"
#include "wtr/watcher.hpp"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

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
auto mk_events(std::filesystem::path const& base_path,
               auto const& path_count,
               std::vector<wtr::watcher::event>* event_list,
               unsigned long options = mk_events_options) -> void
{
  using namespace wtr::watcher;
  using std::iota, std::abs, std::filesystem::exists;

  /* - The path count must be even
     - The path count must be greater than 1 */
  REQUIRE(path_count % 2 == 0);
  REQUIRE(path_count > 1);

  std::vector<int> path_indices(path_count);

  if (options & mk_events_reverse)
    iota(path_indices.rbegin(), path_indices.rend(), -path_count / 2);
  else
    iota(path_indices.begin(), path_indices.end(), -path_count / 2);

  for (auto& i : path_indices) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto const path = base_path / std::to_string(i < 0 ? abs(i) - 1 : abs(i));
    if ((options & mk_events_reverse) ? i >= 0 : i < 0) {
      event_list->push_back(
        wtr::event{path, wtr::event::what::create, wtr::event::kind::file});
      auto _ = std::ofstream{path};
    }
    else {
      event_list->push_back(
        wtr::event{path, wtr::event::what::destroy, wtr::event::kind::file});
      std::filesystem::remove(path);
    }
  }

  if (options & mk_events_die_after)
    event_list->push_back(
      wtr::event{std::string("s/self/die@").append(base_path.string()),
                 wtr::event::what::destroy,
                 wtr::event::kind::watcher});
}

inline auto mk_revents(auto const& watch_path,
                       auto const& path_count,
                       auto const& event_list,
                       unsigned long options = mk_events_options)
{
  return mk_events(watch_path,
                   path_count,
                   event_list,
                   options |= mk_events_reverse);
}

inline auto check_event_lists_eq(auto const& event_sent_list,
                                 auto const& event_recv_list)
{
  auto const max_i = event_sent_list.size() > event_recv_list.size()
                     ? event_recv_list.size()
                     : event_sent_list.size();
  for (size_t i = 0; i < max_i; ++i) {
    if (event_sent_list[i].kind != wtr::watcher::event::kind::watcher) {
      if (event_sent_list[i].where != event_recv_list[i].where)
        std::cerr << "Bad .where field... (" << event_sent_list[i].where
                  << " vs " << event_recv_list[i].where << "):"
                  << "\n Sent:\n " << event_sent_list[i] << "\n Received:\n "
                  << event_recv_list[i] << "\n";
      if (event_sent_list[i].what != event_recv_list[i].what)
        std::cerr << "Bad .what field... (" << event_sent_list[i].what << " vs "
                  << event_recv_list[i].what << "):"
                  << "\n Sent:\n " << event_sent_list[i] << "\n Received:\n "
                  << event_recv_list[i] << "\n";
      if (event_sent_list[i].kind != event_recv_list[i].kind)
        std::cerr << "Bad .kind field... (" << event_sent_list[i].kind << " vs "
                  << event_recv_list[i].kind << "):"
                  << "\n Sent:\n " << event_sent_list[i] << "\n Received:\n "
                  << event_recv_list[i] << "\n";
      CHECK(event_sent_list[i].where == event_recv_list[i].where);
      CHECK(event_sent_list[i].what == event_recv_list[i].what);
      CHECK(event_sent_list[i].kind == event_recv_list[i].kind);
    }
  }

  std::cerr << "Last event in sent list: " << event_sent_list.back() << "\n";
  std::cerr << "Last event in recv list: " << event_recv_list.back() << "\n";

  CHECK(event_sent_list.size() == event_recv_list.size());
}

inline auto check_event_lists_eq(auto const& lists)
{
  auto [event_sent_list, event_recv_list] = lists;
  check_event_lists_eq(event_sent_list, event_recv_list);
}

} /* namespace test_watcher */
} /* namespace wtr */
