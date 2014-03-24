//----------------------------------------------------------------------------
/// \file variant_tree_error.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// Defines variant_tree_error classes for ease of configuration error reporting
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

#ifndef _UTXX_VARIANT_TREE_ERROR_HPP_
#define _UTXX_VARIANT_TREE_ERROR_HPP_

#include <utxx/variant_tree_path.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>
#include <utxx/error.hpp>
#include <stdexcept>

namespace utxx {

// Errors thrown on bad data
typedef boost::property_tree::ptree_bad_data    variant_tree_bad_data;
typedef boost::property_tree::ptree_bad_path    variant_tree_bad_path;
typedef boost::property_tree::file_parser_error variant_tree_parser_error;
/**
 * \brief Exception class for configuration related errors.
 * Example use:
 *   <tt>throw variant_tree_error(a_path, "Test ") << 1 << " result:" << 2;</tt>
 */
class variant_tree_error : public detail::streamed_exception {
    std::string m_path;
#if __cplusplus >= 201103L
    void stream() {}

    template <class T, class... Args>
    void stream(const T& a, Args... args) { *this << a; stream(args...); }
#endif
public:
    variant_tree_error(const std::string& a_path) : m_path(a_path) {}

#if __cplusplus >= 201103L
    template <class... Args>
    variant_tree_error(const std::string& a_path, Args... args)
        : variant_tree_error(a_path)
    {
        stream(args...);
    }

    template <class... Args>
    variant_tree_error(const tree_path& a_path, Args... args)
        : variant_tree_error(a_path.dump())
    {
        stream(args...);
    }
#else
    template <class T>
    variant_tree_error(const std::string& a_path, T a) : m_path(a_path) {
        *this << a;
    }

    template <class T1, class T2>
    variant_tree_error(const std::string& a_path, T1 a1, T2 a2) : m_path(a_path) {
        *this << a1 << a2;
    }

    template <class T1, class T2, class T3>
    variant_tree_error(const std::string& a_path, T1 a1, T2 a2, T3 a3) : m_path(a_path) {
        *this << a1 << a2 << a3;
    }

    variant_tree_error(const tree_path& a_path) : m_path(a_path.dump()) {}

    template <class T>
    variant_tree_error(const tree_path& a_path, T a) : m_path(a_path.dump()) {
        *this << a;
    }

    template <class T1, class T2>
    variant_tree_error(const tree_path& a_path, T1 a1, T2 a2) : m_path(a_path.dump()) {
        *this << a1 << a2;
    }

    template <class T1, class T2, class T3>
    variant_tree_error(const tree_path& a_path, T1 a1, T2 a2, T3 a3) : m_path(a_path.dump()) {
        *this << a1 << a2 << a3;
    }

    template <class T1, class T2, class T3, class T4>
    variant_tree_error(const tree_path& a_path, T1 a1, T2 a2, T3 a3, T4 a4)
        : m_path(a_path.dump()) {
        *this << a1 << a2 << a3 << a4;
    }

    template <class T1, class T2, class T3, class T4, class T5>
    variant_tree_error(const tree_path& a_path, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
        : m_path(a_path.dump()) {
        *this << a1 << a2 << a3 << a4 << a5;
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    variant_tree_error(const tree_path& a_path, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
        : m_path(a_path.dump()) {
        *this << a1 << a2 << a3 << a4 << a5 << a6;
    }
#endif

    virtual ~variant_tree_error() throw() {}

    const std::string& path() const { return m_path; }

    virtual std::string str() const {
        std::stringstream s;
        s << "Config error [" << m_path << "]: " << m_out->str();
        return s.str();
    }
};

} // namespace utxx

#endif // _UTXX_VARIANT_TREE_ERROR_HPP_

