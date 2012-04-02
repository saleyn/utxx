/**
 * Supplementary classes for the <logger> class.
 *-----------------------------------------------------------------------------
 * Copyright (c) 2003-2009 Serge Aleynikov <serge@aleynikov.org>
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
#ifndef _UTIL_LOGGER_IMPL_IPP_
#define _UTIL_LOGGER_IMPL_IPP_

namespace util {

template <int N>
log_msg_info::log_msg_info(logger& a_logger, log_level lv, const char (&filename)[N], size_t ln)
    : m_logger(a_logger)
    , m_level(lv)
    , m_src_location_len(
            m_logger.show_location()
            ? snprintf(m_src_location, sizeof(m_src_location), " [%s:%ld]",
                path::basename(filename, filename+N), ln)
            : 0
      )
{
}

inline void log_msg_info::log(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    BOOST_SCOPE_EXIT( (&args) ) { va_end(args); } BOOST_SCOPE_EXIT_END;
    m_logger.log(*this, fmt, args);
}

inline int logger_impl::format_message(
    char* buf, size_t size, bool add_new_line,
    bool a_show_ident, bool a_show_location,
    const timestamp& a_tv,
    const log_msg_info& info, const char* fmt, va_list args
) throw (badarg_error)
{
    int len = a_tv.write(m_log_mgr->timestamp_type(), buf, size);
    len += sprintf(buf+len, "|%-7s|", logger::log_level_to_str(info.level()));
    if (a_show_ident)
        len += sprintf(buf+len, "%s|", info.get_logger().ident().c_str());
    int n = vsnprintf(buf+len, size-len-2, fmt, args);

    if (n < 0)
        throw badarg_error("Error formatting string:", fmt, ' ',
            (info.has_src_location() ? info.src_location() : ""));
    len += n;

    if (info.has_src_location() && a_show_location)
        len += snprintf(buf+len, size-len-1, (add_new_line ? "%s\n" : "%s"), info.src_location());
    else if (add_new_line)
        buf[len++] = '\n';
    return len;
}

} // namespace util

#endif  // _UTIL_LOGGER_IMPL_IPP_
