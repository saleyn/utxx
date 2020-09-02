// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// @file  ReactorAIOReader.hpp
//------------------------------------------------------------------------------
/// \brief Disk file reader for I/O multiplexing event reactor
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

#include <utxx/io/ReactorAIOReader.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/scope_exit.hpp>
#include <utxx/error.hpp>
#include <utxx/path.hpp>
#include <sys/eventfd.h>
#include <assert.h>
#include <libaio.h>
#include <fcntl.h>
#include <utility>
#include <string>

#include <sys/syscall.h>    /* for __NR_* definitions */

// NB: For some reason when using standard definition of io_getevents(2)
// from <linux/aio.h>, a call to io_getevents() sporadically results in
// EINVAL or EFAULT errors.  This is not observed when using the inlined
// definition below:

namespace {
  inline int io_getevents_ex(io_context_t ctx, long min_nr, long max_nr,
      struct io_event *events, struct timespec *timeout)
  {
    return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
  }
}

namespace utxx {
namespace io   {

//------------------------------------------------------------------------------
// Inlined implementation
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
inline void AIOReader::
AIOReader::Clear()
{
  if (m_efd >= 0)      { ::close(m_efd);      m_efd     = -1;      }
  if (m_file_fd >= 0)  { ::close(m_file_fd);  m_file_fd = -1;      }
  if (m_ctx)           { ::io_destroy(m_ctx); m_ctx    = nullptr;  }
}

//------------------------------------------------------------------------------
inline void AIOReader::Init(int a_efd, const char* a_filename)
{
  Clear();
  m_efd       = a_efd;
  m_filename  = a_filename;
  m_position  = 0;
  m_offset    = 0;
  m_async_ops = 0;

  if (a_efd < 0)
    UTXX_THROW(utxx::io_error, errno, "invalid eventfd descriptor");

  int flags = O_RDONLY | O_NONBLOCK | O_LARGEFILE;
  m_file_fd = ::open(a_filename, flags);

  if (m_file_fd < 0)
    UTXX_THROW(utxx::io_error, errno, "cannot open ", a_filename);

  // Using RAII idiom to cleanup in case of exception:
  utxx::scope_exit guard([this]() { ::close(m_file_fd); });

  m_file_size = utxx::path::file_size(m_file_fd);
  m_ctx       = nullptr;

  if (io_setup(128, &m_ctx) < 0)
    UTXX_THROW(utxx::io_error, errno,
                "failed to do AIO setup on file '", a_filename, "'");

  m_piocb[0] = &m_iocb[0];

  guard.disable(true);
}

//------------------------------------------------------------------------------
inline int AIOReader::
AsyncRead(char* a_buf, size_t a_size)
{
  auto rem = Remaining();
  // Is there an attempt to read past the end of file or before completion
  // of previous read request?
  if (UNLIKELY(rem == 0 || m_async_ops)) {
    errno =  rem == 0 ? ENODATA : EALREADY;
    return -1;
  }

  auto sz  = std::min<long>(rem, a_size);
  ::io_prep_pread(&m_iocb[0], m_file_fd, a_buf, sz, m_offset);
  ::io_set_eventfd(&m_iocb[0], m_efd);
  m_iocb[0].data = (void*)this;
  int rc = io_submit(m_ctx, 1, m_piocb);

  if (LIKELY(rc >= 0))
    m_async_ops++;
  else
    errno = -rc;

  m_position = m_offset;

  return rc;
}

//------------------------------------------------------------------------------
inline int AIOReader::
CheckEvents()
{
  // Check if there's any outstanding read completion events by reading
  // eventfd:
  int       n;
  eventfd_t events = 0;
  while(UNLIKELY((n = ::eventfd_read(m_efd, &events) < 0) && errno == EINTR));

  return UNLIKELY(n < 0) ? n : events;
}

//------------------------------------------------------------------------------
inline std::pair<long, const char*> AIOReader::
ReadEvents(int a_events)
{
  // Since we are using only 1 asynchronous completion operation at any time
  // for reading from a file by giving it a sufficiently large buffer, we
  // shouldn't get more than one notification:
  assert(a_events <= 1);

  if (UNLIKELY(a_events == 0))
    return std::make_pair(0L, "");

  static struct timespec tmo = {.tv_sec = 0, .tv_nsec = 0};
  int n;

  struct io_event events[1];

  // (2) Get the pending completion event:
  do {
    n = ::io_getevents_ex(m_ctx, 0, a_events, events, &tmo);
  } while (UNLIKELY(n == -EINTR));

  // We can get a negative return here only if there are bad arguments or
  // bad AIO conext:
  if (UNLIKELY(n == 0))
    return std::make_pair(0, "");
  else if (UNLIKELY(n < 0)) {
    errno = -n;
    return std::make_pair(-1L, "io_getevents()");
  }

  // Got a successful completion event.
  // We don't need to loop over n since n is at most 1 at this point
  assert(n <= 1);
  assert(&m_iocb[0] == events[0].obj);

  m_async_ops = std::max<int>(0, m_async_ops - n);

  long got  = events[0].res;
  m_offset += got;

  return std::make_pair(got, "");
}

} // namespace io
} // namespace utxx
