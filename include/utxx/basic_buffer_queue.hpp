//----------------------------------------------------------------------------
/// \file    basic_buffer_queue.hpp
/// \authors Serge Aleynikov, Dmitriy Kargapolov
//----------------------------------------------------------------------------
/// \brief Basic queue for buffered output.
///
/// The queue provides an ability to commit batches of updates to boost::asio
/// socket at a time, while active writes can still continue to be buffered.
//----------------------------------------------------------------------------
// Created: 2010-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
Copyright (c) 2012 Serge Aleynikov <saleyn@gmail.com>

Use, modification and distribution are subject to the Boost Software
License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
at http://www.boost.org/LICENSE_1_0.txt)

***** END LICENSE BLOCK *****
*/
#pragma once

#include <deque>
#include <boost/system/error_code.hpp>
#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/asio/write.hpp>

namespace utxx {

template <typename BufferType, typename Alloc = std::allocator<char> >
class basic_buffer_queue {
    using buf_t = BufferType;
    using alloc = typename Alloc::template rebind<buf_t>::other;
    using deque = std::deque<buf_t, alloc>;

    deque  m_q1, m_q2;
    deque* m_out_queues[2];     ///< Queues of outgoing data
                                ///< The first queue is used for accumulating
                                ///< messages, while the second queue is used
                                ///< for writing them to socket.
    char  m_available_queue;    ///< Index of the queue used for cacheing
    bool  m_is_writing;         ///< Is asynchronous write in progress?

    template <class Socket, class Handler>
    class write_wrapper {
        basic_buffer_queue& m_bq;
        Socket& m_socket;
        Handler m_handler;
    public:
        write_wrapper(basic_buffer_queue& a_bq, Socket& a_socket,
                Handler a_hdlr) :
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
        m_is_writing = true;
        flip_queues(); // Work on the data accumulated in the available_queue.
        boost::asio::async_write(a_socket, *m_out_queues[writing_queue()],
            boost::asio::transfer_all(),
            write_wrapper<Socket, Handler>(*this, a_socket, a_h));
    }

    /// Swap available and writing queue indexes.
    void   flip_queues()           { m_available_queue = !m_available_queue; }
    /// Index of the queue used for writing to socket
    size_t writing_queue()   const { return !m_available_queue; }
    /// Index of the queue used for cacheing data to be writen to socket.
    size_t available_queue() const { return m_available_queue; }

    template <class Socket, class Handler>
    void handle_write(const boost::system::error_code& ec, Socket& a_socket,
            Handler a_h) {
        if (ec) {
            a_h(ec);
            return;
        }
        m_out_queues[writing_queue()]->clear();
        m_is_writing = false;
        do_write_internal(a_socket, a_h);
    }

public:
    explicit basic_buffer_queue(const Alloc& a_alloc = Alloc())
        : m_q1(a_alloc), m_q2(a_alloc)
        , m_available_queue(0)
        , m_is_writing(false)
    {
        m_out_queues[0] = &m_q1;
        m_out_queues[1] = &m_q2;
    }

    /// Enqueue data to the queue without initiating a socket write.
    void enqueue(const buf_t& a_buf) {
        m_out_queues[available_queue()]->push_back(a_buf);
    }

    /// Initiate asynchronous socket write.
    template <class Socket, class Handler>
    void async_write(Socket& a_socket, Handler a_h) {
        do_write_internal(a_socket, a_h);
    }

    /// Enqueue data and initiate asynchronous socket write.
    template <class Socket, class Handler>
    void async_write(Socket& a_socket, const buf_t& a_buf, Handler a_h) {
        m_out_queues[available_queue()]->push_back(a_buf);
        do_write_internal(a_socket, a_h);
    }
};

} // namespace utxx
