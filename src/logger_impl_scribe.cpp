//----------------------------------------------------------------------------
/// \file  logger_impl_scribe.cpp
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementating synchronous file writer for the
/// <tt>logger</tt> class.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-25
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/logger/logger_impl_scribe.hpp>

#ifdef HAVE_THRIFT_H

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <boost/format.hpp>
#include <utxx/url.hpp>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

namespace utxx {

static logger_impl_mgr::impl_callback_t f = &logger_impl_scribe::create;
static logger_impl_mgr::registrar reg("scribe", f);

logger_impl_scribe::logger_impl_scribe(const char* a_name)
    : m_name(a_name)
    , m_server_addr("uds:///var/run/scribed")
    , m_levels(LEVEL_NO_DEBUG)
    , m_show_location(true)
    , m_show_ident(false)
    , m_reconnecting(0)
{}

void logger_impl_scribe::finalize()
{
    if (!m_engine.running()) {
        m_engine.close_file(m_fd);
        m_engine.stop();
    }
    disconnect();
}

std::ostream& logger_impl_scribe::dump(std::ostream& out,
    const std::string& a_prefix) const
{
    out << a_prefix << "logger." << name() << '\n'
        << a_prefix << "    address        = " << m_server_addr.to_string() << '\n'
        << a_prefix << "    timeout        = " << m_server_timeout << '\n'
        << a_prefix << "    levels         = " << logger::log_levels_to_str(m_levels) << '\n'
        << a_prefix << "    show-location  = " << (m_show_location ? "true" : "false") << '\n'
        << a_prefix << "    show-indent    = " << (m_show_ident    ? "true" : "false") << '\n';
    return out;
}

static void thrift_output(const char* a_msg) {
    LOG_ERROR((a_msg));
}

bool logger_impl_scribe::init(const variant_tree& a_config)
    throw(badarg_error)
{
    ::apache::thrift::GlobalOutput.setOutputFunction(&thrift_output);

    finalize();

    std::stringstream str;
    str << "tcp://localhost:" << DEFAULT_PORT;
    std::string url = a_config.get<std::string>("logger.scribe.address", str.str());
    if (!m_server_addr.parse(url))
        throw std::runtime_error(
            std::string("Invalid scribe server address [logger.scribe.address]: ") + url);

    m_server_timeout= a_config.get<int>("logger.scribe.timeout", DEFAULT_TIMEOUT);

    // See comments in the beginning of the logger_impl_scribe.hpp on
    // thread safety.
    m_levels        = logger::parse_log_levels(a_config.get<std::string>(
                        "logger.scribe.levels", logger::default_log_levels));
    m_show_location = a_config.get<bool>(
                        "logger.scribe.show-location",
                        this->m_log_mgr && this->m_log_mgr->show_location());
    m_show_ident    = a_config.get<bool>(
                        "logger.scribe.show-ident",
                        this->m_log_mgr && this->m_log_mgr->show_ident());

    if (m_levels != NOLOGGING) {
        try {
            connect();
        } catch (const std::exception& e) {
            throw std::runtime_error(
                (boost::format("Failed to open connection to scribe server %s: %s")
                    % m_server_addr % e.what()).str().c_str());
        }

        // If this implementation started as part of the logging framework,
        // install it in the slots of the logger for use with LOG_* macros
        if (m_log_mgr) {
            // Install log_msg callbacks from appropriate levels
            for(int lvl = 0; lvl < logger_impl::NLEVELS; ++lvl) {
                log_level level = logger::signal_slot_to_level(lvl);
                if ((m_levels & static_cast<int>(level)) != 0)
                    this->add_msg_logger(level,
                        on_msg_delegate_t::from_method<
                            logger_impl_scribe, &logger_impl_scribe::log_msg>(this));
            }
            // Install log_bin callback
            this->add_bin_logger(
                on_bin_delegate_t::from_method<
                    logger_impl_scribe, &logger_impl_scribe::log_bin>(this));
        }
    }

    m_fd = m_engine.open_stream(m_name.c_str(),
                                boost::bind(&logger_impl_scribe::writev,
                                            this->shared_from_this(), _1, _2, _3, _4),
                                NULL,
                                m_socket->getSocketFD()
                               );
    if (!m_fd)
        throw std::runtime_error("Error opening scribe logging stream!");

    m_engine.set_reconnect(m_fd,
                           boost::bind(&logger_impl_scribe::on_reconnect,
                                       this->shared_from_this(), _1));
    m_engine.start();

    return true;
}

int logger_impl_scribe::connect() {
    namespace at = apache::thrift;

    m_socket.reset(
        m_server_addr.proto == UDS
            ? new at::transport::TSocket(m_server_addr.path)
            : new at::transport::TSocket(m_server_addr.addr, m_server_addr.port_int()));
    if (!m_socket)
        throw std::runtime_error("Failed to create scribe socket");

    m_socket->setConnTimeout(m_server_timeout);
    m_socket->setRecvTimeout(m_server_timeout);
    m_socket->setSendTimeout(m_server_timeout);

    /*
     * We don't want to send resets to close the connection. Among
     * other badness it also reduces data reliability. On getting a
     * rest, the receiving socket will throw any data the receving
     * process has not yet read.
     *
     * echo 5 > /proc/sys/net/ipv4/tcp_fin_timeout to set the TIME_WAIT
     * timeout on a system.
     * sysctl -a | grep tcp
     */
    m_socket->setLinger(0, 0);

    m_transport.reset(new at::transport::TFramedTransport(m_socket));
    if (!m_transport)
        throw std::runtime_error("Failed to create scribe framed transport");
    m_protocol.reset(new at::protocol::TBinaryProtocol(m_transport));
    if (!m_protocol)
        throw std::runtime_error("Failed to create scribe protocol");
    m_protocol->setStrict(false, false);

    m_transport->open();

    int attempts = m_reconnecting;
    m_reconnecting = 0;

    return attempts;
}

void logger_impl_scribe::disconnect() {
    if (connected())
        m_transport->close();
}

// Called by m_engine when writev() call resulted in an error
int logger_impl_scribe::on_reconnect(typename async_logger_engine::stream_info& a_si)
{
    int res = -1;

    try {
        int attempts = connect();

        if (m_reconnecting > 0)
            LOG_INFO(("Successfully reconnected to scribe server at %s (attempts=%d)",
                      m_server_addr.to_string().c_str(), attempts));

        res = m_socket->getSocketFD();
    } catch(std::exception& e) {
        if (!m_reconnecting++) {
            LOG_ERROR(("Failed to reconnect to scribe server at %s: %s",
                       m_server_addr.to_string().c_str(), e.what()));
        }
    }

    return res;
}

int logger_impl_scribe::writev(typename async_logger_engine::stream_info& a_si,
                               const char* a_categories[],
                               const iovec* a_data, size_t a_size)
{
    namespace atp = ::apache::thrift::protocol;
    int32_t xfer = 0;

    try {
        int32_t cseqid = 0;
        xfer = m_protocol->writeMessageBegin("Log", atp::T_CALL, cseqid);

        {
            xfer += m_protocol->writeStructBegin("scribe_Log_pargs");
            xfer += m_protocol->writeFieldBegin("messages", atp::T_LIST, 1);

            xfer += write_items(a_categories, a_data, a_size);

            xfer += m_protocol->writeFieldEnd();

            xfer += m_protocol->writeFieldStop();
            xfer += m_protocol->writeStructEnd();
        }

        xfer += m_protocol->writeMessageEnd();
        xfer += m_protocol->getTransport()->writeEnd();
        m_protocol->getTransport()->flush();

        // Wait for ack
        recv_log_reply();
    } catch (std::exception& e) {
        LOG_ERROR(("Error writing data to scribe: %s", e.what()));
        m_transport.reset();
        xfer = -1;
    }

    return xfer;
}

void logger_impl_scribe::log_msg(
    const log_msg_info& info, const timeval* a_tv, const char* fmt, va_list args)
    throw(io_error)
{
    char buf[logger::MAX_MESSAGE_SIZE];
    int len = logger_impl::format_message(buf, sizeof(buf), true,
                m_show_ident, m_show_location, a_tv, info, fmt, args);
    send_data(info.level(), info.category(), buf, len);
}

void logger_impl_scribe::log_bin(
    const std::string& a_category, const char* a_msg, size_t a_size) throw(io_error)
{
    send_data(LEVEL_LOG, a_category, a_msg, a_size);
}

void logger_impl_scribe::send_data(
    log_level level, const std::string& a_category, const char* a_msg, size_t a_size)
    throw(io_error)
{
    if (!m_engine.running())
        throw io_error("Logger terminated!");

    char* p = m_engine.allocate(a_size);

    if (!p) {
        std::stringstream s("Out of memory allocating ");
        s << a_size << " bytes!";
        throw io_error(s.str());
    }

    memcpy(p, a_msg, a_size);

    m_engine.write(m_fd, a_category, p, a_size);
}

int logger_impl_scribe::write_string(const char* a_str, int a_size)
{
    uint32_t result = m_protocol->writeI32(a_size);
    if (a_size > 0) {
        m_transport->write((uint8_t*)a_str, a_size);
        result += a_size;
    }
    return result;
}

int logger_impl_scribe::write_items(
    const char* a_categories[], const iovec* a_data, size_t a_size)
{
    namespace atp = ::apache::thrift::protocol;

    uint32_t xfer = m_protocol->writeListBegin(atp::T_STRUCT, a_size);
    for (uint32_t i=0; i < a_size; ++i) {
        xfer += m_protocol->writeStructBegin("LogEntry");

        xfer += m_protocol->writeFieldBegin("category", atp::T_STRING, 1);
        xfer += write_string(
            a_categories[i], a_categories[i] ? strlen(a_categories[i]) : 0);
        xfer += m_protocol->writeFieldEnd();

        xfer += m_protocol->writeFieldBegin("message", atp::T_STRING, 2);
        xfer += write_string(
            static_cast<const char*>(a_data[i].iov_base), a_data[i].iov_len);
        xfer += m_protocol->writeFieldEnd();

        xfer += m_protocol->writeFieldStop();
        xfer += m_protocol->writeStructEnd();
    }
    xfer += m_protocol->writeListEnd();
    return (int)xfer;
}

logger_impl_scribe::scribe_result_code
logger_impl_scribe::recv_log_reply()
{
    namespace atp = ::apache::thrift::protocol;
    int32_t rseqid = 0;
    std::string fname;
    atp::TMessageType mtype;

    m_protocol->readMessageBegin(fname, mtype, rseqid);
    if (mtype == atp::T_EXCEPTION) {
        ::apache::thrift::TApplicationException x;
        x.read(m_protocol.get());
        m_protocol->readMessageEnd();
        m_protocol->getTransport()->readEnd();
        throw x;
    }
    if (mtype != atp::T_REPLY) {
        m_protocol->skip(atp::T_STRUCT);
        m_protocol->readMessageEnd();
        m_protocol->getTransport()->readEnd();
    }
    if (fname.compare("Log") != 0) {
        m_protocol->skip(atp::T_STRUCT);
        m_protocol->readMessageEnd();
        m_protocol->getTransport()->readEnd();
    }
    scribe_result_code rc;
    bool is_set;
    read_scribe_result(rc, is_set);
    m_protocol->readMessageEnd();
    m_protocol->getTransport()->readEnd();

    if (is_set)
        return rc;

    throw ::apache::thrift::TApplicationException(
        ::apache::thrift::TApplicationException::MISSING_RESULT,
        "Scribe log failed: unknown result");
}

uint32_t logger_impl_scribe::read_scribe_result(scribe_result_code& a_rc, bool& a_is_set)
{
    namespace atp = ::apache::thrift::protocol;
    uint32_t xfer = 0;
    std::string fname;
    atp::TType ftype;
    int16_t fid;

    a_is_set = false;

    xfer += m_protocol->readStructBegin(fname);

    using atp::TProtocolException;

    while (true)
    {
        xfer += m_protocol->readFieldBegin(fname, ftype, fid);
        if (ftype == atp::T_STOP) {
            break;
        }
        switch (fid) {
        case 0:
            if (ftype == atp::T_I32) {
                int32_t ecast7;
                xfer += m_protocol->readI32(ecast7);
                a_rc = (scribe_result_code)ecast7;
                a_is_set = true;
            } else {
                xfer += m_protocol->skip(ftype);
            }
            break;
        default:
            xfer += m_protocol->skip(ftype);
            break;
        }
        xfer += m_protocol->readFieldEnd();
    }

    xfer += m_protocol->readStructEnd();

    return xfer;
}


} // namespace utxx

#endif // HAVE_THRIFT_H
