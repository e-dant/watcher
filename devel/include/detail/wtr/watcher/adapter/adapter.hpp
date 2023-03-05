#pragma once

/*  path */
#include <filesystem>
/*  async
    future */
#include <future>
/*  shared_ptr
    unique_ptr */
#include <memory>
/*  mutex
    scoped_lock */
#include <mutex>
/*  unordered_map */
#include <unordered_map>
/*  watch
    event
    callback */
#include <wtr/watcher.hpp>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {

struct future {
  using shared = std::shared_ptr<future>;
  using evw = ::wtr::watcher::event::what;
  using evk = ::wtr::watcher::event::kind;

  mutable std::mutex lk{};
  bool closed{false};
  std::future<bool> work{};
};

auto open(std::filesystem::path const& path,
          ::wtr::watcher::event::callback const& callback) noexcept
  -> future::shared
{
  using evw = ::wtr::watcher::event::what;
  using evk = ::wtr::watcher::event::kind;

  auto fut = std::make_shared<future>();

  callback({"s/self/live@" + path.string(), evw::create, evk::watcher});

  fut->work = std::async(std::launch::async,
                         [path, callback, fut]() noexcept -> bool
                         {
                           return watch(path,
                                        callback,
                                        [fut]() noexcept -> bool
                                        {
                                          auto _ = std::scoped_lock{fut->lk};
                                          return ! fut->closed;
                                        });
                         });

  return fut;
};

auto close(future::shared const& fut) noexcept -> bool
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
