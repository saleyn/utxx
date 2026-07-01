// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_platform.hpp
//------------------------------------------------------------------------------
/// \brief Platform abstraction: epoll (Linux) ↔ kqueue (macOS/BSD)
///
/// On Linux the reactor uses:
///   epoll_create1 / epoll_ctl / epoll_wait — I/O multiplexer
///   eventfd                               — wakeup / counter events
///   timerfd_create / timerfd_settime      — one-shot & periodic timers
///   signalfd                              — signal delivery as fd reads
///   libaio                                — async file reads
///
/// On macOS (and BSD) the reactor uses:
///   kqueue / kevent                       — I/O multiplexer
///   A pipe (rd+wr fds)                    — wakeup / counter events
///   kqueue EVFILT_TIMER                   — timers
///   kqueue EVFILT_SIGNAL                  — signals
///   (no AIO support — AddFile disabled)
///
/// All consumer code should include this header instead of the platform
/// headers directly.  The public API surface:
///
///   Epoll event-flag aliases (always available regardless of platform):
///     REACTOR_EV_IN, REACTOR_EV_OUT, REACTOR_EV_ERR,
///     REACTOR_EV_HUP, REACTOR_EV_RDHUP, REACTOR_EV_ET, REACTOR_EV_ONESHOT
///
///   Type aliases:
///     reactor_event_t   — the OS event structure (epoll_event / kevent)
///     reactor_handle_t  — the multiplexer handle (int epoll fd / kqueue fd)
///
///   Free functions (implemented per-platform in reactor_platform.cpp):
///     reactor_handle_t reactor_create();
///     void             reactor_destroy(reactor_handle_t);
///     void             reactor_add   (reactor_handle_t, int fd, uint32_t ev, uintptr_t udata);
///     void             reactor_mod   (reactor_handle_t, int fd, uint32_t ev, uintptr_t udata);
///     void             reactor_del   (reactor_handle_t, int fd);
///     int              reactor_wait  (reactor_handle_t, reactor_event_t*, int maxev, int
///     timeout_ms); int              reactor_ev_fd (const reactor_event_t&); uint32_t
///     reactor_ev_mask(const reactor_event_t&); bool             reactor_is_error   (uint32_t
///     mask); bool             reactor_is_readable(uint32_t mask); bool
///     reactor_is_writable(uint32_t mask);
///
///   Eventfd (counter / wakeup):
///     int  reactor_eventfd_create();          — create; returns fd (or pipe-rd)
///     int  reactor_eventfd_write_fd(int efd); — Linux: same fd; macOS: pipe-wr
///     int  reactor_eventfd_read (int efd, uint64_t& val); — read counter
///     int  reactor_eventfd_write(int write_fd, uint64_t val);
///     void reactor_eventfd_close(int efd);    — closes all underlying fds
///
///   Timerfd:
///     int  reactor_timerfd_create();
///     int  reactor_timerfd_settime(int fd, uint64_t initial_ms, uint64_t interval_ms);
///     int  reactor_timerfd_read(int fd, uint64_t& expirations);
///
///   Signalfd (deliver signals via the multiplexer):
///     int  reactor_signalfd_create(const sigset_t&);
///     int  reactor_signalfd_read(int fd, int& signo, int& code);
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
//------------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <signal.h>
#include <stdexcept>
#include <sys/types.h>

//------------------------------------------------------------------------------
// Platform detection
//------------------------------------------------------------------------------
#if defined(__linux__)
#define REACTOR_OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define REACTOR_OS_MACOS 1
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define REACTOR_OS_BSD    1
#define REACTOR_OS_KQUEUE 1
#else
#error "reactor: unsupported platform"
#endif

#if defined(REACTOR_OS_MACOS) || defined(REACTOR_OS_BSD)
#define REACTOR_OS_KQUEUE 1
#endif

//------------------------------------------------------------------------------
// Platform-specific headers
//------------------------------------------------------------------------------
#if defined(REACTOR_OS_LINUX)
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#else
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace utxx {

//------------------------------------------------------------------------------
// Unified event-flag constants
// On Linux these are the real epoll flags.
// On kqueue platforms these are synthetic bitmask values; reactor_platform.cpp
// translates them to/from struct kevent filters/flags.
//------------------------------------------------------------------------------
#if defined(REACTOR_OS_LINUX)

using reactor_event_t                        = struct epoll_event;
using reactor_handle_t                       = int;

static constexpr uint32_t REACTOR_EV_IN      = EPOLLIN;
static constexpr uint32_t REACTOR_EV_OUT     = EPOLLOUT;
static constexpr uint32_t REACTOR_EV_ERR     = EPOLLERR;
static constexpr uint32_t REACTOR_EV_HUP     = EPOLLHUP;
static constexpr uint32_t REACTOR_EV_RDHUP   = EPOLLRDHUP;
static constexpr uint32_t REACTOR_EV_ET      = EPOLLET;
static constexpr uint32_t REACTOR_EV_ONESHOT = EPOLLONESHOT;

#else // kqueue

struct reactor_event_t {
  int      fd;                                      // file descriptor the event is for
  uint32_t mask;                                    // REACTOR_EV_* bitmask
};
using reactor_handle_t                       = int; // kqueue fd

static constexpr uint32_t REACTOR_EV_IN      = 0x0001u;
static constexpr uint32_t REACTOR_EV_OUT     = 0x0002u;
static constexpr uint32_t REACTOR_EV_ERR     = 0x0004u;
static constexpr uint32_t REACTOR_EV_HUP     = 0x0008u;
static constexpr uint32_t REACTOR_EV_RDHUP   = 0x0010u;
static constexpr uint32_t REACTOR_EV_ET      = 0x0020u;
static constexpr uint32_t REACTOR_EV_ONESHOT = 0x0040u;

#endif

//------------------------------------------------------------------------------
// Multiplexer operations
//------------------------------------------------------------------------------
reactor_handle_t reactor_create();
void             reactor_destroy(reactor_handle_t h);

/// Register a new fd with events (REACTOR_EV_* bitmask). udata is user data
/// stored in the event (fd value on Linux; any pointer-sized value on kqueue).
void             reactor_add(reactor_handle_t h, int fd, uint32_t ev, uintptr_t udata = 0);

/// Modify the event mask of a registered fd.
void             reactor_mod(reactor_handle_t h, int fd, uint32_t ev, uintptr_t udata = 0);

/// Deregister a fd.
void             reactor_del(reactor_handle_t h, int fd);

/// Wait for events. Returns number of events (≥0) or -1 on error (errno set).
/// timeout_ms < 0 means block indefinitely.
int        reactor_wait(reactor_handle_t h, reactor_event_t* evbuf, int maxev, int timeout_ms);

/// Extract the fd from a fired event.
inline int reactor_ev_fd(const reactor_event_t& ev)
{
#if defined(REACTOR_OS_LINUX)
  return ev.data.fd;
#else
  return ev.fd;
#endif
}

/// Extract the event mask from a fired event.
inline uint32_t reactor_ev_mask(const reactor_event_t& ev)
{
#if defined(REACTOR_OS_LINUX)
  return ev.events;
#else
  return ev.mask;
#endif
}

inline bool reactor_is_error(uint32_t mask)
{ return mask & (REACTOR_EV_ERR | REACTOR_EV_HUP); }
inline bool reactor_is_readable(uint32_t mask)
{ return mask & REACTOR_EV_IN; }
inline bool reactor_is_writable(uint32_t mask)
{ return mask & REACTOR_EV_OUT; }

//------------------------------------------------------------------------------
// Eventfd abstraction
//
// Linux: a single eventfd(2) fd is used for both reading and writing.
// macOS: a non-blocking pipe pair; the read end is "efd", the write end is
//        stored internally and returned by reactor_eventfd_write_fd().
//        Both ends must be closed with reactor_eventfd_close().
//------------------------------------------------------------------------------
/// Create an event descriptor.  Returns the read fd (≥0) or -1 on error.
int  reactor_eventfd_create();

/// Return the fd to write to.  On Linux this is the same fd; on macOS/BSD it
/// is the write end of the underlying pipe.
int  reactor_eventfd_write_fd(int efd);

/// Read the counter value.  Returns 0 on success, -1 on error (errno set).
int  reactor_eventfd_read(int efd, uint64_t& val);

/// Write (add) to the counter.  Returns 0 on success, -1 on error.
int  reactor_eventfd_write(int write_fd, uint64_t val);

/// Close all fds associated with efd.  Safe to call with efd == -1.
void reactor_eventfd_close(int efd);

//------------------------------------------------------------------------------
// Timerfd abstraction
//
// Linux: timerfd_create + timerfd_settime.
// macOS: a kqueue timer-ident allocated from a per-process counter; the
//        "fd" returned is a synthetic integer used as the kevent identifier.
//        The actual timer fires through the kqueue handle passed to
//        reactor_timerfd_arm() and is read via reactor_timerfd_read() which
//        always returns 1 expiration (kqueue timers don't accumulate).
//------------------------------------------------------------------------------
/// Create a timer.  Returns a timer-fd/ident (≥0) or -1 on error.
int  reactor_timerfd_create();

/// Arm (or re-arm) the timer.
///   initial_ms   — delay before first fire (0 = fire immediately once)
///   interval_ms  — repeat interval (0 = one-shot)
/// On Linux uses timerfd_settime(TFD_TIMER_ABSTIME).
/// On macOS registers EVFILT_TIMER on the provided kqueue handle.
int  reactor_timerfd_arm(
    reactor_handle_t kq, int timer_id, uint64_t initial_ms, uint64_t interval_ms);

/// Read the expiration count into *expirations.  Returns 0 on success.
int  reactor_timerfd_read(int timer_id, uint64_t& expirations);

/// Destroy a timer.
void reactor_timerfd_close(reactor_handle_t kq, int timer_id);

//------------------------------------------------------------------------------
// Signalfd abstraction
//
// Linux: signalfd(2).
// macOS: EVFILT_SIGNAL kevents on the kqueue; the "fd" returned is a synthetic
//        per-signal slot.  reactor_signalfd_read() is called with the raw kevent
//        data to extract signo/code.
//------------------------------------------------------------------------------
/// Block the signals in a_mask (sigprocmask SIG_BLOCK) and register them on
/// the kqueue (macOS) or create a signalfd (Linux).
/// Returns a signal-fd/handle (≥0) or -1 on error.
int  reactor_signalfd_create(reactor_handle_t kq, const sigset_t& mask, int sigq_capacity);

/// Read one signal delivery.  Returns 0 on success, -1 on error.
int  reactor_signalfd_read(int sigfd, int& signo, int& code);

/// Destroy a signalfd.
void reactor_signalfd_close(reactor_handle_t kq, int sigfd, const sigset_t& mask);

} // namespace utxx

//------------------------------------------------------------------------------
// Backwards-compat aliases so existing reactor code compiles unmodified on
// Linux (where the real epoll constants are already in scope).
// On macOS the REACTOR_EV_* constants are used directly.
//------------------------------------------------------------------------------
#if defined(REACTOR_OS_LINUX)
// Nothing: EPOLLIN etc. are already defined by <sys/epoll.h>
#else
// Provide EPOLL* names as aliases to ease porting of call sites that use them
// in direct comparisons (e.g. (a_events & EPOLLET)).
static constexpr uint32_t EPOLLIN      = utxx::REACTOR_EV_IN;
static constexpr uint32_t EPOLLOUT     = utxx::REACTOR_EV_OUT;
static constexpr uint32_t EPOLLERR     = utxx::REACTOR_EV_ERR;
static constexpr uint32_t EPOLLHUP     = utxx::REACTOR_EV_HUP;
static constexpr uint32_t EPOLLRDHUP   = utxx::REACTOR_EV_RDHUP;
static constexpr uint32_t EPOLLET      = utxx::REACTOR_EV_ET;
static constexpr uint32_t EPOLLONESHOT = utxx::REACTOR_EV_ONESHOT;
// Linux-only flags that don't exist on macOS — define as 0 so existing OR
// expressions compile correctly (they simply have no effect).
static constexpr uint32_t EPOLLPRI     = 0;
static constexpr uint32_t EPOLLRDNORM  = 0;
static constexpr uint32_t EPOLLRDBAND  = 0;
static constexpr uint32_t EPOLLWRNORM  = 0;
static constexpr uint32_t EPOLLWRBAND  = 0;
static constexpr uint32_t EPOLLMSG     = 0;
static constexpr uint32_t EPOLLWAKEUP  = 0;
#endif
