#pragma once

#include <atomic>

#ifdef __linux__
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif

namespace detail::wtr::watcher {

/*  A semaphore which is pollable on Linux.
    On all platforms, this behaves like an
    atomic boolean flag. (On non-Linux, this
    literally is an atomic boolean flag.)
    On Linux, this is an eventfd in semaphore
    mode, which means that it can be waited on
    with poll() and friends. */

class semabin {
public:
  enum state { pending, released, error };

private:
  mutable std::atomic<state> is = pending;

public:
#ifdef __linux__

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

#else

  inline auto release() noexcept -> enum state { return this->is = released; }

  inline auto state() const noexcept -> enum state { return this->is; }

#endif
};

} /*  namespace detail::wtr::watcher */
