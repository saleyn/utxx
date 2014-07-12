//----------------------------------------------------------------------------
/// \file   path.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Collection of general purpose functions for path manipulation.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-06
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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
#ifndef _UTXX_URL_HPP_
#define _UTXX_URL_HPP_

#include <stdlib.h>     /* atoi */
#include <string>

namespace utxx {

    /// Types of connections supported by this class.
    enum connection_type { UNDEFINED, TCP, UDP, UDS, FILENAME };

    namespace detail {
        /// Convert connection type to string.
        const char* connection_type_to_str(connection_type a_type);
    }

    /// Server ddress information holder
    struct addr_info {
        connection_type proto;
        std::string     addr;
        std::string     port;
        std::string     path;

        addr_info() : m_is_ipv4(false) {}
        addr_info(const std::string& a_url) { parse(a_url); }

        /// Parse a URL in the form PROTO://ADDRESS[:PORT][/PATH]
        ///
        /// PROTO can be one of "tcp", "udp", "uds", "file"
        bool parse(const std::string& a_url);

        bool is_ipv4() const { return m_is_ipv4; }

        int                 port_int()  const { return atoi(port.c_str()); }
        std::string const&  proto_str() const { return m_proto; }

        std::string to_string() const;

        friend inline std::ostream& operator<< (std::ostream& out, const addr_info& a) {
            return out << a.to_string();
        }
    private:
        std::string m_proto;
        bool        m_is_ipv4;
    };

    /// @return true if \a a_addr is in NNN.NNN.NNN.NNN format.
    bool is_ipv4_addr(const std::string& a_addr);

    /// URL Parsing
    /// @param a_url string in the form <tt>tcp://host:port</tt>
    inline bool parse_url(const std::string& a_url, addr_info& a_info) {
        return a_info.parse(a_url);
    }

} // namespace utxx

#endif // _UTXX_URL_HPP_

