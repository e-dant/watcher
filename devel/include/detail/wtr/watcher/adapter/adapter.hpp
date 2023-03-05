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

#if WTR_WATCHER_O == 1

namespace o {
class adapter {

private:
  using evw = ::wtr::watcher::event::what;
  using evk = ::wtr::watcher::event::kind;

  std::filesystem::path path{};
  ::wtr::watcher::event::callback callback{};
  bool res{false};
  mutable std::mutex lk{};
  mutable bool alive{false};
  mutable std::future<void> future{};

  auto open_async() noexcept -> void
  {
    if (! this->alive) {
      this->alive = true;

      callback(
        {"s/self/live@" + this->path.string(), evw::create, evk::watcher});

      this->future =
        std::async(std::launch::async,
                   [this]() noexcept
                   {
                     this->res =
                       watch(this->path,
                             this->callback,
                             [this]() noexcept -> bool
                             {
                               auto _ = std::scoped_lock<std::mutex>{this->lk};
                               return this->alive;
                             });
                   });
    }

    else
      callback({"e/self/already_alive@" + this->path.string(),
                evw::create,
                evk::watcher});
  }

public:
  adapter(std::filesystem::path const& path,
          ::wtr::watcher::event::callback const& callback) noexcept
      : path{path},
        callback{callback}
  {
    using namespace std::chrono_literals;

    this->open_async();

    // condition variable, this should be...
    while (! this->alive) std::this_thread::sleep_for(1us);
  };

  adapter(adapter const&) = delete;
  adapter(adapter&&) = delete;
  adapter& operator=(adapter const&) = delete;
  adapter& operator=(adapter&&) = delete;

  ~adapter() noexcept = default;  // { this->close(); };

  auto close() const noexcept -> bool
  {
    using namespace std::chrono_literals;

    while (! this->lk.try_lock())
      ;

    if (this->alive) {
      this->alive = false;
      this->lk.unlock();
      this->future.get();
      return this->res;
    }

    else {
      this->lk.unlock();
      return false;
    }
  };
};
}  // namespace o

#else

namespace f {
enum class word { live, die };

struct message {
  word w{word::live};
  size_t id{0};
};

namespace {

/*  next_state message =
      id = random
      message = { die, id }
      return { live, id }

    next_state message =
      return { die, id } */
inline message next_state(std::shared_ptr<message> const& m) noexcept
{
  auto random_id = []() noexcept -> size_t
  {
    auto rng{std::mt19937{std::random_device{}()}};
    return std::uniform_int_distribution<size_t>{}(rng);
  };

  static auto mtx{std::mutex{}};

  auto _ = std::scoped_lock<std::mutex>{mtx};

  auto const w{m->w};

  m->w = word::die;

  m->id = m->id > 0 ? m->id : random_id();

  return message{w, m->id};
};

} /* namespace */

inline size_t adapter(std::filesystem::path const& path,
                      ::wtr::watcher::event::callback const& callback,
                      std::shared_ptr<message> const& previous) noexcept
{
  using evw = ::wtr::watcher::event::what;
  using evk = ::wtr::watcher::event::kind;

  /*  This map associates watchers with (maybe not unique) paths. */
  static auto lifetimes{std::unordered_map<size_t, std::filesystem::path>{}};
  static auto lifetimes_mtx{std::mutex{}};

  auto const msg = next_state(previous);

  /*  Creates a watcher at some path with a unique lifetime
      The lifetime ends whenever we receive the `die` word
      in this watcher's message.

      True if a new watcher was created without error. */
  auto const& live = [id = msg.id, &path, &callback]() -> bool
  {
    /*  Returns a functor to check if we're still living.
        The functor is unique to every watcher. */
    auto const& create_lifetime =
      [id, &path, &callback]() noexcept -> std::function<bool()>
    {
      auto _ = std::scoped_lock{lifetimes_mtx};

      auto const maybe_node = lifetimes.find(id);

      /*  If this watcher wasn't alive when we were called, then
          return a functor which is true until we end the lifetime.

          True if `id` exists in `lifetimes`. */
      if (maybe_node == lifetimes.end()) [[likely]] {
        lifetimes.emplace(id, path);

        callback({"s/self/live@" + path.string(), evw::create, evk::watcher});

        return [id]() noexcept -> bool
        {
          auto _ = std::scoped_lock{lifetimes_mtx};

          return lifetimes.find(id) != lifetimes.end();
        };
      }

      /*  Or we return a functor that always returns false. */
      else {
        callback(
          {"e/self/already_alive@" + path.string(), evw::create, evk::watcher});

        return []() constexpr noexcept -> bool { return false; };
      }
    };

    return watch(path, callback, create_lifetime()) ? true : false;
  };

  /*  Removes a watcher's `id` from the lifetime container.
      This ends the watcher's lifetime. The predicate functor
      for a watcher without an `id` in this container always
      returns false. The watchers know how to die after that.

      True if `id` existed in the container and was removed. */
  auto const& die = [id = msg.id]() noexcept -> bool
  {
    auto _ = std::scoped_lock{lifetimes_mtx};

    auto const maybe_node = lifetimes.find(id);

    if (maybe_node != lifetimes.end()) [[likely]] {
      lifetimes.erase(maybe_node->first);

      return true;
    }

    else
      return false;
  };

  switch (msg.w) {
    case word::live : return live();

    case word::die : return die();

    default : return false;
  }
};

}  // namespace f

#endif

}  // namespace adapter
}  // namespace watcher
}  // namespace wtr
}  // namespace detail
