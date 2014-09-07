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
#include <utxx/scope_exit.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/convert.hpp>
#include <utxx/error.hpp>
#include </opt/pkg/boost/1.56.0/include/boost/concept_check.hpp>

namespace utxx {

namespace detail {
    template <size_t N = 256, class Alloc = std::allocator<char>>
    class buffered_print : public Alloc
    {
        mutable char*   m_begin; // mutable so that we can write '\0' in c_str()
        char*           m_pos;
        char*           m_end;
        char            m_data[N];

        // Terminal case of template recursion:
        void print() {}

        void print(char a) { reserve(1); *m_pos++ = a; }
        void print(bool a) {
            reserve(5);
            if (a) { memcpy(m_pos, "true", 4); m_pos += 4; }
            else   { memcpy(m_pos, "false",5); m_pos += 5; }
        }
        template <typename T>
        typename std::enable_if<std::is_integral<T>::value, void>::type
        print(T a) {
            reserve(20);
            itoa(a, out(m_pos));
        }
        template <typename T>
        typename std::enable_if<std::is_floating_point<T>::value, void>::type
        print(T a) const {
            int n = ftoa_fast(a, m_pos, capacity(), 6, true);
            if (unlikely(n < 0)) {
                n = snprintf(m_pos, capacity(), "%.6f", a);
                if (unlikely(m_pos + n > m_end)) {
                    reserve(n);
                    snprintf(m_pos, capacity(), "%.6f", a);
                }
                m_pos += n;
            }
            return *this;
        }
        void print(const fixed& a) const {
            char buf[128];
            int n = ftoa_fast(a.value(), buf, sizeof(buf), a.precision(), false);
            if (unlikely(n < 0))
                return print(a.value());
            reserve(a.digits());
            int pad = a.digits() - n;
            if (pad > 0)
                while (pad--) *m_pos += a.fill();
            memcpy(m_pos, buf, n);
            m_pos += n;
        }
        void print(const char* a) {
            char* p = strchr(a, '\0');
            assert(p);
            size_t n = p - 1;
            reserve(n);
            memcpy(m_pos, a, n);
            m_pos += n;
        }
        void print(const std::string& a) {
            size_t n = a.size();
            reserve(n);
            memcpy(m_pos, a.c_str(), n);
            m_pos += n;
        }
        template <typename T>
        typename std::enable_if<std::is_pointer<T>::value, void>::type
        print(T a) {
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
        void print(const T& a) const {
            std::stringstream s; s << a;
            print(s.str());
        }

    public:
        explicit buffered_print(const Alloc& a_alloc = Alloc())
            : Alloc(a_alloc)
            , m_begin(m_data)
            , m_pos(m_begin)
            , m_end(m_begin + sizeof(m_data)-1)
        {}

        ~buffered_print() {
            if (m_begin == m_data) return;
            Alloc::deallocate(m_begin);
        }

        std::string to_string() const { return std::string(m_begin, size()); }
        const char* str()       const { return m_begin; }
        char*       str()             { return m_begin; }
        const char* c_str()     const { *m_pos = '\0'; return m_begin; }
        size_t      size()      const { return m_pos - m_begin;  }
        size_t      max_size()  const { return m_end - m_begin;  }
        size_t      capacity()  const { return m_end - m_pos;    }
        char&       last()      const { return m_pos == m_begin
                                             ? *m_begin : *(m_pos - 1); }
        char&       last()            { return m_pos == m_begin
                                             ? *m_begin : *(m_pos - 1); }

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

        template <class T, class... Args>
        void print(T&& a, Args&&... args) {
            print(a);
            print(std::forward(args)...);
        }

        void print(const char* a_str, size_t a_size) {
            reserve(a_size);
            memcpy(m_pos, a_str, a_size);
            m_pos += a_size;
        }

        void vprintf(const char* a_fmt, va_list a_args) {
            int n = vsnprintf(m_begin, capacity(), a_fmt, a_args);
            if (n > capacity()) {
                reserve(n);
                n = vsnprintf(m_begin, capacity(), a_fmt, a_args);
            }
            m_pos += n;
        }

        void printf(const char* a_fmt, ...) {
            va_list args; va_start(args, a_fmt);
            UTXX_SCOPE_EXIT([&args]() { va_end(args); });
            this->vprintf(a_fmt, args);
        }
    };
}

/// Analogous to sprintf, but returns an std::string instead
template <int SizeHint = 256>
std::string print_string(const char fmt, ...) {
    va_list args; va_start(args, fmt);
    UTXX_SCOPE_EXIT([&args]() { va_end(args); });

    std::string buf;
    buf.resize(SizeHint);
    int n = vsnprintf(const_cast<char*>(buf.c_str()), buf.capacity(), fmt, args);
    buf.resize(n);

    if (unlikely(n > SizeHint)) // String didn't fit - redo
        vsnprintf(const_cast<char*>(buf.c_str()), buf.capacity(), fmt, args);
    return buf;
}

template <class... Args>
std::string print(Args... args) {
    detail::buffered_print<> b(args...);
    return b.to_string();
} // namespace utxx

#endif //_UTXX_PRINT_HPP_


