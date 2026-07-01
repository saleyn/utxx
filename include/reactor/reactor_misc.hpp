// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_misc.hpp
//------------------------------------------------------------------------------
/// \brief Miscellaneous Reactor I/O utility functions
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <errno.h>
#include <limits>
#include <netinet/in.h>
#include <reactor/reactor_types.hpp>
#include <signal.h>
#include <string>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace utxx {

//------------------------------------------------------------------------------
// Miscellaneous functions
//------------------------------------------------------------------------------

/// Convert EPoll events bitmask to a descriptive string
std::string EPollEvents(int a_events);

/// Get interface name/ip address responsible for routing \a a_ip address.
void        GetIfAddr(char const* a_ip, std::string& a_ifname, std::string& a_ifip);

/// Get interface ip address from name.
in_addr     GetIfAddr(std::string const& a_ifname);

/// Get interface name for a given interface IP address
std::string GetIfName(in_addr a_ifaddr);

/// Check if a Unix Domain Socket identified by \a a_uds_filename is alive
bool        IsUDSAlive(const std::string& a_uds_filename);

/// Set/clear file descriptor's blocking mode
bool        Blocking(int a_fd, bool a_block);

/// Get blocking property of a file descriptor
inline bool IsBlocking(int a_fd)
{ return !(fcntl(a_fd, F_GETFL, 0) & O_NONBLOCK); }

/// Read the error code on a given socket file descriptor
inline int SocketError(int fd)
{
  int       err = 0;
  socklen_t len = sizeof(err);
  auto      ok  = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&err, &len) == 0;
  return ok ? err : errno;
}

/// Write data to a file descriptor, retrying on EINTR.
#if defined(MSG_NOSIGNAL)
inline int Send(int fd, const char* buf, size_t sz, int flags = MSG_NOSIGNAL)
#else
inline int Send(int fd, const char* buf, size_t sz, int flags = 0)
#endif
{
  int rc;
  while ((rc = ::send(fd, buf, sz, flags)) < 0 && errno == EINTR);
  return rc;
}

/// Write data to a UDP socket, retrying on EINTR.
inline int SendTo(
    int sock, const char* buf, std::size_t sz, int flags, const sockaddr* dest, socklen_t destlen)
{
  int rc;
  while ((rc = ::sendto(sock, buf, sz, flags, dest, destlen)) < 0 && errno == EINTR);
  return rc;
}

//------------------------------------------------------------------------------
/// @brief Read from a non-blocking FD into a buffer until EAGAIN.
///
/// @tparam Action (dynamic_io_buffer const*) -> number_of_bytes_consumed
/// @tparam DebugAction void(char const* buf, size_t sz)
///
/// @return true  if EAGAIN was hit (or an error); false if a_max_reads reached
//------------------------------------------------------------------------------
template <typename Action, typename DebugAction>
inline bool ReadUntilEAgain(
    int a_fd, dynamic_io_buffer& a_buf, Action const& a_action, DebugAction const& a_debug_action,
    std::string const& a_name = "", int a_max_reads = std::numeric_limits<int>::max())
{
  for (int i = 0; i < a_max_reads; i++) {
    size_t space = a_buf.capacity();
    if (UNLIKELY(space == 0))
      UTXX_THROW(utxx::badarg_error, "buffer overflow (fd='", a_name, "')");

    void* buf = a_buf.wr_ptr();
    long  n;

    while (UNLIKELY((n = ::read(a_fd, buf, space)) < 0 && errno == EINTR));

    if (UNLIKELY(n < 0 && errno == EAGAIN))
      return true;
    else if (UNLIKELY(n == 0))
      return true; // EOF (pipe closed / peer disconnected)
    else if (UNLIKELY(n < 0)) {
      int ec = SocketError(a_fd);
      if (ec == 0) ec = errno;
      if (ec == 0) return true;
      UTXX_THROWX_IO_ERROR(ec, "error on fd='", a_name, "'");
    }

    a_debug_action(static_cast<char const*>(buf), n);

    assert(n > 0);
    a_buf.commit(n);

    int consumed = a_action(a_buf);
    if (consumed > 0) a_buf.read_and_crunch(consumed);
  }
  return false;
}

/// Overload without a debug action
template <typename Action>
inline void ReadUntilEAgain(
    int fd, dynamic_io_buffer& b, Action const& a, std::string const& a_name = "",
    int a_max_reads = std::numeric_limits<int>::max())
{
  ReadUntilEAgain(fd, b, a, [](char const*, long) {}, a_name, a_max_reads);
}

} // namespace utxx
