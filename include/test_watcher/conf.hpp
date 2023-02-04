#pragma once

/* round */
#include <cmath>
/* move */
#include <utility>

namespace wtr {
namespace test_watcher {

auto seperator(auto const& test_name, char const symbol = '-') {
  using std::string, std::round;

  auto len = 79; /* same as catch2 */
  auto title = "[ watcher/test/" + string(test_name) + " ]";
  auto gap_len = round((len - title.length()) / 2);
  auto side_bar = string(gap_len, symbol);
  auto&& full = side_bar + title + side_bar;
  return std::move(full);
}

} /* namespace test_watcher */
} /* namespace wtr */
