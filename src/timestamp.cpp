//----------------------------------------------------------------------------
/// \file  timestamp.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of timestamp class.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-09-10
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

#include <utxx/convert.hpp>
#include <utxx/timestamp.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/string.hpp>
#include <utxx/time.hpp>
#include <utxx/error.hpp>
#include <stdio.h>

namespace utxx {

boost::mutex            timestamp::s_mutex;
thread_local long       timestamp::s_next_local_midnight_nseconds = 0;
thread_local long       timestamp::s_next_utc_midnight_nseconds   = 0;
thread_local time_t     timestamp::s_utc_nsec_offset              = 0;
thread_local char       timestamp::s_local_timestamp[16];
thread_local char       timestamp::s_utc_timestamp[16];
thread_local char       timestamp::s_local_timezone[8];

#ifdef DEBUG_TIMESTAMP
volatile long timestamp::s_hrcalls;
volatile long timestamp::s_syscalls;
#endif

namespace {
    static const char* s_values[] = {
        "none", "date", "date-time", "date-time-msec", "date-time-usec",
        "time", "time-msec", "time-usec",
    };
}

stamp_type parse_stamp_type(const std::string& a_line) {
    stamp_type t = find_index<stamp_type>(s_values, a_line, (stamp_type)-1, true);

    if (int(t) == -1)
        throw badarg_error("parse_stamp_tpe: invalid timestamp type: ", a_line);
    return t;
}

const char* to_string(stamp_type a_type) {
    assert(size_t(a_type) < length(s_values));
    return s_values[a_type];
}

char* timestamp::write_date(char* a_buf, time_t a_utc_seconds, bool a_utc,
                            size_t eos_pos, char a_sep, bool a_use_cached_date)
{
    long nsec = a_utc_seconds*1000000000L;

    // If not same day - update cached string value
    if (unlikely(nsec >= s_next_utc_midnight_nseconds))
        update_midnight_nseconds(now_utc());

    auto today_utc_midnight = s_next_utc_midnight_nseconds - 86400000000000L;

    if (a_sep || !a_use_cached_date || nsec < today_utc_midnight)
        return internal_write_date(a_buf, a_utc_seconds, a_utc, eos_pos, a_sep);
    else {
        strncpy(a_buf, a_utc ? s_utc_timestamp : s_local_timestamp, 9);
        if (eos_pos) { a_buf[eos_pos] = '\0'; return a_buf + eos_pos; }
        return a_buf + 9;
    }
}

void timestamp::update_midnight_nseconds(time_val a_now)
{
    auto s = a_now.sec();
    struct tm tm;
    localtime_r(&s, &tm);
    s_utc_nsec_offset = tm.tm_gmtoff * 1000000000L;
    auto now_midnight_nsecs   = a_now.nanoseconds() -
                                a_now.nanoseconds() % (86400L * 1000000000L);
    auto local_midnight_nsecs = now_midnight_nsecs - s_utc_nsec_offset;

    s_next_utc_midnight_nseconds   = now_midnight_nsecs + 86400L * 1000000000L;
    s_next_local_midnight_nseconds = a_now.nanoseconds() >= local_midnight_nsecs
                                   ? (s_next_utc_midnight_nseconds - s_utc_nsec_offset)
                                   : local_midnight_nsecs;

    strncpy(s_local_timezone, tm.tm_zone, sizeof(s_local_timezone)-1);
    s_local_timezone[sizeof(s_local_timezone)-1] = '\0';

    // the mutex is not needed here at all - s_timestamp lives in TLS storage
    internal_write_date(s_local_timestamp, s, false, 9, '\0');
    internal_write_date(s_utc_timestamp,   s, true,  9, '\0');
}

size_t timestamp::format_size(stamp_type a_tp)
{
    switch (a_tp) {
        case NO_TIMESTAMP:          return 0;
        case TIME:                  return 8;
        case TIME_WITH_USEC:        return 15;
        case TIME_WITH_MSEC:        return 12;
        case DATE:                  return 8;
        case DATE_TIME:             return 17;
        case DATE_TIME_WITH_USEC:   return 24;
        case DATE_TIME_WITH_MSEC:   return 21;
        default:                    break;
    }
    assert(false);  // should never get here
    return 0;
}

int timestamp::format(stamp_type a_tp, time_val tv, char* a_buf, size_t a_sz,
                      bool a_utc, bool a_day_chk, bool a_use_cached_date)
{
    BOOST_ASSERT((a_tp < DATE_TIME && a_sz > 14) || a_sz > 25);

    if (unlikely(a_tp == NO_TIMESTAMP)) {
        a_buf[0] = '\0';
        return 0;
    }

    auto pair = tv.split();

    // If small time is given, it's a relative value.
    bool rel = pair.first < 86400L;
    auto now = a_day_chk || rel ? update().split() : pair;

    if (rel)
        pair.first += now.first;

    char* p;

    switch (a_tp) {
        case TIME:
        case TIME_WITH_USEC:
        case TIME_WITH_MSEC: {
            auto sec = a_utc ? pair.first
                             : (pair.first + s_utc_nsec_offset / 1000000000L);
            p  = time_val::write_time(sec, pair.second, a_buf, a_tp);
            return p - a_buf;
        }
        case DATE:
            p = write_date(a_buf, pair.first, a_utc, 8, '\0', a_use_cached_date);
            return 8;
        case DATE_TIME:
        case DATE_TIME_WITH_USEC:
        case DATE_TIME_WITH_MSEC: {
            auto sec = a_utc ? pair.first
                             : (pair.first + s_utc_nsec_offset / 1000000000L);
            p = write_date(a_buf, pair.first, a_utc, 0, '\0', a_use_cached_date);
            p = time_val::write_time(sec, pair.second, p, stamp_type(a_tp+3));
            return p - a_buf;
        }
        default:
            strcpy(a_buf, "UNDEFINED");
            return -1;
    }
}

time_val timestamp::from_string(const char* a_datetime, size_t n, bool a_utc) {
    if (unlikely(n < 8 ||
                (n > 8 && (n < 17 || a_datetime[8]  != '-' ||
                           a_datetime[11] != ':' || a_datetime[14] != ':'))))
        throw badarg_error("Invalid time format: ", std::string(a_datetime, n));

    const char* p = a_datetime;

    auto parse = [&p](int digits) {
        int i = 0; const char* end = p + digits;
        for (; p != end; ++p) i = (10*i) + (*p - '0');
        return i;
    };

    unsigned hour = 0, min = 0, sec = 0, usec = 0;
    int      year = parse(4);
    unsigned mon  = parse(2);
    unsigned day  = parse(2);
    if (n > 8) {
        p++;
        hour = parse(2); p++;
        min  = parse(2); p++;
        sec  = parse(2);

        const char* dot = p++;

        if (*dot == '.' && n > 17)
        {
            const char* end = p + std::min<size_t>(6, n - (p - a_datetime));
            while (*p >= '0' && *p <= '9' && p != end)
                usec = 10*usec + (*p++ - '0');

            int len = p - dot - 1;
            switch (len)
            {
                case 3:  usec *= 1000; break;
                case 6:  break;
                default: throw badarg_error("Invalid millisecond format: ",
                                            std::string(a_datetime, end - a_datetime));
            }
        }
    }

    return a_utc
         ? time_val::universal_time(year, mon, day, hour, min, sec, usec)
         : time_val::local_time(year, mon, day, hour, min, sec, usec);
}

} // namespace utxx
