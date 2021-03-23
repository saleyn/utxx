// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  ReactorFdInfo.hpp
//------------------------------------------------------------------------------
/// \brief I/O multiplexing event reactor's file descriptor state
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

#include <utxx/io/ReactorFdInfo.hpp>
#include <utxx/io/Reactor.hxx>

namespace utxx {
namespace io   {

//------------------------------------------------------------------------------
inline FdInfo::FdInfo
(
  Reactor*            a_owner,
  std::string const&  a_name,
  int                 a_fd,
  FdTypeT             a_fd_type,
  ErrHandler  const&  a_on_error,
  void*               a_instance,
  void*               a_opaque,
  int                 a_rd_bufsz,
  int                 a_wr_bufsz,
  dynamic_io_buffer*  a_wr_buf,
  ReadSizeEstim       a_read_sz_fun,
  TriggerT            a_trigger
)
  : m_owner           (a_owner)
  , m_name            (a_name)
  , m_fd              (a_fd)
  , m_fd_type         (FdTypeT::UNDEFINED)
  , m_handler         ()
  , m_on_error        (a_on_error)
  , m_read_at_least   (std::move(a_read_sz_fun))
  , m_instance        (a_instance)
  , m_opaque          (a_opaque)
  , m_rd_buff_ptr     (a_rd_bufsz ? new dynamic_io_buffer(a_rd_bufsz) : nullptr)
  , m_rd_buff         (m_rd_buff_ptr.get())
  , m_wr_buff_ptr     (!a_wr_buf && a_wr_bufsz
                      ? new dynamic_io_buffer(a_wr_bufsz) : nullptr)
  , m_wr_buff         (a_wr_buf   ? a_wr_buf : m_wr_buff_ptr.get())
  , m_trigger         (a_trigger)
  , m_ident           (Ident(a_owner, a_fd, a_name))
{
  int       type;
  socklen_t len  = sizeof(type);
  if (m_fd > -1 && a_fd_type == FdTypeT::UNDEFINED &&
      getsockopt(m_fd, SOL_SOCKET, SO_TYPE, &type, &len) == 0) {
    switch (type) {
      case SOCK_STREAM:    m_fd_type = FdTypeT::Stream;    break;
      case SOCK_DGRAM:     m_fd_type = FdTypeT::Datagram;  break;
      case SOCK_SEQPACKET: m_fd_type = FdTypeT::SeqPacket; break;
      default:                                             break;
    }
  }
}

//------------------------------------------------------------------------------
inline std::string FdInfo::
Ident(Reactor* a_reactor, int a_fd, std::string const& a_name)
{
  if (!a_reactor)
    return utxx::to_string("[@", a_fd, '(', a_name, ")] ");
  auto s = a_reactor->Ident();
  auto it = s.find(']');
  if (it != std::string::npos)
    s.erase(it);
  return utxx::to_string(s, '@', a_fd, '(', a_name, ")] ");
}

//------------------------------------------------------------------------------
template<typename Action>
inline void FdInfo::
SetHandler(HType a_type, const Action& a)
{
  m_handler = HandlerT(a_type, a);
}

//------------------------------------------------------------------------------
inline int FdInfo::
Debug() const
{
  assert(m_owner);
  return m_owner->Debug();
}

//------------------------------------------------------------------------------
inline int FdInfo::
ReportError(IOType a_tp, int a_ec, const std::string& a_err,
      const utxx::src_info& a_si, bool a_throw)
{
  // Call the alternative method taking the src_info as an rvalue
  return ReportError(a_tp, a_ec, a_err, utxx::src_info(a_si), a_throw);
}

//------------------------------------------------------------------------------
template <class... Args>
inline void FdInfo::
Log(utxx::log_level a_level, utxx::src_info&& a_si, Args&&... args)
{
  assert(m_owner);
  return m_owner->Log(a_level, std::move(a_si), std::forward<Args>(args)...);
}

//------------------------------------------------------------------------------
// TODO: use rcvmmsg() for performance
//------------------------------------------------------------------------------
template<typename Action, typename ReadDebugAction>
inline std::pair<int, bool> FdInfo::
ReadUntilEAgain(Action const& a_action, ReadDebugAction const& a_debug_act)
{
  UTXX_PRETTY_FUNCTION();

  int   bytes = 0;
  int   got;
  long  space;
  char* buf;

  struct iovec iov[1];
  struct msghdr msg;
  struct sockaddr_in peeraddr;        // Remote/src addr goes here
  char   ctlbuf[256];

  if (m_with_pkt_info) {
    //memset(&msg, '\0', sizeof(msg));
    msg.msg_name        = (void*)&peeraddr;
    msg.msg_namelen     = sizeof (peeraddr);
    msg.msg_iov         = iov;
    msg.msg_iovlen      = 1;
    msg.msg_flags       = 0;
    msg.msg_control     = ctlbuf;
    msg.msg_controllen  = sizeof(ctlbuf);
  }

  // We must continue until EAGAIN is encountered or a_max_reads limit
  // is reached, or we'll lose an edge-triggered event:
  do {
    buf   = m_rd_buff->wr_ptr();
    space = m_rd_buff->capacity(); // This is a remaining capacity!

    if (UNLIKELY(space == 0)) {
      errno  = ENOBUFS;
      int rc = ReportError(IOType::Read, errno, "buffer overflow", UTXX_SRC);
      return std::make_pair(rc, true);
    }

    do {
      // TODO: use rcvmmsg()
      // http://man7.org/linux/man-pages/man2/recvmmsg.2.html
      if (m_with_pkt_info) {
        iov[0].iov_base = buf;
        iov[0].iov_len  = space;
        got = ::recvmsg(m_fd, &msg, 0);
      } else
        got = ::read(m_fd, buf, space);
    } while (UNLIKELY(got < 0 && errno == EINTR));

    if (UNLIKELY(got <= 0)) {
      if (got == 0) {
        // Connection is closed
        return std::make_pair(got, false);
      }
      // errno == EGAIN means that no more data is available.
      // Action is not to be invoked here if there is no new data:
      bool handled = errno == EAGAIN;
      return std::make_pair(got, handled);
    }

    // Otherwise: successful read returning 0 or more bytes read.
    // If it's 0 - on connected sockets this means that it got disconnected.
    // Commit the data into the buffer invoke the Action, and continue.

    if (m_with_pkt_info) {
      m_sock_src_addr = peeraddr.sin_addr.s_addr;
      m_sock_src_port = peeraddr.sin_port;
      m_sock_if_addr  = 0;
      m_sock_dst_addr = 0;
      int cont        = 1 + m_pkt_time_stamps;
      // Control messages are always accessed via macros
      // http://www.kernel.org/doc/man-pages/online/pages/man3/cmsg.3.html
      for(auto cm = CMSG_FIRSTHDR(&msg); cm && cont; cm = CMSG_NXTHDR(&msg, cm)) {
        // ignore the control headers that don't match what we want
        if (cm->cmsg_level == IPPROTO_IP && cm->cmsg_type == IP_PKTINFO)
        {
          auto pi = reinterpret_cast<struct in_pktinfo*>(CMSG_DATA(cm));
          m_sock_if_addr   = pi->ipi_spec_dst.s_addr; // Iface addr
          m_sock_dst_addr  = pi->ipi_addr.s_addr;     // Mcast addr
          cont--;
        } else if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPNS) {
          auto   ts  = reinterpret_cast<timespec const*>(CMSG_DATA(cm));
          assert(ts != nullptr);
          m_ts_wire  = time_val(*ts);
          if (LIKELY(m_pkt_time_stamps))
            cont--;
        }
      }
    }

    DebugFunEval::run(a_debug_act, buf, got);

    if (LIKELY(got > 0))
      m_rd_buff->commit(got);

    bytes += got;

    // See if there are enough bytes available in the buffer
    if (m_read_at_least) {
      try {
        // "got"  is the actual number of bytes in the buffer
        // "need" is the expected msg size:
        auto p    = m_rd_buff->rd_ptr();
        auto have = m_rd_buff->size();
        int  need = m_read_at_least(p, have);
        if  (need > got) {
          if (UNLIKELY(need > 100*1024*1024)) {
            errno = EMSGSIZE;
            auto e = utxx::to_string("suspicious read size = ", need);
            return std::make_pair
              (ReportError(IOType::Read, errno, e, UTXX_SRC), true);
          }
          // The msg in the buffer is still incomplete. Enlarge
          // the buffer to the expected msg size of necessary
          m_rd_buff->reserve(need);
          return std::make_pair(bytes, false);
        }
      } catch (utxx::runtime_error& e) {
        return std::make_pair
            (ReportError(IOType::Read, 0, e.str(), e.src()), true);
      } catch (std::exception& e) {
        return std::make_pair
            (ReportError(IOType::Read, 0, e.what(),UTXX_SRC), true);
      }
    }

    // Execute the action on the data in the buffer.
    // It returns the number of bytes consumed or a negative value
    // indicating that there was an error, and that further reading
    // must be aborted.
    try {
      int consumed = a_action(*m_rd_buff, got);
      // m_rd_buff might be freed by the a_action
      if (UNLIKELY(consumed < 0) || !m_rd_buff)
        return std::make_pair(consumed, false);
      else if (LIKELY(consumed  > 0))
        // Can move partially incomplete data to the front of buffer
        m_rd_buff->read_and_crunch(consumed);

    } catch (utxx::runtime_error& e) {
      return std::make_pair
          (ReportError(IOType::Read, 0, e.str(), e.src()), true);
    } catch (std::exception& e) {
      return std::make_pair
          (ReportError(IOType::Read, 0, e.what(), UTXX_SRC), true);
    }

  } while (/*got == space && */ m_trigger == EDGE_TRIGGERED);

  return std::make_pair(bytes, true);
}

//------------------------------------------------------------------------------
inline long FdInfo::
Handle(uint32_t a_events)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  switch (Type()) {
    case HType::IO:     return HandleIO    (a_events);
    case HType::RawIO:  return HandleRawIO (a_events);
    case HType::Pipe:   return HandlePipe  (a_events);
    case HType::File:   return HandleFile  (a_events);
    case HType::Event:  return HandleEvent (a_events, true);
    case HType::Timer:  return HandleTimer (a_events);
    case HType::Accept: return HandleAccept(a_events);
    case HType::Signal: return HandleSignal(a_events);
    default:
      UTXX_RLOG(this, DEBUG, "fd=", m_fd, " undefined handler type");
      return -1;
  }
}

//------------------------------------------------------------------------------
// Inlined on critical path
// The actual read/write ops are invoked from here:
// (*) for read,  it is  ReadUntilEAgain()
// (*) for write, ...???
//------------------------------------------------------------------------------
inline long FdInfo::
HandleIO(uint32_t a_events)
{
  UTXX_PRETTY_FUNCTION();

  bool handled;
  int  rc;

  // FD error condition is handled by the caller prior to making this call
  if (Reactor::IsReadable(a_events)) {
    // Perform Reading:
    assert(m_rd_buff);

    try {

      // Read handler for edge-triggered and level-triggered FDs.
      // In this case a continuous read loop is required until we get EAGAIN:
      std::tie(rc, handled) = ReadUntilEAgain
      (
        // Read action -- innermost-level call-back:
        [this](dynamic_io_buffer& a_buf, int a_last_bytes) {
          // Invoke the user-level call-back:
          return m_handler.AsIO().rh(*this, a_buf);
        },

        // Debug action:
        m_rd_debug
      );

      // When m_fd < 0, it means that the file descriptor was closed by user
      if (handled || rc > 0 || m_fd < 0)
        return rc;

      rc = SocketError(m_fd);
      auto err = rc ? strerror(rc) : "connection closed by peer";
      return ReportError(IOType::Read, rc, err, UTXX_SRCX);

    } catch (utxx::runtime_error& e) {
      return ReportError(IOType::UserCode, 0, e.str(), src_info(e.src()));
    } catch (std::exception& e) {
      return ReportError(IOType::UserCode, 0, e.what(), UTXX_SRCX);
    }
  } else if (Reactor::IsWritable(a_events)) {
    // Perform writing:
    assert(m_handler.AsIO().wh);
    assert(m_wr_buff);

    // Unlike io.rh which is an after-read call-back, io.wh should
    // typically contain a write/send syscall itself.
    // TODO: better call "write" immediately here:
    try   { return m_handler.AsIO().wh(*this, *m_wr_buff); }
    catch ( utxx::runtime_error& e) {
      return ReportError(IOType::UserCode, 0, e.str(),  src_info(e.src())); }
    catch ( std::exception& e) {
      return ReportError(IOType::UserCode, 0, e.what(), UTXX_SRCX);
    }
  } else {
    return 0;
  }

  // It is really impossible to get here.
  return ReportError(IOType::AppLogic, 0, "LOGIC ERROR: unhandled branch", UTXX_SRCX);
}

//------------------------------------------------------------------------------
// Inlined on critical path
inline long FdInfo::
HandleRawIO (uint32_t a_events)
{
  UTXX_PRETTY_FUNCTION();

  auto io_type = (a_events & EPOLLOUT) ? IOType::Write : IOType::Read;

  assert( m_handler.AsRawIO() != nullptr );
  try   { m_handler.AsRawIO()(*this, io_type, a_events); }
  catch ( utxx::runtime_error& e ) {
    return ReportError(IOType::Read, 0, e.what(), e.src()); }
  catch ( std::exception& e ) {
    return ReportError(IOType::Read, 0, e.what(), UTXX_SRCX);
  }

  return 0;
}

//------------------------------------------------------------------------------
inline long FdInfo::
HandleEvent (uint32_t a_events, bool a_invoke_handler)
{
  UTXX_PRETTY_FUNCTION();

  static const char s_err[] = "error reading eventfd";

  if (UNLIKELY(!Reactor::IsReadable(a_events)))
    return 0;

  int64_t events = 0;
  int n;

  // TODO: We can optimize this code in some cases by eliminating the reads.
  // When using edge-triggered notifications, if you signal an event by
  // modifying a descriptor's epoll registration then you will get an event
  // through epoll_wait telling you the current state of the descriptor:

  // Signal:
  //      epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, ...);

  // Wait:
  //      num_events = epoll_wait(...)
  //      for (int i = 0; i < num_events; ++i)
  //          if (events[i].data.fd == event_fd)
  //              ... event_fd got signaled!

  // which is saving the need for read(2) system calls

  do    { n = ::read(m_fd, &events, sizeof(events)); }
  while (UNLIKELY(n < 0 && errno == EINTR));

  if (UNLIKELY(n <= 0)) {
    // In case of edge-triggered FD, EAGAIN clears the edge:
    if (n < 0 && errno == EAGAIN)
      return 0;

    return ReportError(IOType::Read, errno, s_err, UTXX_SRCX);
  }

  if (a_invoke_handler) {
    try   { m_handler.AsEvent()(*this, events); }
    catch ( utxx::runtime_error& e ) {
      return ReportError(IOType::UserCode, 0, e.what(), e.src()); }
    catch ( std::exception& e ) {
      return ReportError(IOType::UserCode, 0, e.what(), UTXX_SRCX);
    }
  }

  return events;
}

//------------------------------------------------------------------------------
inline long FdInfo::
HandleTimer (uint32_t a_events)
{
  UTXX_PRETTY_FUNCTION();

  if (UNLIKELY(!Reactor::IsReadable(a_events)))
    return 0;

  int      got;
  uint64_t exp = 0;

  while (UNLIKELY((got = ::read(m_fd,&exp,sizeof(exp)) < 0) && errno == EINTR));

  if (UNLIKELY(got < 0))
    return ReportError(IOType::Read, errno, "error reading from timerfd", UTXX_SRCX);

  // got == 0 means no timer expirations since last read
  // Invoke the user callback
  try   { m_handler.AsTimer()(*this, exp); }
  catch ( utxx::runtime_error& e ) {
    return ReportError(IOType::UserCode, 0, e.what(), e.src()); }
  catch ( std::exception& e ) {
    return ReportError(IOType::UserCode, 0, e.what(), UTXX_SRCX);
  }

  return got;
}

} // namespace io
} // namespace utxx
