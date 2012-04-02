/**
 * Back-end plugin implementating asynchronous file writer for the <logger>
 * class.
 *-----------------------------------------------------------------------------
 * Copyright (c) 2009 Serge Aleynikov <serge@aleynikov.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *-----------------------------------------------------------------------------
 * Created: 2009-11-25
 * $Id$
 */
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <limits.h>
#include <fcntl.h>
#include <util/logger/logger_impl_async_file.hpp>

namespace util {

static boost::function< logger_impl* (void) > f = &logger_impl_async_file::create;
static logger_impl_mgr::registrar reg("async_file", f);

void logger_impl_async_file::finalize()
{
    m_terminated = true;
    atomic::memory_barrier();
    m_stack.signal();
    if (m_thread) {
        m_thread->join();
        m_thread = NULL;
    }
    if (m_fd)
        fsync(m_fd);
    close(m_fd);
}

bool logger_impl_async_file::init(const boost::property_tree::ptree& a_config)
    throw(badarg_error,io_error) 
{
    BOOST_ASSERT(this->m_log_mgr);
    finalize();

    try {
        m_filename = a_config.get<std::string>("logger.async_file.filename");
    } catch (boost::property_tree::ptree_bad_data&) {
        throw badarg_error("logger.async_file.filename not specified");
    }

    m_append = a_config.get<bool>("logger.async_file.append", true);
    m_mode   = a_config.get<int> ("logger.async_file.mode", 0644);
    m_levels = logger::parse_log_levels(
        a_config.get<std::string>("logger.async_file.levels", logger::default_log_levels));

    m_fd = open(m_filename.c_str(),
                m_append ? O_CREAT|O_APPEND|O_WRONLY|O_LARGEFILE
                         : O_CREAT|O_WRONLY|O_TRUNC|O_LARGEFILE,
                m_mode);
    unsigned long timeout = m_timeout.tv_sec * 1000 + m_timeout.tv_nsec / 1000000ul;

    m_show_location   = a_config.get<bool>("logger.async_file.show_location",
                                           this->m_log_mgr->show_location());
    m_show_ident      = a_config.get<bool>("logger.async_file.show_ident",
                                           this->m_log_mgr->show_ident());
    timeout           = a_config.get<int> ("logger.async_file.timeout", timeout);
    m_timeout.tv_sec  = timeout / 1000;
    m_timeout.tv_nsec = timeout % 1000 * 1000000ul;

    if (m_fd < 0)
        throw io_error(errno, "Error opening file: ", m_filename);

    // Install log_msg callbacks from appropriate levels
    for(int lvl = 0; lvl < logger_impl::NLEVELS; ++lvl) {
        log_level level = logger::signal_slot_to_level(lvl);
        if ((m_levels & static_cast<int>(level)) != 0)
            this->m_log_mgr->add_msg_logger(level, msg_binder[lvl],
                on_msg_delegate_t::from_method<
                    logger_impl_async_file, &logger_impl_async_file::log_msg>(this));
    }
    // Install log_bin callback
    this->m_log_mgr->add_bin_logger(
        bin_binder,
        on_bin_delegate_t::from_method<
            logger_impl_async_file, &logger_impl_async_file::log_bin>(this));

    m_error.clear();
    m_terminated = false;
    m_thread     = new boost::thread(boost::ref(*this));
    return true;
}

void logger_impl_async_file::operator()()
{
    if (m_barrier)
        m_barrier->wait();

    while (1) {
        async_data* nd = static_cast<async_data*>(
            m_terminated ? m_stack.try_reset(true) : m_stack.reset(&m_timeout, true)
        );
        if (nd == NULL) {
            if (m_terminated)
                break;
            else
                continue;
        }
        if (write_data(nd) < 0) {
            m_terminated = true;
            char buf[128];
            m_error = std::string("Error writing to file: ")
                    + (strerror_r(errno, buf, sizeof(buf)) == 0 ? buf : "unknown");
        }
    }
}

void logger_impl_async_file::log_msg(
    const log_msg_info& info, const timestamp& a_tv, const char* fmt, va_list args)
    throw(std::runtime_error) 
{
    char buf[logger::MAX_MESSAGE_SIZE];
    int len = logger_impl::format_message(buf, sizeof(buf), true, 
                m_show_ident, m_show_location, a_tv, info, fmt, args);
    send_data(info.level(), buf, len);
}

void logger_impl_async_file::log_bin(const char* msg, size_t size) 
    throw(std::runtime_error) 
{
    send_data(LEVEL_LOG, msg, size);
}

void logger_impl_async_file::send_data(log_level level, const char* msg, size_t size)
    throw(io_error)
{
    if (m_terminated)
        throw io_error(m_error.empty() ? "Logger terminated!" : m_error.c_str());

    size_t alloc_sz = async_data::allocation_size(size);
    void* pdat      = m_allocator.allocate(alloc_sz);
    async_data* p   = static_cast<async_data*>(memory::cached_allocator<char>::to_node(pdat));

    if (p == NULL) {
        std::stringstream s("Out of memory allocating ");
        s << size << " bytes!";
        m_error = s.str();
        m_terminated = true;
        throw io_error(m_error);
    }

    p->size  = size;
    p->level = level;
    memcpy(p->data(), msg, size);

    m_stack.push(p);
}

int logger_impl_async_file::write_data(async_data* p)
{
    struct iovec iov[IOV_MAX];
    size_t n = 0, size = 0;

    while (p != NULL) {
        iov[n].iov_base = p->data();
        iov[n].iov_len  = p->size;
        async_data* nxt = p->next();
        size += p->size;
        p = nxt;
        if (++n == IOV_MAX || p == NULL) {
            int res = writev(m_fd, iov, n);
            for (size_t i=0; i < n; ++i)
                m_allocator.free_node(async_data::to_node(iov[i].iov_base));
            if (res < 0)
                return res;
            n = 0;
        }
    }

    //fsync(m_fd);
    return 0;
}

} // namespace util
