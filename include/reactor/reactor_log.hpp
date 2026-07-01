// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_log.hpp
//------------------------------------------------------------------------------
/// \brief Reactor logging functions and UTXX_RLOG macro
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <reactor/compat.hpp>

namespace utxx {

//------------------------------------------------------------------------------
/// Default log implementation — writes a timestamped line to std::cerr.
//------------------------------------------------------------------------------
template <class... Args>
inline void DefaultLog(log_level a_lev, src_info&& a_si, Args&&... args)
{
  buffered_print buf;
  buf << timestamp::to_string(DATE_TIME_WITH_USEC);
  buf << " [" << log_level_to_string(a_lev)[0] << "] ";
  buf.print(std::forward<Args>(args)...);
  buf << " [" << (a_si.srcloc() ? a_si.srcloc() : "") << "]\n";
  std::cerr.write(buf.c_str(), buf.size());
}

} // namespace utxx
