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

auto seperator(auto const& test_name, char const symbol = '-')
{
  using std::string, std::round;

  auto len = 79; /* same as catch2 */
  auto title = "[ watcher/test/" + string(test_name) + " ]";
  auto gap_len = round((len - title.length()) / 2);
  auto side_bar = string(gap_len, symbol);
  auto&& full = side_bar + title + side_bar;
  return std::move(full);
}

auto show_conf(auto const& test_name, auto const& test_store_path,
               auto const&... watch_paths)
{
  using std::cout, std::string,
      wtr::test_watcher::prior_fs_events_clear_milliseconds,
      wtr::test_watcher::death_after_test_milliseconds;

  auto wps = string{watch_paths...};

  cout << seperator(test_name) << "\n" << test_name << " =>"
       << "\n\n test store path =>\n  " << test_store_path.string()
       << "\n\n watch paths =>\n  " << wps
       << "\n prior fs events clear milliseconds =>\n  "
       << prior_fs_events_clear_milliseconds.count()
       << "\n\n death after test milliseconds =>\n  "
       << death_after_test_milliseconds.count() << "\n";
}

} /* namespace test_watcher */
} /* namespace wtr */
