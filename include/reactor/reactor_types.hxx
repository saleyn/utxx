// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_types.hxx
//------------------------------------------------------------------------------
/// \brief I/O multiplexing event reactor types — inline implementations
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <reactor/reactor_types.hpp>

namespace utxx {

//------------------------------------------------------------------------------
// HandlerT accessors
//------------------------------------------------------------------------------
#define ACHK(Tp, Val)          \
  assert(m_type == HType::Tp); \
  return Val
#define ACHKN(Tp, Val)                             \
  assert(m_type == HType::Tp && (Val) != nullptr); \
  return Val

inline const HandlerT::IO& HandlerT::AsIO() const
{ ACHK(IO, m_rw); }
inline const RawIOHandler& HandlerT::AsRawIO() const
{ ACHKN(RawIO, m_rio); }
inline const FileHandler& HandlerT::AsFile() const
{ ACHKN(File, m_ih); }
inline const PipeHandler& HandlerT::AsPipe() const
{ ACHKN(Pipe, m_ih); }
inline const EventHandler& HandlerT::AsEvent() const
{ ACHKN(Event, m_eh); }
inline const EventHandler& HandlerT::AsTimer() const
{ ACHKN(Timer, m_eh); }
inline const SigHandler& HandlerT::AsSignal() const
{ ACHKN(Signal, m_sh); }
inline const AcceptHandler& HandlerT::AsAccept() const
{ ACHKN(Accept, m_ah); }
#undef ACHKN
#undef ACHK

//------------------------------------------------------------------------------
inline void HandlerT::operator=(HandlerT&& a_rhs)
{
  switch (a_rhs.m_type) {
    case HType::IO:
      m_rw.rh = std::move(a_rhs.m_rw.rh);
      m_rw.wh = std::move(a_rhs.m_rw.wh);
      break;
    case HType::RawIO:  m_rio = std::move(a_rhs.m_rio); break;
    case HType::File:
    case HType::Pipe:   m_ih = std::move(a_rhs.m_ih); break;
    case HType::Event:
    case HType::Timer:  m_eh = std::move(a_rhs.m_eh); break;
    case HType::Signal: m_sh = std::move(a_rhs.m_sh); break;
    case HType::Accept: m_ah = std::move(a_rhs.m_ah); break;
    default:            break;
  }
  m_type       = a_rhs.m_type;
  a_rhs.m_type = HType::UNDEFINED;
}

//------------------------------------------------------------------------------
inline void HandlerT::operator=(const HandlerT& a_rhs)
{
  Clear();
  switch (a_rhs.m_type) {
    case HType::IO:
      m_rw.rh = a_rhs.m_rw.rh;
      m_rw.wh = a_rhs.m_rw.wh;
      break;
    case HType::RawIO:  m_rio = a_rhs.m_rio; break;
    case HType::File:
    case HType::Pipe:   m_ih = a_rhs.m_ih; break;
    case HType::Event:
    case HType::Timer:  m_eh = a_rhs.m_eh; break;
    case HType::Signal: m_sh = a_rhs.m_sh; break;
    case HType::Accept: m_ah = a_rhs.m_ah; break;
    default:            break;
  }
  m_type = a_rhs.m_type;
}

} // namespace utxx
