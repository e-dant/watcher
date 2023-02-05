#pragma once

/*  path */
#include <filesystem>
/*  shared_ptr */
#include <memory>
/*  mutex
    scoped_lock */
#include <mutex>
/*  mt19937
    random_device
    uniform_int_distribution */
#include <random>
/*  unordered_map */
#include <unordered_map>
/*  watch
    event
    callback */
#include <wtr/watcher.hpp>

namespace wtr {
namespace watcher {
namespace detail {
namespace adapter {
/* @pragma/tool/hone/insert namespace { */

enum class word { live, die };

struct message {
  word word{word::live};
  size_t id{0};
};

/*
    carry message { live, 0 } =
      id = random
      message { die, id }
      { live, id }

    carry message { die, id } =
      { die, id }
 */
inline message carry(std::shared_ptr<message> m) noexcept {
  auto random_id = []() noexcept -> size_t {
    auto rng{std::mt19937{std::random_device{}()}};
    return std::uniform_int_distribution<size_t>{}(rng);
  };

  static auto mtx{std::mutex{}};

  auto _ = std::scoped_lock<std::mutex>{mtx};

  auto const w{m->word};

  m->word = word::die;

  m->id = m->id > 0 ? m->id : random_id();

  return message{w, m->id};
};

inline size_t adapter(std::filesystem::path const& path,
                      event::callback const& callback,
                      std::shared_ptr<message> previous) noexcept {
  using evw = ::wtr::watcher::event::what;
  using evk = ::wtr::watcher::event::kind;

  /*  This map associates watchers with (maybe not unique) paths. */
  static auto lifetimes{std::unordered_map<size_t, std::filesystem::path>{}};

  /*  A mutex to synchronize access to the container. */
  static auto lifetimes_mtx{std::mutex{}};

  auto const msg = carry(previous);

  /*  Returns a functor to check if we're still living.
      The functor is unique to every watcher. */
  auto const& live = [id = msg.id, &path, &callback]() -> bool {
    auto const& create_lifetime =
      [id, &path, &callback]() noexcept -> std::function<bool()> {
      auto _ = std::scoped_lock{lifetimes_mtx};

      auto const maybe_node = lifetimes.find(id);

      if (maybe_node == lifetimes.end()) [[likely]] {
        lifetimes[id] = path;

        callback({"s/self/live@" + path.string(), evw::create, evk::watcher});

        return [id]() noexcept -> bool {
          auto _ = std::scoped_lock{lifetimes_mtx};

          return lifetimes.find(id) != lifetimes.end();
        };

      } else {
        callback(
          {"e/self/already_alive@" + path.string(), evw::create, evk::watcher});

        return []() constexpr noexcept -> bool { return false; };
      }
    };

    return watch(path, callback, create_lifetime()) ? id : 0;
  };

  auto const& die = [id = msg.id]() noexcept -> size_t {
    auto _ = std::scoped_lock{lifetimes_mtx};

    auto const maybe_node = lifetimes.find(id);

    if (maybe_node != lifetimes.end()) [[likely]] {
      size_t id = maybe_node->first;

      lifetimes.erase(maybe_node);

      return id;

    } else
      return 0;
  };

  switch (msg.word) {
    case word::live : return live();

    case word::die : return die();

    default : return false;
  }
}

/* @pragma/tool/hone/insert } */
} /* namespace adapter */
} /* namespace detail */
} /* namespace watcher */
} /* namespace wtr */
