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
/*  mt19937
    random_device
    uniform_int_distribution */
#include <random>
/*  unordered_map */
#include <thread>
#include <unordered_map>
/*  watch
    event
    callback */
#include <wtr/watcher.hpp>

namespace detail {
namespace wtr {
namespace watcher {
namespace adapter {

struct adapter_t {
  using evw = ::wtr::watcher::event::what;
  using evk = ::wtr::watcher::event::kind;

  mutable std::mutex lk{};
  bool closed{false};
  std::future<bool> work{};
};

auto open(std::filesystem::path const& path,
          ::wtr::watcher::event::callback const& callback) noexcept
  -> std::shared_ptr<adapter_t>
{
  using evw = ::wtr::watcher::event::what;
  using evk = ::wtr::watcher::event::kind;

  auto a = std::make_shared<adapter_t>();

  callback({"s/self/live@" + path.string(), evw::create, evk::watcher});

  a->work = std::async(std::launch::async,
                       [path, callback, a]() noexcept -> bool
                       {
                         return watch(path,
                                      callback,
                                      [a]() noexcept -> bool
                                      {
                                        auto _ = std::scoped_lock{a->lk};
                                        return ! a->closed;
                                      });
                       });

  return a;
};

auto close(std::shared_ptr<adapter_t> const& a) noexcept -> bool
{
  if (! a->closed) {
    {
      auto _ = std::scoped_lock{a->lk};
      a->closed = true;
    }
    return a->work.get();
  }

  else
    return false;
};

}  // namespace adapter
}  // namespace watcher
}  // namespace wtr
}  // namespace detail
