#pragma once

/* system_clock::now, milliseconds */
#include <chrono>
#include <ratio>

namespace wtr {
namespace test_watcher {

inline auto ms_now()
{
  auto time = std::chrono::system_clock::now();
  auto normalized_time = time.time_since_epoch().count();
  auto ms_time = std::chrono::milliseconds(normalized_time);
  return ms_time;
}

inline auto ms_duration(auto from)
{
  return ms_now().count() - from;
}

inline auto ms_duration(std::chrono::milliseconds from)
{
  return (ms_now() - from).count();
}

} /* namespace test_watcher */
} /* namespace wtr */
