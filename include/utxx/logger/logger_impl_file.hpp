//----------------------------------------------------------------------------
/// \file   logger_impl_file.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementating synchronous file writer for the
/// <logger> class.
///
/// This implementation allows multiple threads to call the <LOG_*> 
/// logging macros concurrently.
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
#include <utxx/enum.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/thread.hpp>

namespace utxx {

class logger_impl_file: public logger_impl {
    UTXX_ENUM(split_ord, int,
      FIRST,
      LAST,
      ROTATE
    );

    std::string  m_name;
    std::string  m_filename;
    bool         m_append;
    std::string  m_symlink;
    uint32_t     m_levels;
    mode_t       m_mode;
    int          m_fd;
    boost::mutex m_mutex;
    bool         m_no_header;
    std::string  m_orig_filename;
    size_t       m_split_size;
    int          m_split_parts;
    split_ord    m_split_order;
    char         m_split_delim;
    int          m_split_part;
    int          m_split_part_last;
    int          m_split_parts_digits;
    size_t       m_split_filename_index;

    logger_impl_file(const char* a_name)
        : m_name(a_name), m_append(true)
        , m_levels(LEVEL_NO_DEBUG)
        , m_mode(0644), m_fd(-1), m_no_header(false)
        , m_split_size(0), m_split_parts(0), m_split_order(split_ord::FIRST)
        , m_split_delim('_')
        , m_split_part(0), m_split_part_last(0)
        , m_split_parts_digits(0), m_split_filename_index(-1)
    {}

    void finalize() {
        if (m_fd > -1) { close(m_fd); m_fd = -1; }
    }

    void        modify_file_name(bool increment = true);
    int         parse_file_index(std::string const& a_filename) const;

    void        create_symbolic_link();
    bool        open_file();
public:
    static logger_impl_file* create(const char* a_name) {
        return new logger_impl_file(a_name);
    }

    virtual ~logger_impl_file() {
        finalize();
    }

    const std::string& name() const { return m_name; }

    /// Get full file name of the give part or a wildcard if part < 0
    std::string get_file_name(int part, bool with_dir = true) const;

    /// Dump all settings to stream
    std::ostream& dump(std::ostream& out, const std::string& a_prefix) const;

    bool init(const variant_tree& a_config);

    void log_msg(const logger::msg& a_msg, const char* a_buf, size_t a_size);


};

} // namespace utxx

#endif

