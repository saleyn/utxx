//----------------------------------------------------------------------------
/// \file   error.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Definition of exception classes.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
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
#ifndef _IO_UTXX_ERROR_HPP_
#define _IO_UTXX_ERROR_HPP_

#include <exception>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <execinfo.h>
#include <signal.h>
#include <assert.h>
#include <iostream>
#include <utxx/string.hpp>
#include <boost/shared_ptr.hpp>

namespace utxx {

/// Thread-safe function returning error string.
static inline std::string errno_string(int a_errno) {
    /*
    char buf[256];
    if (strerror_r(a_errno, buf, sizeof(buf)) < 0)
        return "";
    */
    std::string buf(strerror(a_errno));
    return buf;
}

class traced_exception : public std::exception {
    void*  m_backtrace[25];
    size_t m_size;
    char** m_symbols;
public:
    virtual const char* what() const throw() { return ""; }

    size_t      backtrace_size() const { return m_size; }
    const char* backtrace_frame(size_t i) {
        assert(i < m_size);
        return m_symbols[i];
    }

    std::ostream& print_backtrace(const char* a_prefix = "") const {
        return print_backtrace(std::cerr, a_prefix);
    }

    std::ostream& print_backtrace(std::ostream& out, const char* a_prefix = "") const {
        for(size_t i = 0; i < m_size; i++)
            out << a_prefix << m_symbols[i] << std::endl;
        return out;
    }

    traced_exception() : m_symbols(NULL) {
        m_size    = backtrace(m_backtrace, length(m_backtrace));
        m_symbols = backtrace_symbols(m_backtrace, m_size);
    }

    virtual ~traced_exception() throw() {
        if (m_symbols) free(m_symbols);
    }
};

namespace detail {
    class streamed_exception : public std::exception {
    protected:
        boost::shared_ptr<std::stringstream> m_out;
        mutable std::string m_str;

#if __cplusplus >= 201103L
        streamed_exception& output() { return *this; }

        template <class T, class... Args>
        streamed_exception& output(const T& h, const Args&... t) {
            *m_out << h;
            return output(t...);
        }
#endif

    public:
        streamed_exception() : m_out(new std::stringstream()) {}
        virtual ~streamed_exception() throw() {}

        template <class T>
        streamed_exception& operator<< (const T& a) { *m_out << a; return *this; }

        virtual const char* what()  const throw() { m_str = str(); return m_str.c_str(); }
        virtual std::string str()   const { return m_out->str();  }
    };
}

/**
 * \brief Exception class with ability to stream arguments to it.
 *   <tt>throw io_error(errno, "Test", 1, " result:", 2);</tt>
 *   <tt>throw io_error(errno, "Test") << 1 << " result:" << 2;</tt>
 * so we choose to reallocate the m_out string instead.
 */
class runtime_error : public detail::streamed_exception {
protected:
    runtime_error() {}
public:
    runtime_error(const std::string& a_str) { *this << a_str; }

#if __cplusplus >= 201103L

    template <class T, class... Args>
    runtime_error(const T& a_first, Args&&... a_rest) {
        *this << a_first << ' ';
        output(std::forward<Args>(a_rest)...);
    }

    /// This streaming operator allows throwing exceptions in the form:
    /// <tt>throw runtime_error("Test") << ' ' << 10 << " failed";
    template <class T>
    runtime_error& operator<< (T&& a) {
        *static_cast<detail::streamed_exception*>(this) << a; return *this;
    }
#else

    template <class T>
    runtime_error(const char* a_str, T a_arg) {
        *this << a_str << ' ' << a_arg;
    }

    template <class T1, class T2>
    runtime_error(const char* a_str, T1 a1, T2 a2) {
        *this << a_str << a1 << a2;
    }

    template <class T1, class T2, class T3>
    runtime_error(const char* a_str, T1 a1, T2 a2, T3 a3) {
        *this << a_str << a1 << a2 << a3;
    }

    /// Use for reporting errors in the form:
    ///    runtime_error("Bad data! (expected=", 1, ", got=", 3, ")");
    template <class T1, class T2, class T3, class T4>
    runtime_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4) {
        *this << a_str << a1 << a2 << a3 << a4;
    }

    template <class T1, class T2, class T3, class T4, class T5>
    runtime_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) {
        *this << a_str << a1 << a2 << a3 << a4 << a5;
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    runtime_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6) {
        *this << a_str << a1 << a2 << a3 << a4 << a5 << a6;
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    runtime_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7) {
        *this << a_str << a1 << a2 << a3 << a4 << a5 << a6 << a7;
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    runtime_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8) {
        *this << a_str << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8;
    }

    /// This streaming operator allows throwing exceptions in the form:
    /// <tt>throw runtime_error("Test") << ' ' << 10 << " failed";
    template <class T>
    runtime_error& operator<< (T a) {
        *static_cast<detail::streamed_exception*>(this) << a; return *this;
    }
#endif

    virtual ~runtime_error() throw() {}

};

/**
 * \brief Exception class for I/O related errors.
 * Note that we can't have a noncopyable stringstream as a 
 * member if we allow the following use:
 *   <tt>throw io_error(errno, "Test") << 1 << " result:" << 2;</tt>
 * so we choose to reallocate the m_out string instead.
 */
class io_error : public runtime_error {
public:
    io_error(int a_errno) : runtime_error(errno_string(a_errno))
    {}

    io_error(int a_errno, const char* a_prefix) {
        *this << a_prefix << ": " << errno_string(a_errno);
    }

    io_error(int a_errno, const std::string& a_prefix) {
        *this << a_prefix << ": " << errno_string(a_errno);
    }

#if __cplusplus >= 201103L

    template <class T, class... Args>
    io_error(int a_errno, T&& a_prefix, Args&&... args) {
        *this << std::forward<T>(a_prefix);
        output(std::forward<Args>(args)...) << ": " << errno_string(a_errno);
    }

#else

    template <class T>
    io_error(int a_errno, const char* a_prefix, T a_arg) {
        *this << a_prefix << a_arg << ": " << errno_string(a_errno);
    }

    template <class T1, class T2>
    io_error(int a_errno, const char* a_prefix, T1 a1, T2 a2) {
        *this << a_prefix << a1 << a2 << ": " << errno_string(a_errno);
    }

    template <class T1, class T2, class T3>
    io_error(int a_errno, const char* a_prefix, T1 a1, T2 a2, T3 a3) {
        *this << a_prefix << a1 << a2 << a3 << ": " << errno_string(a_errno);
    }

    template <class T1, class T2, class T3, class T4>
    io_error(int a_errno, const char* a_prefix, T1 a1, T2 a2, T3 a3, T4 a4) {
        *this << a_prefix << a1 << a2 << a3 << a4 << ": " << errno_string(a_errno);
    }

    template <class T>
    io_error(const char* a_str, T a_arg) {
        *this << a_str << ' ' << a_arg;
    }

    template <class T1, class T2>
    io_error(const char* a_str, T1 a1, T2 a2) {
        *this << a_str << a1 << a2;
    }

    template <class T1, class T2, class T3>
    io_error(const char* a_str, T1 a1, T2 a2, T3 a3) {
        *this << a_str << a1 << a2 << a3;
    }

    /// Use for reporting errors in the form:
    ///    io_error("Bad data! (expected=", 1, ", got=", 3, ")");
    template <class T1, class T2, class T3, class T4>
    io_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4) {
        *this << a_str << a1 << a2 << a3 << a4;
    }

    template <class T1, class T2, class T3, class T4, class T5>
    io_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) {
        *this << a_str << a1 << a2 << a3 << a4 << a5;
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    io_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6) {
        *this << a_str << a1 << a2 << a3 << a4 << a5 << a6;
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    io_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7) {
        *this << a_str << a1 << a2 << a3 << a4 << a5 << a6 << a7;
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    io_error(const char* a_str, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8) {
        *this << a_str << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8;
    }
#endif

    virtual ~io_error() throw() {}

    /// This streaming operator allows throwing exceptions in the form:
    /// <tt>throw io_error("Test") << ' ' << 10 << " failed";
    template <class T>
    io_error& operator<< (T a) {
        *static_cast<detail::streamed_exception*>(this) << a; return *this;
    }
};


typedef runtime_error gen_error;         ///< General error
typedef runtime_error sys_error;         ///< System error
typedef runtime_error encode_error;      ///< Encoding error.
typedef runtime_error decode_error;      ///< Decoding error.
typedef runtime_error badarg_error;      ///< Bad arguments error.

} // namespace IO_UTXX_NAMESPACE

#endif // _IO_UTXX_ERROR_HPP_

