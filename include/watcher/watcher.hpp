#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <watcher/concepts.hpp>
#include <watcher/platform.hpp>
#include <watcher/adapter.hpp>
#include <watcher/status.hpp>

namespace water {

namespace watcher {

using namespace concepts;

// anonymous namespace
// for "private" variables
// (via internal linkage)
namespace {}

/* @brief watcher/run
 * @param closure (optional):
 *  A callback to perform when the files
 *  being watched change.
 *  @see Callback
 * Monitors `path_to_watch` for changes.
 * Executes the given closure when they
 * happen. */
template <const auto delay_ms = 16>
bool run(const Path auto& path,
         const Callback auto& callback) requires
    std::is_integral_v<decltype(delay_ms)> {
      return adapter::run<delay_ms>(path, callback);
}

}  // namespace watcher
}  // namespace water
