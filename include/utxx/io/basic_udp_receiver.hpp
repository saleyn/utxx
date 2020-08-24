//----------------------------------------------------------------------------
/// \file   basic_udp_receiver.hpp
/// \author Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
//----------------------------------------------------------------------------
/// \brief Basic UDP packet receiver.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Omnibius, LLC
// Created: 2012-05-27
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
#ifndef _UTXX_IO_BASIC_UDP_RECEIVER_HPP_
#define _UTXX_IO_BASIC_UDP_RECEIVER_HPP_

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <utxx/buffered_queue.hpp>

namespace utxx {
namespace io {

template <typename Derived, size_t BufSize = 16*1024>
class basic_udp_receiver : private boost::noncopyable {
public:
    typedef basic_io_buffer<BufSize> buffer_type;

    /// Constructor
    basic_udp_receiver(boost::asio::io_service& a_io_service)
        : m_io_service(a_io_service)
        , m_socket(a_io_service)
    {}

    void init(int a_port, size_t a_buf_sz = 0) {
        init(boost::asio::ip::udp::endpoint(
                boost::asio::ip::udp::v4(), a_port),
             a_buf_sz);
    }

    void init(const std::string& a_host, const std::string& a_service, size_t a_buf_sz = 0) {
        boost::asio::ip::udp::resolver l_resolver(m_io_service);
        boost::asio::ip::udp::resolver::query l_query(a_host, a_service);
        boost::asio::ip::udp::endpoint l_ep = *l_resolver.resolve(l_query);
        init(l_ep, a_buf_sz);
    }

    void init(boost::asio::ip::udp::endpoint a_endpoint, size_t a_buf_sz = 0) {
        m_socket.open(a_endpoint.protocol());

        boost::asio::socket_base::reuse_address l_opt(true);
        m_socket.set_option(l_opt);

        if (a_buf_sz) {
            boost::asio::socket_base::receive_buffer_size l_opt(a_buf_sz);
            m_socket.set_option(l_opt);
        }

        m_socket.bind(a_endpoint);
    }

    /// Begin reading from socket
    void start()    { m_rx_bytes = 0; async_read(); }
    /// Request to stop
    void stop()     { m_socket.close(); }

    /// Accessor to internal socket
    boost::asio::ip::udp::socket& socket() { return m_socket; }

    /// Total number of bytes received
    size_t rx_bytes() const { return m_rx_bytes; }

private:
    /// Start asynchronous socket read.
    void async_read() {
        m_socket.async_receive_from(buffer_space(m_in_buffer), m_sender_endpoint,
            boost::bind(&basic_udp_receiver::handle_read,
                this, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e, std::size_t n) {
        if (!e && n > 0) {
            m_rx_bytes += n;
            m_in_buffer.commit(n);
            static_cast<Derived*>(this)->on_data(m_in_buffer);
            m_in_buffer.crunch();
            async_read();
        }
    }

    /// The io_service used to perform asynchronous operations.
    boost::asio::io_service& m_io_service;

    /// Socket for basic_udp_receiver
    boost::asio::ip::udp::socket m_socket;

    /// Sender address
    boost::asio::ip::udp::endpoint m_sender_endpoint;

    /// Buffer for incoming data.
    buffer_type m_in_buffer;

    /// Total number of bytes read from socket
    size_t m_rx_bytes;
};

} // namespace io
} // namespace utxx

#endif // _UTXX_IO_BASIC_UDP_RECEIVER_HPP_
