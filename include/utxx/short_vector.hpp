//----------------------------------------------------------------------------
/// \file   short_vector.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Generic string processing functions.
//----------------------------------------------------------------------------
// Copyright (c) 2016 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-07-02
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2016 Serge Aleynikov <saleyn@gmail.com>

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

#include <type_traits>
#include <memory>
#include <cstring>
#include <vector>
#include <cassert>

namespace utxx {

    //--------------------------------------------------------------------------
    /// Representation of a short vector.
    /// @tparam T        element type
    /// @tparam MaxItems staticly allocated buffer space to hold this many items
    /// @tparam Alloc    custom allocator
    /// @tparam AddItems extra spare items to allocate in calls to allocator
    ///
    /// Short vectors up to \a MaxItems don't involve memory allocations.
    /// Short vector can be set to have NULL value by using "set_null()" method.
    /// NULL vectors are represented by having size() = -1.
    //--------------------------------------------------------------------------
    template <typename T, int MaxItems,
              typename Alloc = std::allocator<T>, int AddItems = 0>
    class basic_short_vector : private Alloc  {
        using Base           = Alloc;

        static_assert(sizeof(T) == sizeof(typename Alloc::value_type), "Wrong size");
    public:
        using value_type     = T;
        using iterator       = T*;
        using const_iterator = const T*;

        static constexpr size_t MaxCapacity() { return MaxItems; }

        basic_short_vector(std::initializer_list<T> a_list, const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxItems)
        {
          resize  (a_list.size());
          auto q = a_list.begin();
          for (int i=0; i < int(a_list.size()); ++i)
              (T&)((*this)[i]) = std::move(*q++);
        }
        explicit
        basic_short_vector(const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxItems) {}
        explicit
        basic_short_vector(const T* a, size_t n, const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxItems) { set(a, n); }
        basic_short_vector(std::tuple<const T*, size_t>&& a, const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxItems) { set(std::move(a)); }
        explicit
        basic_short_vector(basic_short_vector&& a, const Alloc& ac = Alloc())
            : Base(ac)                                          { assign<false>(std::move(a));}
        explicit
        basic_short_vector(basic_short_vector const& a, const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxItems) { set(a); }
        template <int N>
        basic_short_vector(const T (&a)[N], const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxItems) { set(a, N); }

        ~basic_short_vector() { deallocate(); }

        void set(std::tuple<const T*, size_t>&& a) { set(std::get<0>(a), std::get<1>(a)); }
        void set(const std::vector<T>&          a) { set(&a[0],     a.size()); }
        void set(const basic_short_vector&      a) { set(a.begin(), a.size()); }
        void set(const T* a, int n) {
            assert(n >= -1);
            if (n > int(m_max_sz))  {
                deallocate();
                if (n > MaxItems)
                    std::tie(m_val, m_max_sz) = do_init_alloc(n);
            }
            do_copy(m_val, a, n);
            m_sz = n;
        }


        /// Set to empty string without deallocating memory
        void clear() { m_sz = 0; }

        /// Deallocate memory (if allocated) and set to empty string
        void reset() { deallocate(); clear(); }

        void push_back(typename std::conditional<sizeof(T) <= sizeof(long), T, const T&>::type a) {
            append(&a, 1);
        }

        void append(const T* a, size_t n) {
            assert(a);
            auto oldsz = is_null() ? 0 : m_sz;
            auto sz    = oldsz+n;
            if  (sz   <= capacity())
                do_copy(m_val + oldsz, a, n);
            else {
                T* p;   int max_sz;
                std::tie(p, max_sz) = do_init_alloc(sz);
                do_copy(p, m_val, oldsz);
                do_copy(p+oldsz,  a,  n);
                deallocate();
                m_max_sz = max_sz;
                m_val    = p;
            }
            m_sz = sz;
        }

        /// Allocate capacity but don't change current size.
        /// \see resize()
        void reserve(size_t a_capacity) {
            if (a_capacity <= capacity())
                return;

            T* p;   int max_sz;
            std::tie(p, max_sz) = do_init_alloc(a_capacity);
            do_copy(p, m_val, m_sz);
            deallocate();
            m_max_sz = max_sz;
            m_val    = p;
        }

        void operator= (const T*                       a) { set(a); }
        void operator= (std::tuple<const T*, size_t>&& a) { set(std::move(a)); }
        void operator= (basic_short_vector&&           a) { assign<true>(std::move(a)); }
        void operator= (const basic_short_vector&      a) { set(a); }
        void operator= (const std::vector<T>&          a) { set(a); }

        bool operator==(const std::vector<T>& a) const {
            if (m_sz != int(a.size()) || m_sz < 0) return false;
            return do_cmp(&a[0]);
        }

        bool operator==(const basic_short_vector& a) const {
            if (m_sz != a.m_sz) return false;
            if (m_sz  <      0) return true;
            return do_cmp(a.m_val);
        }

        bool operator!=(const basic_short_vector& a) const { return !operator==(a); }

        typename std::conditional<sizeof(T) <= sizeof(long), T, const T&>::
        type operator[](int n)    const { assert(n >= 0 && n < m_sz); return m_val[n]; }
        T&   operator[](int n)          { assert(n >= 0 && n < m_sz); return m_val[n]; }

        operator const T*()       const { return m_val;          }
        const T* data()           const { return m_val;          }
        T*       data()                 { return m_val;          }
        int      size()           const { return m_sz;           }
        void     size(size_t n)         { assert(n <= m_max_sz); m_sz = n; }
        void     resize(size_t n)       { reserve(n); m_sz = n;  }

        size_t   capacity()       const { return m_max_sz;       }
        bool     allocated()      const { return m_val != m_buf; }

        bool     is_null()        const { return m_sz < 0;       }
        void     set_null()             { m_sz=-1;               }

        iterator begin()                { return m_val;          }
        iterator end()                  { return is_null() ? m_val : m_val+m_sz; }

        const_iterator begin()    const { return m_val;          }
        const_iterator end()      const { return is_null() ? m_val : m_val+m_sz; }

        const_iterator cbegin()   const { return m_val;          }
        const_iterator cend()     const { return is_null() ? m_val : m_val+m_sz; }
    private:
        T*       m_val;
        int      m_sz;
        unsigned m_max_sz;
        T        m_buf[MaxItems];

        void deallocate() {
            m_sz = 0;
            if (allocated()) {
                Base::deallocate(m_val, m_max_sz);
                m_max_sz = MaxItems;
                m_val    = m_buf;
            }
            assert(m_max_sz == MaxItems);
        }

        template <bool Reset>
        void assign(basic_short_vector&& a) {
            if (Reset) deallocate();
            if (a.allocated()) { m_val = a.m_val; a.m_val = a.m_buf; }
            else               { m_val = m_buf; do_copy(m_val, a.m_buf, a.m_sz); }
            m_sz       = a.m_sz;
            m_max_sz   = a.m_max_sz;
            a.m_max_sz = MaxItems;
            a.m_sz     = 0;
        }

        std::pair<T*,int> do_init_alloc(int n) {
            auto max_sz = n + AddItems;
            auto p      = Base::allocate(max_sz);
            if (!std::is_pod<T>::value)
                for (auto q = p, e = p+max_sz; q != e; ++q)
                    new (q) T();
            return std::make_pair(p, max_sz);
        }

        static void do_copy(T* a_dst, const T* a, int n) {
            if (n > 0) {
                assert(a);
                if (std::is_pod<T>::value) {
                    memcpy((char*)a_dst, (char*)a, n*sizeof(T));
                    return;
                }
                auto p = a_dst;
                for (auto q = a, e = a+n; q != e; *p++ = *q++);
            }
        }

        bool do_cmp(const T* q) const {
            if (std::is_pod<T>::value)
                return memcmp(m_val, q, m_sz) == 0;
            else
                for (auto p = cbegin(), e = cend(); p != e; ++p, ++q)
                    if (*p != *q)
                        return false;
            return true;
        }

        static_assert(sizeof(typename Alloc::value_type) == sizeof(T), "Invalid size");
    };

} // namespace utxx
