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

This file may be included in different open-source projects

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
#ifndef _UTIL_TIMESTAMP_HPP_
#define _UTIL_TIMESTAMP_HPP_

#include <util/high_res_timer.hpp>
#include <util/time_val.hpp>
#include <boost/thread.hpp>
#include <time.h>

namespace util {

enum stamp_type {
      NO_TIMESTAMP
    , TIME
    , TIME_WITH_MSEC
    , TIME_WITH_USEC
    , DATE_TIME
    , DATE_TIME_WITH_MSEC
    , DATE_TIME_WITH_USEC
};

class timestamp {
protected:
    static boost::mutex             s_mutex;
    static __thread hrtime_t        s_last_hrtime;
    static __thread struct timeval  s_last_time;
    static __thread time_t          s_midnight_seconds;
    static __thread time_t          s_utc_offset;
    static __thread char            s_timestamp[16];

    time_val m_tv;

    #ifdef DEBUG_TIMESTAMP
    size_t m_hrcalls;
    size_t m_syscalls;
    #endif

    static void update_midnight_seconds(const time_val& a_now);

public:
    /// Suggested buffer space type needed for format() calls.
    typedef char buf_type[32];

    #ifdef DEBUG_TIMESTAMP
    timestamp() : m_hrcalls(0), m_syscalls(0) {}
    #endif

    inline static void write_timestamp(
        char* timestamp, time_t seconds, size_t eos_pos = 8)
    {
        unsigned long n = seconds / 86400;
        n = seconds - n*86400;
        int hour = n / 3600;        n -= hour*3600;
        int min  = n / 60;
        int sec  = n - min*60;      n = hour / 10;
        timestamp[0] = '0' + n;     hour -= n*10;
        timestamp[1] = '0' + hour;
        timestamp[2] = ':';         n = min / 10;
        timestamp[3] = '0' + n;     min -= n*10;
        timestamp[4] = '0' + min;
        timestamp[5] = ':';         n = sec / 10;
        timestamp[6] = '0' + n;     sec -= n*10;
        timestamp[7] = '0' + sec;
        timestamp[eos_pos] = '\0';
    }

    /// Update internal timestamp by calling gettimeofday().
    void now() {
        m_tv.now();
        m_tv.copy_to(s_last_time);  // thread safe - we use TLV storage 
    }

    /// Return last timestamp obtained by calling update() or now().
    const time_val& last_time() const       { return m_tv; }

    /// Return the number of seconds from epoch to midnight 
    /// in UTC.
    static time_t utc_midnight_seconds()    { return s_midnight_seconds; }

    /// Return the number of seconds from epoch to midnight 
    /// in local time zone.
    static time_t local_midnight_seconds() {
        return s_midnight_seconds - s_utc_offset;
    }

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
    size_t hrcalls()  const { return m_hrcalls; }
    size_t syscalls() const { return m_syscalls; }
    #endif

    /// Implementation of this function tries to reduce the overhead of calling
    /// time clock functions by cacheing old results and using high-resolution
    /// timer to determine a need for gettimeofday call.
    void update();

    int update_and_write(stamp_type a_tp, char* a_buf, size_t a_sz) {
        update();
        return format(a_tp, &m_tv, a_buf, a_sz);
    }

    /// Write formatted timestamp string to the given \a a_buf buffer.
    /// The function always put a NULL terminator at the last written
    /// position.
    /// @param a_sz is the size of a_buf buffer and must be greater than 26.
    /// @return number of bytes written or -1 if \a a_tp is not known.
    int write(stamp_type a_tp, char* a_buf, size_t a_sz) const {
        return format(a_tp, m_tv, a_buf, a_sz);
    }

    /// Write a timeval structure to \a a_buf.
    inline static int format(stamp_type a_tp,
        const time_val& tv, char* a_buf, size_t a_sz) {
        return format(a_tp, reinterpret_cast<const struct timeval*>(&tv), a_buf, a_sz);
    }

    template <int N>
    static int format(stamp_type a_tp, const struct timeval* tv, char (&a_buf)[N]) {
        return format(a_tp, tv, a_buf, N);
    }

    static int format(stamp_type a_tp,
        const struct timeval* tv, char* a_buf, size_t a_sz);

    std::string to_string(stamp_type a_tp = TIME_WITH_USEC) const {
        return to_string(m_tv, a_tp);
    }

    static std::string to_string(
        const time_val& a_tv, stamp_type a_tp=TIME_WITH_USEC)
    {
        return to_string(reinterpret_cast<const struct timeval*>(&a_tv), a_tp);
    }

    static std::string to_string(const struct timeval* a_tv,
        stamp_type a_tp=TIME_WITH_USEC)
    {
        char buf[32]; format(a_tp, a_tv, buf, sizeof(buf));
        return std::string(buf);
    }

};

//---------------------------------------------------------------------------
// Testing timestamp interface functions
//---------------------------------------------------------------------------

struct test_timestamp : public timestamp {
    /// Use this function for testing when you need to set current time
    /// to times different from now.  Otherwise in production code
    /// always use update() instead.
    void update(const time_val& a_now, hrtime_t a_hrnow) {
        m_tv = a_now;
        m_tv.copy_to(s_last_time);

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

} // namespace util


#endif // _UTIL_TIMESTAMP_HPP_
