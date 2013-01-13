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
#include <stdio.h>

namespace utxx {

boost::mutex            timestamp::s_mutex;
__thread hrtime_t       timestamp::s_last_hrtime;
__thread struct timeval timestamp::s_last_time = {0,0};
__thread time_t         timestamp::s_midnight_seconds = 0;
__thread time_t         timestamp::s_utc_offset = 0;
__thread char           timestamp::s_timestamp[16];

#ifdef DEBUG_TIMESTAMP
volatile long timestamp::s_hrcalls;
volatile long timestamp::s_syscalls;
#endif

static int internal_write_date(
    char* a_buf, time_t a_utc_seconds, bool a_utc, size_t eos_pos)
{
    struct tm tm;
    if (a_utc)
        gmtime_r(&a_utc_seconds, &tm);
    else
        localtime_r(&a_utc_seconds, &tm);
    int y = 1900 + tm.tm_year;
    int m = tm.tm_mon + 1;
    int d = tm.tm_mday;
    int n = y / 1000;
    a_buf[0] = '0' + n; y -= n*1000; n = y/100;
    a_buf[1] = '0' + n; y -= n*100;  n = y/10;
    a_buf[2] = '0' + n; y -= n*10;
    a_buf[3] = '0' + y; n  = m / 10;
    a_buf[4] = '0' + n; m -= n*10;
    a_buf[5] = '0' + m; n  = d / 10;
    a_buf[6] = '0' + n; d -= n*10;
    a_buf[7] = '0' + d;
    a_buf[8] = '-';
    a_buf[eos_pos] = '\0';

    return tm.tm_gmtoff;
}

void timestamp::write_date(
    char* a_buf, time_t a_utc_seconds, bool a_utc, size_t eos_pos)
{
    // If same day - use cached string value
    if (!a_utc && labs(a_utc_seconds - s_last_time.tv_sec) < 86400) {
        strncpy(a_buf, s_timestamp, 9);
        a_buf[eos_pos] = '\0';
    } else {
        internal_write_date(a_buf, a_utc_seconds, a_utc, eos_pos);
    }
}

void timestamp::update_midnight_seconds(const time_val& a_now)
{
    // FIXME: it doesn't seem like the mutex is needed here at all
    // since s_timestamp lives in TLS storage
    boost::mutex::scoped_lock guard(s_mutex);
    s_utc_offset = internal_write_date(
        s_timestamp, a_now.timeval().tv_sec, false, 9);

    s_midnight_seconds  = a_now.sec() - a_now.sec() % 86400;
}

const time_val& timestamp::now() {
    // thread safe - we use TLV storage
    s_last_time   = time_val::universal_time().timeval();
    s_last_hrtime = high_res_timer::gettime();
    return last_time();
}

void timestamp::update_slow()
{
    now();
    #ifdef DEBUG_TIMESTAMP
    atomic::inc(&s_syscalls);
    #endif

    // FIXME: the method below will produce incorrect time stamps during
    // switch to/from daylight savings time because of the unaccounted
    // utc_offset change.
    if (unlikely(s_midnight_seconds == 0 ||
                 last_time().sec() > s_midnight_seconds)) {
        update_midnight_seconds(last_time());
    }
}

int timestamp::format(stamp_type a_tp,
    const struct timeval* tv, char* a_buf, size_t a_sz, bool a_utc)
{
    BOOST_ASSERT((a_tp < DATE_TIME && a_sz > 14) || a_sz > 26);

    if (unlikely(a_tp == NO_TIMESTAMP)) {
        a_buf[0] = '\0';
        return 0;
    }

    if (unlikely(s_midnight_seconds == 0)) {
        timestamp ts; ts.update();
    }

    // If small time is given, it's a relative value.
    bool l_rel  = tv->tv_sec < 86400;
    time_t sec  = l_rel ? tv->tv_sec : tv->tv_sec + (a_utc ? 0 : s_utc_offset);
    long l_usec = tv->tv_usec;

    switch (a_tp) {
        case TIME:
            write_time(a_buf, sec, 8);
            return 8;
        case TIME_WITH_USEC:
            write_time(a_buf, sec, 15);
            a_buf[8] = '.';
            itoa_right(a_buf+9, 6, l_usec, '0');
            return 15;
        case TIME_WITH_MSEC:
            write_time(a_buf, sec, 12);
            a_buf[8] = '.';
            itoa_right(a_buf+9, 3, l_usec / 1000, '0');
            return 12;
        case DATE_TIME:
            write_date(a_buf, l_rel ? s_last_time.tv_sec : tv->tv_sec, a_utc, 9);
            write_time(a_buf+9, sec, 8);
            return 17;
        case DATE_TIME_WITH_USEC:
            write_date(a_buf, l_rel ? s_last_time.tv_sec : tv->tv_sec, a_utc, 9);
            write_time(a_buf+9, sec, 15);
            a_buf[17] = '.';
            itoa_right(a_buf+18, 6, l_usec, '0');
            return 24;
        case DATE_TIME_WITH_MSEC:
            write_date(a_buf, l_rel ? s_last_time.tv_sec : tv->tv_sec, a_utc, 9);
            write_time(a_buf+9, sec, 12);
            a_buf[17] = '.';
            itoa_right(a_buf+18, 3, l_usec / 1000, '0');
            return 21;
        default:
            strcpy(a_buf, "UNDEFINED");
            return -1;
    }
}

} // namespace utxx
