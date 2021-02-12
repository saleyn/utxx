// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  ReactorFdInfo.cpp
//------------------------------------------------------------------------------
/// \brief Support classes implementation for I/O multiplexing event reactor
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
#include <utxx/io/ReactorFdInfo.hpp>
#include <utxx/io/ReactorLog.hpp>
#include <utxx/io/Reactor.hpp>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace utxx {
namespace io   {

using namespace std;

//------------------------------------------------------------------------------
// FdInfo
//------------------------------------------------------------------------------
FdInfo::
~FdInfo()
{
  if (m_fd >= 0)
    m_owner->Remove(m_fd);
  Clear();
}

//------------------------------------------------------------------------------
void FdInfo::
Clear()
{
  if (m_fd != -1)
    m_owner->Remove(m_fd, false);
  Reset();
}

//------------------------------------------------------------------------------
void FdInfo::
Reset()
{
  assert(m_fd == -1);

  m_handler.Clear();
  m_on_error      = nullptr;
  m_read_at_least = nullptr;
  m_instance      = nullptr;
  m_opaque        = nullptr;
  m_rd_buff_ptr.reset();
  m_rd_buff       = nullptr;
  m_wr_buff_ptr.reset();
  m_wr_buff       = nullptr;
  m_rd_debug      = nullptr;
  m_file_reader.reset();
  m_exec_cmd.reset();
  m_with_pkt_info = false;
  m_sock_src_addr = 0;
  m_sock_src_port = 0;
  m_sock_dst_addr = 0;
  m_sock_dst_port = 0;
  m_sock_if_addr  = 0;

  // TODO: For now leave the cleaned FD's state without deallocating it.
  //       It'll be recycled when this file descriptor is reused.
  //m_owner->m_fds[fd].reset();
}

//------------------------------------------------------------------------------
FdInfo::
FdInfo(FdInfo&& a_rhs) noexcept
  : m_owner         (a_rhs.m_owner)
  , m_name          (std::move(a_rhs.m_name))
  , m_fd            (a_rhs.m_fd)
  , m_handler       (std::move(a_rhs.m_handler))
  , m_on_error      (std::move(a_rhs.m_on_error))
  , m_read_at_least (a_rhs.m_read_at_least)
  , m_instance      (a_rhs.m_instance)
  , m_opaque        (a_rhs.m_opaque)
  , m_rd_buff_ptr   (std::move(a_rhs.m_rd_buff_ptr))
  , m_rd_buff       (m_rd_buff_ptr.get())
  , m_wr_buff_ptr   (std::move(a_rhs.m_wr_buff_ptr))
  , m_wr_buff       (a_rhs.m_wr_buff)
  , m_rd_debug      (std::move(a_rhs.m_rd_debug))
  , m_trigger       (a_rhs.m_trigger)
  , m_with_pkt_info (a_rhs.m_with_pkt_info)
  , m_sock_src_addr (a_rhs.m_sock_src_addr)
  , m_sock_src_port (a_rhs.m_sock_src_port)
  , m_sock_dst_addr (a_rhs.m_sock_dst_addr)
  , m_sock_dst_port (a_rhs.m_sock_dst_port)
  , m_sock_if_addr  (a_rhs.m_sock_if_addr )

{
  a_rhs.m_fd = -1;
}

//------------------------------------------------------------------------------
FdInfo& FdInfo::
operator= (FdInfo&& a_rhs)
{
  assert(m_fd == -1);
  if (Type() != HType::UNDEFINED)
    Clear();

  m_owner         = a_rhs.m_owner;
  m_fd            = a_rhs.m_fd;       a_rhs.m_fd = -1;
  m_handler       = std::move(a_rhs.m_handler);
  m_name          = std::move(a_rhs.m_name);
  m_on_error      = std::move(a_rhs.m_on_error);
  m_read_at_least = a_rhs.m_read_at_least;
  m_instance      = a_rhs.m_instance;
  m_opaque        = a_rhs.m_opaque;
  m_rd_buff_ptr   = std::move(a_rhs.m_rd_buff_ptr);
  m_rd_buff       = m_rd_buff_ptr.get();
  m_wr_buff_ptr   = std::move(a_rhs.m_wr_buff_ptr);
  m_wr_buff       = a_rhs.m_wr_buff;
  m_rd_debug      = std::move(a_rhs.m_rd_debug);
  m_file_reader   = std::move(a_rhs.m_file_reader);
  m_exec_cmd      = std::move(a_rhs.m_exec_cmd);
  m_trigger       = a_rhs.m_trigger;
  m_with_pkt_info = a_rhs.m_with_pkt_info;
  m_sock_src_addr = a_rhs.m_sock_src_addr;
  m_sock_src_port = a_rhs.m_sock_src_port;
  m_sock_dst_addr = a_rhs.m_sock_dst_addr;
  m_sock_dst_port = a_rhs.m_sock_dst_port;
  m_sock_if_addr  = a_rhs.m_sock_if_addr ;

  return *this;
}

//------------------------------------------------------------------------------
void FdInfo::
EnableDgramPktInfo(bool a_enable)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  if (m_fd_type != FdTypeT::Datagram)
    UTXX_THROWX_RUNTIME_ERROR("Cannot ", a_enable ? "enable":"disable",
                              " PktInfo for a non-UDP socket!");
  m_with_pkt_info   = a_enable;
  socklen_t len     = sizeof(a_enable);

  if (setsockopt(m_fd, SOL_IP, IP_PKTINFO, &a_enable, sizeof(a_enable)) < 0)
    UTXX_THROWX_IO_ERROR(errno, "Error setting IP_PKTINFO option on ", m_name);
  if (getsockopt(m_fd, SOL_IP, IP_PKTINFO, &a_enable, &len) < 0 || !a_enable)
    UTXX_THROWX_IO_ERROR
      (errno, "Error IP_PKTINFO option couldn't be set on ", m_name);

  // NOTE: even with IP_PKTINFO enabled recvmsg() doesn't provide sin_port
  // so we have to retrieve it using getsockname():
  struct sockaddr_in     si;
  static const socklen_t si_len = sizeof(si);
  m_sock_dst_port =
    (m_owner && !m_owner->UseGetSockName()) ||
    getsockname(m_fd, (sockaddr*)&si, const_cast<socklen_t*>(&si_len)) < 0
      ? 0
      : si.sin_port;
}

//------------------------------------------------------------------------------
void FdInfo::
PktTimeSamples(bool a_enable)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  if (m_fd_type != FdTypeT::Datagram)
    UTXX_THROWX_RUNTIME_ERROR("Cannot set pkt time samples on a non-UDP socket!");

  m_pkt_time_samples = a_enable;

  if (setsockopt(m_fd, SOL_SOCKET, SO_TIMESTAMPNS, &a_enable, sizeof(a_enable)) < 0)
    UTXX_THROWX_IO_ERROR(errno, "Error setting SO_TIMESTAMPNS option on ", m_name);

  m_ts_wire.clear();
}

//------------------------------------------------------------------------------
void FdInfo::
PktTimeSamples(bool a_enable)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  if (m_fd_type != FdTypeT::Datagram)
    UTXX_THROWX_RUNTIME_ERROR("Cannot set pkt time samples on a non-UDP socket!");

  m_pkt_time_samples = a_enable;

  if (setsockopt(m_fd, SOL_SOCKET, SO_TIMESTAMPNS, &a_enable, sizeof(a_enable)) < 0)
    UTXX_THROWX_IO_ERROR(errno, "Error setting SO_TIMESTAMPNS option on ", m_name);

  m_ts_wire.clear();
}

//------------------------------------------------------------------------------
inline int FdInfo::
ReportError(IOType a_tp, int a_ec, const std::string& a_err,
      utxx::src_info&& a_si, bool a_throw)
{
  UTXX_PRETTY_FUNCTION();

  UTXX_SCOPE_EXIT([this]() { Clear(); });

  auto err = a_ec
           ? utxx::to_string(a_err, " (", a_ec, ": ", strerror(a_ec), ')')
           : a_err;

  if (!m_on_error) {
    if (!a_throw) {
      UTXX_RLOG(this, ERROR, std::move(a_si), this->Ident(), "UNHANDLED error: ", a_err);
      return -1;
    }

    UTXX_SRC_THROW(utxx::runtime_error, a_si, m_ident, err);
  }

  assert(m_on_error);

  // Invoke the user-level callback
  m_on_error(*this, a_tp, err, std::forward<utxx::src_info>(a_si));
  //RSLOG(m_owner, DEBUG, std::move(a_si), "clearing handler on error");
  return -1;
}

//------------------------------------------------------------------------------
long FdInfo::
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
// TODO:
inline long FdInfo::HandlePipe(uint32_t a_events)
{
  UTXX_PRETTY_FUNCTION();

  // FD error condition is handled by the caller prior to making this call
  if (Reactor::IsReadable(a_events)) {
    // Perform Reading:
    assert(m_rd_buff);

    try {
      bool handled;
      int  rc;

      // Read handler for edge-triggered and level-triggered FDs.
      // In this case a continuous read loop is required until we get EAGAIN:
      std::tie(rc, handled) = ReadUntilEAgain
      (
        // Read action -- innermost-level call-back:
        [this](dynamic_io_buffer& a_buf, int a_last_bytes) {
          // Invoke the user-level call-back:
          return m_handler.AsPipe()(*this, a_buf);
        },

        // Debug action:
        m_rd_debug
      );

      if (handled || rc >= 0)
        return rc;

      rc = SocketError(m_fd);
      return ReportError(IOType::Read, rc, "error in Pipe handler", UTXX_SRCX);

    } catch (utxx::runtime_error& e) {
      return ReportError(IOType::UserCode, 0, e.str(), src_info(e.src()));
    } catch (std::exception& e) {
      return ReportError(IOType::UserCode, 0, e.what(), UTXX_SRCX);
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
inline long FdInfo::
HandleFile(uint32_t a_events)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  // Check if there are any pending completion events ready
  auto rc = HandleEvent(a_events, false);

  if (UNLIKELY(rc <= 0))
    return rc;   // Error reporting is already done by HandleEventFD

  auto file = m_file_reader.get();
  assert(file);

  const char* err;
  long        size;

  // Get file read completion event
  std::tie   (size, err) = file->ReadEvents(rc);

  if (UNLIKELY(size < 0))
    return ReportError(IOType::Read, errno, err, UTXX_SRCX);
  else if (UNLIKELY(size == 0)) {
    // Did we reach the end of file?
    if (file->Remaining() == 0) {
      // Note: it's the caller's responsibility to call Clear() on this
      // file descriptor when done reading the file
      m_handler.AsFile()(*this, *m_rd_buff);
      return 0;
    }

    return ReportError(IOType::Read, 0, utxx::to_string
                      ("no file input events detected (remaining: ",
                       file->Remaining(), ')'), UTXX_SRCX);
  }

  // Data was asynchronously written to the buffer:
  m_rd_buff->commit(size);

  bool invoke_cb  = true;
  auto got        = m_rd_buff->size();

  if (m_read_at_least) {
    auto p    = m_rd_buff->rd_ptr();
    auto need = m_read_at_least(p, got);
    // "got"  is the actual number of bytes in the buffer
    // "need" is the expected msg size:
    if (need > got) {
      if (UNLIKELY(need > 100*1024*1024)) {
        auto e = utxx::to_string("suspicious read size = ", need);
        return ReportError(IOType::Read, EMSGSIZE, e, UTXX_SRCX);
      }
      // The msg in the buffer is still incomplete. Enlarge
      // the buffer to the expected msg size of necessary
      m_rd_buff->reserve(need);
      invoke_cb = false;
    }
  }

  // Invoke the user-level call-back:
  if (LIKELY(invoke_cb)) {
    try   {
      int rc = m_handler.AsFile()(*this, *m_rd_buff);
      if (UNLIKELY(rc < 0))
        return ReportError(IOType::Read, 0,
          utxx::to_string("error processing file data ",
                        file->Filename(), " at pos=",
                        file->Position(), " of ", file->Size(),
                        (errno ? ": " : ""),
                        (errno ? strerror(errno) : "")), UTXX_SRCX);
    } catch ( utxx::runtime_error& e ) {
      return ReportError(IOType::UserCode, 0, e.str(), e.src());
    } catch ( std::exception& e ) {
      return ReportError(IOType::UserCode, 0, e.what(), UTXX_SRCX);
    }

    // NOTE: It's the callee's responsibility to adjust the pointer
    // on the buffer by calling:

    //   m_rd_buff->read_and_crunch(consumed_bytes);
  }

  // Did we reach the end of file?
  if (UNLIKELY(!file->Remaining())) {
    UTXX_RLOG(this, DEBUG, "file read done (offset=",
      file->Offset(), ", size=",
      file->Size(),   ')');

    if (m_on_error)
      m_on_error(*this, IOType::EndOfFile, "end-of-file reached", UTXX_SRCX);

    rc = got - m_rd_buff->size();

    // Remove the handler.
    // Note after the following call 'this' pointer is no longer valid!!!
    Clear();
    return rc;
  }

  auto capacity = m_rd_buff->capacity();

  if (UNLIKELY(capacity == 0))
    return ReportError(IOType::UserCode,0,"not enough buffer capacity",UTXX_SRC);

  // Schedule the next read
  rc = file->AsyncRead(m_rd_buff->wr_ptr(), capacity);

  if (UNLIKELY(rc < 0))
    return ReportError(IOType::Read, errno, "error in AsyncRead", UTXX_SRC);

  // Return the number of bytes consumed
  return rc;
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

//------------------------------------------------------------------------------
inline long FdInfo::
HandleSignal(uint32_t a_events)
{
  UTXX_PRETTY_FUNCTION();

  if (UNLIKELY(!Reactor::IsReadable(a_events)))
    return 0;

  assert(m_rd_buff != nullptr);

  // Purge the fd's read buffer
  try {
    bool     handled;
    int      rc;
    std::tie(rc, handled) = ReadUntilEAgain
    (
      [this](dynamic_io_buffer const& a_buf, int a_last_bytes) -> int {
        auto  p = reinterpret_cast<const signalfd_siginfo*>(a_buf.rd_ptr());
        auto  e = reinterpret_cast<const signalfd_siginfo*>(a_buf.wr_ptr());

        // Invoke signal-handling callbacks
        for (auto next = p+1; next <= e; p = next++)
          m_handler.AsSignal()(*this, p->ssi_signo, p->ssi_code);

        // Return number of bytes consumed
        return int(reinterpret_cast<const char*>(p) - a_buf.rd_ptr());
      },
      nullptr // No debug action
    );

    if (LIKELY(handled || rc >= 0))
      return rc;

    return ReportError(IOType::Read, errno, "error reading from signalfd", UTXX_SRCX);
  } catch ( utxx::runtime_error& e ) {
    return ReportError(IOType::UserCode, 0, e.what(), e.src());
  } catch ( std::exception& e ) {
    return ReportError(IOType::UserCode, 0, e.what(), UTXX_SRCX);
  }
}

//------------------------------------------------------------------------------
inline long FdInfo::
HandleAccept(uint32_t a_events)
{
  UTXX_PRETTY_FUNCTION();

  if (UNLIKELY(!Reactor::IsReadable(a_events)))
    return 0;

  // TODO: Since at this time there's only AddUDSListener and no INET listener
  // we only handle accepts for UDS sockets

  struct sockaddr_un addr;
  struct sockaddr*   paddr = (struct sockaddr*)&addr;
  socklen_t          len   = sizeof(addr);
  addr.sun_family          = AF_UNIX;

  // FD error condition is handled by the caller prior to making this call
  while (true) {
    int cli_fd;

    while (UNLIKELY((cli_fd = accept(m_fd, paddr, &len)) < 0 && errno==EINTR));

    if (cli_fd < 0) {
      if (LIKELY(errno == EAGAIN))
        break;
      return ReportError(IOType::Accept, errno, "error in accept(2)", UTXX_SRCX);
    }

    len -= offsetof(struct sockaddr_un, sun_path); // len of pathname
    addr.sun_path[len] = 0;

    Blocking(cli_fd, false);

    // If callback returns true, client added this new fd to the reactor.
    // Else - close the socket.
    if (!m_handler.AsAccept()(*this, addr.sun_path, cli_fd))
      ::close(cli_fd);
  }

  return 0;
}

} // namespace io
} // namespace utxx
