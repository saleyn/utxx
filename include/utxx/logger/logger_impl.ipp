//----------------------------------------------------------------------------
/// \file   logger_impl.ipp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Supplementary classes for the <logger> class.
//----------------------------------------------------------------------------
// Copyright (C) 2003-2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-25
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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
#ifndef _UTXX_LOGGER_IMPL_IPP_
#define _UTXX_LOGGER_IMPL_IPP_

namespace utxx {

template <int N>
inline log_msg_info::log_msg_info(
    logger& a_logger, log_level lv, const char (&filename)[N], size_t ln)
    : m_logger(a_logger)
    , m_level(lv)
    , m_src_location_len(
            m_logger.show_location()
            ? snprintf(m_src_location, sizeof(m_src_location), " [%s:%zd]",
                path::basename(filename, filename+N), ln)
            : 0
      )
{
}

inline void log_msg_info::log(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    BOOST_SCOPE_EXIT( (&args) ) { va_end(args); } BOOST_SCOPE_EXIT_END;
    m_logger.log(*this, fmt, args);
}

} // namespace utxx

#endif  // _UTXX_LOGGER_IMPL_IPP_
