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
#ifndef _UTXX_IO_BUFFER_HPP_
#define _UTXX_IO_BUFFER_HPP_

#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/detail/consuming_buffers.hpp>
#include <boost/type_traits/has_trivial_constructor.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <algorithm>
#include <deque>
#include <string.h>
#include <utxx/error.hpp>
#include <utxx/compiler_hints.hpp>

namespace utxx {

/**
 * \brief Basic buffer providing stack space for data up to N bytes and
 *        heap space when reallocation of space over N bytes is needed.
 * A typical use of the buffer is for I/O operations that require tracking
 * of produced and consumed space. 
 */
template <int N, typename Alloc = std::allocator<char> >
class basic_io_buffer: boost::noncopyable {
    typedef typename Alloc::template rebind<char>::other alloc_t;
protected:
    char    m_data[N];
    char*   m_begin;
    char*   m_end;
    char*   m_rd_ptr;
    char*   m_wr_ptr;
    size_t  m_wr_lwm;
    alloc_t m_allocator;

 public:
    /// Construct the I/O buffer.
    /// @param a_lwm is the low-memory watermark that should be set to 
    ///         exceed the maximum expected message size to be stored
    ///         in the buffer. When the buffer has less than that amount
    ///         of space left, it'll call the crunch() function.
    explicit basic_io_buffer(size_t a_lwm = 0, const Alloc& a_alloc = Alloc())
        : m_begin(m_data), m_end(m_data+N)
        , m_rd_ptr(m_data), m_wr_ptr(m_data), m_wr_lwm(0)
        , m_allocator(reinterpret_cast<const alloc_t&>(a_alloc))
    {}

    ~basic_io_buffer() { deallocate(); }

    /// Reset read/write pointers. Any unread content will be lost.
    void reset() { m_rd_ptr = m_wr_ptr = m_begin; }

    /// Deallocate any heap space occupied by the buffer.
    void deallocate() {
        if (m_begin != m_data && N) {
            m_allocator.deallocate(m_begin, max_size());
            m_begin = m_data;
            m_end   = m_begin + N;
        }
        reset();
    }

    /// Ensure there's enough total space in the buffer to hold \a n bytes.
    void reserve(size_t n) {
        size_t old_size = max_size();
        if (n <= old_size)
            return;
        size_t rd_offset = m_rd_ptr - m_begin;
        size_t wr_offset = m_wr_ptr - m_begin;
        char*  old_begin = m_begin;
        m_begin = m_allocator.allocate(n);
        m_end   = m_begin + n;
        if (wr_offset > 0)
            memcpy(m_begin, old_begin, wr_offset);
        m_rd_ptr = m_begin + rd_offset;
        m_wr_ptr = m_begin + wr_offset;
        if (old_begin != m_data)
            m_allocator.deallocate(old_begin, old_size);
    }

    /// Make sure there's enough space in the buffer for n additional bytes.
    void capacity(size_t n) {
        if (capacity() >= n)
            return;
        crunch();
        size_t got = capacity();
        if (got >= n)
            return;
        reserve(max_size() + n - got);
    }

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
    bool is_low_space()     const { return m_wr_ptr >= m_end - m_wr_lwm; }

    /// Current read pointer.
    const char* rd_ptr()    const { return m_rd_ptr; }
    char*       rd_ptr()          { return m_rd_ptr; }

    /// Current write pointer.
    const char* wr_ptr()    const { return m_wr_ptr; }
    char*       wr_ptr()          { return m_wr_ptr; }

    /// End of buffer space.
    const char* end()       const { return m_end;    }

    /// Returns true if the buffer space was dynamically allocated.
    bool        allocated() const { return m_begin != m_data; }

    /// Convert this buffer to boost::asio::buffer for writing.
    boost::asio::const_buffers_1 space() const {
        return boost::asio::buffer(m_wr_ptr, capacity());
    }
    boost::asio::mutable_buffers_1 space() {
        return boost::asio::buffer(m_wr_ptr, capacity());
    }

    /// Set the low memory watermark indicating when the buffer should be 
    /// automatically crunched by the read() call without releasing allocated memory.
    void wr_lwm(size_t a_lwm)     { m_wr_lwm = a_lwm; }

    /// Read \a n bytes from the buffer and increment the rd_ptr() by \a n.
    /// @returns NULL if there is not enough data in the buffer 
    ///         to read \a n bytes or else returns the rd_ptr() pointer's
    ///         value preceeding the increment of its position by \a n.
    /// @param n is the number of bytes to read.
    char* read(int n) {
        if (unlikely((size_t)n > size()))
            return NULL;
        char* p = m_rd_ptr;
        if (likely(n)) {
            m_rd_ptr += n;
            BOOST_ASSERT(m_rd_ptr <= m_wr_ptr);
        }
        return p;
    }

    /// Do the same action as read(n), and call crunch() function
    /// when the buffer's capacity gets less than the wr_lwm() value.
    /// @param n is the number of bytes to read.
    /// @returns -1 if there is not enough data in the buffer 
    ///         to read \a n bytes.
    int read_and_crunch(int n) {
        if (unlikely(read(n) == NULL))
            return -1;
        if (capacity() < m_wr_lwm)
            crunch();
        BOOST_ASSERT(m_rd_ptr <= m_wr_ptr);
        return n;
    }

    /// Write \a n bytes to a buffer from a given source \a a_src.
    /// @return pointer to the next possible buffer write location.
    char* write(const char* a_src, size_t n) {
        capacity(n);
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
            if (unlikely(m_begin + sz > m_rd_ptr)) // overlap of memory block
                memmove(m_begin, m_rd_ptr, sz);
            else
                memcpy(m_begin, m_rd_ptr, sz);
        }
        m_rd_ptr = m_begin;
        m_wr_ptr = m_begin + sz;
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

    T*          wr_ptr()        { return reinterpret_cast<T*>(super::m_wr_ptr); }
    const T*    wr_ptr()  const { return reinterpret_cast<T*>(super::m_wr_ptr); }

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
            : basic_io_buffer<0, Alloc>(0, a_alloc)
        {
            this->reserve(a_initial_size);
        }
    };

} // namespace detail

/**
 * \brief Generic buffer providing heap space for I/O data.
 */
typedef detail::basic_dynamic_io_buffer<std::allocator<char> > dynamic_io_buffer;

/**
 * \brief A buffer providing space for input and output I/O operations.
 */
template <int InBufSize, int OutBufSize = InBufSize>
struct io_buffer {
    basic_io_buffer<InBufSize>  in;
    basic_io_buffer<OutBufSize> out;
};

namespace detail {
    /**
     * \brief used by buffered_queue
     */
    template <bool False, typename Alloc>
    struct basic_buffered_queue {
        typedef boost::asio::const_buffer buf_t;
        typedef std::deque<buf_t, Alloc> deque;
        basic_buffered_queue(const Alloc&) {}
        void deallocate(deque *) {}
    };

    template <typename Alloc>
    struct basic_buffered_queue<true, Alloc> {
        typedef boost::asio::const_buffer buf_t;
        typedef std::deque<buf_t, Alloc> deque;
        typedef typename Alloc::template rebind<char>::other alloc_t;
        alloc_t m_allocator;

        basic_buffered_queue(const Alloc& a_alloc) : m_allocator(a_alloc) {}

        /// Allocate space to be later enqueued to this buffer.
        template <class T>
        T* allocate(size_t a_size) {
            return static_cast<T*>(m_allocator.allocate(a_size));
        }

        /// Deallocate space used by buffer.
        void deallocate(deque *p) {
            BOOST_FOREACH(buf_t& b, *p) {
                m_allocator.deallocate(
                    const_cast<char *>(boost::asio::buffer_cast<const char *>(b)),
                    boost::asio::buffer_size(b));
            }
        }
    };
}

/**
 * \brief A stream that can be used for asyncronous output with boost::asio.
 * There are a couple of optimizations implemented to reduce
 * the number of system calls. Two queues are being used for writing.
 * One queue (identified by <tt>available_queue()</tt>)
 * is accumulating outgoing messages from the caller.  The second queue
 * (identified by <tt>writing_queue()</tt>) contains a list of outgoing
 * messages that are being written to the socket using scattered I/O.
 */

template <bool IsOwner = true, typename Alloc = std::allocator<char> >
class buffered_queue : public detail::basic_buffered_queue<IsOwner, Alloc> {
    typedef detail::basic_buffered_queue<IsOwner, Alloc> base;
    typedef typename base::deque deque;
    deque  m_q1, m_q2;
    deque* m_out_queues[2];     ///< Queues of outgoing data
                                ///< The first queue is used for accumulating
                                ///< messages, while the second queue is used
                                ///< for writing them to socket.
    char  m_available_queue;    ///< Index of the queue used for cacheing
    bool  m_is_writing;         ///< Is asynchronous write in progress?

    template <class Socket, class Handler>
    class write_wrapper {
        buffered_queue& m_bq;
        Socket& m_socket;
        Handler m_handler;
    public:
        write_wrapper(buffered_queue& a_bq, Socket& a_socket, Handler a_hdlr) :
            m_bq(a_bq), m_socket(a_socket), m_handler(a_hdlr) {
        }
        void operator()(const boost::system::error_code& ec,
            std::size_t bytes_transferred) {
            m_bq.handle_write(ec, m_socket, m_handler);
        }
    };

    template <class Socket, class Handler>
    void do_write_internal(Socket& a_socket, Handler a_h) {
        if (m_is_writing || m_out_queues[available_queue()]->empty()) {
            // Nothing more to write to a socket - going idle
            a_h(boost::system::error_code());
            return;
        }

        typedef boost::asio::detail::consuming_buffers<
            boost::asio::const_buffer, deque> buffers_t;
        buffers_t buffers(*m_out_queues[available_queue()]);
        m_is_writing = true;
        flip_queues(); // Work on the data accumulated in the available_queue.
        boost::asio::async_write(a_socket, buffers, boost::asio::transfer_all(),
            write_wrapper<Socket, Handler>(*this, a_socket, a_h));
    }
public:
    explicit buffered_queue(const Alloc& a_alloc = Alloc())
        : base(a_alloc)
        , m_q1(a_alloc), m_q2(a_alloc)
        , m_available_queue(0)
        , m_is_writing(false)
    {
        m_out_queues[0] = &m_q1;
        m_out_queues[1] = &m_q2;
    }

    /// Swap available and writing queue indexes.
    void   flip_queues()              { m_available_queue = !m_available_queue; }
    /// Index of the queue used for writing to socket
    size_t writing_queue()      const { return !m_available_queue; }
    /// Index of the queue used for cacheing data to be writen to socket.
    size_t available_queue()    const { return m_available_queue; }

    /// Enqueue data to the queue without initiating a socket write.
    void enqueue(const boost::asio::const_buffer& a_buf) {
        m_out_queues[available_queue()]->push_back(a_buf);
    }

    /// Initiate asynchronous socket write.
    template <class Socket, class Handler>
    void async_write(Socket& a_socket, Handler a_h) {
        do_write_internal(a_socket, a_h);
    }

    /// Enqueue data and initiate asynchronous socket write.
    template <class Socket, class Handler>
    void async_write(Socket& a_socket, const boost::asio::const_buffer& a_buf, Handler a_h) {
        m_out_queues[available_queue()]->push_back(a_buf);
        do_write_internal(a_socket, a_h);
    }

    template <class Socket, class Handler>
    void handle_write(const boost::system::error_code& ec, Socket& a_socket, Handler a_h) {
        if (ec) {
            a_h(ec);
            return;
        }
        deallocate(m_out_queues[writing_queue()]);
        m_out_queues[writing_queue()]->clear();
        m_is_writing = false;
        do_write_internal(a_socket, a_h);
    }

};

} // namespace IO_UTXX_NAMESPACE

#endif // _UTXX_IO_BUFFER_HPP_

