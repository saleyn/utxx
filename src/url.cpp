//----------------------------------------------------------------------------
/// \file  url.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of url parsing functions.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-09-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/url.hpp>
#include <utxx/string.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

namespace utxx {

namespace detail {

    const char* connection_type_to_str(connection_type a_type)
    {
        switch (a_type) {
            case TCP:       return "tcp";
            case UDP:       return "udp";
            case UDS:       return "uds";
            case FILENAME:  return "file";
            case CMD:       return "cmd";
            default:        return "undefined";
        }
    }

} // namespace detail

void addr_info::operator=(addr_info const& a_rhs) {
    url   = a_rhs.url;
    proto = a_rhs.proto;
    addr  = a_rhs.addr;
    mcast = a_rhs.mcast;
    ip    = a_rhs.ip;
    iface = a_rhs.iface;
    port  = a_rhs.port;
    path  = a_rhs.path;
}

void addr_info::operator=(addr_info&& a_rhs) {
    url   = std::move(a_rhs.url);
    proto = a_rhs.proto;
    addr  = std::move(a_rhs.addr);
    mcast = std::move(a_rhs.mcast);
    ip    = std::move(a_rhs.ip);
    iface = std::move(a_rhs.iface);
    port  = std::move(a_rhs.port);
    path  = std::move(a_rhs.path);
}

bool addr_info::assign(
    connection_type    a_proto, std::string const& a_addr, uint16_t a_port,
    std::string const& a_path,  std::string const& a_iface,
    std::string const& a_mcast)
{
    proto = a_proto;
    addr  = a_addr;
    mcast = a_mcast;
    ip    = is_ipv4_addr(a_addr) ? a_addr : "";
    port  = std::to_string(a_port);
    path  = a_path;
    iface = a_iface;

    url   = utxx::to_string(detail::connection_type_to_str(proto), "://",
                      mcast.empty()   ? "" : "@", mcast,   addr,
                      a_iface.empty() ? "" : ";", a_iface, ':', port,
                      path.empty() || path[0] == '/' ? "" : "/", path);
    m_is_ipv4 = is_ipv4_addr(addr);

    return m_is_ipv4 && a_proto != UNDEFINED;
}

void addr_info::clear()
{
    url.clear();
    proto = UNDEFINED;
    mcast.clear();
    addr.clear();
    ip.clear();
    iface.clear();
    port.clear();
    path.clear();
}

std::string addr_info::to_string() const
{
    std::stringstream s;
    s << m_proto << "://";
    if (!mcast.empty()) s << mcast << '@';
    if (!addr.empty())  s << addr;
    if (!iface.empty()) s << ';' << iface;
    if (!port.empty())  s << ':' << port;
    if (!path.empty())  s << path;
    return s.str();
}

bool is_ipv4_addr(const std::string& a_addr)
{
    static const boost::regex s_re("^(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})$");
    boost::smatch m;

    bool res = boost::regex_search(a_addr, m, s_re);
    return res
        && std::stoi(m[1]) < 256
        && std::stoi(m[2]) < 256
        && std::stoi(m[3]) < 256
        && std::stoi(m[4]) < 256;
}

//----------------------------------------------------------------------------
// URL Parsing
//----------------------------------------------------------------------------
bool addr_info::parse(const std::string& a_url) {
    static std::map<std::string, std::string> proto_to_port{
        {"http", "80"},{"https", "443"},{"ssl", ""},
        {"file", ""}  ,{"uds",   ""}   ,{"cmd",   ""}
    };

    clear();

    // The following gives named groups ("(?<group_name>...)")
    // NOTE: This group syntax is not supported by C++11, but supported by
    //       boost::regex
    //
    // (?:(?<proto>cmd|file|uds|pipe)://(?<path>[^\n]+))|(?:(?<proto>[a-z]+)://)(?<addr>(?:(?<mcast>\d{1,3}(?:.\d{1,3}){3})@)?(?<ip>(?:[a-zA-Z][a-zA-Z\d.-]+)|(?:\d{1,3}(?:.\d{1,3}){0,3}))(?:;(?<iface>[a-zA-Z][a-zA-Z0-9_-]*)?)?)(?::(?<port>\d{1,5}))?(?:(?<path>/[^ \n]+))?
    // Gives 7 groups:
    // For   "http://2.3.4.5@1.2.3.4;eth1:2000/abc123+"
    // proto: http
    // mcast: 2.3.4.5
    // ip:    1.2.3.4
    // iface: eth1
    // port:  2000
    // path:  /abc123+

    // Parse examples:
    // http://1 abc
    // http://1.2.3.4 abc
    // http://1.2.3.4:2000
    // https://2.3.4.5@1.2.3.4:2000
    // http://2.3.4.5@1.2.3.4:2000/abc123+
    // udp://224.0.31.26;:14314/314SA
    // udp://224.0.31.26;eth0:14314/314SA
    // udp://224.0.31.26@1.2.3.4;eth0:14314/314SA
    // http://abc.com:200
    // cmd:///bin/bash -x ls /dir
    // file:///bin/bash.txt

    static const boost::regex s_url_re(
        "(?:(?<proto>cmd|file|uds|pipe)://(?<path>[^\n]+))|"
        "(?:(?<proto>[a-z]+)://)(?<addr>(?:(?<mcast>\\d{1,3}(?:.\\d{1,3}){3})@)?"
        "(?<ip>(?:[a-zA-Z][a-zA-Z\\d.-]+)|(?:\\d{1,3}(?:.\\d{1,3}){0,3}))"
        "(?:;(?<iface>[a-zA-Z][a-zA-Z0-9_-]*)?)?)"
        "(?::(?<port>\\d{1,5}))?(?:(?<path>/[^ \n]+))?"
        , boost::regex::perl);

    boost::smatch m;
    auto res = boost::regex_match(a_url, m, s_url_re);
    if  (res) {
        m_proto = m["proto"];
        path    = m["path"];
        ip      = m["ip"];
        addr    = m["addr"];
        if (addr.empty() && !ip.empty()) addr = ip;
        port    = m["port"];
        if (port.empty()) port = proto_to_port[m_proto];
        path    = m["path"];
        boost::to_lower(m_proto);
    }

    if      (m_proto == "tcp"  ||
             m_proto == "ssl"  ||
             m_proto == "http" ||
             m_proto == "https")    proto = TCP;
    else if (m_proto == "udp")      proto = UDP;
    else if (m_proto == "uds")      proto = UDS;
    else if (m_proto == "file")     proto = FILENAME;
    else if (m_proto == "cmd"  ||
             m_proto == "pipe")     proto = CMD;
    else                            proto = UNDEFINED;

    url       = a_url;
    m_is_ipv4 = is_ipv4_addr(ip);
    return res;
}

} // namespace utxx
