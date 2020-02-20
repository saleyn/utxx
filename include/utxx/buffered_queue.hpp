//----------------------------------------------------------------------------
/// \file   buffered_queue.hpp
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

#include <utxx/buffer.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/detail/consuming_buffers.hpp>

namespace utxx {

template <typename Buffer>
inline boost::asio::const_buffers_1 buffer_space(const Buffer& a) {
    return boost::asio::buffer(a.wr_ptr(), a.capacity());
}

template <typename Buffer>
inline boost::asio::mutable_buffers_1 buffer_space(Buffer& a) {
    return boost::asio::buffer(a.wr_ptr(), a.capacity());
}

namespace detail {
    /**
     * \brief used by buffered_queue
     */
    template <bool False, typename Alloc>
    struct basic_buffered_queue {
        using buf_t     = boost::asio::const_buffer;
        using buf_alloc = typename Alloc::template rebind<buf_t>::other;
        using deque     = std::deque<buf_t, buf_alloc>;
        basic_buffered_queue(const Alloc&) {}
        void deallocate(deque *) {}
    };

    template <typename Alloc>
    struct basic_buffered_queue<true, Alloc> {
        using buf_t     = boost::asio::const_buffer;
        using buf_alloc = typename Alloc::template rebind<buf_t>::other;
        using deque     = std::deque<buf_t, buf_alloc>;
        using alloc_t   = typename Alloc::template rebind<char>::other;
        alloc_t m_allocator;

        basic_buffered_queue(const Alloc& a_alloc) : m_allocator(a_alloc) {}

        /// Allocate space to be later enqueued to this buffer.
        template <class T>
        T* allocate(size_t a_size) {
            return static_cast<T*>(m_allocator.allocate(a_size));
        }

        /// Deallocate space used by buffer.
        void deallocate(deque* p) {
            for (auto& b : *p) {
                m_allocator.deallocate(
                    const_cast<char *>(boost::asio::buffer_cast<const char *>(&b)),
                    boost::asio::buffer_size(b));
            }
        }
    };
}

#if (BOOST_VERSION < 107000)
// TODO: boost 1.70 changed consuming_buffers to take 3 arguments: fix the code above

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

#endif // Boost version < 1.70.0

} // namespace utxx
