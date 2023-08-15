#pragma once

#include "wtr/watcher.hpp"
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {

struct future {
  using shared = std::shared_ptr<future>;

  mutable std::mutex lk{};
  std::future<bool> work{};
  bool closed{false};
};

inline auto open(
  std::filesystem::path const& path,
  ::wtr::watcher::event::callback const& callback) noexcept -> future::shared
{
  auto fut = std::make_shared<future>();

  callback(
    {"s/self/live@" + path.string(),
     ::wtr::watcher::event::what::create,
     ::wtr::watcher::event::kind::watcher});

  fut->work = std::async(
    std::launch::async,
    [path, callback, fut]() noexcept -> bool
    {
      return watch(
        path,
        callback,
        [fut]() noexcept -> bool
        {
          auto _ = std::scoped_lock{fut->lk};
          return ! fut->closed;
        });
    });

  return fut;
};

inline auto close(future::shared const& fut) noexcept -> bool
{
  if (! fut->closed) {
    {
      auto _ = std::scoped_lock{fut->lk};
      fut->closed = true;
    }
    return fut->work.get();
  }

  else
    return false;
};

}  // namespace adapter
}  // namespace watcher
}  // namespace wtr
}  // namespace detail
