// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  ReactorMisc.hpp
//------------------------------------------------------------------------------
/// \brief Miscelaneous Reactor I/O utility functions
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string>
#include <netinet/in.h>
#include <utxx/compiler_hints.hpp>
#include <utxx/buffer.hpp>
#include <utxx/error.hpp>
#include <utxx/io/ReactorTypes.hpp>

namespace utxx {
namespace io   {

using utxx::src_info;

//------------------------------------------------------------------------------
// Miscelaneous functions
//------------------------------------------------------------------------------

/// Convert EPoll events to string
std::string EPollEvents(int a_events);

/// Get interface name/ip address responsible for routing \a a_ip address.
/// @param a_ip     destination IP address
/// @param a_ifname set to the name of the interface used for routing the IP
/// @param a_ifip   set to the addr of the interface used for routing the IP
void GetIfAddr(char const* a_ip, std::string& a_ifname, std::string& a_ifip);

/// Get interface ip address from name.
/// @return in case of an error returns 0 address, otherwise the IP address that
///         corresponds to the given interface name.
in_addr GetIfAddr(std::string const& a_ifname);

/// Get interface name for a given interface IP address
std::string GetIfName(in_addr a_ifaddr);

/// Check if a Unix Domain Socket identified by \a a_uds_filename is alive
bool IsUDSAlive(const std::string& a_uds_filename);

/// Set/clear file descriptor's blocking mode
bool Blocking(int a_fd, bool a_block);

/// Get blocking property of a file descriptor
inline bool IsBlocking(int a_fd) { return !(fcntl(a_fd,F_GETFL,0) & O_NONBLOCK); }

/// Read the error code on a given socket file descriptor
inline int SocketError(int fd) {
  // Do not use the readily-available "errno", extract the code explicitly
  int       err = 0;
  socklen_t len = sizeof(err);

  auto   ok = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&err, &len) == 0;
  return ok ? err : errno;
}

/// Write data to file descriptor.
/// Same as send(2), but loops in case the operation is interrupted by signal.
inline int Send(int fd, const char* buf, size_t sz, int flags = MSG_NOSIGNAL) {
  int    rc;
  while((rc = ::send(fd, buf, sz, flags)) < 0 && errno == EINTR);
  return rc;
}

/// Write data to udp socket.
/// Same as send(2), but loops in case the operation is interrupted by signal.
inline int SendTo
(
  int sock, const char* buf, std::size_t sz, int flags,
  const sockaddr* dest, socklen_t destlen
) {
  int    rc;
  while((rc = ::sendto(sock, buf, sz, flags, dest, destlen)) < 0 && errno==EINTR);
  return rc;
}

//------------------------------------------------------------------------------
/// @brief Read from a non-blocking FD into a buffer until EAGAIN.
///
/// Fill in a buffer from a non-blocking FD (eg a socket) while data are
/// available, and invoke a user-provided generic action to consume that
/// data.
///
/// @tparam Action (dynamic_io_buffer const*) -> number_of_bytes_consumed
///   Action can scan the buffer, but does not update it; "read" (actually
///   meaning "pop") and "crunch" are performed by "ReadUnyilEAgain" itself.
///
/// @tparam DebugAction void (char const* buf, size_t sz);
///   DebugAction allows to insert some debug processing of the data read
///   from the file secriptor.  It does nothing by default.
///
/// @param a_fd           file descriptor
/// @param a_buf          user-space buffer to read data into from socket
/// @param a_action       functor to act on data put in buffer
/// @param a_debug_action functor to be called to process raw data
/// @param a_max_reads    maximum number of reads (should be INT_MAX if
///                       \a a_fd is edge-triggered, or 1 if level-driggered)
/// @return true  - if received EAGAIN error or some other error condition;
///         false - if terminated because of reaching \a a_max_reads limit.
//------------------------------------------------------------------------------
template<typename Action, typename ReadDebugAction>
inline bool ReadUntilEAgain
(
  int                      a_fd,
  dynamic_io_buffer&       a_buf,
  Action            const& a_action,
  ReadDebugAction   const& a_debug_action,
  std::string       const& a_name      = "",
  int                      a_max_reads = std::numeric_limits<int>::max()
)
{
  // We must continue until EAGAIN is encountered or a_max_reads limit
  // is reached, or we'll lose an edge-triggered event:
  for (int i=0; i < a_max_reads; i++) {
    size_t space = a_buf.capacity(); // This is a remaining capacity!
    if (UNLIKELY(space == 0))
      UTXX_THROW(utxx::badarg_error, "buffer overflow (fd='", a_name, "')");

    void* buf = a_buf.wr_ptr();
    long  n;

    // Skip EINTR:
    while (UNLIKELY((n = ::read(a_fd, buf, space) < 0) && errno==EINTR));

    if (UNLIKELY(n < 0 && errno == EAGAIN))
      // No more data is available -- this is a normal end of reading.
      // Action is not to be invoked here, as there is no new data:
      return true;
    else if (UNLIKELY(n <= 0)) {
      int ec = SocketError(a_fd);
      if (ec == 0) // Disconnected by user
          return true;
      // This is a real read error: propagate an exception:
      UTXX_THROW(utxx::io_error, ec, "error on fd='", a_name, "')");
    }

    a_debug_action(static_cast<char const*>(buf), n);

    // Otherwise: successful read, commit the data into the buffer,
    // invoke the Action, and continue:
    assert(n > 0),
    a_buf.commit(n);

    int consumed  = a_action(a_buf);
    if (consumed  > 0)
      // Can move partially incomplete data to the beginning of buffer
      a_buf.read_and_crunch(consumed);
  }
  // This point is only reached if a_max_reads is specified:
  return false;
}

//------------------------------------------------------------------------------
// Same as prevous function but without DebugAction
//------------------------------------------------------------------------------
template<typename Action>
inline void ReadUntilEAgain(int fd, dynamic_io_buffer& b,
                      Action const& a, std::string const& a_name = "",
                      int a_max_reads = std::numeric_limits<int>::max())
{ ReadUntilEAgain(fd, b, a, [](char const*, long) {}, a_name, a_max_reads); }

} // namespace io
} // namespace utxx
