//----------------------------------------------------------------------------
/// \file  time_val.hpp
//----------------------------------------------------------------------------
/// \brief Derived implementation of ACE TimeVal to represent timeval
/// structure.
/// \author:  Serge Aleynikov
/// The original version is included in
/// (http://www.cs.wustl.edu/~schmidt/ACE.html).
//----------------------------------------------------------------------------
// ACE Library Copyright (c) 1993-2009 Douglas C. Schmidt
// Created: 2003-07-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2003 Serge Aleynikov <saleyn@gmail.com>

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


#ifndef _UTXX_TIME_VAL_HPP_
#define _UTXX_TIME_VAL_HPP_

#include <boost/static_assert.hpp>
#include <utxx/detail/gettimeofday.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/time.hpp>
#include "time.hpp"
#include <cstddef>
#include <stdint.h>
#if __cplusplus >= 201103L
#include <chrono>
#include <tuple>
#endif
#include <ctime>
#include <sys/time.h>
#include <math.h>

namespace utxx {
    class time_val;

    /// Indication of use of absolute time
    struct abs_time {
        long usec;
        abs_time(long s=0, long us=0) : usec(s*1000000 + us) {}
    };
    /// Indication of use of relative time
    struct rel_time {
        long usec;
        rel_time(long s=0, long us=0) : usec(s*1000000 + us) {}
    };

    struct usecs {
        long usec;
        explicit usecs(long   us) : usec(us)       {}
        explicit usecs(size_t us) : usec(long(us)) {}
        explicit usecs(int    us) : usec(us)       {}
    };

    struct secs {
        long usec;
        explicit secs(size_t s) : usec(long(s) * 1000000)  {}
        explicit secs(long   s) : usec(s * 1000000)        {}
        explicit secs(int    s) : usec(long(s) * 1000000)  {}
        explicit secs(double s) : usec(long(round(s*1e6))) {}
    };

    /// A helper class for dealing with 'struct timeval' structure. This class adds ability
    /// to perform arithmetic with the structure leaving the same footprint.
    class time_val
    {
        static const long   N10e6 = 1000000;
        static const size_t N10e9 = 1000000000u;
        int64_t             m_tv;

    public:
        time_val() : m_tv(0) {}
        time_val(secs   s)          : m_tv(s.usec)           {}
        time_val(usecs us)          : m_tv(us.usec)          {}
        time_val(long _s, long _us) : m_tv(_s * N10e6 + _us) {}
        time_val(const time_val& a) : m_tv(a.m_tv)           {}
        time_val(time_val tv, long _s)           : m_tv(tv.m_tv + _s*N10e6)       {}
        time_val(time_val tv, long _s, long _us) : m_tv(tv.m_tv + _s*N10e6 + _us) {}
        time_val(time_val tv, double interval) { set(tv, interval); }

        explicit time_val(const struct timeval& a) : m_tv(long(a.tv_sec)*N10e6 + a.tv_usec){}
        explicit time_val(struct tm& a_tm)         : m_tv(long(mktime(&a_tm))*N10e6)       {}

        /// Set time to abosolute time given by \a a
        explicit time_val(abs_time a)      : m_tv(a.usec) {}

        /// Set time to relative offset from now given by \a a time.
        explicit time_val(rel_time a)      : m_tv(now(0, a.usec).m_tv) {}

        time_val(int y, unsigned m, unsigned d, bool a_utc = true) {
            if (a_utc)
                m_tv = long(mktime_utc(y, m, d)) * N10e6;
            else {
                struct tm tm = { 0,0,0, int(d), int(m)-1, y-1900, 0,0,-1 };
                m_tv = long(mktime(&tm)) * N10e6;
            }
        }

        time_val(int      y,    unsigned mon, unsigned d,
                 unsigned h,    unsigned mi,  unsigned s,
                 unsigned usec, bool a_utc = true) {
            if (a_utc)
                m_tv = long(mktime_utc(y,mon,d, h,mi,s)) * N10e6 + usec;
            else {
                struct tm tm = { int(s),int(mi),int(h), int(d), int(mon)-1, y-1900, 0,0,-1 };
                m_tv = long(mktime(&tm)) * N10e6 + usec;
            }
        }

        explicit time_val(boost::posix_time::ptime a) { *this = a; }

        #if __cplusplus >= 201103L
        time_val(time_val&& a) : m_tv(a.m_tv) {}

        template <class Clock, class Duration = typename Clock::Duration>
        time_val(const std::chrono::time_point<Clock, Duration>& a_tp) {
            using std::chrono::microseconds;
            using std::chrono::seconds;
            using std::chrono::duration_cast;

            auto duration = a_tp.time_since_epoch();
            auto sec      = duration_cast<seconds>(duration);
            auto usec     = duration_cast<microseconds>(duration - sec);
            m_tv          = long(sec.count()) * N10e6 + usec.count();
        }

        std::tuple<int, unsigned, unsigned> to_ymd(bool a_utc = true) const {
            struct tm tm;
            time_t s = sec();
            if (a_utc) gmtime_r   (&s, &tm);
            else       localtime_r(&s, &tm);
            return std::make_tuple(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
        }

        /// Split time into hours/minutes/seconds
        static std::tuple<unsigned, unsigned, unsigned> to_hms(time_t a_time) {
            long     n = a_time /   86400;  // Avoid more expensive '%'
            unsigned s = a_time - n*86400;
            unsigned h = s / 3600; s -= h * 3600;
            unsigned m = s / 60;   s -= m * 60;
            return std::make_tuple(h, m, s);
        }

        /// Split time into hours/minutes/seconds
        std::tuple<unsigned, unsigned, unsigned> to_hms() const { return to_hms(sec()); }

        std::tuple<int, unsigned, unsigned, unsigned, unsigned, unsigned>
        to_ymdhms(bool a_utc = true) const {
            struct tm tm;
            time_t s = sec();
            if (a_utc) gmtime_r   (&s, &tm);
            else       localtime_r(&s, &tm);
            return std::make_tuple(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                                   tm.tm_hour,      tm.tm_min,   tm.tm_sec);
        }

        #endif

        struct timeval          timeval()  const {
            struct timeval tv; copy_to(tv); return tv;
        }
        struct timespec         timespec() const {
            auto pair = split();
            return (struct timespec){pair.first, pair.second*1000};
        }
        time_t   sec()                     const { return m_tv / N10e6;  }
        long     usec()                    const { return m_tv - (m_tv / N10e6) * N10e6; }
        time_t   msec()                    const { long x=m_tv/1000; return x - (x/1000)*1000;}
        long     nsec()                    const { return usec() * 1000; }

        long     value()                   const { return m_tv; }
        long     microseconds()            const { return m_tv; }
        double   seconds()                 const { return double(m_tv) / N10e6; }
        long     milliseconds()            const { return m_tv / 1000; }
        long     nanoseconds()             const { return m_tv * 1000; }

        void sec (long s)               { m_tv = s * N10e6 + usec(); }
        void usec(long us)              { m_tv = sec() * N10e6 + us; }
        void microseconds(uint64_t us)  { m_tv = us; }
        void milliseconds(size_t   ms)  { m_tv = ms * 1000; }
        void nanosec     (uint64_t ns)  { m_tv = ns / 1000; }
        bool empty()              const { return m_tv == 0; }

        void clear()                    { m_tv = 0; }

        void set(time_t a_sec)          { m_tv = a_sec * N10e6; }

        void set(time_val tv, long _s=0, long _us=0) { m_tv = tv.add(_s, _us).m_tv; }

        void set(const struct timeval& tv, long _s=0, long _us=0) {
            m_tv = long(tv.tv_sec + _s)*N10e6 + tv.tv_usec + _us;
        }
        void set(time_val tv, double interval) { m_tv = tv.m_tv + interval * N10e6; }

        std::pair<long, long> split() const {
            long s  = m_tv / N10e6;
            long us = m_tv - s * N10e6;
            return std::make_pair(s, us);
        }
        void copy_to(struct timeval& tv) const {
            auto pair = split();
            tv.tv_sec = pair.first; tv.tv_usec = pair.second;
        }

        double  diff(time_val t)      const { return double(m_tv - t.m_tv) / N10e6; }
        int64_t diff_usec(time_val t) const { return m_tv - t.m_tv; }
        int64_t diff_msec(time_val t) const { return (m_tv - t.m_tv) / 1000; }

        time_val& add(long _s, long _us)    { m_tv += _s*N10e6 + _us; return *this; }
        time_val  add(long _s, long _us) const { return time_val(*this, _s, _us);   }

        void add(double interval)           { m_tv += long(round(interval * 1e6));  }

        time_val& add_sec (long s)          { m_tv += s * N10e6; return *this; }
        time_val  add_sec (long s)    const { return time_val(*this, s);       }
        time_val  add_msec(long ms)   const { return time_val(0, ms*1000);     }
        time_val& add_msec(long ms)         { m_tv += ms*1000;   return *this; }

        time_val& add_usec(long us)         { m_tv += us;        return *this; }
        time_val  add_usec(long us)   const { return   time_val(*this, 0, us); }

        void now()                          { m_tv = universal_time().m_tv; }

        time_val now(long add_s)      const {
            struct timeval tv; ::gettimeofday(&tv, 0);
            return time_val(tv.tv_sec + add_s, tv.tv_usec);
        }

        time_val now(long add_s, long add_us) const {
            struct timeval tv; ::gettimeofday(&tv, 0);
            return time_val(tv.tv_sec + add_s, tv.tv_usec + add_us);
        }

        static time_val universal_time() {
            struct timeval tv; ::gettimeofday(&tv, 0); return time_val(tv);
        }

        /// Construct a time_val from UTC "y/m/d-H:M:S"
        static time_val universal_time(int year, int month, int day,
                                       int hour, int min,   int sec, int usec = 0) {
            static __thread int    s_y, s_m, s_d;
            static __thread time_t s_ymd;

            if (year != s_y || month != s_m || day != s_d) {
                s_ymd = mktime_utc(year, month, day);
                s_y   = year;
                s_m   = month;
                s_d   = day;
            }

            return time_val((s_ymd + hour*3600 + min*60 + sec), usec);
        }

        /// Construct a time_val from local "y/m/d-H:M:S"
        static time_val local_time(int year, int month, int day,
                                   int hour, int min,   int sec, int usec = 0) {
            static __thread int    s_y, s_m, s_d;
            static __thread time_t s_ymd;

            if (year != s_y || month != s_m || day != s_d) {
                struct tm tm = {0, 0, 0, day, month-1, year-1900, 0, 0, -1};
                s_ymd = ::mktime(&tm); // Returns local time
                s_y   = year;
                s_m   = month;
                s_d   = day;
            }

            return time_val((s_ymd + hour*3600 + min*60 + sec), usec);
        }

        static double now_diff(time_val start) {
            return double((universal_time() - start).m_tv) / 1e6;
        }
        static long now_diff_usec(time_val start)  { return universal_time().m_tv - start.m_tv;}
        static long now_diff_msec(time_val start)  { return now_diff_usec(start)/1000; }

        time_val operator- (time_val tv)     const { return usecs(m_tv - tv.m_tv);     }
        time_val operator+ (time_val tv)     const { return usecs(m_tv + tv.m_tv);     }

        time_val operator- (usecs us)        const { return usecs(m_tv - us.usec);     }
        time_val operator+ (usecs us)        const { return usecs(m_tv + us.usec);     }
        time_val operator- (secs  s)         const { return usecs(m_tv - s.usec);      }
        time_val operator+ (secs  s)         const { return usecs(m_tv + s.usec);      }

        time_val operator- (double interval) const { return operator-(secs(interval)); }
        time_val operator+ (double interval) const { return operator+(secs(interval)); }

        time_val operator- (const struct timeval& tv) const { return operator-(time_val(tv)); }
        time_val operator+ (const struct timeval& tv) const { return operator+(time_val(tv)); }

        void     operator-= (const struct timeval& tv)      { m_tv -= time_val(tv).m_tv;}
        void     operator-= (time_val tv)                   { m_tv -= tv.value(); }
        void     operator-= (usecs     v)                   { m_tv -= v.usec;     }
        void     operator-= (secs      v)                   { m_tv -= v.usec;     }
        void     operator+= (time_val tv)                   { m_tv += tv.value(); }
        void     operator+= (usecs     v)                   { m_tv += v.usec;     }
        void     operator+= (secs      v)                   { m_tv += v.usec;     }
        void     operator+= (double interval)               { add(interval);      }

        time_val& operator= (time_val t)                    { m_tv = t.value(); return *this;}
        time_val& operator= (const struct timeval& t)       { m_tv = time_val(t).m_tv; return *this; }

        void operator= (boost::posix_time::ptime a_rhs) {
            using namespace boost::posix_time;
            static const ptime epoch(boost::gregorian::date(1970,1,1));
            time_duration diff = a_rhs - epoch;
            m_tv = diff.total_microseconds();
        }

        bool operator== (time_val tv) const { return m_tv == tv.value(); }
        bool operator!= (time_val tv) const { return m_tv != tv.value(); }
        bool operator>  (time_val tv) const { return m_tv >  tv.value(); }
        bool operator>= (time_val tv) const { return m_tv >= tv.value(); }
        bool operator<  (time_val tv) const { return m_tv <  tv.value(); }
        bool operator<= (time_val tv) const { return m_tv <= tv.value(); }
    };

    template <typename T>
    time_val& time_val_cast(T& tv) {
        BOOST_STATIC_ASSERT(sizeof(T) == sizeof(time_val));
        return reinterpret_cast<time_val&>(tv);
    }

    template <typename T>
    const time_val& time_val_cast(const T& tv) {
        BOOST_STATIC_ASSERT(sizeof(T) == sizeof(time_val));
        return reinterpret_cast<const time_val&>(tv);
    }

    /// Same as gettimeofday() call
    inline time_val now_utc() { return time_val::universal_time(); }

    /// Convert time_val to boost::posix_time::ptime
    inline boost::posix_time::ptime
    to_ptime (time_val a_tv) {
        auto pair = a_tv.split();
        return boost::posix_time::from_time_t(pair.first)
             + boost::posix_time::microsec   (pair.second);
    }

#if __cplusplus >= 201103L
    /// Convert time_val to chrono::time_point
    inline std::chrono::time_point<std::chrono::system_clock>
    to_time_point(time_val a_tv) {
        auto pair = a_tv.split();
        return std::chrono::system_clock::from_time_t(pair.first)
             + std::chrono::microseconds(pair.second);
    }

    /// Convert std::chrono::time_point to timespec
    template <class Clock, class Duration = typename Clock::Duration>
    struct timespec to_timespec(const std::chrono::time_point<Clock, Duration>& a_tp) {
        using std::chrono::nanoseconds;
        using std::chrono::seconds;
        using std::chrono::duration_cast;

        auto duration = a_tp.time_since_epoch();
        auto sec      = duration_cast<seconds>(duration);
        auto nanosec  = duration_cast<nanoseconds>(duration - sec);
        return timespec{sec.count(), nanosec.count()}; 
    }
#endif

    /// Simple timer for measuring interval of time
    ///
    /// Example usage1:
    /// \code
    ///     timer t; do_something(); double elapsed = t.elapsed();
    /// \endcode
    /// Example usage2:
    /// \code
    ///     time_val time;
    ///     {
    ///         timer t(time);
    ///         do_something();
    ///     }
    ///     double elapsed = time.seconds();
    /// \endcode
    class timer {
        time_val  m_started;
        time_val  m_elapsed;
        time_val* m_result;

        void check_stop() { if (m_elapsed.empty()) stop(); }
    public:
        timer() : m_started(time_val::universal_time()), m_result(NULL) {}

        explicit timer(time_val& tv)
            : m_started(time_val::universal_time()), m_result(&tv)
        {}

        ~timer() {
            if (m_result) { check_stop(); *m_result = m_elapsed; }
        }

        void   stop()  { m_elapsed = time_val::universal_time() - m_started;         }
        void   reset() { m_elapsed.clear(); m_started = time_val::universal_time();  }

        const time_val& elapsed_time()      { check_stop(); return m_elapsed;  }
        /// Returns elapsed seconds (with fractional usec)
        double elapsed()                    { check_stop();
                                              return m_elapsed.seconds();      }
        double elapsed_msec()               { return elapsed() * 1000;         }
        double elapsed_usec()               { return elapsed() * 1000000;      }

        double latency_usec(size_t a_count) { return elapsed_usec() / a_count; }
        double latency_msec(size_t a_count) { return elapsed()*1000 / a_count; }
        double latency_sec (size_t a_count) { return elapsed()      / a_count; }

        /// Given number of iterations \a a_count returns iterations per second
        double speed(size_t a_count)        { return double(a_count)/elapsed();}
    };

} // namespace utxx

#endif // _UTXX_TIME_VAL_HPP_
