//----------------------------------------------------------------------------
/// \file  print.hpp
//----------------------------------------------------------------------------
/// \brief This file contains implementation of printing functions.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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
#ifndef _UTXX_PRINT_HPP_
#define _UTXX_PRINT_HPP_

#include <string>
#include <type_traits>
#include <cstdarg>
#include <iomanip>
#include <utxx/scope_exit.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/convert.hpp>
#include <utxx/error.hpp>

namespace utxx {

/// Output a float to stream formatted with fixed precision
struct fixed {
    fixed(double a_val, int a_digits, int a_precision, char a_fill = ' ')
        : m_value(a_val), m_digits(a_digits), m_precision(a_precision)
        , m_fill(a_fill)
    {}

    inline friend std::ostream& operator<<(std::ostream& out, const fixed& f) {
        return out << std::fixed << std::setfill(f.m_fill)
                   << std::setw(f.m_digits)
                   << std::setprecision(f.m_precision) << f.m_value;
    }

    double value()     const { return m_value;     }
    int    digits()    const { return m_digits;    }
    int    precision() const { return m_precision; }
    char   fill()      const { return m_fill;      }

private:
    double m_value;
    int    m_digits;
    int    m_precision;
    char   m_fill;
};


namespace detail {
    template <size_t N = 256, class Alloc = std::allocator<char>>
    class basic_buffered_print : public Alloc
    {
        mutable char*   m_begin; // mutable so that we can write '\0' in c_str()
        char*           m_pos;
        char*           m_end;
        char            m_data[N];

        void deallocate() {
            if (m_begin == m_data) return;
            Alloc::deallocate(m_begin, max_size());
        }

        void do_print(char a) { reserve(1); *m_pos++ = a; }
        void do_print(bool a) {
            reserve(5);
            if (a) { memcpy(m_pos, "true", 4); m_pos += 4; }
            else   { memcpy(m_pos, "false",5); m_pos += 5; }
        }
        void do_print(long a) {
            reserve(20);
            itoa(a, out(m_pos));
        }
        void do_print(int a) {
            reserve(10);
            itoa(a, out(m_pos));
        }
        void do_print(double a)
        {
            int n = ftoa_left(a, m_pos, capacity(), 6, true);
            if (unlikely(n < 0)) {
                n = snprintf(m_pos, capacity(), "%.6f", a);
                if (unlikely(m_pos + n > m_end)) {
                    reserve(n);
                    snprintf(m_pos, capacity(), "%.6f", a);
                }
            }
            m_pos += n;
        }
        void do_print(fixed&& a) {
            reserve(a.digits());
            ftoa_right(a.value(), m_pos, a.digits(), a.precision(), a.fill());
            m_pos += a.digits();
        }
        void do_print(const char* a) {
            const char* p = strchr(a, '\0');
            assert(p);
            size_t n = p - a;
            reserve(n);
            memcpy(m_pos, a, n);
            m_pos += n;
        }
        void do_print(std::string&& a) {
            size_t n = a.size();
            reserve(n);
            memcpy(m_pos, a.c_str(), n);
            m_pos += n;
        }
        void do_print(const std::string& a) {
            size_t n = a.size();
            reserve(n);
            memcpy(m_pos, a.c_str(), n);
            m_pos += n;
        }
        template <typename T>
        void do_print(typename std::enable_if<std::is_pointer<T>::value, T&&>::type a) {
            auto  x = reinterpret_cast<uint64_t>(a);
            char* p = m_pos + 2;
            auto  n = capacity() - 2;
            int   i = itoa_hex(x, p, n);
            if (unlikely(i > n)) {
                reserve(i + 2);
                p = m_pos + 2;
                i = itoa_hex(x, p, n);
            }
            *m_pos++ = '0';
            *m_pos++ = 'x';
            m_pos   += i;
        }
        template <class T>
        void do_print(const T& a) {
            std::stringstream s; s << a;
            print(s.str());
        }
    public:
        explicit basic_buffered_print(const Alloc& a_alloc = Alloc())
            : Alloc(a_alloc)
            , m_begin(m_data)
            , m_pos(m_begin)
            , m_end(m_begin + sizeof(m_data)-1)
        {}

        ~basic_buffered_print() {
            deallocate();
        }

        void reset() {
            deallocate();
            m_begin = m_pos = m_data;
            m_end   = m_begin + sizeof(m_data)-1;
        }

        std::string to_string() const { return std::string(m_begin, size()); }
        const char* str()       const { return m_begin; }
        char*       str()             { return m_begin; }
        const char* c_str()     const { *m_pos = '\0'; return m_begin; }
        size_t      size()      const { return m_pos - m_begin;  }
        size_t      max_size()  const { return m_end - m_begin;  }
        size_t      capacity()  const { return m_end - m_pos;    }
        const char* pos()       const { return m_pos;   }
        char*       pos()             { return m_pos;   }
        char&       last()      const { return m_pos == m_begin
                                             ? *m_begin : *(m_pos - 1); }
        char&       last()            { return m_pos == m_begin
                                             ? *m_begin : *(m_pos - 1); }
        const char* end()       const { return m_end;   }

        /// Reserve space in the buffer to hold additional \a a_sz bytes
        void reserve(size_t a_sz) {
            if (m_pos + a_sz <= m_end) return;
            auto sz = max_size() + a_sz + N;
            char* p = Alloc::allocate(sz);
            strncpy(p, m_begin, size());
            m_pos   = p + size();
            m_end   = p + sz;
            if (m_begin != m_data)  delete [] m_begin;
            m_begin = p;
        }

        /// Advance the pointer at the end of the buffer to \a n bytes.
        /// Call this function if the content of this buffer needs to be
        /// written to by external snprintf:
        /// \code
        /// size_t c = buf.capacity();
        /// size_t n = snprintf(buf.str(), c, "%s", "Something");
        /// if (n > c) {
        ///     // write didn't fit - repeat
        ///     buf.reserve(n);
        ///     n = snprintf(buf.str(), c, "%s", "Something");
        /// }
        /// buf.advance(n);
        /// \endcode
        void advance(size_t n) { m_pos += n; }

        /// Remove the trailing character equal to \a ch.
        /// If the buffer doesn't end with \a ch, nothing is removed.
        void chop(char ch = '\n')
        { if (m_begin < m_pos && *(m_pos-1) == ch) --m_pos; }

        /// Remove the trailing character.
        /// If the buffer is empty, nothing is removed.
        void chop() { if (m_begin < m_pos) --m_pos; }

        // Terminal case of template recursion:
        void print() {}

        template <class T, class... Args>
        void print(T&& a, Args&&... args) {
            do_print(std::move(a));
            print(std::forward<Args>(args)...);
        }

        void sprint(const char* a_str, size_t a_size) {
            reserve(a_size);
            memcpy(m_pos, a_str, a_size);
            m_pos += a_size;
        }

        int vprintf(const char* a_fmt, va_list a_args) {
            int n = vsnprintf(m_pos, capacity(), a_fmt, a_args);
            if (n < 0)
                return n;
            if (size_t(n) > capacity()) {
                reserve(n);
                n = vsnprintf(m_pos, capacity(), a_fmt, a_args);
            }
            m_pos += n;
            return n;
        }

        int printf(const char* a_fmt, ...) {
            va_list args; va_start(args, a_fmt);
            UTXX_SCOPE_EXIT([&args]() { va_end(args); });
            return this->vprintf(a_fmt, args);
        }

        template <typename T>
        basic_buffered_print<N>& operator<< (T&& a) {
            print(std::forward<T>(a));
            return *this;
        }
    };
}

/// Analogous to sprintf, but returns an std::string instead
template <int SizeHint = 256>
std::string print_string(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    UTXX_SCOPE_EXIT([&args]() { va_end(args); });

    std::string buf;
    buf.resize(SizeHint);
    int n = vsnprintf(const_cast<char*>(buf.c_str()), buf.capacity(), fmt, args);

    if (unlikely(n < 0))
        throw io_error(errno, "print_string(): error formatting arguments");

    buf.resize(n);

    if (unlikely(n > SizeHint)) // String didn't fit - redo
        vsnprintf(const_cast<char*>(buf.c_str()), buf.capacity(), fmt, args);
    return buf;
}

/// Print arguments to string.
/// This function is a faster alternative to printf and std::stringstream.
template <class... Args>
std::string print(Args&&... args) {
    detail::basic_buffered_print<> b;
    b.print(std::forward<Args>(args)...);
    return b.to_string();
}

using buffered_print = detail::basic_buffered_print<>;

} // namespace utxx

#endif //_UTXX_PRINT_HPP_
