#pragma once

#include <atomic>

#ifdef __linux__
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

namespace detail::wtr::watcher {

/*  A semaphore-like construct which can be
    used with poll and friends on Linux and
    dispatch on Apple. This class is emulates
    a binary semaphore; An atomic boolean flag.

    On other platforms, this is an atomic flag
    which can be checked in a sleep, wake loop,
    ideally with a generous sleep time.

    On Linux, this is an eventfd in semaphore
    mode. The file descriptor is exposed for
    use with poll and friends.

    On macOS, this is a dispatch_semaphore_t,
    which can be scheduled with dispatch.
*/

class semabin {
public:
  enum state { pending, released, error };

private:
  mutable std::atomic<state> is = pending;

public:
#if defined(__linux__)

  int const fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE);

  inline auto release() noexcept -> state
  {
    auto write_ev = [this]()
    {
      if (eventfd_write(this->fd, 1) == 0)
        return released;
      else
        return error;
    };

    if (this->is == released)
      return released;
    else
      return this->is = write_ev();
  }

  inline auto state() const noexcept -> state
  {
    auto read_ev = [this]()
    {
      uint64_t _ = 0;
      if (eventfd_read(this->fd, &_) == 0)
        return released;
      else if (errno == EAGAIN)
        return pending;
      else
        return error;
    };

    if (this->is == pending)
      return pending;
    else
      return this->is = read_ev();
  }

  inline ~semabin() noexcept { close(this->fd); }

#elif defined(__APPLE__)

  dispatch_semaphore_t const sema = []()
  { return dispatch_semaphore_create(0); }();

  inline auto release() noexcept -> state
  {
    auto write_ev = [this]()
    {
      if (dispatch_semaphore_signal(this->sema) == 1)
        return released;
      else
        return error;
    };

    if (this->is == released)
      return released;
    else
      return this->is = write_ev();
  }

  inline auto state() const noexcept -> state
  {
    auto read_ev = [this]()
    {
      if (dispatch_semaphore_wait(this->sema, DISPATCH_TIME_NOW) == 0)
        return released;
      else
        return pending;
    };

    if (this->is == pending)
      return pending;
    else
      return this->is = read_ev();
  }

  inline ~semabin() noexcept { dispatch_release(this->sema); }

#else

  inline auto release() noexcept -> enum state { return this->is = released; }

  inline auto state() const noexcept -> enum state { return this->is; }

#endif
};

} /*  namespace detail::wtr::watcher */
