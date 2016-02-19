//----------------------------------------------------------------------------
/// \file  bitmap.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Bitmap index suitable for indexing up to 64 or 4096 values on a
/// 64bit platform with fast iteration between adjacent items.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-12-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _UTXX_BITMAP_HPP_
#define _UTXX_BITMAP_HPP_

#include <boost/assert.hpp>
#include <utxx/meta.hpp>
#include <utxx/atomic.hpp>
#include <utxx/detail/bit_count.hpp>
#include <sstream>
#include <stdio.h>

namespace utxx {

template <int N, typename T = unsigned long>
class bitmap_low {
    T m_data;

    void valid(unsigned int i) const { BOOST_ASSERT(i <= max); }
public:
    typedef T value_type;

    bitmap_low() : m_data(0) {
        BOOST_STATIC_ASSERT(1 <= N && N <= sizeof(long)*8);
    }

    explicit bitmap_low(T a_mask) : m_data(a_mask) {}

    static const unsigned int max  = N - 1;
    static const unsigned int cend = N;

    T               value()  const { return m_data; }
    unsigned int    end()    const { return cend; }
    bool            empty()  const { return m_data == 0; }
    void            clear()        { m_data = 0; }

    void fill()                    { m_data = uint64_t(-1); }
    void set(unsigned int i)       { valid(i); m_data |= 1ul << i; }
    void clear(unsigned int i)     { valid(i); m_data ^= m_data & (1ul << i); }
    bool is_set(unsigned int i) const { valid(i); return m_data & (1ul << i); }
    int  first() const { return m_data ? atomic::bit_scan_forward(m_data) : end(); }
    int  last()  const { return m_data ? atomic::bit_scan_reverse(m_data) : end(); }
    int  count() const { return bitcount(m_data); }

    bool operator[] (unsigned int i) const { return is_set(i); }
    void operator=  (const bitmap_low<N>& rhs) { m_data = rhs.value(); }

    /// @param <i> is the bit to search from in the forward direction.
    ///            Valid range [0 ... max-1].
    /// @return position of next enabled bit or <end()> if not found.
    int next(unsigned int i) const { 
        T val = m_data >> ++i;
        return val && i <= max ? atomic::bit_scan_forward(val)+i : end(); 
    }

    /// @param <i> is the bit to search from in the reverse direction.
    ///            Valid range [1 ... max].
    /// @return position of previous enabled bit or <end()> if not found.
    int prev(unsigned int i) const { 
        BOOST_ASSERT(i <= max);
        T val = m_data & ((1ul << i)-1);
        return val && i >= 0 ? atomic::bit_scan_reverse(val) : end();
    }

    std::ostream& print(std::ostream& out) const {
        std::stringstream s;
        for(int i=max+1; i > 0; --i) {
            if (i != max+1 && i%8 == 0) s << '-';
            s << (is_set(i-1) ? '1' : '0');
        }
        return out << s.str();
    }
};

template <int N, typename T = unsigned long>
class bitmap_high: protected bitmap_low<sizeof(long)*8, T> {
    typedef bitmap_low<sizeof(long)*8, T> base;
public:
    // the following static members are made public just for testing
    static const int s_lo_dim  = base::max + 1;
    static const int s_hi_sft  = log<s_lo_dim, 2>::value;
    static const int s_lo_mask = base::max;
    static const int s_hi_dim  = N / (sizeof(long)*8) +
                               ((N & (N - 1)) == 0 ? 0 : 1);
private:
    base m_data[s_hi_dim];

    void valid(unsigned int i) const { BOOST_ASSERT(i <= max); }
public:
    bitmap_high() {
        BOOST_STATIC_ASSERT(sizeof(long)*8 < N && N <= sizeof(long)*sizeof(long)*64);
    }

    static const unsigned int   max = N - 1;
    unsigned int                end()   const { return max+1; }
    bool                        empty() const { return base::value() == 0; }

    void fill() {
        for (int i=0; i < s_hi_dim; ++i) m_data[i].fill();
        base::fill();
    }

    void clear() {
        for (int i=0; i < s_hi_dim; ++i) m_data[i].clear();
        base::clear();
    }

    void set(unsigned int i) {
        valid(i); 
        unsigned int hi = i>>s_hi_sft, lo = i&s_lo_mask;
        m_data[hi].set(lo);
        base::set(m_data[hi].value() ? hi : 0);
    }

    void clear(unsigned int i) {
        valid(i); 
        unsigned int hi = i>>s_hi_sft, lo = i&s_lo_mask;
        m_data[hi].clear(lo);
        if (!m_data[hi].value())
            base::clear(hi);
    }

    bool operator[] (unsigned int i) const { return is_set(i); }
    void operator=  (const bitmap_high<N>& rhs) {
       for (int i=0; i < s_hi_dim; ++i) m_data[i] = rhs.value();
       this->base::operator= (rhs.base::value());
    }
    
    bool is_set(unsigned int i) const {
        valid(i);
        unsigned int hi = i>>s_hi_sft, lo = i&s_lo_mask;
        return base::is_set(hi) && m_data[hi].is_set(lo);
    }
    int first()  const {
        int hi=base::first();
        return base::value() ? (hi<<s_hi_sft | m_data[hi].first()):end();
    }
    int last()   const {
        int hi=base::last();
        return base::value() ? (hi<<s_hi_sft | m_data[hi].last() ):end();
    }
    int count() const {
        int sum = 0;
        for (unsigned int i = base::first(); i != base::end(); i = base::next(i)) {
            sum += bitcount(m_data[i].value());
        }
        return sum;
    }
    int next(unsigned int i) const { 
        unsigned int hi = i>>s_hi_sft, lo = i&s_lo_mask;
        if (lo < base::max) {
            lo = m_data[hi].next(lo);
            if (lo != base::end()) return (hi << s_hi_sft | lo);
        }
        if (hi == base::max) return end();
        hi = base::next(hi);
        return hi == base::end() ? end() : (hi << s_hi_sft | m_data[hi].first());
    }
    int prev(unsigned int i) const { 
        unsigned int hi = i>>s_hi_sft, lo = i&s_lo_mask;
        if (lo > 0) {
            lo = m_data[hi].prev(lo);
            if (lo != base::end()) return (hi << s_hi_sft | lo);
        }
        if (hi == 0) return end();
        hi = base::prev(hi);
        return m_data[hi].value() ? (hi << s_hi_sft | m_data[hi].last()) : end();
    }

    std::ostream& print(std::ostream& out, const char* sep = "\n") const {
        std::stringstream s;
        for(int i=s_hi_dim-1; i >= 0; --i) {
            char buf[9];
            if (i != s_lo_dim-1 && (i+1)%8 != 0 && i != s_hi_dim-1) s << '-';
            if ((i+1)%8 == 0 || (s_hi_dim < 8 && i == (s_hi_dim-1))) {
                sprintf(buf, "%s%02d: ", sep, i+1);
                s << buf;
            }
            sprintf(buf, "%016lx", m_data[i].value());
            s << buf;
        }
        return out << s.str();
    }
};

typedef bitmap_low<16>    bitmap16;
typedef bitmap_low<32>    bitmap32;
typedef
    boost::mpl::if_c<sizeof(long)==8,
                     bitmap_low<48>,
                     bitmap_high<48> >::type bitmap48;
typedef
    boost::mpl::if_c<sizeof(long)==8,
                     bitmap_low<64>,
                     bitmap_high<64> >::type bitmap64;
typedef bitmap_high<128>  bitmap128;
typedef bitmap_high<256>  bitmap256;
typedef bitmap_high<512>  bitmap512;
typedef bitmap_high<1024> bitmap1024;
typedef bitmap_high<4096> bitmap4096;

} // namespace utxx

#endif // _HPCL_BITMAP_HPP_

