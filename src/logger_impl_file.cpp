//----------------------------------------------------------------------------
/// \file  logger_impl_file.cpp
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementing synchronous file writer for the
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
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <utxx/logger/logger_impl_file.hpp>
#include <utxx/logger/logger_impl.hpp>
#include <utxx/path.hpp>
#include <utxx/time_val.hpp>
#include <boost/thread.hpp>
#include <cmath>

namespace utxx {

static logger_impl_mgr::impl_callback_t f = &logger_impl_file::create;
static logger_impl_mgr::registrar reg("file", f);

std::ostream& logger_impl_file::dump(std::ostream& out,
    const std::string& a_prefix) const
{
    out << a_prefix << "logger." << name() << '\n'
        << a_prefix << "    filename       = " << m_filename << '\n'
        << a_prefix << "    append         = " << (m_append ? "true" : "false")       << '\n'
        << a_prefix << "    mode           = " << m_mode << '\n';
    if (!m_symlink.empty()) out <<
           a_prefix << "    symlink        = " << m_symlink << '\n';
    out << a_prefix << "    levels         = " << log_levels_to_str(m_levels) << '\n'
        << a_prefix << "    no-header      = " << (m_no_header  ? "true" : "false")   << '\n'
        << a_prefix << "    splitting      = " << (m_split_size ? "true" : "false")   << '\n';
    if (m_split_size) {
        out << a_prefix << "      size         = " << m_split_size  << '\n'
            << a_prefix << "      parts        = " << m_split_parts << '\n'
            << a_prefix << "      order        = " << m_split_order << '\n'
            << a_prefix << "      delimiter    = " << m_split_delim << '\n';
    }
    return out;
}

bool logger_impl_file::init(const variant_tree& a_config)
{
    BOOST_ASSERT(this->m_log_mgr);
    finalize();

    try {
        m_filename = a_config.get<std::string>("logger.file.filename");
        m_filename = m_log_mgr->replace_env_and_macros(m_filename);
    } catch (boost::property_tree::ptree_bad_data&) {
        UTXX_THROW_BADARG_ERROR("logger.file.filename not specified");
    }

    m_append       = a_config.get("logger.file.append",       true);
    m_no_header    = a_config.get("logger.file.no-header",   false);
    m_mode         = a_config.get("logger.file.mode",         0644);
    m_symlink      = a_config.get("logger.file.symlink",        "");
    auto split_sz  = a_config.get("logger.file.split-size",      0);
    m_split_size   = size_t(split_sz);
    m_split_parts  = a_config.get("logger.file.split-parts",     0);
    m_split_delim  = a_config.get("logger.file.split-delim",   "_")[0];
    m_split_order  = split_ord::from_string(a_config.get("logger.file.split-order", "last"), true);

    if (split_sz  < 0)
        UTXX_THROW_BADARG_ERROR("logger.file.split-size cannot be negative: ",  split_sz);
    if (m_split_parts < 0)
        UTXX_THROW_BADARG_ERROR("logger.file.split-parts cannot be negative: ", m_split_parts);
    if (m_split_order == split_ord::ROTATE && m_split_parts == 0)
        UTXX_THROW_BADARG_ERROR("logger.file.split-parts cannot be zero when split-order is rotation!");

    m_split_parts_digits =
        !m_split_parts ? 0 : static_cast<int>(std::ceil(std::log10(m_split_parts))
                                              + ((m_split_parts%10 == 0) ? 1 : 0));
    m_orig_filename = m_filename;

    if (m_split_size) {
        m_split_filename_index = m_orig_filename.find_last_of('.');
        if(m_split_filename_index==std::string::npos)
            UTXX_THROW_RUNTIME_ERROR("logger.file.split-size: filename must have extension for file split feature.");
        // Determine the name of most recent log file
        if (!m_symlink.empty() && path::file_exists(m_symlink)) {
            // If symlink exists, get the file index from the symlinked filename
            auto         file = path::file_readlink(m_symlink);
            m_split_part_last = parse_file_index(file);
        } else {
            // Otherwise, get the file index from the mast modified log file
            std::string latest_filename;
            time_val    latest_time;
            auto on_file = [&](auto& dir, auto& filename, auto& stat, bool join_dir) {
                auto nm  = join_dir ? path::join(dir, filename) : filename;
                auto tm  = time_val(stat.st_mtim);
                if  (tm  > latest_time) {
                    latest_time     = tm;
                    latest_filename = nm;
                }
                return std::make_pair(nm, tm);
            };
            path::list_files<std::pair<std::string, time_val>>
                (on_file, get_file_name(-1), FileMatchT::WILDCARD, true);
            m_split_part_last = latest_filename.empty() ? 1 : parse_file_index(latest_filename);
        }
        modify_file_name(false);
    }

    auto levels = a_config.get("logger.file.levels", "");

    m_levels = levels.empty()
             ? m_log_mgr->level_filter()
             : parse_log_levels(levels);

    auto lll = __builtin_ffs(m_levels)-1;
    auto lev = __builtin_ffs(m_log_mgr->level_filter())-1;
    if  (lll < lev)
        UTXX_THROW_RUNTIME_ERROR("File logger's levels filter '", levels,
                                 "' is less granular than logger's default '",
                                 logger::log_levels_to_str(m_log_mgr->min_level_filter()),
                                 "'");

    if (m_levels != NOLOGGING) {
        open_file(false);

        // Install log_msg callbacks from appropriate levels
        for(int lvl = 0; lvl < logger::NLEVELS; ++lvl) {
            log_level level = logger::signal_slot_to_level(lvl);
            if ((m_levels & static_cast<int>(level)) != 0)
                this->add(level,
                    logger::on_msg_delegate_t::from_method
                        <logger_impl_file, &logger_impl_file::log_msg>(this));
        }
    }
    return true;
}

void logger_impl_file::write_file_header(bool exists, bool rotated)
{
    if (m_no_header)
        return;

    char buf[256];
    char* p = buf, *end = buf + sizeof(buf);

    tzset();

    auto ll = log_level_to_string(as_log_level(__builtin_ffs(m_levels)), false);
    int  tz = -timezone;
    int  hh = abs(tz / 3600);
    int  mm = abs(tz % 60);
    p += snprintf(p, end - p, "# %s at: %s %c%02d:%02d (MinLevel: %s)",
                  rotated ? "Log rotated" : "Logging started",
                  timestamp::to_string(DATE_TIME).c_str(),
                  tz > 0 ? '+' : '-', hh, mm, ll.c_str());
    if (!exists) {
        *p++ = '\n';
        *p++ = '#';
        if (!this->m_log_mgr ||
            this->m_log_mgr->timestamp_type() != stamp_type::NO_TIMESTAMP)
            p += snprintf(p, end - p, "Timestamp|");
        p += snprintf(p, end - p, "Level|");
        if (this->m_log_mgr) {
            if (this->m_log_mgr->show_ident())
                p += snprintf(p, end - p, "Ident|");
            if (this->m_log_mgr->show_thread() != logger::thr_id_type::NONE)
                p += snprintf(p, end - p, "Thread|");
            if (this->m_log_mgr->show_category())
            p += snprintf(p, end - p, "Category|");
        }
        p += snprintf(p, end - p, "Message");
        if (this->m_log_mgr && this->m_log_mgr->show_location())
            p += snprintf(p, end - p, " [File:Line%s]",
                        this->m_log_mgr->show_fun_namespaces() ? " Function" : "");
    }
    *p++ = '\n';

    if (write(m_fd, buf, p - buf) < 0)
        UTXX_THROW_IO_ERROR(errno, "Error writing log header to file: ", m_filename);
}

void logger_impl_file::modify_file_name(bool increment)
{
    switch (m_split_order) {
        case split_ord::FIRST:
            if (!increment)
                m_split_part = m_split_part_last;
            else {
                int min_index_found = 0;
                for (auto i=m_split_part_last; i > 0; --i) {
                    auto oldn  = get_file_name(i);
                    if (path::file_exists(oldn))
                        min_index_found = i;
                }
                if (min_index_found == 0) // No log file parts found
                    m_split_part = 1;
                else if (min_index_found > 1)
                    m_split_part = min_index_found-1; // Don't need to rename the files
                else {
                    m_split_part = 1;
                    for (auto i=m_split_part_last; i > 0; --i) {
                        auto oldn  = get_file_name(i);
                        auto newn  = get_file_name(i+1);
                        if (m_split_parts && i >= m_split_parts) // Reached max count?
                            path::file_unlink(oldn);
                        else if (path::file_exists(oldn) && !path::file_rename(oldn, newn))
                            UTXX_LOG_ERROR("Unable to rename log file '%s' to '%s': %s",
                                           oldn.c_str(), newn.c_str(), strerror(errno));
                    }
                    if (!m_split_parts || m_split_part_last < m_split_parts)
                        m_split_part_last++;
                }
            }
            break;
        case split_ord::LAST:
            if (increment) {
                if (m_split_part_last == m_split_parts && m_split_parts) {
                    auto newn = get_file_name(1);
                    if (!path::file_unlink(newn))
                        UTXX_LOG_ERROR("Unable to delete log file '%s': %s",
                                       newn.c_str(), strerror(errno));
                    for (auto i=2; i <= m_split_part_last; ++i) {
                        auto oldn = get_file_name(i);
                        auto newn = get_file_name(i-1);
                        if (path::file_exists(oldn) && !path::file_rename(oldn, newn))
                            UTXX_LOG_ERROR("Unable to rename log file '%s' to '%s': %s",
                                           oldn.c_str(), newn.c_str(), strerror(errno));
                    }
                }
                if (!m_split_parts || m_split_part_last < m_split_parts)
                    m_split_part_last++;
            }
            m_split_part = m_split_part_last;
            break;
        case split_ord::ROTATE:
            if (increment) {
                m_split_part_last = m_split_parts == m_split_part_last ? 1 : m_split_part_last+1;
                auto         name = get_file_name(m_split_part_last);
                path::file_unlink(name);
            }
            m_split_part          = m_split_part_last;
            break;
        default:
            UTXX_THROW_BADARG_ERROR("Unsupported logger's split file order: ", m_split_order);
    }
    m_filename = get_file_name(m_split_part);
}

std::string logger_impl_file::get_file_name(int part, bool with_dir) const
{
    char sfx[128];
    if (part < 0) // Special case - return wildcard file name
        sprintf(sfx, "%c*",   m_split_delim);
    else
        sprintf(sfx, "%c%0*d", m_split_delim, m_split_parts_digits, part);

    auto s = m_orig_filename;
    s.insert(m_split_filename_index,sfx);
    return with_dir ? s : path::basename(s);
}

int logger_impl_file::parse_file_index(std::string const& a_file) const
{
    if (a_file.length() > m_split_filename_index+1) {
        char s[128]; uint i=0;
        for (auto p = &a_file[m_split_filename_index+1]; *p >= '0' && *p <= '9' && i < sizeof(s)-1; ++p)
            s[i++]  = *p;
        s[i] = '\0';
        return std::stoi(s);
    }
    return 1;
}

void logger_impl_file::create_symbolic_link()
{
    if (!m_symlink.empty()) {
        m_symlink = m_log_mgr->replace_env_and_macros(m_symlink);
        if (!utxx::path::file_symlink(m_filename, m_symlink, true))
            UTXX_THROW_IO_ERROR(errno, "Error creating symlink ", m_symlink,
                                       " -> ", m_filename, ": ");
    }
}

void logger_impl_file::log_msg(const logger::msg& a_msg, const char* a_buf, size_t a_size)
{
    if (m_split_size) {
        struct stat stat_buf;
        int rc = fstat(m_fd, &stat_buf);
        if (rc < 0)
            UTXX_THROW_RUNTIME_ERROR("Unable to read file size for file "+m_filename);
        if ((size_t)stat_buf.st_size >= m_split_size) {
            finalize();
            modify_file_name();
            open_file(true);
        }
    }
    if (write(m_fd, a_buf, a_size) < 0)
        UTXX_THROW_IO_ERROR(errno, "Error writing to file: ", m_filename, ' ', a_msg.src_location());
}

bool logger_impl_file::open_file(bool rotated)
{
    auto exists = path::file_exists(m_filename);
    m_fd = open(m_filename.c_str(),
                O_CREAT|O_WRONLY|O_LARGEFILE | (m_append ? O_APPEND : O_TRUNC),
                m_mode);

    if (m_fd < 0)
        UTXX_THROW_IO_ERROR(errno, "Error opening file ", m_filename);

    create_symbolic_link();

    // Write field information
    write_file_header(exists, rotated);

    return exists;
}

} // namespace utxx
