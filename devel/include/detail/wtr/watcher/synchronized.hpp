#pragma once

#ifdef __APPLE__

#define WTR_USE_POSIX_SYNC

#include <optional>

#ifdef WTR_USE_POSIX_SYNC
#include <pthread.h>
#else
#include <mutex>
#endif

class Mutex {
public:
#ifdef WTR_USE_POSIX_SYNC
  using Primitive = pthread_mutex_t;
#else
  using Primitive = std::mutex;
#endif

private:
#ifdef WTR_USE_POSIX_SYNC
  inline static auto make_primitive() -> Primitive
  {
    pthread_mutexattr_t attr = {};
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_t mtx = {};
    pthread_mutex_init(&mtx, &attr);
    return mtx;
  }

  inline static auto try_take(pthread_mutex_t& mtx) -> bool
  {
    return pthread_mutex_trylock(&mtx) == 0;
  }

  inline static auto eventually_take(pthread_mutex_t& mtx) -> bool
  {
    return pthread_mutex_lock(&mtx) == 0;
  }

  inline static auto try_leave(pthread_mutex_t& mtx) -> bool
  {
    return pthread_mutex_unlock(&mtx) == 0;
  }

  inline static auto destroy(pthread_mutex_t& mtx) -> void
  {
    pthread_mutex_destroy(&mtx);
  }
#else
  inline static auto make_primitive() -> Primitive { return {}; }

  inline static auto try_take(std::mutex& mtx) -> bool
  {
    return mtx.try_lock();
  }

  inline static auto eventually_take(std::mutex& mtx) -> bool
  {
    mtx.lock();
    return true;
  }

  inline static auto try_leave(std::mutex& mtx) -> bool
  {
    mtx.unlock();
    return true;
  }

  inline static auto destroy(std::mutex& mtx) -> void {}
#endif

  Primitive mtx = make_primitive();

  inline auto try_take() -> bool { return try_take(mtx); }

  inline auto eventually_take() -> bool { return eventually_take(mtx); }

  inline auto try_leave() -> bool { return try_leave(mtx); }

  class Synchronized {
  private:
    Mutex& mtx;

    inline Synchronized(Mutex& mtx)
        : mtx{mtx}
    {}

  public:
    inline static auto try_from(Mutex& mtx) -> std::optional<Synchronized>
    {
      if (mtx.try_take())
        return Synchronized{mtx};
      else
        return std::nullopt;
    }

    inline static auto eventually_from(Mutex& mtx) -> Synchronized
    {
      mtx.eventually_take();
      return Synchronized{mtx};
    }

    inline ~Synchronized() { mtx.try_leave(); }
  };

public:
  inline auto try_sync() -> std::optional<Synchronized>
  {
    return Synchronized::try_from(*this);
  }

  inline auto eventually_sync() -> Synchronized
  {
    return Synchronized::eventually_from(*this);
  }

  inline ~Mutex() { destroy(mtx); }
};

#endif
