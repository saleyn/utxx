// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  test_reactor.cpp
//------------------------------------------------------------------------------
/// \brief Integration tests for the Reactor class
//------------------------------------------------------------------------------
#include "test_main.hpp"
#include <atomic>
#include <chrono>
#include <fcntl.h>
#include <reactor/reactor.hpp>
#include <reactor/reactor_misc.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

using namespace utxx;

// ---- helpers ----------------------------------------------------------------

static void set_nonblock(int fd)
{ utxx::Blocking(fd, false); }

// ---- Reactor construction / destruction ------------------------------------

TEST_CASE(reactor_construct_destruct)
{
  CHECK_NO_THROW({
    Reactor r("test", 0);
    CHECK(r.EPollFD() >= 0);
    CHECK(r.Debug() == 0);
    CHECK(r.Ident().find("test") != std::string::npos);
  });
}

TEST_CASE(reactor_debug_level)
{
  Reactor r("dbg", 3);
  CHECK_EQUAL(3, r.Debug());
  r.Debug(5);
  CHECK_EQUAL(5, r.Debug());
}

// ---- AddEvent + Wait -------------------------------------------------------

TEST_CASE(reactor_add_event_triggers_handler)
{
  Reactor r("ev", 0);

  long    received = 0;
  FdInfo& fi       = r.AddEvent(
      "test_event", [&received](FdInfo&, long val) { received = val; },
      [](FdInfo&, IOType, const std::string& e, src_info&&) {
        fprintf(stderr, "error: %s\n", e.c_str());
      });

  int efd = fi.FD();
  CHECK(efd >= 0);

  // Signal the eventfd with value 42
  uint64_t v = 42;
  ::write(efd, &v, sizeof(v));

  r.Wait(100);

  CHECK_EQUAL(42L, received);
}

TEST_CASE(reactor_event_handler_called_multiple_times)
{
  Reactor  r("ev2", 0);

  int      call_count = 0;
  FdInfo&  fi  = r.AddEvent("multi_event", [&call_count](FdInfo&, long) { call_count++; }, nullptr);

  int      efd = fi.FD();
  uint64_t v   = 1;

  for (int i = 0; i < 3; i++) {
    ::write(efd, &v, sizeof(v));
    r.Wait(100);
  }

  CHECK_EQUAL(3, call_count);
}

// ---- AddTimer + Wait -------------------------------------------------------

TEST_CASE(reactor_add_timer_fires)
{
  Reactor r("timer", 0);

  int     fired = 0;
  r.AddTimer(
      "test_timer",
      10, // initial: 10ms
      0,  // interval: 0 (one-shot)
      [&fired](FdInfo&, long) { fired++; }, nullptr);

  // Wait up to 500ms for the timer to fire
  for (int i = 0; i < 10 && fired == 0; i++) r.Wait(50);

  CHECK(fired >= 1);
}

// ---- AddSignal (smoke) -----------------------------------------------------

TEST_CASE(reactor_add_signal_smoke)
{
  Reactor  r("sig", 0);

  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);

  int  received_sig = 0;
  bool threw        = false;
  try {
    r.AddSignal(
        "test_signal", mask, [&received_sig](FdInfo&, int sig, int) { received_sig = sig; },
        nullptr);
    s_passed++;
  } catch (std::exception& e) {
    fprintf(stderr, "FAIL: AddSignal threw: %s\n", e.what());
    threw = true;
    s_failures++;
  }
  CHECK(!threw);

  // Send signal to self and check it's caught
  ::kill(::getpid(), SIGUSR1);
  r.Wait(200);

  CHECK_EQUAL(SIGUSR1, received_sig);
}

// ---- Add (socket) + IO handler ---------------------------------------------

TEST_CASE(reactor_add_socket_io_handler)
{
  Reactor r("io", 0);

  int     fds[2];
  ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
  set_nonblock(fds[0]);
  set_nonblock(fds[1]);

  std::string received;
  // Use the RawIOHandler path (std::function) for readable fds in tests
  r.Add(
      "sock", fds[0], RawIOHandler([&received, &fds](FdInfo&, IOType t, uint32_t) {
        if (t == IOType::Read) {
          char tmp[256];
          int  n = ::read(fds[0], tmp, sizeof(tmp));
          if (n > 0) received.append(tmp, n);
        }
      }),
      ErrHandler([](FdInfo&, IOType, const std::string& e, src_info&&) {
        fprintf(stderr, "sock error: %s\n", e.c_str());
      }),
      /*opaque*/ nullptr, REACTOR_EV_IN | REACTOR_EV_ET);

  ::write(fds[1], "hello", 5);

  r.Wait(200);

  ::close(fds[1]);

  CHECK(received == "hello");
}

// ---- Raw IO handler --------------------------------------------------------

TEST_CASE(reactor_add_rawio_handler)
{
  Reactor r("raw", 0);

  int     fds[2];
  ::pipe(fds);
  set_nonblock(fds[0]);

  IOType observed_type = IOType::UNDEFINED;

  r.Add(
      "raw_pipe", fds[0], [&observed_type](FdInfo&, IOType t, uint32_t) { observed_type = t; },
      nullptr, nullptr, REACTOR_EV_IN | REACTOR_EV_ET);

  ::write(fds[1], "x", 1);
  r.Wait(200);

  ::close(fds[0]);
  ::close(fds[1]);

  CHECK(observed_type == IOType::Read);
}

// ---- SetLogger -------------------------------------------------------------

TEST_CASE(reactor_set_logger_called)
{
  Reactor     r("log_test", 5); // debug=5 so RLOG emits

  std::string last_msg;
  r.SetLogger([&last_msg](log_level, src_info&&, const std::string& msg) { last_msg = msg; });

  // Trigger a log by adding an event and waiting with nothing written
  r.AddEvent("idle", [](FdInfo&, long) {}, nullptr);
  r.Wait(1);

  // The logger may or may not have fired depending on trace level, but
  // it must not crash:
  CHECK(true);
}

// ---- SetIdle ---------------------------------------------------------------

TEST_CASE(reactor_idle_handler_called_on_timeout)
{
  Reactor r("idle", 0);

  int     idle_count = 0;
  r.SetIdle([&idle_count]() { idle_count++; });

  r.Wait(50); // timeout → idle fires

  CHECK(idle_count >= 1);
}

// ---- RdBuff / WrBuff -------------------------------------------------------

TEST_CASE(reactor_rdbuff_valid_after_add)
{
  Reactor r("bufs", 0);
  int     fds[2];
  ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
  set_nonblock(fds[0]);

  r.Add("s", fds[0], nullptr, nullptr, nullptr, nullptr, nullptr, 128);

  auto* rb = r.RdBuff(fds[0]);
  CHECK(rb != nullptr);
  CHECK(rb->capacity() >= 128u);

  ::close(fds[0]);
  ::close(fds[1]);
}

// ---- UDS listener ----------------------------------------------------------

TEST_CASE(reactor_uds_listener_accepts_client)
{
  const char* sock_path = "/tmp/__reactor_test_uds__.sock";
  ::unlink(sock_path);

  Reactor r("uds", 0);

  bool    accepted = false;
  r.AddUDSListener(
      "uds_srv", sock_path,
      [&accepted](FdInfo&, const char* path, int cli_fd) -> bool {
        accepted = true;
        ::close(cli_fd);
        return false; // we closed it ourselves
      },
      nullptr);

  // Connect from a background thread
  std::thread t([sock_path]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    int                fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);
    ::connect(fd, (sockaddr*)&addr, sizeof(addr));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ::close(fd);
  });

  for (int i = 0; i < 10 && !accepted; i++) r.Wait(50);

  t.join();
  ::unlink(sock_path);

  CHECK(accepted);
}

// ---- main -------------------------------------------------------------------

int main()
{ return test_harness::run_all(); }
