//----------------------------------------------------------------------------
/// \file  basic_buffer_queue.hpp
//----------------------------------------------------------------------------
/// \brief I/O vector handling primitives.
/// \authors Serge Aleynikov, Dmitriy Kargapolov
//----------------------------------------------------------------------------
// Created: 2011-09-24
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2012 Serge Aleynikov <saleyn@gmail.com>

Use, modification and distribution are subject to the Boost Software
License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
at http://www.boost.org/LICENSE_1_0.txt)

***** END LICENSE BLOCK *****
*/

#ifndef _UTXX_IO_IOVECTOR_HPP_
#define _UTXX_IO_IOVECTOR_HPP_

#include <sys/uio.h>
#include <string.h>
#include <vector>
#include <boost/assert.hpp>
#include <utxx/compiler_hints.hpp>

namespace utxx {

    inline static iovec make_iovec(const char* a_bytes, size_t a_sz) {
        iovec v = { (void*) a_bytes, a_sz };
        return v;
    }

    inline static iovec make_iovec(const char* a_bytes) {
        BOOST_ASSERT(a_bytes != NULL);
        iovec v = { (void*) a_bytes, strlen(a_bytes) };
        return v;
    }

} // namespace utxx

namespace utxx {
namespace io {

class iovector: std::vector<iovec> {

    typedef std::vector<iovec> base;
    typedef base::iterator iterator;
    typedef base::const_iterator const_iterator;

    size_t m_offset;
    size_t m_length;

public:

    iovector() :
        m_offset(0), m_length(0) {
    }

    iovector(size_t n) :
        base(n), m_offset(0), m_length(0) {
    }

    iovector(const iovector & a_rhs) :
        base(a_rhs.size()), m_offset(0), m_length(a_rhs.length()) {
        iterator l_it = begin();
        for (const_iterator it = a_rhs.begin(), e = a_rhs.end(); it
                != e; ++it)
            *l_it++ = *it;
    }

    template<int M>
    iovector(const iovec(& a_data)[M]) :
        base(M), m_offset(0), m_length(0) {
        for (int i = 0; i < M; m_length += a_data[i++].iov_len)
            base::operator [](i) = a_data[i];
    }

    iovector(const iovec *a_data, size_t a_size) :
        base(a_size), m_offset(0), m_length(0) {
        for (size_t i = 0; i < a_size; m_length += a_data[i++].iov_len)
            base::operator [](i) = a_data[i];
    }

    const iovec & operator[](size_t i) const {
        BOOST_ASSERT(i < size());
        return *(base::begin() + m_offset + i);
    }

    void push_back(const char *a_bytes, size_t a_size) {
        base::push_back(make_iovec(a_bytes, a_size));
        m_length += a_size;
    }

    void push_back(const iovec & a_vec) {
        base::push_back(a_vec);
        m_length += a_vec.iov_len;
    }

    /**
     * number of items in the vector
     */
    inline size_t size() const { return base::size() - m_offset; }

    /**
     * total data length
     */
    inline size_t length() const { return m_length; }

    inline bool empty() const { return !size(); }

    void clear() {
        base::clear();
        m_offset = 0;
        m_length = 0;
    }

    inline const iovec * to_iovec() const {
        return &*begin();
    }

    inline const iovec & operator ->() const {
        return base::operator [](m_offset);
    }

    inline const iovec * operator &() const {
        return &base::operator [](m_offset);
    }

    inline iterator begin() {
        return base::begin() + m_offset;
    }

    inline iterator end() { return base::end(); }

    inline const_iterator begin() const {
        return base::begin() + m_offset;
    }

    inline const_iterator end() const { return base::end(); }

    int copy_to(char *a_buf, size_t a_size) const {
        if (a_size < m_length)
            return -1;
#ifndef NDEBUG
        const char *l_begin = a_buf;
#endif
        for (const_iterator it = begin(), e = end(); it != e; a_buf
                += it->iov_len, ++it)
            memcpy(a_buf, it->iov_base, it->iov_len);
        BOOST_ASSERT((size_t)(a_buf - l_begin) == m_length);
        return m_length;
    }

    void add(const iovec *a_data, size_t a_len) {
        BOOST_ASSERT(a_len > 0 && a_data != NULL);
        base::resize(base::size() + a_len);
        iterator it = end() - a_len;
        for (const iovec *p = a_data, *e = a_data + a_len; p != e;
                m_length += p->iov_len, ++p, ++it)
            *it = *p;
    }

    void erase(size_t a_size) {
        if (unlikely(a_size > m_length))
            a_size = m_length;
        m_length -= a_size;
        iterator it;
        for (it = begin(); it < end() && a_size >= it->iov_len;
                a_size -= it->iov_len, ++it)
            m_offset++;
        if (a_size) {
            BOOST_ASSERT(it < end());
            it->iov_len -= a_size;
            it->iov_base = static_cast<char*> (it->iov_base) + a_size;
        }
    }
};

} // namespace io
} // namespace utxx

#endif // _UTXX_IO_IOVECTOR_HPP_
