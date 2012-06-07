/**
 * \file
 * \brief Shared Buffer Queue
 *
 * \author Dmitriy Kargapolov <dmitriy dot kargapolov at gmail dot com>
 * \version 1.0
 * \since 05 June 2012
 */

/*
 * Copyright (C) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTIL_SHARED_BUFFER_QUEUE_HPP_
#define _UTIL_SHARED_BUFFER_QUEUE_HPP_

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <util/basic_buffer_queue.hpp>

namespace util {

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

    void enqueue(const shared_const_buffer& a_buf) {
        base_t::enqueue(a_buf);
    }

    void enqueue(const boost::asio::const_buffer& a_buf) {
        base_t::enqueue(shared_const_buffer(a_buf, nodel()));
    }
};

} // namespace util

#endif // _UTIL_SHARED_BUFFER_QUEUE_HPP_
