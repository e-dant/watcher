#pragma once

/* cout,
   endl */
#include <iostream>
/* round */
#include <cmath>
/* move */
#include <utility>
/* prior_fs_events_clear_milliseconds,
   death_after_test_milliseconds */
#include <test_watcher/constant.hpp>

namespace wtr {
namespace test_watcher {

auto seperator(auto test_name, char symbol = '-')
{
  using std::string, std::round;

  auto len = 79; /* same as catch2 */
  auto title = "[ watcher/test/" + string(test_name) + " ]";
  auto gap_len = round((len - title.length()) / 2);
  auto side_bar = string(gap_len, symbol);
  auto&& full = side_bar + title + side_bar;
  return std::move(full);
}

auto show_conf(auto test_name, auto test_store_path, auto watch_path)
{
  using std::cout, std::endl,
      wtr::test_watcher::prior_fs_events_clear_milliseconds,
      wtr::test_watcher::death_after_test_milliseconds;

  cout << seperator(test_name) << endl
       << test_name << ":" << endl
       << " test_store_path: " << test_store_path.string() << endl
       << " watch_path: " << watch_path << endl
       << " prior_fs_events_clear_milliseconds: "
       << prior_fs_events_clear_milliseconds.count() << endl
       << " death_after_test_milliseconds: "
       << death_after_test_milliseconds.count() << endl;
}

} /* namespace test_watcher */
} /* namespace wtr */
