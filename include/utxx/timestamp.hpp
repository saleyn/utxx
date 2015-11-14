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
    , DATE
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

/// Timestamp caching and formating functions
///
/// Initially this class was created to speed up overhead of gettimeofday()
/// calls. However on modern CPUs supporting constant_tsc, this is no longer
/// necessary, so the caching logic is deprecated.
class timestamp {
protected:
    static const long DAY_NSEC = 86400L * 1000000000L;

    static boost::mutex             s_mutex;
    static thread_local long        s_next_utc_midnight_nseconds;
    static thread_local long        s_next_local_midnight_nseconds;
    static thread_local long        s_utc_nsec_offset;
    static thread_local char        s_utc_timestamp[16];
    static thread_local char        s_local_timestamp[16];

    #ifdef DEBUG_TIMESTAMP
    static volatile long s_hrcalls;
    static volatile long s_syscalls;
    #endif

    static char* internal_write_date(
        char* a_buf, time_t a_utc_seconds, bool a_utc, size_t eos_pos, char a_sep);

    static void update_midnight_nseconds(time_val a_now);

    static void update_slow();

    static void check_midnight_seconds() {
        if (likely(s_next_utc_midnight_nseconds)) return;
        timestamp ts; ts.update();
    }

public:
    /// Suggested buffer space type needed for format() calls.
    typedef char buf_type[32];

    /// Write local date in format: YYYYMMDD, YYYY-MM-DD. If \a eos_pos > 8
    /// the function appends '-' at the end of the YYYYMMDD string.
    /// The function sets a_buf[eos_pos] = '\0'.
    /// @param a_sep if not '\0' will insert date delimiting character.
    /// @return pointer past the last written character
    static char* write_date(
        char* a_buf, time_t a_utc_seconds, bool a_utc=false,
        size_t eos_pos=8, char a_sep = '\0');

    /// Write time as string.
    /// Possible formats:
    ///   HHMMSSsss, HHMMSSssssss, HHMMSS.sss, HHMMSS.ssssss, HH:MM:SS[.ssssss].
    /// @param a_buf   the output buffer.
    /// @param a_time  is the time to use.
    /// @param a_type  determines the time formatting.
    ///                Valid values: TIME, TIME_WITH_MSEC, TIME_WITH_USEC.
    /// @param a_utc   If true - use UTC time.
    /// @param a_delim controls the ':' delimiter ('\0' means no delimiter)
    /// @param a_sep   defines the fractional second separating character ('\0' means - none)
    /// @return pointer past the last written character
    static char* write_time(
        char* a_buf, time_val a_time, stamp_type a_type, bool a_utc=false,
        char  a_delim = '\0', char a_sep = '.');

    inline static char* write_time(
        char* a_buf, time_t seconds, size_t eos_pos = 8, char a_delim = ':')
    {
        unsigned hour,min,sec;
        std::tie(hour,min,sec) = time_val::to_hms(seconds);
        char* p = a_buf;
        int n = hour / 10;
        *p++  = '0' + n;    hour -= n*10;
        *p++  = '0' + hour; n = min / 10;
        if (a_delim) *p++  = a_delim;
        *p++  = '0' + n;    min -= n*10;
        *p++  = '0' + min;  n = sec / 10;
        if (a_delim) *p++  = a_delim;
        *p++  = '0' + n;    sec -= n*10;
        *p++  = '0' + sec;
        if (eos_pos) {
            a_buf[eos_pos] = '\0';
            char* end = a_buf + eos_pos;
            return end < p ? end : p;
        }
        return p;
    }

    /// Update internal timestamp by calling gettimeofday().
    /// This function is a syntactic sugar for update().
    static time_val now() { return update(); }

    /// Implementation of this function tries to reduce the overhead of calling
    /// time clock functions by cacheing old results and using high-resolution
    /// timer to determine a need for gettimeofday call.
    static time_val update() {
        auto now = now_utc();

        check_day_change(now);
        return now;
    }

    static void check_day_change(time_val a_now) {
        // FIXME: the method below will produce incorrect time stamps during
        // switch to/from daylight savings time because of the unaccounted
        // utc_offset change.
        if (unlikely(a_now.nanoseconds() >= s_next_utc_midnight_nseconds))
            update_midnight_nseconds(a_now);
    }

    /// Return the number of seconds from epoch to midnight in UTC.
    static time_t utc_midnight_seconds()   { return utc_midnight_nseconds().sec();  }
    /// Return the number of seconds from epoch to midnight in local time.
    static time_t local_midnight_seconds() { return local_midnight_nseconds().sec();}

    /// Return the number of nanoseconds from epoch to midnight in UTC.
    static time_val utc_midnight_nseconds()   {
        return nsecs(s_next_utc_midnight_nseconds - DAY_NSEC);
    }
    /// Return the number of nanoseconds from epoch to midnight in local time.
    static time_val local_midnight_nseconds() {
        return nsecs(s_next_local_midnight_nseconds - DAY_NSEC);
    }

    /// Number of seconds since midnight in local time zone for a given UTC time.
    static time_t local_seconds_since_midnight(time_t a_utc_time) {
        time_t tm = a_utc_time + utc_offset();
        return tm % 86400;
    }

    /// Return offset from UTC in seconds.
    static time_t utc_offset() {
        if (unlikely(!s_next_utc_midnight_nseconds)) {
            timestamp ts; ts.update();
        }
        return s_utc_nsec_offset / 1000000000L;
    }

    /// Convert a timestamp to the number of microseconds
    /// since midnight in local time.
    static int64_t local_usec_since_midnight(time_val a_now_utc) {
        check_midnight_seconds();
        long diff = a_now_utc.diff_nsec(local_midnight_nseconds());
        if (unlikely(diff < 0)) diff = -diff % DAY_NSEC;
        return diff / 1000;
    }

    /// Convert a timestamp to the number of microseconds
    /// since midnight in UTC.
    static int64_t utc_usec_since_midnight(time_val a_now_utc) {
        check_midnight_seconds();
        long diff = a_now_utc.diff_nsec(utc_midnight_nseconds());
        if (unlikely(diff < 0)) diff = -diff % DAY_NSEC;
        return diff / 1000;
    }

    #ifdef DEBUG_TIMESTAMP
    static long hrcalls()  { return s_hrcalls; }
    static long syscalls() { return s_syscalls; }
    #endif

    inline static int update_and_write(stamp_type a_tp,
            char* a_buf, size_t a_sz, bool a_utc=false) {
        return format(a_tp, update(), a_buf, a_sz, a_utc, false);
    }

    /// Write formatted timestamp string to the given \a a_buf buffer.
    /// The function always put a NULL terminator at the last written
    /// position.
    /// @param a_sz is the size of a_buf buffer and must be greater than 25.
    /// @return number of bytes written or -1 if \a a_tp is not known.
    inline static int write(stamp_type a_tp, char* a_buf, size_t a_sz, bool a_utc=false) {
        return format(a_tp, update(), a_buf, a_sz, a_utc, false);
    }

    template <int N>
    inline static int write(stamp_type a_tp, char (&a_buf)[N], bool a_utc=false) {
        return format(a_tp, update(), a_buf, N, a_utc, false);
    }

    /// Number of bytes needed to hold the string representation of \a a_tp.
    static size_t format_size(stamp_type a_tp);

    /// Write a time_val to \a a_buf.
    /// @param tv current time
    /// @param a_buf output buffer
    /// @param a_sz  output buffer size
    /// @param a_utc write UTC time
    /// @param a_day_chk check for day change since last call
    /// @return number of written bytes
    static int format(stamp_type a_tp,
        time_val tv, char* a_buf, size_t a_sz, bool a_utc=false, bool a_day_chk=true);

    template <int N>
    inline static int format(stamp_type a_tp, time_val tv,
            char (&a_buf)[N], bool a_utc = false) {
        return format(a_tp, tv, a_buf, N, a_utc);
    }

    inline static std::string to_string(stamp_type a_tp = TIME_WITH_USEC, bool a_utc=false) {
        return to_string(now_utc(), a_tp, a_utc);
    }

    inline static std::string to_string(
            time_val a_tv, stamp_type a_tp=TIME_WITH_USEC, bool a_utc=false) {
        buf_type buf; format(a_tp, a_tv, buf, sizeof(buf), a_utc);
        return std::string(buf);
    }

    /// Parse time_val from string in format: YYYYMMDD-hh:mm:ss[.sss[sss]]
    static time_val from_string(const char* a_datetime, size_t n, bool a_utc = true);
};

inline std::ostream& operator<< (std::ostream& out, time_val a) {
    return out << timestamp::to_string(a, TIME_WITH_USEC, false);
}

//---------------------------------------------------------------------------
// Testing timestamp interface functions
//---------------------------------------------------------------------------

struct test_timestamp : public timestamp {
    /// Use this function for testing when you need to set current time
    /// to times different from now.  Otherwise in production code
    /// always use timestamp::update() instead.
    void update(time_val a_now) {
        if (unlikely(a_now.nanoseconds() >= s_next_utc_midnight_nseconds))
            update_midnight_nseconds(a_now);
    }

    /// The function will reset midnight seconds
    /// offset so that update(a_now, a_hrnow) can be used to set it
    /// based on the controled timestamp.  Use this function 
    /// for testing in combination with update(a_now, a_hrnow).
    /// Note: it only resets the midnight seconds in the current thread's TLS
    static void reset() {
        s_next_local_midnight_nseconds = 0;
        s_next_utc_midnight_nseconds   = 0;
    }

    /// Use for testing only.
    void now() {}
private:
    void update();
    int  update_and_write(stamp_type a_tp, char* a_buf, size_t a_sz);
};

} // namespace utxx

#ifdef DEBUG_TIMESTAMP
#include <utxx/../../src/timestamp.cpp>
#endif

#endif // _UTXX_TIMESTAMP_HPP_
