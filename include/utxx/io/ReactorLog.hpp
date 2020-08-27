// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  ReactorLog.hpp
//------------------------------------------------------------------------------
/// \brief Reactor log functions
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
#include <utxx/buffer.hpp>
#include <utxx/error.hpp>
#include <utxx/timestamp.hpp>
#include <utxx/logger/logger.hpp>

#define RLOG(Obj, Level, ...) \
    do { \
        if (UNLIKELY((Obj)->Debug() >= utxx::as_int<utxx::LEVEL_##Level>())) \
            (Obj)->Log(utxx::LEVEL_##Level, \
                       UTXX_SRCX, (Obj)->Ident(), ##__VA_ARGS__); \
    } while (0)

namespace utxx {
namespace io   {

//------------------------------------------------------------------------------
/// Default logging implementation
//------------------------------------------------------------------------------
template <class... Args>
inline void DefaultLog(log_level a_lev, utxx::src_info&& a_si, Args&&... args)
{
		utxx::buffered_print buf;
		buf << utxx::timestamp::to_string(utxx::DATE_TIME_WITH_USEC);
		buf << " [" << utxx::log_level_to_string(a_lev)[0] << "] ";
		buf.print(std::forward<Args>(args)...);
		// Calculate the length of the "filename:line"
		int len = a_si.srcloc_len();
		for (auto p = a_si.srcloc()+a_si.srcloc_len()-1; p != a_si.srcloc(); --p)
				if (*p == '/') {
						len -= p - a_si.srcloc() + 1;
						break;
				}
		// Make sure the buffer has enough space for the "filename:line"
		buf.reserve(len + 4);
		buf.pos() = a_si.to_string(buf.pos(), buf.capacity(), " [", "]\n");
		std::cerr << buf.to_string();
}

} // namespace io
} // namespace utxx