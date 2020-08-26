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

#include <libaio.h>
#include <string>

namespace utxx {
namespace io   {

//------------------------------------------------------------------------------
// AIOReader
//------------------------------------------------------------------------------
/// @brief File reader using Linux Asynchronous I/O.
/// Since epoll doesn't support regular file descriptors we must use the
/// workaround of using eventfd(2) for epoll notifications and AIO for
/// asynchronous I/O completion delivery events.
class AIOReader {
public:
  AIOReader() {}
  AIOReader(int a_efd, const char* a_filename) { Init(a_efd, a_filename); }

  ~AIOReader() { Clear(); }

  /// Post-construction initialization
  void Init(int a_efd, const char* a_filename);
  void Clear();

  /// Schedule an asynchronous read
  int AsyncRead(char* a_buf, size_t a_size);

  /// Get number of pending read completion notifications
  int CheckEvents();

  /// Read completion notification events
  std::pair<long, const char*> ReadEvents(int a_events);

  /// File offset including the size of the last read buffer.
  /// This value is incremented by ReadEvents
  long               Offset()    const { return m_offset;              }
  /// File offset excluding the size of the last read buffer.
  /// This value is incremented by AsyncRead, is equal to 0 the first time
  /// ReadEvents() returns, and is alway less than Offset() by the size of
  /// the last read buffer.
  long               Position()  const { return m_position;            }
  /// Total file size read at initialization
  long               Size()      const { return m_file_size;           }
  /// Remaining size of the file to be read
  long               Remaining() const { return m_file_size - m_offset;}

  int                EventFD()   const { return m_efd;                 }
  const std::string& Filename()  const { return m_filename;            }
private:
  int                m_efd           = -1;   ///< EventFD (not owned)
  int                m_file_fd       = -1;   ///< File FD (owned)
  io_context_t       m_ctx           =  nullptr;
  std::string        m_filename;
  int                m_async_ops     =  0;   ///< Async ops "in flight"
  long               m_position      =  0;   ///< Number of bytes processed.
  long               m_offset        =  0;   ///< Current file offset
  long               m_file_size     =  0;
  struct iocb        m_iocb[1];
  struct iocb*       m_piocb[1];
};

} // namespace io
} // namespace utxx

#include <utxx/io/ReactorAIOReader.hxx>