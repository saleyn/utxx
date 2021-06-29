// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  Reactor.cpp
//------------------------------------------------------------------------------
/// \brief I/O multiplexing event reactor
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
#include <utxx/io/ReactorLog.hpp>
#include <utxx/io/Reactor.hpp>
#include <boost/filesystem/operations.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/scope_exit.hpp>
#include <utxx/signal_block.hpp>
#include <cassert>
#include <regex>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sched.h>
#include <fcntl.h>
#include <stdio.h>

namespace utxx {
namespace io   {

using namespace std;

//------------------------------------------------------------------------------
// Reactor
//------------------------------------------------------------------------------
Reactor
::~Reactor()
{
  if (m_own_efd) {
    (void) close(m_epoll_fd);
    m_epoll_fd = -1;
  }
}

//------------------------------------------------------------------------------
void Reactor
::SetBuffer(int a_fd, bool a_is_read, dynamic_io_buffer* a_buf)
{
  UTXX_PRETTY_FUNCTION();
  auto& info = Get(a_fd, UTXX_SRCX);
  auto& bufp = a_is_read ? info.m_rd_buff_ptr : info.m_wr_buff_ptr;
  auto& buf  = a_is_read ? info.m_rd_buff     : info.m_wr_buff;

  buf = a_buf;
  bufp.reset();
}

//------------------------------------------------------------------------------
void Reactor
::ResizeBuffer(int a_fd, bool a_is_read, size_t a_size)
{
  UTXX_PRETTY_FUNCTION();

  if (!a_size)
    return;

  auto& info = Get(a_fd, UTXX_SRCX);
  auto& bufp = a_is_read ? info.m_rd_buff_ptr : info.m_wr_buff_ptr;
  auto& buf  = a_is_read ? info.m_rd_buff     : info.m_wr_buff;

  if (buf)
    // Resize previously allocated buffer
    buf->reserve(a_size);
  else {
    // No buffer previously allocated - allocate the buffer
    bufp.reset(new dynamic_io_buffer(a_size));
    buf = bufp.get();
  }
}

//------------------------------------------------------------------------------
void Reactor
::EPollAdd(int fd, std::string const& a_nm, uint a_events, utxx::src_info&& a_si)
{
  epoll_event ev = { a_events, {0} };
  ev.data.fd = fd;

  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
    UTXX_SRC_THROW(utxx::io_error, std::move(a_si),
                   errno, "epoll_ctl failed adding '", a_nm, "', fd=", fd);
}

//------------------------------------------------------------------------------
FdInfo* Reactor
::Set
(
  std::string const&  a_name,
  int                 a_fd,
  FdTypeT             a_fd_type,
  uint32_t            a_events,
  utxx::src_info&&    a_src,
  ErrHandler const&   a_on_error,
  void*               a_instance,
  void*               a_opaque,
  int                 a_rd_bufsz,
  int                 a_wr_bufsz,
  dynamic_io_buffer*  a_wr_buf,
  ReadSizeEstim       a_read_sz_fun,
  TriggerT            a_trig
)
{
  assert(a_fd >= 0);

  EPollAdd(a_fd, a_name, a_events, std::move(a_src));

  if (a_fd >= int(m_fds.size())) {
    size_t n = a_fd + 64;
    m_fds.resize(n);
  }

  auto p = new FdInfo(this, a_name,  a_fd,     a_fd_type,  a_on_error,
                      a_instance,    a_opaque, a_rd_bufsz, a_wr_bufsz, a_wr_buf,
                      a_read_sz_fun, a_trig);
  m_fds[a_fd].reset(p);
  return p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor
::Add
(
  string const&      a_name    ,
  int                a_fd      ,
  RWIOHandler const& a_on_read ,
  RWIOHandler const& a_on_write,
  ErrHandler  const& a_on_error,
  void*              a_inst    ,
  void*              a_opaque  ,
  int                a_rd_bufsz,
  int                a_wr_bufsz,
  dynamic_io_buffer* a_wr_buf  ,
  ReadSizeEstim      a_read_at_least,
  TriggerT           a_trigger
)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  if (UNLIKELY(a_fd < 0))
    UTXX_THROWX_BADARG_ERROR("invalid fd=", a_fd);

  uint32_t events = EPOLLIN | EPOLLERR | EPOLLRDHUP |
            (a_trigger == EDGE_TRIGGERED ? EPOLLET : 0) |
            (a_on_write ? EPOLLOUT : 0);

  UTXX_RLOG(this, TRACE5, "adding IO handler '", a_name, "', fd=", a_fd,
      ", RdBuf=", a_rd_bufsz, ", WrBuf=", a_wr_bufsz,
      ", Events=", EPollEvents(events), ", Opaque=", a_opaque);

  auto p = Set(a_name, a_fd, FdTypeT::UNDEFINED, events, UTXX_SRCX,
               a_on_error, a_inst, a_opaque, a_rd_bufsz, a_wr_bufsz,
               a_wr_buf, a_read_at_least, a_trigger);
  p->SetHandler(HType::IO, HandlerT::IO{a_on_read, a_on_write});

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor
::Add
(
  const std::string&  a_name,
  int                 a_fd,
  RawIOHandler const& a_on_io,
  ErrHandler   const& a_on_error,
  void*               a_opaque,
  uint32_t            a_events,
  size_t              a_rd_bufsz
)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  if (UNLIKELY(a_fd < 0))
    UTXX_THROWX_BADARG_ERROR("invalid fd=", a_fd);

  UTXX_RLOG(this, TRACE5, "adding RawIO handler '",
        a_name, "', fd=", a_fd, ", Events=", EPollEvents(a_events),
        ", Opaque=", a_opaque);

  auto trigger = (a_events & EPOLLET) ? EDGE_TRIGGERED : LEVEL_TRIGGERED;
  auto p       = Set(a_name, a_fd, FdTypeT::UNDEFINED, a_events, UTXX_SRCX,
                     a_on_error, nullptr, a_opaque,
                     a_rd_bufsz, 0, nullptr, nullptr, trigger);
  p->SetHandler(HType::RawIO, a_on_io);

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor
::AddFile
(
  std::string   const& a_name    ,
  std::string   const& a_filename,
  FileHandler   const& a_on_read ,
  ErrHandler    const& a_on_error,
  void*                a_instance,
  void*                a_opaque,
  size_t               a_bufsz,
  ReadSizeEstim        a_read_at_least,
  TriggerT             a_trigger
)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  int efd = eventfd(0, EFD_NONBLOCK);
  if (efd < 0)
    UTXX_THROWX_IO_ERROR(errno, "eventfd");

  utxx::scope_exit guard1([efd]() { ::close(efd); });

  auto p = Set(a_name, efd, FdTypeT::File,
               EPOLLIN|EPOLLET|EPOLLERR, UTXX_SRCX,
               a_on_error,
               a_instance,      // a_instance
               a_opaque,        // a_opaque
               a_bufsz,
               0,               // a_wr_bufsz
               nullptr,         // a_wr_buf
               a_read_at_least, // read_fun
               a_trigger        // level or edge triggered
              );

  p->SetHandler(HType::File, a_on_read);

  UTXX_RLOG(this, TRACE5, "adding File reader '", a_name, "' for file ", a_filename,
     ", Opaque=",  a_opaque);

  guard1.disable();

  auto reader = new AIOReader(efd, a_filename.c_str());
  p->SetFileReader(reader);

  assert(p->RdBuff());

  p->FileReader()->AsyncRead(p->RdBuff()->wr_ptr(), p->RdBuff()->capacity());

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor
::AddPipe
(
  std::string   const& a_name    ,
  std::string   const& a_command ,
  PipeHandler   const& a_on_read ,
  ErrHandler    const& a_on_error,
  void*                a_opaque,
  size_t               a_rd_bufsz, // When =/= 0, buffer is created
  ReadSizeEstim        a_read_at_least
)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  // TODO: implement
  UTXX_THROWX_RUNTIME_ERROR("not implemented!");

  std::unique_ptr<POpenCmd> exe;

  try {
    exe = std::unique_ptr<POpenCmd>(new POpenCmd(a_command));
  } catch (std::exception& e) {
    UTXX_THROWX_BADARG_ERROR('[', a_name, "] cannot open pipe: ", e.what());
  }

  UTXX_RLOG(this, TRACE5, "adding Pipe handler '", a_name, "', fd=", exe->FD(),
     ", Opaque=",  a_opaque);

  auto p = Set(a_name, exe->FD(), FdTypeT::Pipe,
               EPOLLIN|EPOLLET|EPOLLERR, UTXX_SRCX,
               a_on_error, nullptr, a_opaque, a_rd_bufsz, 0,
               nullptr,    nullptr, LEVEL_TRIGGERED);
  p->SetHandler(HType::Pipe, a_on_read);
  p->m_exec_cmd.reset(exe.release());

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor
::AddUDSListener
(
  std::string   const& a_name,
  std::string   const& a_file_path,
  AcceptHandler const& a_on_accept,
  ErrHandler    const& a_on_error,
  void*                a_opaque,
  int                  a_permissions
)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  if (utxx::path::file_exists(a_file_path))
  {
    if (IsUDSAlive(a_file_path))
      UTXX_THROWX_RUNTIME_ERROR
        ("'", a_name, "' cannot start server - UDS file busy: '",
         a_file_path, "'");

    if (!utxx::path::file_unlink(a_file_path))
      UTXX_THROWX_RUNTIME_ERROR
        ("'", a_name, "' couldn't remove existing UDS file: '",
         a_file_path, "'");
  }

  int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd < 0)
    UTXX_THROWX_IO_ERROR(errno, '[', a_name,
                 "] couldn't create an UDS server socket");

  int on = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  Blocking(listen_fd, false);

  struct sockaddr_un address = { 0 };
  address.sun_family = AF_UNIX;
  strncpy(address.sun_path, a_file_path.c_str(), sizeof(address.sun_path));
  address.sun_path[sizeof(address.sun_path)-1] = '\0';

  if (bind(listen_fd, (struct sockaddr *)&address,
       sizeof(struct sockaddr_un)) < 0)
    UTXX_THROWX_IO_ERROR(errno, '[', a_name,
                 "] bind of fd=" , listen_fd, " failed");

  if (listen(listen_fd, 5) < 0)
    UTXX_THROWX_IO_ERROR(errno, '[', a_name,
                 "] listen on fd=" , listen_fd, " failed");
  //boost::filesystem::permissions
  //  (a_file_path, boost::filesystem::perms(a_permissions));

  uint events = EPOLLIN | EPOLLET | EPOLLERR;

  UTXX_RLOG(this, TRACE5, "adding UDS Listener '",
      a_name, ", Events=", EPollEvents(events), "', fd=", listen_fd,
      ", Opaque=", a_opaque);

  auto p = Set(a_name, listen_fd, FdTypeT::UNDEFINED,
               events, UTXX_SRCX, a_on_error);
  p->SetHandler(HType::Accept, a_on_accept);

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor
::AddEvent
(
  std::string   const& a_name,
  EventHandler  const& a_on_read,
  ErrHandler    const& a_on_error,
  void*                a_opaque
)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  int fd = eventfd(0, EFD_NONBLOCK);

  if (fd < 0)
    UTXX_THROWX_IO_ERROR(errno, "eventfd");

  UTXX_RLOG(this, TRACE5, "adding Event '", a_name, ", fd=", fd,
     ", Opaque=", a_opaque);

  auto p = Set(a_name, fd, FdTypeT::Event,
               EPOLLIN | EPOLLET | EPOLLERR, UTXX_SRCX,
               a_on_error, nullptr, a_opaque);
  p->SetHandler(HType::Event, a_on_read);

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor
::AddTimer
(
  string       const& a_name,
  uint32_t            a_initial_msec,
  uint32_t            a_interval_msec,
  EventHandler        a_fun,
  ErrHandler          a_on_error,
  void*               a_opaque
)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  int fd;

  if ((fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK)) < 0)
    UTXX_THROWX_IO_ERROR(errno, "cannot create timer");

  auto     next = utxx::now_utc().add_msec(a_initial_msec);
  timespec interval{a_interval_msec / 1000, (a_interval_msec % 1000) * 1000000};
  struct   itimerspec           timeout;

  timeout.it_value.tv_sec     = next.sec();
  timeout.it_value.tv_nsec    = next.nsec();
  timeout.it_interval.tv_sec  = interval.tv_sec;
  timeout.it_interval.tv_nsec = interval.tv_nsec;

  if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &timeout, NULL) < 0)
    UTXX_THROWX_IO_ERROR(errno, "timerfd_settime");

  UTXX_RLOG(this, TRACE5, "adding Timer '", a_name, "', fd=", fd, ", initial=",
      utxx::fixed(double(a_initial_msec)/1000,  3), ", interval=",
      utxx::fixed(double(a_interval_msec)/1000, 3), ", Opaque=", a_opaque);

  uint32_t events = EPOLLIN | EPOLLET | EPOLLERR;

  auto p = Set(a_name, fd, FdTypeT::Timer, events, UTXX_SRCX, a_on_error,
               nullptr, a_opaque, 0, 0, nullptr,   nullptr,   LEVEL_TRIGGERED);
  p->SetHandler(HType::Timer, a_fun);

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor
::AddSignal
(
  std::string const& a_name,
  sigset_t    const& a_mask,
  SigHandler  const& a_fun,
  ErrHandler  const& a_on_error,
  void*              a_opaque,
  int                a_sigq_capacity
)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  int fd;

  // Block the signals that we handle using signalfd(), so they so that they
  // aren't handled according to their default dispositions.
  if (sigprocmask(SIG_BLOCK, &a_mask, nullptr) < 0)
    UTXX_THROWX_IO_ERROR
      (errno, "Cannot block signals ", utxx::sig_members(a_mask));

  if ((fd = signalfd(-1, &a_mask, SFD_NONBLOCK | SFD_CLOEXEC)) < 0)
    UTXX_THROWX_IO_ERROR(errno, "Cannot create signalfd");

  UTXX_RLOG(this, TRACE3, "adding Signal handler '", a_name, "', fd=", fd,
      ", signals=", utxx::sig_members(a_mask), ", Opaque=", a_opaque);

  auto rd_bufsz = sizeof(signalfd_siginfo)*a_sigq_capacity;
  auto p = Set(a_name, fd, FdTypeT::Signal,
               EPOLLIN | EPOLLET | EPOLLERR, UTXX_SRCX, a_on_error,
               nullptr, a_opaque, rd_bufsz, 0, nullptr, nullptr, LEVEL_TRIGGERED);
  p->SetHandler(HType::Signal, a_fun);

  return *p;
}

//------------------------------------------------------------------------------
void Reactor
::Remove(int& a_fd, bool a_clear_fdinfo)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  if (UNLIKELY(a_fd < 0 || !m_fds[a_fd]))
    return;

  // Otherwise: "a_fd" is valid (>= 0):
  if (UNLIKELY(a_fd >= int(m_fds.size())))
    UTXX_THROWX_BADARG_ERROR("invalid fd=", a_fd);

  // We make a copy of fd and clear the a_fd in order to avoid infinite
  // recursion when calling the Clear() method below (since the first line of
  // this function exits immediately if a_fd < 0) in cases when Remove() was
  // called from within m_fds[...].Clear():
  auto p = m_fds[a_fd].get();

  if (!p)
    return;

  if (a_clear_fdinfo)
    CloseFD(a_fd);

  p->FD(-1);
  p->Clear();
}

//------------------------------------------------------------------------------
dynamic_io_buffer* Reactor
::SubscribeWrite(int a_fd, bool a_on)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  if (UNLIKELY(a_fd < 0 || a_fd >= int(m_fds.size())))
    UTXX_THROWX_BADARG_ERROR("invalid fd=", a_fd);

  auto p = m_fds[a_fd].get();

  if (UNLIKELY(!p))
    UTXX_THROWX_BADARG_ERROR("empty fd=", a_fd);

  auto& info = *p;

  if (UNLIKELY(!info.m_wr_buff))
    UTXX_THROWX_BADARG_ERROR("write buffer not assigned!");

  auto mask = EPOLLIN | EPOLLET | EPOLLERR | EPOLLRDHUP | (a_on ? EPOLLOUT:0);

  epoll_event ev{ .events = mask, .data{0} };
  ev.data.fd = a_fd;

  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, a_fd, &ev) == -1)
    UTXX_THROWX_IO_ERROR(errno, "failed to modify events on fd=", a_fd);
  return info.WrBuff();
}

} // namespace io
} // namespace utxx
