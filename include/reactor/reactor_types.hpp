// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_types.hpp
//------------------------------------------------------------------------------
/// \brief I/O multiplexing event reactor types
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <memory>
#include <reactor/compat.hpp>

namespace utxx {

class Reactor;
class FdInfo;

//------------------------------------------------------------------------------
// Handler type enum
//------------------------------------------------------------------------------
enum class HType : int {
  UNDEFINED = -1,
  IO        = 0,
  RawIO,
  File,
  Pipe,
  Event,
  Timer,
  Signal,
  Accept,
  Error
};

inline const char* to_string(HType v)
{
  switch (v) {
    case HType::UNDEFINED: return "UNDEFINED";
    case HType::IO:        return "IO";
    case HType::RawIO:     return "RawIO";
    case HType::File:      return "File";
    case HType::Pipe:      return "Pipe";
    case HType::Event:     return "Event";
    case HType::Timer:     return "Timer";
    case HType::Signal:    return "Signal";
    case HType::Accept:    return "Accept";
    case HType::Error:     return "Error";
    default:               return "UNKNOWN";
  }
}

//------------------------------------------------------------------------------
// IOType enum
//------------------------------------------------------------------------------
enum class IOType : int {
  UNDEFINED = -1,
  Init      = 0,
  Read,
  Write,
  Connect,
  Disconnect,
  Accept,
  EndOfFile,
  Decoding,
  System,
  UserCode,
  UserData,
  Auth,
  Fatal,
  AppLogic
};

inline const char* to_string(IOType v)
{
  switch (v) {
    case IOType::UNDEFINED:  return "UNDEFINED";
    case IOType::Init:       return "Init";
    case IOType::Read:       return "Read";
    case IOType::Write:      return "Write";
    case IOType::Connect:    return "Connect";
    case IOType::Disconnect: return "Disconnect";
    case IOType::Accept:     return "Accept";
    case IOType::EndOfFile:  return "EndOfFile";
    case IOType::Decoding:   return "Decoding";
    case IOType::System:     return "System";
    case IOType::UserCode:   return "UserCode";
    case IOType::UserData:   return "UserData";
    case IOType::Auth:       return "Auth";
    case IOType::Fatal:      return "Fatal";
    case IOType::AppLogic:   return "AppLogic";
    default:                 return "UNKNOWN";
  }
}

//------------------------------------------------------------------------------
// Handler function typedefs
//------------------------------------------------------------------------------

// Fast raw function pointer for the hot I/O path
typedef int (*RWIOHandler)(FdInfo& a_fi, dynamic_io_buffer& a_buf);

using IOHandler     = utxx::function<int(FdInfo& a_fi, dynamic_io_buffer& a_buf)>;
using PipeHandler   = IOHandler;
using FileHandler   = IOHandler;

using RawIOHandler  = utxx::function<void(FdInfo& a_fi, IOType, uint32_t a_events)>;
using EventHandler  = utxx::function<void(FdInfo&, long a_value)>;
using SigHandler    = utxx::function<void(FdInfo&, int a_signal, int a_si_code)>;
using AcceptHandler = utxx::function<bool(FdInfo&, const char* a_cli_path, int a_cli_fd)>;
using ErrHandler    = utxx::function<void(
    FdInfo&, IOType a_type, std::string const& a_error, utxx::src_info&& a_errsrc)>;

typedef size_t (*ReadSizeEstim)(const char* a_buf, size_t a_size);

using ReadDebugAction = utxx::function<void(const char* a_buf, size_t a_size)>;
using IdleHandler     = utxx::function<void()>;
using Logger = utxx::function<void(utxx::log_level, utxx::src_info&&, const std::string&)>;

//------------------------------------------------------------------------------
// HandlerT — internal union of per-event-type handlers
//------------------------------------------------------------------------------
class HandlerT {
public:
  struct IO {
    RWIOHandler rh;
    RWIOHandler wh;
  };

  HandlerT() noexcept : m_type(HType::UNDEFINED) {}
  HandlerT(std::nullptr_t) noexcept : HandlerT() {}

  HandlerT(HType, std::nullptr_t) : HandlerT() {}
  HandlerT(HType t, IO const& h) : m_type(t), m_rw(h) {}
  HandlerT(HType t, RWIOHandler const& h) : m_type(t), m_rw{h, nullptr} {}
  HandlerT(HType t, RawIOHandler const& h) : m_type(t), m_rio(h) {}
  HandlerT(HType t, IOHandler const& h) : m_type(t), m_ih(h) {}
  HandlerT(HType t, EventHandler const& h) : m_type(t), m_eh(h) {}
  HandlerT(HType t, SigHandler const& h) : m_type(t), m_sh(h) {}
  HandlerT(HType t, AcceptHandler const& h) : m_type(t), m_ah(h) {}

  HandlerT(const HandlerT& a_rhs) : HandlerT() { *this = a_rhs; }
  HandlerT(HandlerT&& a_rhs) : HandlerT() { *this = std::move(a_rhs); }

  void                 operator=(const HandlerT& a_rhs);
  void                 operator=(HandlerT&& a_rhs);

  HType                Type() const { return m_type; }

  const HandlerT::IO&  AsIO() const;
  const RawIOHandler&  AsRawIO() const;
  const FileHandler&   AsFile() const;
  const PipeHandler&   AsPipe() const;
  const EventHandler&  AsEvent() const;
  const EventHandler&  AsTimer() const;
  const SigHandler&    AsSignal() const;
  const AcceptHandler& AsAccept() const;

  void                 Clear() { m_type = HType::UNDEFINED; }

  HType                m_type;
  IO                   m_rw;
  RawIOHandler         m_rio;
  IOHandler            m_ih;
  EventHandler         m_eh;
  SigHandler           m_sh;
  AcceptHandler        m_ah;
};

enum TriggerT { LEVEL_TRIGGERED, EDGE_TRIGGERED };

} // namespace utxx
