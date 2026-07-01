// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  test_reactor_misc.cpp
//------------------------------------------------------------------------------
/// \brief Tests for reactor_misc.hpp utility functions
//------------------------------------------------------------------------------
#include "test_main.hpp"
#include <fcntl.h>
#include <netinet/in.h>
#include <reactor/reactor_misc.hpp>
#include <reactor/reactor_misc.hpp> // double-include guard
#include <reactor/reactor_platform.hpp>
#include <unistd.h>

// ---- EPollEvents ------------------------------------------------------------

TEST_CASE(epoll_events_empty)
{
  auto s = utxx::EPollEvents(0);
  CHECK(s.empty());
}

TEST_CASE(epoll_events_single)
{
  auto s = utxx::EPollEvents(EPOLLIN);
  CHECK(s == "IN");
}

TEST_CASE(epoll_events_multiple)
{
  auto s = utxx::EPollEvents(EPOLLIN | EPOLLOUT);
  CHECK(s.find("IN") != std::string::npos);
  CHECK(s.find("OUT") != std::string::npos);
  CHECK(s.find('|') != std::string::npos);
}

TEST_CASE(epoll_events_error_flags)
{
  auto s = utxx::EPollEvents(EPOLLERR | EPOLLHUP);
  CHECK(s.find("ERR") != std::string::npos);
  CHECK(s.find("HUP") != std::string::npos);
}

TEST_CASE(epoll_events_et_flag)
{
  auto s = utxx::EPollEvents(EPOLLET);
  CHECK(s == "ET");
}

// ---- SocketError ------------------------------------------------------------

TEST_CASE(socket_error_on_valid_socket)
{
  int fd[2];
  ::socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
  int err = utxx::SocketError(fd[0]);
  CHECK_EQUAL(0, err);
  ::close(fd[0]);
  ::close(fd[1]);
}

// ---- IsBlocking / Blocking --------------------------------------------------

TEST_CASE(blocking_set_clear)
{
  int fd[2];
  ::pipe(fd);

  // pipes are blocking by default
  CHECK(utxx::IsBlocking(fd[0]));

  CHECK(utxx::Blocking(fd[0], false) == false);
  CHECK(!utxx::IsBlocking(fd[0]));

  CHECK(utxx::Blocking(fd[0], true) == true);
  CHECK(utxx::IsBlocking(fd[0]));

  ::close(fd[0]);
  ::close(fd[1]);
}

// ---- Send / SendTo ----------------------------------------------------------

TEST_CASE(send_over_socketpair)
{
  int fd[2];
  ::socketpair(AF_UNIX, SOCK_STREAM, 0, fd);

  int rc = utxx::Send(fd[0], "hello", 5);
  CHECK_EQUAL(5, rc);

  char buf[8] = {};
  int  got    = ::recv(fd[1], buf, sizeof(buf), 0);
  CHECK_EQUAL(5, got);
  CHECK(std::string(buf, 5) == "hello");

  ::close(fd[0]);
  ::close(fd[1]);
}

// ---- ReadUntilEAgain --------------------------------------------------------

TEST_CASE(read_until_eagain_reads_all)
{
  int fd[2];
  ::pipe(fd);
  utxx::Blocking(fd[0], false);

  ::write(fd[1], "abcde", 5);
  ::close(fd[1]);

  utxx::dynamic_io_buffer buf(16);
  int                        total_consumed = 0;

  utxx::ReadUntilEAgain(
      fd[0], buf,
      [&total_consumed](utxx::dynamic_io_buffer& b) -> int {
        int n           = b.size();
        total_consumed += n;
        return n;
      },
      [](const char*, long) {}, "pipe");

  ::close(fd[0]);
  CHECK(total_consumed == 5);
}

TEST_CASE(read_until_eagain_stops_on_limit)
{
  int fd[2];
  ::pipe(fd);
  utxx::Blocking(fd[0], false);

  ::write(fd[1], "1234567890", 10);

  utxx::dynamic_io_buffer buf(4);
  int                        calls = 0;

  utxx::ReadUntilEAgain(
      fd[0], buf,
      [&calls](utxx::dynamic_io_buffer& b) -> int {
        calls++;
        int n = b.size();
        return n;
      },
      [](const char*, long) {}, "pipe",
      2 // max_reads = 2
  );

  ::close(fd[0]);
  ::close(fd[1]);
  CHECK(calls <= 2);
}

// ---- IsUDSAlive -------------------------------------------------------------

TEST_CASE(uds_alive_nonexistent_socket)
{ CHECK(!utxx::IsUDSAlive("/tmp/__reactor_no_such_uds_sock__")); }

// ---- GetIfAddr / GetIfName (smoke, may skip on headless CI) -----------------

TEST_CASE(getifaddr_loopback_by_name)
{
  // lo should always be present; if not present the test is a no-op
  in_addr addr = utxx::GetIfAddr("lo");
  if (addr.s_addr != 0) {
    // 127.0.0.1 = 0x0100007f in network byte order
    CHECK(addr.s_addr == htonl(INADDR_LOOPBACK));
  }
}

// ---- main -------------------------------------------------------------------

int main()
{ return test_harness::run_all(); }
