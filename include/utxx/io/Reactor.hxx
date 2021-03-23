// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  Reactor.hxx
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
#pragma once

#include <utxx/io/ReactorLog.hpp>
#include <utxx/io/Reactor.hpp>
#include <utxx/timestamp.hpp>

namespace utxx {
namespace io   {

//------------------------------------------------------------------------------
// Reactor
//------------------------------------------------------------------------------

inline dynamic_io_buffer* Reactor
::RdBuff(int a_fd) { return Get(a_fd, UTXX_SRC).RdBuff(); }

//------------------------------------------------------------------------------
inline dynamic_io_buffer* Reactor
::WrBuff(int a_fd) { return Get(a_fd, UTXX_SRC).WrBuff(); }

//------------------------------------------------------------------------------
inline void Reactor
::Ident(const std::string& a_ident)
{
  m_ident = utxx::to_string('[', a_ident, '@', m_epoll_fd, "] ");
}

//------------------------------------------------------------------------------
inline void Reactor
::CloseFD(int& a_fd)
{
  if (a_fd < 0) return;

  if (m_epoll_fd >= 0) {
    epoll_event event;
    event.events  = 0;
    event.data.fd = a_fd;
    (void)epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, a_fd, &event);
  }

  (void)close(a_fd);
  a_fd = -1;
}

//------------------------------------------------------------------------------
template <typename StreamT>
inline void Reactor
::DefRdDebug(StreamT& out, const FdInfo& a_fi, const char* a_buf, size_t a_sz)
{
  utxx::verbosity::if_enabled(utxx::VERBOSE_WIRE, [&]() {
    out << '['     << a_fi.Name() << ", fd=" << a_fi.FD()
        << "] <- " << a_sz        << " bytes "
        << utxx::to_bin_string(a_buf, std::min(a_sz, 256ul))
        << std::endl;
  });
}

//------------------------------------------------------------------------------
template <class... Args>
inline void Reactor
::Log(utxx::log_level a_level, utxx::src_info&& a_sinfo, Args&&... args)
{
  if (m_logger)
    m_logger(a_level, std::move(a_sinfo),
             utxx::print(std::forward<Args>(args)...));
  else
    DefaultLog(a_level, std::move(a_sinfo), std::forward<Args>(args)...);
}

//------------------------------------------------------------------------------
inline void Reactor
::Wait(int a_timeout_msec)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  static const int N = 256;
  epoll_event  cev[N];

REPEAT:
  int rc = epoll_wait(m_epoll_fd,cev,N,a_timeout_msec);

  if (UNLIKELY(rc < 0)) {
    if (errno==EINTR)
      goto REPEAT;
    UTXX_THROWX_IO_ERROR(errno, "epoll_wait failed");
  }

  int i=0;

  for (epoll_event* p = cev, *end = cev + rc; p != end; ++p) {
    int fd = p->data.fd;

    if (UNLIKELY(fd < 0 || fd >= int(m_fds.size()))) {
      UTXX_RLOG(this, DEBUG, "fd=", fd, " not found!");
      continue;
    }

    UTXX_RLOG(this, TRACE5, "processing ", ++i, '/', rc, ' ',
       (fd < int(m_fds.size()) && m_fds[fd] ? m_fds[fd]->Name() : "INVALID FD "),
       "(fd=", fd, ", events=", EPollEvents(p->events), ')');

    FdInfo& info = *m_fds[fd];

    auto cleanup = [this, &fd, &info]() {
      UTXX_RLOG(&info, TRACE5, "closing fd ", fd, " on negative return from handler");
      CloseFD(fd);
      info.FD(-1);
      info.Clear();
    };

    rc = 0;

    try {
      if (UNLIKELY(IsError(p->events))) {
        auto tp = IsWritable  (p->events) ? IOType::Write : IOType::Read;
        auto ec = SocketError (fd);
        std::string err;

        if (ec == EAGAIN || ec == EINTR)
          continue;
        else if (tp == IOType::Write && ec == EINPROGRESS) {
          err = "failed to connect to remote address";
          tp  = IOType::Connect;
        }
        else if (ec == ENOTSOCK)
          continue;
        else
          err = strerror(ec);

        info.ReportError(tp, 0, err, UTXX_SRCX, false);

        rc = -1;
      } else if (LIKELY(info.m_fd != -1)) {
        //----------------------------------------------------------------------
        // This is normal event notification => invoke associated handler:
        //----------------------------------------------------------------------
        rc = info.Handle(p->events);
      }
    } catch (...) {
      cleanup();
      throw;
    }

    if (UNLIKELY(rc < 0 && errno != EAGAIN))
      cleanup();
  }

  // Done with processing all I/O events
  // If there were no events and we got here on timeout,
  // trigger the idle state handler:
  if (rc == 0 && m_on_idle)
    m_on_idle();
}

} // namespace io
} // namespace utxx

#include <utxx/io/ReactorTypes.hxx>
#include <utxx/io/ReactorFdInfo.hxx>
