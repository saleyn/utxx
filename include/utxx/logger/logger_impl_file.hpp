//----------------------------------------------------------------------------
/// \file   logger_impl_syslog.hpp
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
/// synchronization.  As a workaround for this oddity, the <logger_impl_file>
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
#ifndef _UTXX_LOGGER_FILE_HPP_
#define _UTXX_LOGGER_FILE_HPP_

#include <utxx/logger.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/thread.hpp>

namespace utxx {

class logger_impl_file: public logger_impl {
    std::string  m_name;
    std::string  m_filename;
    bool         m_append;
    std::string  m_symlink;
    bool         m_use_mutex;
    uint32_t     m_levels;
    mode_t       m_mode;
    int          m_fd;
    boost::mutex m_mutex;
    bool         m_no_header;

    logger_impl_file(const char* a_name)
        : m_name(a_name), m_append(true), m_use_mutex(false)
        , m_levels(LEVEL_NO_DEBUG)
        , m_mode(0644), m_fd(-1), m_no_header(false)
    {}

    void finalize() {
        if (m_fd > -1) { close(m_fd); m_fd = -1; }
    }
public:
    static logger_impl_file* create(const char* a_name) {
        return new logger_impl_file(a_name);
    }

    virtual ~logger_impl_file() {
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

#endif

