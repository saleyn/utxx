// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_aio_reader.hpp
//------------------------------------------------------------------------------
/// \brief Disk file reader using Linux AIO for I/O multiplexing event reactor
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <reactor/reactor_platform.hpp>
#include <string>

#if defined(REACTOR_OS_LINUX)
#include <libaio.h>
#endif

namespace utxx {

//------------------------------------------------------------------------------
/// @brief File reader using Linux Asynchronous I/O.
///
/// On Linux: uses eventfd(2) + libaio for async completion notifications.
/// On macOS/BSD: not supported — AIOReader always throws on Init().
//------------------------------------------------------------------------------
class AIOReader {
public:
  AIOReader() {}
  AIOReader(int a_efd, const char* a_filename) { Init(a_efd, a_filename); }

  ~AIOReader() { Clear(); }

  /// Post-construction initialization
  void                         Init(int a_efd, const char* a_filename);
  void                         Clear();

  /// Schedule an asynchronous read
  int                          AsyncRead(char* a_buf, size_t a_size);

  /// Get number of pending read completion notifications
  int                          CheckEvents();

  /// Read completion notification events; returns (bytes_read, err_msg)
  std::pair<long, const char*> ReadEvents(int a_events);

  // clang-format off
  long               Offset()    const { return m_offset;               }
  long               Position()  const { return m_position;             }
  long               Size()      const { return m_file_size;            }
  long               Remaining() const { return m_file_size - m_offset; }
  int                EventFD()   const { return m_efd;                  }
  const std::string& Filename()  const { return m_filename;             }
  // clang-format on

private:
  int         m_efd     = -1;
  int         m_file_fd = -1;
  std::string m_filename;
  int         m_async_ops = 0;
  long        m_position  = 0;
  long        m_offset    = 0;
  long        m_file_size = 0;
#if defined(REACTOR_OS_LINUX)
  io_context_t m_ctx = nullptr;
  struct iocb  m_iocb[1];
  struct iocb* m_piocb[1];
#endif
};

} // namespace utxx

#include <reactor/reactor_aio_reader.hxx>
