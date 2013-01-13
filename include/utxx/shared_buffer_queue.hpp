//----------------------------------------------------------------------------
/// \file   shared_buffer_queue.hpp
/// \author Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
//----------------------------------------------------------------------------
/// \brief Shared Buffer Queue
//----------------------------------------------------------------------------
// Copyright (c) 2012 Omnibius, LLC
// Created: 2012-06-12
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2012 Omnibius, LLC
Author: Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>

Use, modification and distribution are subject to the Boost Software
License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
at http://www.boost.org/LICENSE_1_0.txt)

***** END LICENSE BLOCK *****
*/

#ifndef _UTXX_SHARED_BUFFER_QUEUE_HPP_
#define _UTXX_SHARED_BUFFER_QUEUE_HPP_

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <utxx/basic_buffer_queue.hpp>

namespace utxx {

class shared_const_buffer : public boost::asio::const_buffer {

    typedef boost::function<void()> del_t;

    struct shadow {
         del_t m_del;
         shadow(del_t a_del) : m_del(a_del) {}
         ~shadow() { m_del(); }
    };

    boost::shared_ptr<shadow> m_ptr;

public:
    shared_const_buffer(const boost::asio::const_buffer& a_buf, del_t a_del)
        : boost::asio::const_buffer(a_buf)
        , m_ptr(new shadow(a_del))
    {}
};

template <typename Alloc = std::allocator<char> >
class shared_buffer_queue
        : public basic_buffer_queue<shared_const_buffer, Alloc> {

    typedef basic_buffer_queue<shared_const_buffer, Alloc> base_t;
    typedef boost::function<void()> del_t;

    struct nodel { void operator()() {} };

public:
    explicit shared_buffer_queue(const Alloc& a_alloc = Alloc())
        : base_t(a_alloc) {
    }

    /// Enqueue shared data to the queue without initiating a socket write.
    void enqueue(const shared_const_buffer& a_buf) {
        base_t::enqueue(a_buf);
    }

    /// Enqueue undeletable data to the queue without initiating a socket write.
    void enqueue(const boost::asio::const_buffer& a_buf) {
        base_t::enqueue(shared_const_buffer(a_buf, nodel()));
    }

    /// Initiate asynchronous socket write.
    template <class Socket, class Handler>
    void async_write(Socket& a_socket, Handler a_h) {
        base_t::async_write(a_socket, a_h);
    }

    /// Enqueue shared data and initiate asynchronous socket write.
    template <class Socket, class Handler>
    void async_write(Socket& a_socket, const shared_const_buffer& a_buf,
            Handler a_h) {
        base_t::async_write(a_socket, a_buf, a_h);
    }

    /// Enqueue undeletable data and initiate asynchronous socket write.
    template <class Socket, class Handler>
    void async_write(Socket& a_socket, const boost::asio::const_buffer& a_buf,
            Handler a_h) {
        base_t::async_write(a_socket, shared_const_buffer(a_buf, nodel()), a_h);
    }
};

} // namespace utxx

#endif // _UTXX_SHARED_BUFFER_QUEUE_HPP_
