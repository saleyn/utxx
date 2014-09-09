//----------------------------------------------------------------------------
/// \file  timestamp.hpp
//----------------------------------------------------------------------------
/// \brief Microsecond time querying and fast time string formatting.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-10-15
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project

This file is part of the utxx open-source project.

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
#ifndef _UTXX_TIMESTAMP_HPP_
#define _UTXX_TIMESTAMP_HPP_

#include <utxx/high_res_timer.hpp>
#include <utxx/time_val.hpp>
#include <boost/thread.hpp>
#include <time.h>

#ifdef DEBUG_TIMESTAMP
#include <utxx/atomic.hpp>
#endif

namespace utxx {

enum stamp_type {
      NO_TIMESTAMP
    , TIME
    , TIME_WITH_MSEC
    , TIME_WITH_USEC
    , DATE_TIME
    , DATE_TIME_WITH_MSEC
    , DATE_TIME_WITH_USEC
};

/// Parse a timestamp from string.
/// Parsing is case-insensitive. The value is one of:
/// "none|time|time-msec|time-usec|date-time|date-time-msec|date-time-usec".
/// Throws badarg_error() on unrecognized value.
stamp_type  parse_stamp_type(const std::string& a_line);

/// Convert stamp_type to string.
const char* to_string(stamp_type a_type);

class timestamp {
protected:
    static boost::mutex             s_mutex;
    static __thread hrtime_t        s_last_hrtime;
    static __thread struct timeval  s_last_time;
    static __thread time_t          s_midnight_seconds;
    static __thread time_t          s_utc_offset;
    static __thread char            s_timestamp[16];

    #ifdef DEBUG_TIMESTAMP
    static volatile long s_hrcalls;
    static volatile long s_syscalls;
    #endif

    static void update_midnight_seconds(const time_val& a_now);

    static void update_slow();

public:
    /// Suggested buffer space type needed for format() calls.
    typedef char buf_type[32];

    /// Write local date in format: YYYYMMDD. If \a eos_pos > 8
    /// the function appends '-' at the end of the YYYYMMDD string.
    /// The function sets a_buf[eos_pos] = '\0'.
    static void write_date(
        char* a_buf, time_t a_utc_seconds, bool a_utc=false, size_t eos_pos=8);

    inline static void write_time(
        char* a_buf, time_t seconds, size_t eos_pos = 8)
    {
        unsigned long n = seconds / 86400;
        n = seconds - n*86400;
        int hour = n / 3600;    n -= hour*3600;
        int min  = n / 60;
        int sec  = n - min*60;  n = hour / 10;
        a_buf[0] = '0' + n;     hour -= n*10;
        a_buf[1] = '0' + hour;
        a_buf[2] = ':';         n = min / 10;
        a_buf[3] = '0' + n;     min -= n*10;
        a_buf[4] = '0' + min;
        a_buf[5] = ':';         n = sec / 10;
        a_buf[6] = '0' + n;     sec -= n*10;
        a_buf[7] = '0' + sec;
        a_buf[eos_pos] = '\0';
    }

    /// Update internal timestamp by calling gettimeofday().
    static const time_val& now();

    /// Return last timestamp obtained by calling update() or now().
    static const time_val& last_time() { return time_val_cast(s_last_time); }

    /// Equivalent to calling update() and last_time()
    static const time_val& cached_time()     { update(); return last_time(); }

    /// Return the number of seconds from epoch to midnight 
    /// in UTC.
    static time_t utc_midnight_seconds()    { return s_midnight_seconds; }

    /// Return the number of seconds from epoch to midnight 
    /// in local time zone.
    static time_t local_midnight_seconds() {
        return s_midnight_seconds - s_utc_offset;
    }

    /// Number of seconds since midnight in local time zone for a given UTC time.
    static time_t local_seconds_since_midnight(time_t a_utc_time) {
        time_t tm = a_utc_time + utc_offset();
        return tm % 86400;
    }

    /// Return offset from UTC in seconds.
    static time_t utc_offset() {
        if (unlikely(s_midnight_seconds == 0)) {
            timestamp ts; ts.update();
        }
        return s_utc_offset;
    }

    /// Convert a timestamp to the number of microseconds
    /// since midnight in local time.
    static int64_t local_usec_since_midnight(const time_val& a_now) {
        if (unlikely(s_midnight_seconds == 0)) {
            timestamp ts; ts.update();
        }
        int64_t l_time = a_now.sec() + s_utc_offset - s_midnight_seconds;
        while (l_time < 0) l_time += 86400;
        return l_time * 1000000 + a_now.usec();
    }

    /// Convert a timestamp to the number of microseconds
    /// since midnight in UTC.
    static uint64_t utc_usec_since_midnight(const time_val& a_now) {
        if (unlikely(s_midnight_seconds == 0)) {
            timestamp ts; ts.update();
        }
        int64_t l_time = a_now.sec() - s_midnight_seconds;
        while (l_time < 0) l_time += 86400;
        return l_time * 1000000 + a_now.usec();
    }

    #ifdef DEBUG_TIMESTAMP
    static long hrcalls()  { return s_hrcalls; }
    static long syscalls() { return s_syscalls; }
    #endif

    /// Implementation of this function tries to reduce the overhead of calling
    /// time clock functions by cacheing old results and using high-resolution
    /// timer to determine a need for gettimeofday call.
    static void update() {
        hrtime_t l_hr_now = high_res_timer::gettime();
        hrtime_t l_hrtime_diff = l_hr_now - s_last_hrtime;

        // We allow up to N usec to rely on HR timer readings between
        // successive calls to gettimeofday.
        if ((l_hrtime_diff <= (high_res_timer::global_scale_factor() << 2)) &&
            (l_hr_now      >= s_last_hrtime)) {
            #ifdef DEBUG_TIMESTAMP
            atomic::inc(&s_hrcalls);
            #endif
        } else {
            //std::cout << l_hrtime_diff << std::endl;
            update_slow();
        }
    }

    inline static int update_and_write(stamp_type a_tp,
            char* a_buf, size_t a_sz, bool a_utc=false) {
        update();
        return format(a_tp, &last_time().timeval(), a_buf, a_sz, a_utc);
    }

    /// Write formatted timestamp string to the given \a a_buf buffer.
    /// The function always put a NULL terminator at the last written
    /// position.
    /// @param a_sz is the size of a_buf buffer and must be greater than 25.
    /// @return number of bytes written or -1 if \a a_tp is not known.
    inline static int write(stamp_type a_tp, char* a_buf, size_t a_sz, bool a_utc=false) {
        return format(a_tp, last_time(), a_buf, a_sz, a_utc);
    }

    template <int N>
    inline static int write(stamp_type a_tp, char (&a_buf)[N], bool a_utc=false) {
        return format(a_tp, last_time(), a_buf, N, a_utc);
    }

    /// Number of bytes needed to hold the string representation of \a a_tp.
    static size_t format_size(stamp_type a_tp);

    /// Write a timeval structure to \a a_buf.
    inline static int format(stamp_type a_tp,
        const time_val& tv, char* a_buf, size_t a_sz, bool a_utc = false) {
        return format(a_tp, &tv.timeval(), a_buf, a_sz, a_utc);
    }

    template <int N>
    inline static int format(stamp_type a_tp, const struct timeval* tv,
            char (&a_buf)[N], bool a_utc = false) {
        return format(a_tp, tv, a_buf, N, a_utc);
    }

    static int format(stamp_type a_tp,
        const struct timeval* tv, char* a_buf, size_t a_sz, bool a_utc=false);

    inline static std::string to_string(stamp_type a_tp = TIME_WITH_USEC, bool a_utc=false) {
        return to_string(cached_time(), a_tp, a_utc);
    }

    inline static std::string to_string(
            const time_val& a_tv, stamp_type a_tp=TIME_WITH_USEC, bool a_utc=false) {
        return to_string(&a_tv.timeval(), a_tp, a_utc);
    }

    inline static std::string to_string(const struct timeval* a_tv,
            stamp_type a_tp=TIME_WITH_USEC, bool a_utc=false) {
        buf_type buf; format(a_tp, a_tv, buf, sizeof(buf), a_utc);
        return std::string(buf);
    }

    /// Parse time_val from string in format: YYYYMMDD-hh:mm:ss[.sss[sss]]
    time_val from_string(const char* a_datetime, size_t n, bool a_utc = true);
};

//---------------------------------------------------------------------------
// Testing timestamp interface functions
//---------------------------------------------------------------------------

struct test_timestamp : public timestamp {
    /// Use this function for testing when you need to set current time
    /// to times different from now.  Otherwise in production code
    /// always use timestamp::update() instead.
    void update(const time_val& a_now, hrtime_t a_hrnow) {
        s_last_time = a_now.timeval();

        if (unlikely(s_midnight_seconds == 0 || a_now.sec() > s_midnight_seconds)) {
            update_midnight_seconds(a_now);
        }
        s_last_hrtime = a_hrnow;
    }

    /// The function will reset midnight seconds
    /// offset so that update(a_now, a_hrnow) can be used to set it
    /// based on the controled timestamp.  Use this function 
    /// for testing in combination with update(a_now, a_hrnow).  
    static void reset() { s_midnight_seconds = 0; } 

    /// Use for testing only.
    void now() {}
private:
    void update();
    int update_and_write(stamp_type a_tp, char* a_buf, size_t a_sz);
};

} // namespace utxx

#ifdef DEBUG_TIMESTAMP
#include <utxx/../../src/timestamp.cpp>
#endif

#endif // _UTXX_TIMESTAMP_HPP_
