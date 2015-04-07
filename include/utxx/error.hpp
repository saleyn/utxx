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
#include <boost/shared_ptr.hpp>
#include <boost/current_function.hpp>
#include <sys/socket.h>
#include <string.h>
#include <utxx/compiler_hints.hpp>
#include <utxx/typeinfo.hpp>

// Alias for source location information
#define UTXX_SRC utxx::src_info(UTXX_FILE_SRC_LOCATION, BOOST_CURRENT_FUNCTION)

// Throw given exception with provided source location information
#define UTXX_SRC_THROW(Exception, SrcInfo, ...) throw Exception((SrcInfo), ##__VA_ARGS__)

// Throw given exception with current source location information
#define UTXX_THROW(Exception, ...) UTXX_SRC_THROW(Exception, UTXX_SRC, ##__VA_ARGS__)

// Throw utxx::runtime_error exception with current source location information
#define UTXX_THROW_RUNTIME_ERROR(...) UTXX_SRC_THROW(utxx::runtime_error, UTXX_SRC, ##__VA_ARGS__)

// Throw utxx::badarg_error exception with current source location information
#define UTXX_THROW_BADARG_ERROR(...) UTXX_SRC_THROW(utxx::badarg_error, UTXX_SRC, ##__VA_ARGS__)

// Throw utxx::io_error exception with current source location information
#define UTXX_THROW_IO_ERROR(Errno, ...) UTXX_SRC_THROW(utxx::io_error, UTXX_SRC, (Errno), ##__VA_ARGS__)

namespace utxx {

/// Thread-safe function returning error string.
static inline std::string errno_string(int a_errno) {
    return strerror(a_errno);
}

class not_implemented : public std::exception {
public:
    not_implemented() {}
    virtual ~not_implemented() {}

    virtual const char* what() const throw() { return "Not implemented!"; }
};

class traced_exception : public std::exception {
    static const size_t s_frames = 25;
    void*  m_backtrace[s_frames];
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
            out << a_prefix << detail::demangle(m_symbols[i]) << std::endl;
        return out;
    }

    traced_exception() : m_symbols(NULL) {
        m_size    = backtrace(m_backtrace, s_frames);
        m_symbols = backtrace_symbols(m_backtrace, m_size);
    }

    virtual ~traced_exception() throw() {
        if (m_symbols) free(m_symbols);
    }
};

/// Structure encapsulating source file:line information and source function.
///
/// It can be used for logging and throwing exceptions
class src_info {
    const char* m_srcloc;
    const char* m_fun;
    int         m_srcloc_len;
    int         m_fun_len;
public:
    constexpr src_info() : src_info("", "") {}

    template <int N, int M>
    constexpr src_info(const char (&a_srcloc)[N], const char (&a_fun)[M]) noexcept
        : m_srcloc(a_srcloc), m_fun(a_fun)
        , m_srcloc_len(N-1),  m_fun_len(M-1)
    {
        static_assert(N >= 1, "Invalid string literal! Length is zero!");
        static_assert(M >= 1, "Invalid string literal! Length is zero!");
    }

    constexpr src_info(const char a_srcloc[1], const char a_fun[1]) noexcept
        : m_srcloc(""), m_fun("")
        , m_srcloc_len(0),  m_fun_len(0)
    {}

    src_info(const char* a_srcloc, size_t N, const char* a_fun, size_t M)
        : m_srcloc(a_srcloc), m_fun(a_fun)
        , m_srcloc_len(N),    m_fun_len(M)
    {
        assert(N >  0);
        assert(M >= 0);
    }

    src_info(const src_info& a)             = default;
    src_info& operator=(src_info&& a)       = default;
    src_info(src_info&& a)                  = default;
    src_info& operator=(const src_info& a)  = default;

    const char* srcloc()     const { return m_srcloc;     }
    const char* fun()        const { return m_fun;        }

    int         srcloc_len() const { return m_srcloc_len; }
    int         fun_len()    const { return m_fun_len;    }

    bool        empty()      const { return !m_srcloc_len;}

    operator std::string() { return to_string(); }

    friend inline std::ostream& operator<< (std::ostream& out, const src_info& a) {
        return out << a.to_string("[", "]");
    }

    /// Format and write "file:line function" information to \a a_buf
    /// @param a_pfx             write leading prefix
    /// @param a_sfx             write trailing suffix
    /// @param a_fun_scope_depth controls how many function name's
    ///                          namespace levels should be included in the
    ///                          output (0 - means don't print the function name)
    std::string to_string(const char* a_pfx = "", const char* a_sfx = "",
                          int a_fun_scope_depth = 3) const
    {
        char buf[256];
        char* p = to_string(buf, sizeof(buf), a_pfx, a_sfx, a_fun_scope_depth);
        return std::string(buf, p - buf);
    }

    /// Format and write "file:line function" information to \a a_buf
    /// @param a_pfx             write leading prefix
    /// @param a_sfx             write trailing suffix
    /// @param a_fun_scope_depth controls how many function name's
    ///                          namespace levels should be included in the
    ///                          output (0 - means don't print the function name)
    char* to_string(char* a_buf, size_t a_sz,
                    const char* a_pfx = "", const char* a_sfx = "",
                    int a_fun_scope_depth = 3) const
    {
        const char* end = a_buf + a_sz-1;
        char* p = stpncpy(a_buf, a_pfx, end - a_buf);
        p = to_string(p, end - p, m_srcloc, m_srcloc_len, m_fun, m_fun_len,
                      a_fun_scope_depth);
        p = stpncpy(p, a_sfx, end - p);
        return p;
    }

    /// Format and write "file:line function" information to \a a_buf
    /// @param a_pfx             write leading prefix
    /// @param a_sfx             write trailing suffix
    /// @param a_fun_scope_depth controls how many function name's
    ///                          namespace levels should be included in the
    ///                          output (0 - means don't print the function name)
    static char* to_string(char* a_buf, size_t a_sz,
        const char* a_srcloc, size_t a_sl_len,
        const char* a_srcfun, size_t a_sf_len,
        int a_fun_scope_depth = 3)
    {
        static const char s_sep =
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN64__) || defined(__CYGWIN__)
            '\\';
#else
            '/';
#endif

        auto q = strrchr(a_srcloc, s_sep);
        q = q ? q+1 : a_srcloc;
        auto e   = a_srcloc + a_sl_len;
        // extra byte for possible '\n'
        auto p   = a_buf;
        auto end = a_buf + a_sz - 1;
        auto len = std::min<size_t>(end - p, e - q + 1);
        p = stpncpy(p, q, len);

        if (a_fun_scope_depth && a_sf_len) {
            *p++ = ' ';
            // Function name can contain:
            //   "static void fun()"
            //   "static void scope::fun()"
            //   "void scope::fun()"
            //   "void ns1::ns2::Class<Impl>::Fun()"
            // In the last case if a_write_fun_depth = 2, then we only output:
            //   "Class::Fun"
            static const uint8_t N = 16;
            // tribreces -  records positions of "<...>" delimiters
            // scopes    -  records positions of "::"    delimiters
            int tribrcnt = 0, scope = 1;
            struct { const char* l; const char* r; } tribraces[N];
            const char* scopes[N];
            auto begin = a_srcfun;
            auto inside = false;    // true when we are inside "<...>"
            // We search for '(' to signify the end of input, and skip
            // everything prior to the last space:
            for (q = begin, e = q + a_sf_len; q < e; ++q) {
                switch (*q) {
                    case '(':
                        e = q; break;
                    case ' ':
                        if (!inside) {
                            begin    = q+1;
                            scope    = 1;
                            tribrcnt = 0;
                        }
                        break;
                    case '<':
                        if (tribrcnt < N) {
                            auto& x = tribraces[tribrcnt];
                            x.l = q; x.r = q;
                            inside = true;
                        }
                        break;
                    case '>':
                        if (inside) {
                            tribraces[tribrcnt++].r = q+1;
                            inside = false;
                        }
                        break;
                    case ':':
                        if (*(q+1)==':' && scope<N) { scopes[scope++] = q+2; }
                        break;
                }
            }
            scopes[0] = begin;
            auto start_scope_idx =  scope > a_fun_scope_depth
                                 ? (scope - a_fun_scope_depth) : 0;
            // Does the name has fewer namespace scopes than what's allowed?
            begin = scopes[start_scope_idx];
            // Are there any templated arguments to be trimmed?
            if (!tribrcnt) {
                len = std::min<size_t>(end - p, e - begin);
                p = stpncpy(p, begin, len);
            } else {
                q = begin;
                // Copy all characters excluding what's inside "<...>"
                for (int i=0; i < tribrcnt && p < end; i++) {
                    auto qe = tribraces[i].l;
                    if (qe <= q)
                        continue;
                    len = std::min<size_t>(end - p, qe - q);
                    p = stpncpy(p, q, len);
                    q = tribraces[i].r;
                }
                // Now q points to the character past last '>' found.
                // Copy everything remaining from q to e
                len = std::min<size_t>(end - p, e - q);
                p   = stpncpy(p, q, len);
            }
        }

        return p;
    }
};

namespace detail {
    class streamed_exception : public std::exception {
    protected:
        boost::shared_ptr<std::stringstream> m_out;
        mutable std::string m_str;

        virtual void cache_str() const throw() {
            if (m_str.empty())
                try   { m_str = str(); }
                catch (...) {
                    try { m_str = "Memory allocation error"; }
                    catch (...) {} // Give up
                }
        }

#if __cplusplus >= 201103L
        streamed_exception& output() { return *this; }

        template <class... Args>
        streamed_exception& output(const src_info& sinfo, Args&&... t) {
            *m_out << sinfo.to_string("[", "] ");
            return output(std::forward<Args>(t)...);
        }
        template <class T, class... Args>
        streamed_exception& output(const T& h, Args&&... t) {
            *m_out << h;
            return output(std::forward<Args>(t)...);
        }
#endif

    public:
        streamed_exception() : m_out(new std::stringstream()) {}
        virtual ~streamed_exception() throw() {}

        streamed_exception& operator<< (const src_info& sinfo) {
            *m_out << sinfo.to_string("[", "] ");
            return *this;
        }
        template <class T>
        streamed_exception& operator<< (const T& a) { *m_out << a; return *this; }

        virtual const char* what()      const throw() { cache_str(); return m_str.c_str(); }
        virtual std::string str()       const         { return m_out->str();  }
    };
}

/**
 * \brief Exception class with ability to stream arguments to it.
 *   <tt>throw io_error(errno, "Test", 1, " result:", 2);</tt>
 *   <tt>throw io_error(errno, "Test") << 1 << " result:" << 2;</tt>
 * so we choose to reallocate the m_out string instead.
 */
class runtime_error : public detail::streamed_exception {
    using base = detail::streamed_exception;
protected:
    src_info m_sinfo;

    virtual void cache_str() const throw() override {
        base::cache_str();
        if (!m_sinfo.empty())
            try   { m_str = m_sinfo.to_string("[", "] ") + m_str; }
            catch (...) {}
    }

    runtime_error() {}
public:
    explicit runtime_error(const src_info& a_info) : m_sinfo(a_info) {}
    explicit runtime_error(src_info&& a_info)      : m_sinfo(std::move(a_info)) {}

    runtime_error(const std::string& a_str)  { *this << a_str;  }

    template <class... Args>
    runtime_error(const src_info& a_info, const Args&... a_rest)
        : m_sinfo(a_info)
    {
        output(a_rest...);
    }

    template <class... Args>
    runtime_error(src_info&& a_info, const Args&... a_rest)
        : m_sinfo(std::move(a_info))
    {
        output(a_rest...);
    }

    template <class T, class... Args>
    runtime_error(const T& a_first, const Args&... a_rest) {
        *this << a_first;
        output(a_rest...);
    }

    /// This streaming operator allows throwing exceptions in the form:
    /// <tt>throw runtime_error("Test") << ' ' << 10 << " failed";
    template <class T>
    runtime_error& operator<< (const T& a) {
        *static_cast<detail::streamed_exception*>(this) << a;
        return *this;
    }

    virtual ~runtime_error() throw() {}

    const src_info& src() const { return m_sinfo; }
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
    io_error(src_info&& a_info, int a_errno)
        : runtime_error(std::forward<src_info>(a_info), errno_string(a_errno))
    {}

    io_error(int a_errno) : runtime_error(errno_string(a_errno))
    {}

    io_error(int a_errno, const char* a_prefix) {
        *this << a_prefix << ": " << errno_string(a_errno);
    }

    io_error(int a_errno, const std::string& a_prefix) {
        *this << a_prefix << ": " << errno_string(a_errno);
    }

    template <class T, class... Args>
    io_error(src_info&& a_info, int a_errno, T&& a_prefix, Args&&... args)
        : runtime_error(std::forward<src_info>(a_info))
    {
        *this << std::forward<T>(a_prefix);
        output(std::forward<Args>(args)...) << ": " << errno_string(a_errno);
    }

    template <class T, class... Args>
    io_error(int a_errno, T&& a_prefix, Args&&... args) {
        *this << std::forward<T>(a_prefix);
        output(std::forward<Args>(args)...) << ": " << errno_string(a_errno);
    }

    virtual ~io_error() throw() {}

    /// This streaming operator allows throwing exceptions in the form:
    /// <tt>throw io_error("Test") << ' ' << 10 << " failed";
    template <class T>
    io_error& operator<< (const T& a) {
        *static_cast<detail::streamed_exception*>(this) << a; return *this;
    }
};

#if __cplusplus >= 201103L

/// @brief Exception class for socket-related errors.
class sock_error : public runtime_error {
    std::string get_error(int a_fd) {
        int ec;
        socklen_t errlen = sizeof(ec);

        if (getsockopt(a_fd, SOL_SOCKET, SO_ERROR, (void *)&ec, &errlen) < 0)
            ec = errno;
        
        return errno_string(ec);
    }
public:
    sock_error(src_info&& a_info, int a_fd)
        : runtime_error(std::forward<src_info>(a_info), get_error(a_fd))
    {}

    sock_error(src_info&& a_info, int a_fd, const char* a_prefix)
        : runtime_error(std::forward<src_info>(a_info))
    {
        *this << a_prefix << ": " << get_error(a_fd);
    }

    sock_error(src_info&& a_info, int a_fd, const std::string& a_prefix)
        : runtime_error(std::forward<src_info>(a_info))
    {
        *this << a_prefix << ": " << get_error(a_fd);
    }

    sock_error(int a_fd) : runtime_error(get_error(a_fd))
    {}

    sock_error(int a_fd, const char* a_prefix) {
        *this << a_prefix << ": " << get_error(a_fd);
    }

    sock_error(int a_fd, const std::string& a_prefix) {
        *this << a_prefix << ": " << get_error(a_fd);
    }

    template <class T, class... Args>
    sock_error(int a_fd, T&& a_prefix, Args&&... args) {
        *this << std::forward<T>(a_prefix);
        output(std::forward<Args>(args)...) << ": " << get_error(a_fd);
    }

    virtual ~sock_error() throw() {}

    /// This streaming operator allows throwing exceptions in the form:
    /// <tt>throw sock_error("Test") << ' ' << 10 << " failed";
    template <class T>
    sock_error& operator<< (const T& a) {
        *static_cast<detail::streamed_exception*>(this) << a; return *this;
    }
};

#endif

typedef runtime_error gen_error;         ///< General error
typedef runtime_error sys_error;         ///< System error
typedef runtime_error encode_error;      ///< Encoding error.
typedef runtime_error decode_error;      ///< Decoding error.
typedef runtime_error badarg_error;      ///< Bad arguments error.
typedef runtime_error logic_error;       ///< Program logic error.

} // namespace utxx

#endif // _IO_UTXX_ERROR_HPP_

