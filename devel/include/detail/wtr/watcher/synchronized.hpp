#pragma once

#ifdef __APPLE__

#define WTR_WITH_ATOMIC_OWNERSHIP

#include <optional>

#ifdef WTR_WITH_ATOMIC_OWNERSHIP
#include <atomic>
#include <thread>
#else
#include <mutex>
#endif

class Synchronized {
public:
#ifdef WTR_WITH_ATOMIC_OWNERSHIP
  using Primitive = std::atomic<bool>;
#else
  using Primitive = std::mutex;
#endif

private:
  Primitive& primitive;

  inline Synchronized(Primitive& primitive)
      : primitive{primitive}
  {}

#ifdef WTR_WITH_ATOMIC_OWNERSHIP
  inline static auto exchange_when(
    std::atomic<bool>& current_value,
    bool exchange_when_current_is,
    bool want_value) -> bool
  {
#ifdef WTR_ALLOW_NON_SEQUENTIAL_CONSISTENCY
    constexpr auto relord = std::memory_order_release;
    constexpr auto acqord = std::memory_order_acquire;
#else
    constexpr auto relord = std::memory_order_seq_cst;
    constexpr auto acqord = std::memory_order_seq_cst;
#endif
    return current_value.compare_exchange_strong(
      exchange_when_current_is,
      want_value,
      acqord,
      relord);
  }

  inline static auto try_take(std::atomic<bool>& owner_flag) -> bool
  {
    return exchange_when(owner_flag, false, true);
  }

  inline static auto try_leave(std::atomic<bool>& owner_flag) -> bool
  {
    return exchange_when(owner_flag, true, false);
  }

  inline static auto eventually_take(std::atomic<bool>& owner_flag)
  {
    while (! try_take(owner_flag)) { std::this_thread::yield(); }
  }

#else

  inline static auto try_take(std::mutex& mtx) -> bool
  {
    return mtx.try_lock();
  }

  inline static auto try_leave(std::mutex& mtx) { mtx.unlock(); }

  inline static auto eventually_take(std::mutex& mtx) { mtx.lock(); }

#endif

public:
  inline static auto try_from(Primitive& primitive)
    -> std::optional<Synchronized>
  {
    if (try_take(primitive))
      return Synchronized{primitive};
    else
      return std::nullopt;
  }

  inline static auto eventually_from(Primitive& primitive) -> Synchronized
  {
    eventually_take(primitive);
    return Synchronized{primitive};
  }

  inline ~Synchronized() { try_leave(this->primitive); }
};

#endif
