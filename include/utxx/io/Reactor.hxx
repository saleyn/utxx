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

#include <mqt/io/Reactor.hpp>
#include <utxx/timestamp.hpp>

namespace utxx {
namespace io   {

//------------------------------------------------------------------------------
// Reactor
//------------------------------------------------------------------------------

inline dynamic_io_buffer* Reactor::
RdBuff(int a_fd) { return Get(a_fd, UTXX_SRC).RdBuff(); }

//------------------------------------------------------------------------------
inline dynamic_io_buffer* Reactor::
WrBuff(int a_fd) { return Get(a_fd, UTXX_SRC).WrBuff(); }

//------------------------------------------------------------------------------
inline void Reactor::
Ident(const std::string& a_ident)
{
  m_ident = utxx::to_string('[', a_ident, '@', m_epoll_fd, "] ");
}

//------------------------------------------------------------------------------
inline void Reactor::
CloseFD(int& a_fd)
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
inline void Reactor::
DefRdDebug(StreamT& out, const FdInfo& a_fi, const char* a_buf, size_t a_sz)
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
inline void Reactor::
Log(utxx::log_level a_level, utxx::src_info&& a_sinfo, Args&&... args)
{
  if (m_logger)
    m_logger(a_level, std::move(a_sinfo),
             utxx::print(std::forward<Args>(args)...));
  else
    DefaultLog(a_level, std::move(a_sinfo), std::forward<Args>(args)...);
}

} // namespace io
} // namespace utxx

#include <utxx/io/ReactorTypes.hxx>
#include <utxx/io/ReactorFdInfo.hxx>