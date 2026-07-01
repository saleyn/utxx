// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_platform.cpp
//------------------------------------------------------------------------------
/// \brief Platform abstraction implementations (epoll / kqueue)
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
//------------------------------------------------------------------------------
#include <atomic>
#include <cassert>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <reactor/reactor_platform.hpp>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace utxx {

//==============================================================================
// Linux — thin wrappers around epoll / eventfd / timerfd / signalfd
//==============================================================================
#if defined(REACTOR_OS_LINUX)

//------------------------------------------------------------------------------
// Multiplexer
//------------------------------------------------------------------------------
reactor_handle_t reactor_create()
{
  int fd = ::epoll_create1(0);
  if (fd < 0) throw std::runtime_error(std::string("epoll_create1: ") + strerror(errno));
  return fd;
}

void reactor_destroy(reactor_handle_t h)
{ ::close(h); }

static void epoll_ctl_or_throw(reactor_handle_t h, int op, int fd, uint32_t ev, uintptr_t udata)
{
  struct epoll_event e{};
  e.events   = ev;
  e.data.u64 = udata ? udata : (uint64_t)(uintptr_t)fd;
  if (::epoll_ctl(h, op, fd, &e) < 0)
    throw std::runtime_error(std::string("epoll_ctl: ") + strerror(errno));
}

void reactor_add(reactor_handle_t h, int fd, uint32_t ev, uintptr_t udata)
{ epoll_ctl_or_throw(h, EPOLL_CTL_ADD, fd, ev, udata); }

void reactor_mod(reactor_handle_t h, int fd, uint32_t ev, uintptr_t udata)
{ epoll_ctl_or_throw(h, EPOLL_CTL_MOD, fd, ev, udata); }

void reactor_del(reactor_handle_t h, int fd)
{
  struct epoll_event e{};
  ::epoll_ctl(h, EPOLL_CTL_DEL, fd, &e); // ignore errors on removal
}

int reactor_wait(reactor_handle_t h, reactor_event_t* buf, int maxev, int timeout_ms)
{ return ::epoll_wait(h, buf, maxev, timeout_ms); }

//------------------------------------------------------------------------------
// Eventfd
//------------------------------------------------------------------------------
int reactor_eventfd_create()
{ return ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); }
int reactor_eventfd_write_fd(int efd)
{ return efd; }
void reactor_eventfd_close(int efd)
{
  if (efd >= 0) ::close(efd);
}

int reactor_eventfd_read(int efd, uint64_t& val)
{
  ssize_t n = ::read(efd, &val, sizeof(val));
  return (n == sizeof(val)) ? 0 : -1;
}

int reactor_eventfd_write(int wfd, uint64_t val)
{
  ssize_t n = ::write(wfd, &val, sizeof(val));
  return (n == sizeof(val)) ? 0 : -1;
}

//------------------------------------------------------------------------------
// Timerfd
//------------------------------------------------------------------------------
int reactor_timerfd_create()
{
  int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (fd < 0) throw std::runtime_error(std::string("timerfd_create: ") + strerror(errno));
  return fd;
}

int reactor_timerfd_arm(reactor_handle_t /*kq*/, int fd, uint64_t initial_ms, uint64_t interval_ms)
{
  struct itimerspec ts{};
  ts.it_value.tv_sec     = initial_ms / 1000;
  ts.it_value.tv_nsec    = (initial_ms % 1000) * 1'000'000L;
  ts.it_interval.tv_sec  = interval_ms / 1000;
  ts.it_interval.tv_nsec = (interval_ms % 1000) * 1'000'000L;
  return ::timerfd_settime(fd, 0, &ts, nullptr);
}

int reactor_timerfd_read(int fd, uint64_t& expirations)
{
  ssize_t n = ::read(fd, &expirations, sizeof(expirations));
  return (n == sizeof(expirations)) ? 0 : -1;
}

void reactor_timerfd_close(reactor_handle_t /*kq*/, int fd)
{
  if (fd >= 0) ::close(fd);
}

//------------------------------------------------------------------------------
// Signalfd
//------------------------------------------------------------------------------
int reactor_signalfd_create(reactor_handle_t /*kq*/, const sigset_t& mask, int /*sigq_cap*/)
{
  if (::sigprocmask(SIG_BLOCK, &mask, nullptr) < 0) return -1;
  return ::signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
}

int reactor_signalfd_read(int fd, int& signo, int& code)
{
  struct signalfd_siginfo si{};
  ssize_t                 n = ::read(fd, &si, sizeof(si));
  if (n != sizeof(si)) return -1;
  signo = (int)si.ssi_signo;
  code  = (int)si.ssi_code;
  return 0;
}

void reactor_signalfd_close(reactor_handle_t /*kq*/, int fd, const sigset_t& /*mask*/)
{
  if (fd >= 0) ::close(fd);
}

//==============================================================================
// macOS / BSD — kqueue implementation
//==============================================================================
#else // REACTOR_OS_KQUEUE

#include <sys/event.h>
#include <sys/time.h>

//------------------------------------------------------------------------------
// Multiplexer
//------------------------------------------------------------------------------
reactor_handle_t reactor_create()
{
  int kq = ::kqueue();
  if (kq < 0) throw std::runtime_error(std::string("kqueue: ") + strerror(errno));
  return kq;
}

void reactor_destroy(reactor_handle_t h)
{ ::close(h); }

// Translate REACTOR_EV_* bitmask into one or two kevent registrations.
static void kq_update(reactor_handle_t kq, int fd, uint32_t ev, int op /* EV_ADD/EV_DELETE */)
{
  struct kevent changes[2];
  int           n     = 0;

  uint16_t      flags = (uint16_t)(op | EV_RECEIPT);
  if (op == EV_ADD) {
    if (ev & REACTOR_EV_ET) flags |= EV_CLEAR;
    if (ev & REACTOR_EV_ONESHOT) flags |= EV_ONESHOT;
  }

  if (ev & REACTOR_EV_IN) EV_SET(&changes[n++], fd, EVFILT_READ, flags, 0, 0, (void*)(intptr_t)fd);
  if (ev & REACTOR_EV_OUT)
    EV_SET(&changes[n++], fd, EVFILT_WRITE, flags, 0, 0, (void*)(intptr_t)fd);

  if (!n) return;

  struct kevent result[2];
  int           r = ::kevent(kq, changes, n, result, n, nullptr);
  if (r < 0) throw std::runtime_error(std::string("kevent: ") + strerror(errno));
}

void reactor_add(reactor_handle_t h, int fd, uint32_t ev, uintptr_t /*udata*/)
{ kq_update(h, fd, ev, EV_ADD); }

void reactor_mod(reactor_handle_t h, int fd, uint32_t ev, uintptr_t /*udata*/)
{
  // Remove both filters then re-add with the new mask.
  struct kevent del[2];
  EV_SET(&del[0], fd, EVFILT_READ,  EV_DELETE, 0, 0, nullptr);
  EV_SET(&del[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
  ::kevent(h, del, 2, nullptr, 0, nullptr); // ignore errors (filter may not exist)
  kq_update(h, fd, ev, EV_ADD);
}

void reactor_del(reactor_handle_t h, int fd)
{
  struct kevent del[2];
  EV_SET(&del[0], fd, EVFILT_READ,  EV_DELETE, 0, 0, nullptr);
  EV_SET(&del[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
  ::kevent(h, del, 2, nullptr, 0, nullptr);
}

int reactor_wait(reactor_handle_t h, reactor_event_t* buf, int maxev, int timeout_ms)
{
  // kqueue returns raw struct kevent; we need reactor_event_t{fd, mask}.
  // Use a local kevent array on the stack (maxev is typically 256).
  struct kevent    kev[256];
  int              n = (maxev > 256) ? 256 : maxev;

  struct timespec  ts{};
  struct timespec* tsp = nullptr;
  if (timeout_ms >= 0) {
    ts.tv_sec  = timeout_ms / 1000;
    ts.tv_nsec = (timeout_ms % 1000) * 1'000'000L;
    tsp        = &ts;
  }

  int rc = ::kevent(h, nullptr, 0, kev, n, tsp);
  if (rc < 0) return rc;

  for (int i = 0; i < rc; ++i) {
    buf[i].fd   = (int)kev[i].ident;
    buf[i].mask = 0;
    if (kev[i].filter == EVFILT_READ) buf[i].mask |= REACTOR_EV_IN;
    if (kev[i].filter == EVFILT_WRITE) buf[i].mask |= REACTOR_EV_OUT;
    if (kev[i].flags & EV_ERROR) buf[i].mask |= REACTOR_EV_ERR;
    if (kev[i].flags & EV_EOF) {
      buf[i].mask |= REACTOR_EV_HUP;
      if (kev[i].filter == EVFILT_READ) buf[i].mask |= REACTOR_EV_RDHUP;
    }
  }
  return rc;
}

//------------------------------------------------------------------------------
// Eventfd — implemented as a non-blocking pipe
// The read fd IS the "efd"; we store the write fd in a small table keyed by
// efd so that reactor_eventfd_write_fd(efd) can return it.
//
// We use a small lock-free open-address table (max 256 concurrent eventfds).
//------------------------------------------------------------------------------
struct PipePair {
  int rd;
  int wr;
};

static constexpr int     kMaxEventFds = 256;
static PipePair          s_pipe_table[kMaxEventFds]; // zero-initialised
static std::atomic<bool> s_pipe_init{false};

static void              ensure_pipe_table()
{
  bool expected = false;
  if (s_pipe_init.compare_exchange_strong(expected, true))
    for (auto& p : s_pipe_table) {
      p.rd = -1;
      p.wr = -1;
    }
}

static void pipe_table_set(int rd, int wr)
{
  for (auto& p : s_pipe_table)
    if (p.rd == -1) {
      p.rd = rd;
      p.wr = wr;
      return;
    }
  throw std::runtime_error("reactor: eventfd table full");
}

static int pipe_table_wr(int rd)
{
  for (auto& p : s_pipe_table)
    if (p.rd == rd) return p.wr;
  return -1;
}

static void pipe_table_remove(int rd)
{
  for (auto& p : s_pipe_table)
    if (p.rd == rd) {
      p.rd = -1;
      p.wr = -1;
      return;
    }
}

int reactor_eventfd_create()
{
  ensure_pipe_table();
  int fds[2];
  if (::pipe(fds) < 0) return -1;
  // Make both ends non-blocking + close-on-exec
  for (int f : fds) {
    ::fcntl(f, F_SETFL, O_NONBLOCK);
    ::fcntl(f, F_SETFD, FD_CLOEXEC);
  }
  pipe_table_set(fds[0], fds[1]);
  return fds[0]; // read end = "efd"
}

int reactor_eventfd_write_fd(int efd)
{ return pipe_table_wr(efd); }

void reactor_eventfd_close(int efd)
{
  if (efd < 0) return;
  int wr = pipe_table_wr(efd);
  pipe_table_remove(efd);
  ::close(efd);
  if (wr >= 0) ::close(wr);
}

int reactor_eventfd_read(int efd, uint64_t& val)
{
  // Drain all available bytes; treat each byte as one increment.
  uint8_t buf[256];
  ssize_t total = 0;
  ssize_t n;
  while ((n = ::read(efd, buf, sizeof(buf))) > 0) total += n;
  val = (uint64_t)total;
  return (total > 0 || errno == EAGAIN) ? 0 : -1;
}

int reactor_eventfd_write(int wfd, uint64_t val)
{
  // Write one byte per increment (saturated at PIPE_BUF-1 = 511 bytes).
  uint64_t count = (val > 511) ? 511 : val;
  uint8_t  buf[512];
  ::memset(buf, 1, count);
  ssize_t n = ::write(wfd, buf, (size_t)count);
  return (n > 0) ? 0 : -1;
}

//------------------------------------------------------------------------------
// Timerfd — EVFILT_TIMER on the kqueue
// We allocate synthetic timer IDs from an atomic counter.  The kqueue handle
// is needed to arm/delete the timer.
//------------------------------------------------------------------------------
static std::atomic<int> s_timer_id_gen{1};

int                     reactor_timerfd_create()
{ return s_timer_id_gen.fetch_add(1); }
void reactor_timerfd_close(reactor_handle_t kq, int tid)
{
  struct kevent del;
  EV_SET(&del, tid, EVFILT_TIMER, EV_DELETE, 0, 0, nullptr);
  ::kevent(kq, &del, 1, nullptr, 0, nullptr);
}

int reactor_timerfd_arm(reactor_handle_t kq, int tid, uint64_t initial_ms, uint64_t interval_ms)
{
  // We register two separate timer events: one for the initial delay (oneshot)
  // and one for the repeat.  If initial_ms == interval_ms we just use one.
  struct kevent change;
  uint16_t      flags = EV_ADD | EV_ENABLE;
  if (interval_ms == 0) flags |= EV_ONESHOT;

  // Use the initial delay (or interval if initial==0) as the period.
  uint64_t ms = initial_ms ? initial_ms : interval_ms;
  if (ms == 0) ms = 1;

  EV_SET(&change, tid, EVFILT_TIMER, flags, NOTE_MSECONDS, (intptr_t)ms, nullptr);
  return ::kevent(kq, &change, 1, nullptr, 0, nullptr);
}

int reactor_timerfd_read(int /*tid*/, uint64_t& expirations)
{
  // kqueue timers fire at most once per kevent return; no fd to read.
  expirations = 1;
  return 0;
}

//------------------------------------------------------------------------------
// Signalfd — EVFILT_SIGNAL on the kqueue
// We return a synthetic "fd" equal to the first signal number in the mask.
// Signal delivery data comes directly from the kevent.
//------------------------------------------------------------------------------
int reactor_signalfd_create(reactor_handle_t kq, const sigset_t& mask, int /*sigq_cap*/)
{
  if (::sigprocmask(SIG_BLOCK, &mask, nullptr) < 0) return -1;

  // Register each signal in mask as EVFILT_SIGNAL on the kqueue.
  int first_sig = -1;
  for (int sig = 1; sig < NSIG; ++sig) {
    if (!sigismember(&mask, sig)) continue;
    if (first_sig < 0) first_sig = sig;
    struct kevent change;
    EV_SET(&change, sig, EVFILT_SIGNAL, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    ::kevent(kq, &change, 1, nullptr, 0, nullptr);
  }
  return first_sig; // synthetic "fd" = first signal number
}

int reactor_signalfd_read(int /*fd*/, int& signo, int& code)
{
  // Caller must pass the raw kevent data via a different mechanism.
  // In practice HandleSignal on kqueue uses the kevent ident directly.
  // This function is a no-op placeholder for the kqueue path.
  signo = 0;
  code  = 0;
  return 0;
}

void reactor_signalfd_close(reactor_handle_t kq, int first_sig, const sigset_t& mask)
{
  for (int sig = first_sig; sig < NSIG; ++sig) {
    if (!sigismember(&mask, sig)) continue;
    struct kevent del;
    EV_SET(&del, sig, EVFILT_SIGNAL, EV_DELETE, 0, 0, nullptr);
    ::kevent(kq, &del, 1, nullptr, 0, nullptr);
  }
}

#endif // REACTOR_OS_KQUEUE

} // namespace utxx
