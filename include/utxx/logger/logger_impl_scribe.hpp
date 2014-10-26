//----------------------------------------------------------------------------
/// \file   logger_impl_scribe.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementating synchronous file writer for the
/// <logger> class.
///
/// This implementation allows multiple threads to call the <LOG_*> 
/// logging macros concurrently.  However, its performance is dependent upon
/// the file writing mode.  Linux has a bug ("feature"?) that if a file is
/// open without O_APPEND option, making write(2) calls on a file descriptor
/// is not thread-safe in a way that content written by one thread can be 
/// overwriten by the content written by another thread before that data is
/// written by kernel to the filesystem.  Consequently, concurrent writes
/// to a file descriptor open without O_APPEND flag must be protected by a 
/// mutex.  In our testing it seemed that the Linux kernel implementation of 
/// the write(2) function in presence of the O_APPEND flag is using proper
/// synchronization.  As a workaround for this oddity, the <logger_impl_scribe>
/// uses a mutex to guard the write(2) call when "logger.file.append" option
/// is <false> and doesn't use the mutex otherwise.  This has a rather drastic
/// impact on performance (up to 10x in latency hit).
/// Use the following test cases to see performance impact of using a mutex:
/// <code>
/// "THREAD=3 VERBOSE=1 test_logger --run_test=test_file_perf_overwrite
/// "THREAD=3 VERBOSE=1 test_logger --run_test=test_file_perf_append
/// "THREAD=3 VERBOSE=1 test_logger --run_test=test_file_perf_no_mutex
/// </code>
//----------------------------------------------------------------------------
// Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-25
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
#pragma once

#include <utxx/config.h>

#ifdef UTXX_HAVE_THRIFT_H

#include <utxx/logger.hpp>
#include <utxx/logger/logger_impl.hpp>
#include <utxx/multi_file_async_logger.hpp>
#include <utxx/url.hpp>
#include <sys/types.h>
#include <boost/enable_shared_from_this.hpp>

#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_URL
#undef PACKAGE_VERSION
#undef VERSION
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>

namespace utxx {

class logger_impl_scribe
    : public logger_impl
    , public boost::enable_shared_from_this<logger_impl_scribe>
{
//     struct logger_traits: public multi_file_async_logger_traits {
//         typedef memory::cached_allocator<char> allocator;
//     };

//    typedef basic_multi_file_async_logger<logger_traits> async_logger_engine;

    struct log_item {
        std::string category;
        std::string message;
    };

    typedef basic_multi_file_async_logger<>                 async_logger_engine;
    typedef typename async_logger_engine::stream_state_base stream_state_base;

    enum {
        DEFAULT_PORT    = 1463,
        DEFAULT_TIMEOUT = 5000,
    };

    enum scribe_result_code {
        OK = 0,
        TRY_LATER = 1
    };

    std::string  m_name;
    addr_info    m_server_addr;
    int          m_server_timeout;
    int          m_levels;
    boost::mutex m_mutex;
    int          m_reconnecting;

    async_logger_engine                     m_engine;
    typename async_logger_engine::file_id   m_fd;

    boost::shared_ptr<apache::thrift::transport::TSocket>           m_socket;
    boost::shared_ptr<apache::thrift::transport::TFramedTransport>  m_transport;
    boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol>    m_protocol;

    logger_impl_scribe(const char* a_name);

    void send_data(log_level level, const std::string& a_category,
                   const char* a_msg, size_t a_size) throw(io_error);

    void finalize();


    bool connected() const { return m_transport && m_transport->isOpen(); }
    int  connect();
    void disconnect();

    int writev(typename async_logger_engine::stream_info& a_si,
               const char* a_categories[], const iovec* a_data, size_t size);
    int on_reconnect(typename async_logger_engine::stream_info& a_si);

    int write_string(const char* a_str, int a_size);
    uint32_t read_scribe_result(scribe_result_code& a_rc, bool& a_is_set);
    scribe_result_code recv_log_reply();

    int write_items(const char* a_categories[], const iovec* a_data, size_t a_size);

public:
    static logger_impl_scribe* create(const char* a_name) {
        return new logger_impl_scribe(a_name);
    }

    virtual ~logger_impl_scribe() {
        finalize();
    }

    const std::string& name() const { return m_name; }

    /// Dump all settings to stream
    std::ostream& dump(std::ostream& out, const std::string& a_prefix) const;

    bool init(const variant_tree& a_config)
        throw(badarg_error, io_error);

    void log_msg(const logger::msg& a_msg, const char* a_buf, size_t a_size)
        throw(io_error);
};

} // namespace utxx

#endif // HAVE_THRIFT_H
