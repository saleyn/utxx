// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  ReactorTypes.hpp
//------------------------------------------------------------------------------
/// \brief I/O multiplexing event reactor types
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

#include <utxx/compiler_hints.hpp>
#include <utxx/function.hpp>
#include <utxx/buffer.hpp>
#include <utxx/enum.hpp>
#include <utxx/error.hpp>
#include <utxx/logger/logger_enums.hpp>
#include <boost/align/aligned_allocator.hpp>
#include <memory>

namespace utxx {
namespace io   {

class Reactor;

using utxx::src_info;
using dynamic_io_buffer = utxx::detail::basic_dynamic_io_buffer
                            <boost::alignment::aligned_allocator<char, 512>>;

class FdInfo;

// Handler types
UTXX_ENUM
(HType, int,
  IO,
  RawIO,
  File,
  Pipe,
  Event,
  Timer,
  Signal,
  Accept,
  Error
);

/// Classifier of READ/WRITE operations
UTXX_ENUM
(IOType, int,
  Init,       // Initialization operation
  Read,       // Read operation
  Write,      // Write operation
  Connect,    // Connected to destination address
  Disconnect, // Disconnect from destination address
  Accept,     // Accept a new client connection (for servers)
  EndOfFile,  // Reached end of file
  Decoding,   // Decoding data
  System,     // OS System generated error (e.g. OS signal)
  UserCode,   // User-callback related issue
  UserData,   // Data related issue
  Auth,       // Authentication issue
  Fatal,      // Unrecoverable error - connector must be restarted
  AppLogic    // Application logic issue (e.g. unforseeing condition) RARE!
);

// The following definition is a signature of an member function
// that will be cast to a typed member function using static_cast on the
// a_fi.Instance(). This is more efficient than invoking utxx::function(),
// and is the most frequently called callback, which is the reason for it to
// have a signature distinct for the more type-safe utxx::function()
// alternative
typedef int    (*RWIOHandler)(FdInfo& a_fi, dynamic_io_buffer& a_buf);

using           IOHandler    = utxx::function<
                                int(FdInfo& a_fi, dynamic_io_buffer& a_buf)>;
/// This handler type is for reading data from pipe
using           PipeHandler  = IOHandler;
/// This handler type is for reading data from file
using           FileHandler  = IOHandler;

/// This handler type is for reacting to read/write I/O events without
/// actually having the reactor to automatically read the data from \a a_fd
using           RawIOHandler = utxx::function<
                                void(FdInfo& a_fi, IOType, uint32_t a_events)>;

/// This handler type is for reacting to timer or eventfd events
using           EventHandler = utxx::function<void(FdInfo&, long a_value)>;

/// This handler type is for reacting to signal events
using           SigHandler   = utxx::function<
                                void(FdInfo&, int a_signal, int a_si_code)>;

/// This handler type is for reacting to accept events (client connected)
using           AcceptHandler= utxx::function<bool(FdInfo&,
                    const char* a_cli_path, int a_cli_fd)>;

/// This handler type is for reporting errors
using           ErrHandler   = utxx::function<void(FdInfo&,    IOType a_type,
                                                   std::string const& a_error,
                                                   utxx::src_info&&   a_errsrc)>;
/// Function estimating minimum read size
typedef size_t (*ReadSizeEstim)(const char* a_buf, size_t a_size);

using           ReadDebugAction = utxx::function<void(
                                            const char* a_buf, size_t a_size)>;

/// This hanler type is invoked after reactor processed all I/O callbacks
using           IdleHandler  = utxx::function<void()>;

/// Logger signature
using           Logger       = utxx::function<void (utxx::log_level,
                                utxx::src_info&&, const std::string&)>;

/// Internal union of handlers specific to the dispatched event.
class HandlerT {
public:
  struct IO {
    RWIOHandler   rh;
    RWIOHandler   wh;
  };

  HandlerT() noexcept : m_type(HType::UNDEFINED) {}
  HandlerT(std::nullptr_t) noexcept : HandlerT() {}

  HandlerT(HType,   std::nullptr_t)         : HandlerT() {}
  HandlerT(HType t, IO            const& h) : m_type(t), m_rw(h) {}
  HandlerT(HType t, RWIOHandler   const& h) : m_type(t), m_rw{h,nullptr} {}
  HandlerT(HType t, RawIOHandler  const& h) : m_type(t), m_rio(h){}
  HandlerT(HType t, IOHandler     const& h) : m_type(t), m_ih(h) {}
  HandlerT(HType t, EventHandler  const& h) : m_type(t), m_eh(h) {}
  HandlerT(HType t, SigHandler    const& h) : m_type(t), m_sh(h) {}
  HandlerT(HType t, AcceptHandler const& h) : m_type(t), m_ah(h) {}

  HandlerT(const HandlerT& a_rhs) : HandlerT() { *this = a_rhs; }
  HandlerT(HandlerT&&      a_rhs) : HandlerT() { *this = std::move(a_rhs); }

  void  operator=(const HandlerT& a_rhs);
  void  operator=(HandlerT&&      a_rhs);

  HType Type() const { return m_type; }

  const HandlerT::IO&  AsIO    () const;
  const RawIOHandler&  AsRawIO () const;
  const FileHandler&   AsFile  () const;
  const PipeHandler&   AsPipe  () const;
  const EventHandler&  AsEvent () const;
  const EventHandler&  AsTimer () const;
  const SigHandler&    AsSignal() const;
  const AcceptHandler& AsAccept() const;

  void  Clear() { m_type = HType::UNDEFINED; }
public:
  HType           m_type;
  // The following functors could be a union, but then proper management of
  // initialization/destruction is error-prone:
  IO              m_rw; // Fast I/O
  RawIOHandler    m_rio;
  IOHandler       m_ih; // Generic I/O (file, pipe, etc.)
  EventHandler    m_eh;
  SigHandler      m_sh;
  AcceptHandler   m_ah;
};

enum TriggerT { LEVEL_TRIGGERED, EDGE_TRIGGERED };

} // namespace io
} // namespace utxx
