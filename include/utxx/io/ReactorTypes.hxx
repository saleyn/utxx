// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  ReactorTypes.hxx
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

#include <utxx/io/ReactorTypes.hpp>

namespace utxx {
namespace io   {

//------------------------------------------------------------------------------
// HandlerU
//------------------------------------------------------------------------------
#define ACHK( Tp, Val) assert(m_type == HType::Tp);                   return Val
#define ACHKN(Tp, Val) assert(m_type == HType::Tp && Val != nullptr); return Val

inline const HandlerT::IO&  HandlerT::AsIO    () const { ACHK( IO    , m_rw ); }
inline const RawIOHandler&  HandlerT::AsRawIO () const { ACHKN(RawIO , m_rio); }
inline const FileHandler&   HandlerT::AsFile  () const { ACHKN(File  , m_ih ); }
inline const PipeHandler&   HandlerT::AsPipe  () const { ACHKN(Pipe  , m_ih ); }
inline const EventHandler&  HandlerT::AsEvent () const { ACHKN(Event , m_eh ); }
inline const EventHandler&  HandlerT::AsTimer () const { ACHKN(Timer , m_eh ); }
inline const SigHandler&    HandlerT::AsSignal() const { ACHKN(Signal, m_sh ); }
inline const AcceptHandler& HandlerT::AsAccept() const { ACHKN(Accept, m_ah ); }
#undef ACHKN
#undef ACHK

//------------------------------------------------------------------------------
inline void HandlerT::
operator=(HandlerT&& a_rhs)
{
  switch (a_rhs.m_type) {
    case HType::IO:     { m_rw.rh = std::move(a_rhs.m_rw.rh); m_rw.wh = std::move(a_rhs.m_rw.wh); break; }
    case HType::RawIO:    m_rio   = std::move(a_rhs.m_rio); break;
    case HType::File:
    case HType::Pipe:     m_ih    = std::move(a_rhs.m_ih); break;
    case HType::Event:
    case HType::Timer:    m_eh    = std::move(a_rhs.m_eh); break;
    case HType::Signal:   m_sh    = std::move(a_rhs.m_sh); break;
    case HType::Accept:   m_ah    = std::move(a_rhs.m_ah); break;
    default:              break;
  }
  m_type = a_rhs.m_type;
  a_rhs.m_type = HType::UNDEFINED;
}

//------------------------------------------------------------------------------
inline void HandlerT::
operator=(const HandlerT& a_rhs)
{
  Clear();
  switch (a_rhs.m_type) {
    case HType::IO:     { m_rw.rh = a_rhs.m_rw.rh; m_rw.wh = a_rhs.m_rw.wh; break; }
    case HType::RawIO:    m_rio   = a_rhs.m_rio; break;
    case HType::File:
    case HType::Pipe:     m_ih    = a_rhs.m_ih; break;
    case HType::Event:
    case HType::Timer:    m_eh    = a_rhs.m_eh; break;
    case HType::Signal:   m_sh    = a_rhs.m_sh; break;
    case HType::Accept:   m_ah    = a_rhs.m_ah; break;
    default:              break;
  }
  m_type = a_rhs.m_type;
}

} // namespace io
} // namespace utxx