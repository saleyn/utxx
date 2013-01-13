//----------------------------------------------------------------------------
/// \file variant_config.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// Defines config_tree, config_path, and config_error classes for
/// ease of configuration management.
//----------------------------------------------------------------------------
// Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-06-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is a part of utxx open-source project.

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

#ifndef _UTXX_CONFIG_TREE_HPP_
#define _UTXX_CONFIG_TREE_HPP_

#include <utxx/variant_tree.hpp>
#include <utxx/error.hpp>

namespace utxx {

/// Container for storing configuration data.
typedef variant_tree config_tree;
/// Configuration path derived from boost/property_tree/string_path.hpp
typedef tree_path config_path;
// Errors thrown on bad data
typedef boost::property_tree::ptree_bad_data config_bad_data;
typedef boost::property_tree::ptree_bad_path config_bad_path;

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

    template <class T1, class T2, class T3, class T4>
    config_error(const config_path& a_path, T1 a1, T2 a2, T3 a3, T4 a4)
        : m_path(a_path.dump()) {
        *this << a1 << a2 << a3 << a4;
    }

    template <class T1, class T2, class T3, class T4, class T5>
    config_error(const config_path& a_path, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
        : m_path(a_path.dump()) {
        *this << a1 << a2 << a3 << a4 << a5;
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    config_error(const config_path& a_path, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
        : m_path(a_path.dump()) {
        *this << a1 << a2 << a3 << a4 << a5 << a6;
    }

    virtual ~config_error() throw() {}

    const std::string& path() const { return m_path; }

    virtual std::string str() const {
        std::stringstream s;
        s << "Config error [" << m_path << "]: " << m_out->str();
        return s.str();
    }
};

} // namespace utxx

#endif // _UTXX_CONFIG_TREE_HPP_

