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
#pragma once

#include <exception>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <execinfo.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <sys/socket.h>
#include <boost/current_function.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/typeinfo.hpp>

namespace utxx {
    /// Controls src_info printing defaults
    struct src_info_defaults {
        src_info_defaults(int a_scopes = -1)
            : m_scopes(a_scopes < 0 ? s_print_fun_scopes : a_scopes)
        {}

        int scopes() const { return m_scopes; }

        /// Get default number of of function scopes to print by `src_info`.
        static int  print_fun_scopes() { return s_print_fun_scopes; }

        /// Set default number of of function scopes to print by `src_info`.
        /// Initialized to `3` at startup.
        static void print_fun_scopes(uint8_t a_scopes) {
            s_print_fun_scopes = a_scopes;
        }
    private:
        static int  s_print_fun_scopes;        // Initialized in error.cpp
        int         m_scopes;

        static int  def_print_scopes() { return 3; }
    };
}

/// Pretty function name stripped off of angular brackets and some namespaces
///
/// Optional argument:
///   int ScopeDepth    - number of namespaces to include (default: 3)
#define UTXX_PRETTY_FUNCTION(...) \
    static const std::string __utxx_pretty_function__ = \
        utxx::src_info::pretty_function \
            (BOOST_CURRENT_FUNCTION, utxx::src_info_defaults(__VA_ARGS__).scopes())

/// Alias for source location information
#define UTXX_SRC utxx::src_info(UTXX_FILE_SRC_LOCATION, BOOST_CURRENT_FUNCTION)

/// Alias for source location information using cached pretty function name
///
/// The cached pretty function name must be declared by call to UTXX_PRETTY_FUNCTION
#define UTXX_SRCX utxx::src_info(UTXX_FILE_SRC_LOCATION, __utxx_pretty_function__, true)

#define UTXX_SRCD(si)  (si.empty() ? UTXX_SRC  : std::forward<utxx::src_info>(si))
#define UTXX_SRCXD(si) (si.empty() ? UTXX_SRCX : std::forward<utxx::src_info>(si))

/// Throw given exception with provided source location information
#define UTXX_SRC_THROW(Exception, SrcInfo, ...) \
    throw Exception((SrcInfo), ##__VA_ARGS__)

#define UTXX_SRCD_THROW(Exception, SrcInfo, ...) \
    throw Exception(UTXX_SRCD(SrcInfo), ##__VA_ARGS__)

#define UTXX_SRCXD_THROW(Exception, SrcInfo, ...) \
    throw Exception(UTXX_SRCXD(SrcInfo), ##__VA_ARGS__)


/// Throw given exception with current source location information
#define UTXX_THROW(Exception,  ...) UTXX_SRC_THROW(Exception, UTXX_SRC,  ##__VA_ARGS__)
/// Throw given exception with current source location info and cached function name
#define UTXX_THROWX(Exception, ...) UTXX_SRC_THROW(Exception, UTXX_SRCX, ##__VA_ARGS__)

/// Throw utxx::runtime_error exception with current source location information
#define UTXX_THROW_RUNTIME_ERROR(...) \
    UTXX_SRC_THROW(utxx::runtime_error, UTXX_SRC, ##__VA_ARGS__)
/// Throw utxx::runtime_error exception with current source location and cached fun name
#define UTXX_THROWX_RUNTIME_ERROR(...) \
    UTXX_SRC_THROW(utxx::runtime_error, UTXX_SRCX, ##__VA_ARGS__)

/// Throw utxx::badarg_error exception with current source location information
#define UTXX_THROW_BADARG_ERROR(...) \
    UTXX_SRC_THROW(utxx::badarg_error, UTXX_SRC, ##__VA_ARGS__)
/// Throw utxx::badarg_error exception with current source location and cached fun name
#define UTXX_THROWX_BADARG_ERROR(...) \
    UTXX_SRC_THROW(utxx::badarg_error, UTXX_SRCX, ##__VA_ARGS__)

/// Throw utxx::logic_error exception with current source location information
#define UTXX_THROW_LOGIC_ERROR(...) \
    UTXX_SRC_THROW(utxx::logic_error, UTXX_SRC, ##__VA_ARGS__)
/// Throw utxx::logic_error exception with current source location and cached fun name
#define UTXX_THROWX_LOGIC_ERROR(...) \
    UTXX_SRC_THROW(utxx::logic_error, UTXX_SRCX, ##__VA_ARGS__)

/// Throw utxx::io_error exception with current source location information
#define UTXX_THROW_IO_ERROR(Errno, ...) \
    UTXX_SRC_THROW(utxx::io_error, UTXX_SRC, (Errno), ##__VA_ARGS__)
/// Throw utxx::io_error exception with current source location and cached fun name
#define UTXX_THROWX_IO_ERROR(Errno, ...) \
    UTXX_SRC_THROW(utxx::io_error, UTXX_SRCX, (Errno), ##__VA_ARGS__)

/// Throw utxx::runtime_error exeception indicating that a method is not implemented
#define UTXX_THROW_NOT_IMPLEMENTED() \
    UTXX_SRC_THROW(utxx::runtime_error, UTXX_SRC, "Method not implemented!")

/// Guard expression with a try/catch, and throw utxx::runtime_error on exception
#define UTXX_RETHROW(Expr) \
    try { (Expr); } \
    catch (utxx::runtime_error const& e) { throw; } \
    catch (std::exception      const& e) { UTXX_THROW_RUNTIME_ERROR(e.what()); }

namespace utxx {

/// Thread-safe function returning error string.
inline std::string errno_string(int a_errno) {
    // Note: GNU-specific implementation is thread-safe, and strerror_r doesn't
    // really use the given char buffer, so we chose to call strerror():
    return strerror(a_errno);
}

inline std::string errno_string() { return errno_string(errno); }

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
    uint16_t    m_srcloc_len;
    uint16_t    m_fun_len;
    bool        m_fun_verbatim;
public:
    constexpr src_info()
        : m_srcloc(""), m_fun(""), m_srcloc_len(0), m_fun_len(0)
        , m_fun_verbatim(false)
    {}

    template <uint16_t N, uint16_t M>
    constexpr src_info(const char (&a_srcloc)[N], const char (&a_fun)[M],
                       bool a_fun_verbatim = false) noexcept
        : m_srcloc(a_srcloc), m_fun(a_fun)
        , m_srcloc_len(N-1),  m_fun_len(M-1)
        , m_fun_verbatim(a_fun_verbatim)
    {
        static_assert(N >= 1, "Invalid string literal! Length is zero!");
        static_assert(M >= 1, "Invalid string literal! Length is zero!");
    }

    /// NOTE: only use this version for statically assigned \a a_fun (such as
    ///       in the UTXX_SRCX macro using UTXX_PRETTY_FUNCTION() reference)!
    template <int N>
    constexpr src_info(const char (&a_srcloc)[N], const std::string& a_fun,
                       bool  a_fun_verbatim = false) noexcept
        : m_srcloc(a_srcloc), m_fun(a_fun.c_str())
        , m_srcloc_len(N-1),  m_fun_len(a_fun.size())
        , m_fun_verbatim(a_fun_verbatim)
    {
        static_assert(N >= 1, "Invalid string literal! Length is zero!");
    }

    src_info(const char* a_srcloc, uint16_t N, const char* a_fun, uint16_t M,
             bool  a_fun_verbatim = false) noexcept
        : m_srcloc(a_srcloc), m_fun(a_fun)
        , m_srcloc_len(N),    m_fun_len(M)
        , m_fun_verbatim(a_fun_verbatim)
    {
        assert(N >  0);
        assert(M >= 0);
    }

    src_info(const src_info& a)             = default;
    src_info& operator=(src_info&& a)       = default;
    src_info(src_info&& a)                  = default;
    src_info& operator=(const src_info& a)  = default;

    const char* srcloc()       const { return m_srcloc;       }
    const char* fun()          const { return m_fun;          }

    uint16_t    srcloc_len()   const { return m_srcloc_len;   }
    uint16_t    fun_len()      const { return m_fun_len;      }
    bool        fun_verbatim() const { return m_fun_verbatim; }
    bool        empty()        const { return !m_srcloc_len;  }

    operator    std::string()  const { return to_string();    }

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
                          int a_fun_scope_depth = -1) const
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
                    int a_fun_scope_depth = -1) const
    {
        const char* end = a_buf + a_sz-1;
        char* p = stpncpy(a_buf, a_pfx, end - a_buf);
        p = to_string(p, end - p, m_srcloc, size_t(m_srcloc_len),
                      m_fun, size_t(m_fun_len),
                      a_fun_scope_depth, m_fun_verbatim);
        p = stpncpy(p, a_sfx, end - p);
        return p;
    }

    /// Format function name by stripping angular brackets and extra namespaces
    /// @param a_fun_scope_depth controls how many function name's
    ///                          namespace scopes should be included in the
    ///                          output (0 - means don't print the function name)
    static std::string pretty_function(const char* a_pretty_function,
                                       int a_fun_scope_depth = -1)
    {
        char buf[128];
        char* end = to_string(buf, sizeof(buf), "", 0,
                              a_pretty_function, strlen(a_pretty_function),
                              a_fun_scope_depth);
        return std::string(buf, end - buf);
    }

    /// Format and write "file:line function" information to \a a_buf
    /// @param a_pfx             write leading prefix
    /// @param a_sfx             write trailing suffix
    /// @param a_fun_scope_depth controls how many function name's
    ///                          namespace levels should be included in the
    ///                          output (0 - means don't print the function name)
    /// @param a_fun_verbatim    print function name as is without prettifying
    static char* to_string(char* a_buf, size_t a_sz,
        const char* a_srcloc, size_t a_sl_len,
        const char* a_srcfun, size_t a_sf_len,
        int a_fun_scope_depth = -1, bool a_fun_verbatim = false)
    {
        static const char s_sep =
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN64__) || defined(__CYGWIN__)
            '\\';
#else
            '/';
#endif
        const int fun_scope_depth = a_fun_scope_depth < 0
                                  ? src_info_defaults::print_fun_scopes()
                                  : a_fun_scope_depth;
        const char* q, *e;

        auto p   = a_buf;
        auto end = a_buf + a_sz - 1;

        if (end - p <= 0)
            return p;

        if (a_sl_len) {
            q = strrchr(a_srcloc, s_sep);
            q = q ? q+1 : a_srcloc;
            e = a_srcloc + a_sl_len;
            // extra byte for possible '\n'
            auto len = std::min<size_t>(end - p, e - q + 1);
            p = stpncpy(p, q, len);
        }
        if (fun_scope_depth && a_sf_len && p < end) {
            if (a_sl_len) *p++ = ' ';
            // If asked to print function name as-is, just copy it and return
            if (a_fun_verbatim) {
                auto len = std::min<size_t>(end - p, a_sf_len + 1);
                p = stpncpy(p, a_srcfun, len);
                return p;
            }
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
            const char* scopes[N] = {0};
            scopes[0]   = "";
            auto begin  = a_srcfun;
            auto matched_open_tribrace = -1;
            auto inside = 0;  // > 0 when we are inside "<...>"
            if (strncmp(begin, "static ",   7)==0) begin += 7;
            if (strncmp(begin, "typename ", 9)==0) begin += 9;
            //
            // We search for '(' to signify the end of input, and skip
            // everything prior to the last space:
            for (q = begin, e = a_srcfun+a_sf_len; q < e; ++q) {
                switch (*q) {
                    case '(':
                        if (inside)
                            continue;
                        // This is a rare lambda case, e.g.:
                        //      xxx::(anonymous class)::yyy()
                        // replace with "<lambda>" scope
                        if (strncmp(q+1, "anonymous class)", std::min<size_t>(16,e-q-1)) == 0 && scope<N) {
                            q += 16;
                            continue;
                        }
                        // This is an operator() case, e.g.:
                        //      xxx::operator()(const char*)
                        // Preserve first set of "()"
                        if (strncmp(scopes[scope-1], "operator", 8) == 0 && *(q+1) == ')') {
                            q++;
                            continue;
                        }
                        e = q;
                        break;
                    case ' ':
                        if (inside)
                            continue;
                        while (*(++q) == '*' || *q == '&');
                        begin    = q;
                        scope    = 1;
                        tribrcnt = 0;
                        break;
                    case '<':
                        if (tribrcnt < N) {
                            auto& x = tribraces[tribrcnt++];
                            x.l = q; x.r = q;
                            ++inside;
                        }
                        break;
                    case '>':
                        if (inside) {
                            int i = matched_open_tribrace == -1
                                  ? tribrcnt-1 : matched_open_tribrace-1;
                            tribraces[i].r = q+1;
                            inside = std::max<int>(0, inside-1);
                            matched_open_tribrace = inside ? i : -1;
                        }
                        break;
                    case ':':
                        if (!inside && *(q+1)==':' && scope<N)
                            scopes[scope++] = ++q + 1;
                        break;
                }
            }
            scopes[0] = begin;
            auto start_scope_idx =  scope > fun_scope_depth
                                 ? (scope - fun_scope_depth) : 0;
            // Does the name has fewer namespace scopes than what's allowed?
            begin = scopes[start_scope_idx];
            // Are there any templated arguments to be trimmed?
            if (!tribrcnt) {
                auto len = std::min<size_t>(end - p, e - begin);
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wstringop-truncation"
                p = stpncpy(p, begin, len);
                #pragma GCC diagnostic pop
            } else {
                q = begin;
                // Copy all characters excluding what's inside "<...>"
                for (int i=0; i < tribrcnt && p < end; i++) {
                    auto qe = tribraces[i].l;
                    if (qe <= q)
                        continue;
                    p = stpncpy(p, q, std::min<size_t>(end - p, qe - q));
                    q = tribraces[i].r;
                }
                // Now q points to the character past last '>' found.
                // Copy everything remaining from q to e
                p = stpncpy(p, q, std::min<size_t>(end - p, e - q));
            }
        }

        return p;
    }
};

namespace detail {
    class streamed_exception : public std::exception {
    protected:
        std::shared_ptr<std::stringstream> m_out;
        mutable std::string m_str;

        virtual void cache_str() const throw() {
            if (m_str.empty())
                try   { m_str = str(); }
                catch (...) {
                    try { m_str = "Memory allocation error"; }
                    catch (...) {} // Give up
                }
        }

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
    src_info&&      src()       { return std::move(m_sinfo); }
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
