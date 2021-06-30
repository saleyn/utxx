//----------------------------------------------------------------------------
/// \file   buffer.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file contains implementation of a buffer class used for
/// storing data for I/O operations.
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

#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>
#include <algorithm>
#include <deque>
#include <string.h>
#include <limits.h>
#include <utxx/error.hpp>
#include <utxx/compiler_hints.hpp>

namespace utxx {

namespace detail { template <class Alloc> class basic_dynamic_io_buffer; }

/**
 * \brief Basic buffer providing stack space for data up to N bytes and
 *        heap space when reallocation of space over N bytes is needed.
 * A typical use of the buffer is for I/O operations that require tracking
 * of produced and consumed space.
 */
template <int N, typename Alloc = std::allocator<char>>
class basic_io_buffer
    : boost::noncopyable
    , std::allocator_traits<Alloc>::template rebind_alloc<char>
{
    using alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<char>;
protected:
    char*   m_begin;
    char*   m_end;
    char*   m_rd_ptr;
    char*   m_wr_ptr;
    size_t  m_wr_lwm;
    char    m_data[N];

    // Doesn't free the buffer pointed by m_begin!
    // Only use in move operations!
    void repoint(basic_io_buffer&& a_rhs) {
        if (m_begin != m_data && m_end > m_begin)
            alloc_t::deallocate(m_begin, max_size());

        if (a_rhs.allocated()) {
            // Repoint this object to a_rhs's allocated memory and crunch
            m_rd_ptr      = a_rhs.m_rd_ptr;
            m_wr_ptr      = a_rhs.m_wr_ptr;
            m_begin       = a_rhs.m_begin;
            m_end         = m_begin + a_rhs.max_size();
            assert(m_end == a_rhs.m_end);
            crunch();
        } else {
            auto new_sz   = a_rhs.size();
            if (N < new_sz) {
                m_begin   = alloc_t::allocate(new_sz);
                m_end     = m_begin + new_sz;
            } else {
                m_begin   = m_data;
                m_end     = m_begin+N;
            }
            memcpy(m_begin, a_rhs.m_rd_ptr, a_rhs.size());
            m_rd_ptr      = m_begin + (a_rhs.m_rd_ptr - a_rhs.m_begin);
            m_wr_ptr      = m_begin + (a_rhs.m_wr_ptr - a_rhs.m_begin);
        }
        m_wr_lwm          = a_rhs.m_wr_lwm;
        a_rhs.m_begin     = a_rhs.m_data;
        a_rhs.m_end       = a_rhs.m_begin + N;
        a_rhs.deallocate();
    }
public:
    /// Construct the I/O buffer.
    /// @param a_lwm is the low-memory watermark that should be set to
    ///         exceed the maximum expected message size to be stored
    ///         in the buffer. When the buffer has less than that amount
    ///         of space left, it'll call the crunch() function.
    explicit basic_io_buffer(size_t a_lwm = LONG_MAX, const Alloc& a_alloc = Alloc())
        : alloc_t(reinterpret_cast<const alloc_t&>(a_alloc))
        , m_begin(m_data),  m_end(m_data+N)
        , m_rd_ptr(m_data), m_wr_ptr(m_data), m_wr_lwm(a_lwm)
    {}

    basic_io_buffer(const basic_io_buffer& a_rhs)
        : alloc_t(static_cast<const alloc_t&>(a_rhs))
        , m_begin(m_data),  m_end(m_data+N)
        , m_rd_ptr(m_data), m_wr_ptr(m_data), m_wr_lwm(a_rhs.m_wr_lwm)
    {
        if (a_rhs.size()) {
            reserve(a_rhs.max_size());
            write(a_rhs.rd_ptr(), a_rhs.size());
        }
    }

    #if __cplusplus >= 201103L
    basic_io_buffer(basic_io_buffer&& a_rhs)
        : alloc_t (std::move(static_cast<alloc_t&&>(a_rhs)))
        , m_begin (m_data), m_end(m_data+N)
    {
        repoint(std::forward<basic_io_buffer&&>(a_rhs));
    }

    void operator=(basic_io_buffer&& a_rhs) {
        static_cast<alloc_t*>(this)->operator=(std::move(static_cast<alloc_t&&>(a_rhs)));
        repoint(std::forward<basic_io_buffer&&>(a_rhs));
    }
    #endif

    ~basic_io_buffer() { deallocate(); }

    /// Reset read/write pointers. Any unread content will be lost.
    void reset() { m_rd_ptr = m_wr_ptr = m_begin; }

    /// Deallocate any heap space occupied by the buffer.
    void deallocate() {
        if (m_begin != m_data) {
            if (m_end > m_begin)
                alloc_t::deallocate(m_begin, max_size());
            m_begin = m_data;
            m_end   = m_begin + N;
        }
        reset();
    }

    /// Ensure there's enough total space in the buffer to hold \a n bytes.
    void reserve(size_t n) {
        if (n <= capacity())
            return;
        size_t rd_offset = m_rd_ptr  - m_begin;
        size_t wr_offset = m_wr_ptr  - m_begin;
        auto   dirty_sz  = wr_offset - rd_offset;
        char*  old_begin = m_begin;
        auto   old_sz    = max_size();
        auto   new_sz    = dirty_sz + n;
        m_begin = alloc_t::allocate(new_sz);
        m_end   = m_begin + new_sz;
        if (dirty_sz > 0)
            memcpy(m_begin, old_begin+rd_offset, dirty_sz);
        m_rd_ptr = m_begin;
        m_wr_ptr = m_begin + dirty_sz;
        if (old_begin != m_data)
            alloc_t::deallocate(old_begin, old_sz);
    }

    /// Make sure there's enough space in the buffer for n additional bytes.
    [[deprecated]]
    void capacity(size_t n) { reserve(n); }

    /// Address of internal data buffer
    const char* address()   const { return m_begin; }
    /// Max number of bytes the buffer can hold.
    size_t      max_size()  const { return m_end    - m_begin;  }
    /// Current number of bytes available to read.
    size_t      size()      const { return m_wr_ptr - m_rd_ptr; }
    /// Current number of bytes available to write.
    size_t      capacity()  const { return m_end    - m_wr_ptr; }
    /// Returns true if there's no data in the buffer
    bool        empty()     const { return m_rd_ptr == m_wr_ptr; }
    /// Get the low memory watermark indicating when the buffer should be
    /// automatically crunched by the read() call.
    size_t      wr_lwm()    const { return m_wr_lwm; }

    /// Returns true when the wr_lwm is set and the write pointer
    /// passed the point of wr_lwm bytes from the end of the buffer.
    bool is_low_space()     const {
        return m_wr_ptr >= m_end - (m_wr_lwm == LONG_MAX ? 0 : m_wr_lwm);
    }

    /// Current read pointer.
    const char* rd_ptr()    const { return m_rd_ptr; }
    char*       rd_ptr()          { return m_rd_ptr; }

    /// Current write pointer.
    const char* wr_ptr()    const { return m_wr_ptr; }
    char*       wr_ptr()          { return m_wr_ptr; }
    /// Restore the value of the write pointer
    void        wr_ptr(char* ptr) {
      assert(ptr >= m_rd_ptr);
      m_wr_ptr = ptr;
    }


    /// End of buffer space.
    const char* end()       const { return m_end;    }

    /// Returns true if the buffer space was dynamically allocated.
    bool        allocated() const { return m_begin != m_data; }

    /// Cast this buffer to dynamic_io_buffer
    detail::basic_dynamic_io_buffer<Alloc>&       to_dynamic();
    const detail::basic_dynamic_io_buffer<Alloc>& to_dynamic() const;

    /// Set the low memory watermark indicating when the buffer should be
    /// automatically crunched by the read() call without releasing allocated memory.
    void wr_lwm(size_t a_lwm) {
        if (a_lwm > max_size())
            UTXX_THROW_RUNTIME_ERROR
                ("Low watermark ", a_lwm, " too large (max=", max_size(), ")!");
        m_wr_lwm = a_lwm;
    }

    /// Read \a n bytes from the buffer and increment the rd_ptr() by \a n.
    /// @returns NULL if there is not enough data in the buffer
    ///         to read \a n bytes or else returns the rd_ptr() pointer's
    ///         value preceeding the increment of its position by \a n.
    /// @param n is the number of bytes to read.
    char* read(int n) {
        if (UNLIKELY((size_t)n > size()))
            return NULL;
        char* p = m_rd_ptr;
        if (LIKELY(n)) {
            m_rd_ptr += n;
            BOOST_ASSERT(m_rd_ptr <= m_wr_ptr);
        }
        return p;
    }

    /// Discard \a n bytes from the buffer by incrementing the rd_ptr() by \a n.
    /// @param n is the number of bytes to read.
    void discard(unsigned n) {
        m_rd_ptr += n;
        BOOST_ASSERT(m_rd_ptr <= m_wr_ptr);
    }

    /// Do the same action as read(n), and call crunch() function
    /// when the buffer's capacity gets less than the wr_lwm() value.
    /// @param n is the number of bytes to read.
    /// @returns -1 if there is not enough data in the buffer
    ///         to read \a n bytes.
    int read_and_crunch(int n) {
        if (UNLIKELY(read(n) == NULL))
            return -1;
        if (capacity() < m_wr_lwm)
            crunch();
        BOOST_ASSERT(m_rd_ptr <= m_wr_ptr);
        return n;
    }

    /// Write \a n bytes to a buffer from a given source \a a_src.
    /// @return pointer to the next possible buffer write location.
    char* write(const char* a_src, size_t n) {
        reserve(n);
        char* p = m_wr_ptr;
        commit(n);
        memcpy(p, a_src, n);
        return m_wr_ptr;
    }

    /// Adjust buffer write pointer by \a n bytes.
    void commit(int n)   { BOOST_ASSERT(m_wr_ptr+n <= m_end); m_wr_ptr += n; }

    /// Move any unread data to the beginning of the buffer.
    /// This function is similar to reset(), however, no unread
    /// data gets lost.
    void crunch() {
        if (m_rd_ptr == m_begin)
            return;
        size_t sz = size();
        if (sz) {
            if (UNLIKELY(m_begin + sz > m_rd_ptr)) // overlap of memory block
                memmove(m_begin, m_rd_ptr, sz);
            else
                memcpy(m_begin, m_rd_ptr, sz);
        }
        m_rd_ptr = m_begin;
        m_wr_ptr = m_begin + sz;
    }

    /// Do the same action as discard(n), and call crunch() function
    /// when the buffer's capacity gets less than the wr_lwm() value.
    /// @param n is the number of bytes to discard.
    /// @returns -1 if there is not enough data in the buffer
    ///         to read \a n bytes.
    void discard_and_crunch(unsigned a_discard_bytes) {
        discard(a_discard_bytes);
        if (capacity() < m_wr_lwm) {
            crunch();
            BOOST_ASSERT(m_rd_ptr <= m_wr_ptr);
        }
    }
};

/**
 * This class allocates N records of size sizeof(T) on stack, and
 * dynamically allocates memory on heap when reserve() is called
 * requsting the number of records greater than N.
 * The content of the buffer can be read and written with read() and
 * write() functions.
 */
template <typename T, int N, typename Alloc = std::allocator<char> >
class record_buffers
    : protected basic_io_buffer<sizeof(T)*N, Alloc>
{
    typedef basic_io_buffer<sizeof(T)*N, Alloc> super;

public:
    explicit record_buffers(const Alloc& a_alloc = Alloc())
        : super(0, a_alloc)
    {}

    /// Ensure there's enough total space in the buffer to hold
    /// \a n records of size sizeof(T).
    void reserve(size_t n) { super::reserve(sizeof(T)*n); }

    T*          begin()           { return reinterpret_cast<T*>(super::m_begin); }
    const T*    begin()     const { return reinterpret_cast<const T*>(super::m_begin); }
    const T*    end()       const { return reinterpret_cast<T*>(super::m_end); }

    /// Max number of bytes the buffer can hold.
    size_t      max_size()  const { return end()    - begin();  }
    /// Current number of bytes available to read.
    size_t      size()      const { return wr_ptr() - rd_ptr(); }
    /// Current number of bytes available to write.
    size_t      capacity()  const { return end()    - wr_ptr(); }

    /// Returns true if the buffer space was dynamically allocated.
    bool        allocated() const { return super::allocated(); }

    T*          wr_ptr()        { return reinterpret_cast<T*>(this->m_wr_ptr);  }
    const T*    wr_ptr()  const { return reinterpret_cast<T*>(this->m_wr_ptr);  }
    void        wr_ptr(T* p)    { this->wr_ptr(reinterpret_cast<char*>(p));     }

    T*          rd_ptr()        { return reinterpret_cast<T*>(super::m_rd_ptr); }
    const T*    rd_ptr()  const { return reinterpret_cast<const T*>(super::m_rd_ptr); }

    /// Read the next record from the buffer
    T*          read() {
        if (size() < 1) return NULL;
        return reinterpret_cast<T*>(super::read(sizeof(T)));
    }
    const T* read() const {
        if (size() < 1) return NULL;
        return reinterpret_cast<const T*>(super::read(sizeof(T)));
    }

    /// Write a given record to the buffer.
    /// @return pointer to the next possible buffer write location.
    T* write(const T& a_src) {
        BOOST_ASSERT(capacity() > 0);
        *wr_ptr() = a_src;
        return write(1);
    }

    /// Advance the write pointer by \a n records. This doesn't
    /// actually modify the data in the buffer, but adjusts the
    /// position of wr_ptr().
    T* write(size_t n = 1) { super::commit(n*sizeof(T)); return wr_ptr(); }

    /// Reset the read and write pointers.
    /// Any unread records will be lost.
    void reset() { super::reset(); }

    /// A random-access iterator type that may be used to read elements.
    typedef const T* const_iterator;
    typedef T*       iterator;
};


namespace detail {

    /**
     * \brief Generic buffer providing heap space for I/O data.
     * A typical use of the buffer is for I/O operations that require tracking
     * of produced and consumed space when working with sockets.
     */
    template <typename Alloc = std::allocator<char> >
    class basic_dynamic_io_buffer: public basic_io_buffer<0, Alloc> {
    public:
        explicit basic_dynamic_io_buffer(
            size_t a_initial_size, const Alloc& a_alloc = Alloc())
            : basic_io_buffer<0, Alloc>(LONG_MAX, a_alloc)
        {
            this->reserve(a_initial_size);
        }
    };

} // namespace detail

/// Generic buffer providing heap space for I/O data.
using dynamic_io_buffer = detail::basic_dynamic_io_buffer<std::allocator<char>>;

/// A buffer providing space for input and output I/O operations.
template <int InBufSize, int OutBufSize = InBufSize>
struct io_buffer {
    basic_io_buffer<InBufSize>  in;
    basic_io_buffer<OutBufSize> out;
};

template <int N, typename Alloc>
inline detail::basic_dynamic_io_buffer<Alloc>& basic_io_buffer<N,Alloc>::to_dynamic() {
    return *reinterpret_cast<detail::basic_dynamic_io_buffer<Alloc>*>(this);
}

template <int N, typename Alloc>
inline const detail::basic_dynamic_io_buffer<Alloc>& basic_io_buffer<N,Alloc>::to_dynamic() const {
    return *reinterpret_cast<const detail::basic_dynamic_io_buffer<Alloc>*>(this);
}

} // namespace utxx
