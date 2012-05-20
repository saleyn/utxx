//----------------------------------------------------------------------------
/// \file variant_config.hpp
//----------------------------------------------------------------------------
/// Defines config_tree, config_path, and config_error classes for
/// ease of configuration management.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Omnibius, LLC
// Author:  Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-06-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is a part of util open-source project.

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

#ifndef _UTIL_VARIANT_CONFIG_HPP_
#define _UTIL_VARIANT_CONFIG_HPP_

#include <util/variant_tree.hpp>
#include <util/error.hpp>

namespace util {

typedef variant_tree config_tree;
/// Configuration path derived from boost/property_tree/string_path.hpp
typedef typename variant_tree::path_type    config_path;

inline config_path operator/ (const config_path& a, const std::string& s) {
    config_path t(a);
    t /= s;
    return t;
}

inline void operator/ (config_path& a, const std::string& s) {
    a /= s;
}

inline config_path operator/ (const std::string& a, const config_path& s) {
    config_path t(a);
    t /= s;
    return t;
}

/**
 * \brief Exception class for configuration related errors.
 * Example use:
 *   <tt>throw config_error(a_path, "Test ") << 1 << " result:" << 2;</tt>
 */
class config_error : public detail::streamed_exception {
    std::string m_path;
public:
    config_error(const std::string& a_path) : m_path(a_path) {}

    template <class T>
    config_error(const std::string& a_path, T a) : m_path(a_path) {
        *this << a;
    }

    template <class T1, class T2>
    config_error(const std::string& a_path, T1 a1, T2 a2) : m_path(a_path) {
        *this << a1 << a2;
    }

    template <class T1, class T2, class T3>
    config_error(const std::string& a_path, T1 a1, T2 a2, T3 a3) : m_path(a_path) {
        *this << a1 << a2 << a3;
    }

    config_error(const config_path& a_path) : m_path(a_path.dump()) {}

    template <class T>
    config_error(const config_path& a_path, T a) : m_path(a_path.dump()) {
        *this << a;
    }

    template <class T1, class T2>
    config_error(const config_path& a_path, T1 a1, T2 a2) : m_path(a_path.dump()) {
        *this << a1 << a2;
    }

    template <class T1, class T2, class T3>
    config_error(const config_path& a_path, T1 a1, T2 a2, T3 a3) : m_path(a_path.dump()) {
        *this << a1 << a2 << a3;
    }

    virtual ~config_error() throw() {}

    const std::string& path() const { return m_path; }

    virtual std::string str() const {
        std::stringstream s;
        s << "Config error [" << m_path << "]: " << m_out->str();
        return s.str();
    }
};

} // namespace util

#endif // _UTIL_VARIANT_CONFIG_HPP_

