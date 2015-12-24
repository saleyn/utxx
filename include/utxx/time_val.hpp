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
#pragma once

#include <boost/static_assert.hpp>
#include <utxx/detail/gettimeofday.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/time.hpp>
#include <cstddef>
#include <stdint.h>
#include <type_traits>
#include <chrono>
#include <tuple>
#include <ctime>
#include <sys/time.h>
#include <math.h>

namespace utxx {
    class time_val;

    // See timestamp::parse_stamp_type()
    enum stamp_type {
          NO_TIMESTAMP              // Parsed from "none"
        , DATE                      // Parsed from "date"
        , DATE_TIME                 // Parsed from "date-time"
        , DATE_TIME_WITH_MSEC       // Parsed from "date-time-msec"
        , DATE_TIME_WITH_USEC       // Parsed from "date-time-usec"
        , TIME                      // Parsed from "time"
        , TIME_WITH_MSEC            // Parsed from "time-msec"
        , TIME_WITH_USEC            // Parsed from "time-usec"
    };

    /// Indication of use of absolute time
    struct abs_time {
        long nsec;
        abs_time(long s=0, long us=0) : nsec(s*1000000000L + us*1000L) {}
    };
    /// Indication of use of relative time to now
    struct rel_time {
        long nsec;
        rel_time(long s=0, long us=0) : nsec(s*1000000000L + us*1000L) {}
    };

    struct msecs {
        explicit msecs(long   ms) : m_nsec(ms*1000000L)         {}
        explicit msecs(size_t ms) : m_nsec(long(ms)*1000000L)   {}
        explicit msecs(int    ms) : m_nsec(long(ms)*1000000L)   {}
        long     value() const { return m_nsec/1000000L;   }
        long     nsec()  const { return m_nsec;            }
    private:
        long m_nsec;
    };

    struct usecs {
        explicit usecs(long   us) : m_nsec(us*1000L)            {}
        explicit usecs(size_t us) : m_nsec(long(us)*1000L)      {}
        explicit usecs(int    us) : m_nsec(long(us)*1000L)      {}
        long     value() const { return m_nsec/1000L; }
        long     nsec()  const { return m_nsec;       }
    private:
        long m_nsec;
    };

    struct nsecs {
        explicit nsecs(long   ns) : m_nsec(ns)                  {}
        explicit nsecs(size_t ns) : m_nsec(long(ns))            {}
        long     value() const { return m_nsec; }
        long     nsec()  const { return m_nsec; }
    private:
        long m_nsec;
    };

    struct secs {
        explicit secs(size_t   s) : m_nsec(long(s)*1000000000L) {}
        explicit secs(long     s) : m_nsec(s * 1000000000L)     {}
        explicit secs(int      s) : m_nsec(long(s)*1000000000L) {}
        explicit secs(double   s) : m_nsec(long(s*1e9+0.5))     {}
        long     value() const { return m_nsec/1000000000L; }
        long     nsec()  const { return m_nsec;             }
    private:
        long m_nsec;
    };

    namespace detail {
        /// Print right-aligned integer to string buffer with leading 0's
        inline char* itoar(size_t a_val, char* a_buf, int a_sz) {
            auto e = a_buf+a_sz;
            for(auto p = e-1; p >= a_buf; --p, a_val /= 10u)
                *p = a_val ? ('0' + (a_val % 10u)) : '0';
            return e;
        }

        inline std::string itoar(size_t a_val, int a_width) {
            char buf[64]; auto p = itoar(a_val, buf, std::min<int>(sizeof(buf),a_width));
            return std::string(buf, p-buf);
        }
    }

    /// A helper class for dealing with 'struct timeval' structure. This class adds ability
    /// to perform arithmetic with the structure leaving the same footprint.
    class time_val
    {
        static const long   N10e6 = 1000000;
        static const size_t N10e9 = 1000000000u;
        int64_t             m_tv;

    public:
        time_val() noexcept         : m_tv(0)                  {}
        time_val(secs   s)          : m_tv(s.nsec())           {}
        time_val(msecs ms)          : m_tv(ms.nsec())          {}
        time_val(usecs us)          : m_tv(us.nsec())          {}
        time_val(nsecs ns)          : m_tv(ns.nsec())          {}
        time_val(long s, long us)   : m_tv(s*N10e9 + us*1000)  {}
        time_val(time_val tv, long s)          : m_tv(tv.m_tv + s*N10e9)           {}
        time_val(time_val tv, long s, long us) : m_tv(tv.m_tv + s*N10e9 + us*1000) {}
        time_val(time_val tv, double interval) { set(tv, interval); }

        explicit time_val(const struct timeval&  a) : m_tv(long(a.tv_sec)*N10e9 + long(a.tv_usec)*1000){}
        explicit time_val(const struct timespec& a) : m_tv(long(a.tv_sec)*N10e9 + a.tv_nsec){}
        explicit time_val(struct tm& a_tm)          : m_tv(long(mktime(&a_tm))*N10e9)       {}

        /// Set time to abosolute time given by \a a
        explicit time_val(abs_time a) : m_tv(a.nsec) {}

        /// Set time to relative offset from now given by \a a time.
        explicit time_val(rel_time a) : m_tv(universal_time().m_tv + a.nsec) {}

        time_val(int y, unsigned m, unsigned d, bool a_utc = true) {
            if (a_utc)
                m_tv = long(mktime_utc(y, m, d)) * N10e9;
            else {
                struct tm tm = { 0,0,0, int(d), int(m)-1, y-1900, 0,0,-1 };
                m_tv = long(mktime(&tm)) * N10e9;
            }
        }

        time_val(int      y,    unsigned mon, unsigned d,
                 unsigned h,    unsigned mi,  unsigned s,
                 unsigned usec, bool a_utc = true) {
            if (a_utc)
                m_tv = long(mktime_utc(y,mon,d, h,mi,s)) * N10e9 + long(usec)*1000;
            else {
                struct tm tm = { int(s),int(mi),int(h), int(d), int(mon)-1, y-1900, 0,0,-1 };
                m_tv = long(mktime(&tm)) * N10e9 + long(usec)*1000;
            }
        }

        explicit time_val(boost::posix_time::ptime a) { *this = a; }

        time_val(const time_val& a)             = default;
        time_val(time_val&& a)                  = default;
        time_val& operator=(const time_val& a)  = default;
        time_val& operator=(time_val&&      a)  = default;

        template <class Clock, class Duration = typename Clock::Duration>
        time_val(const std::chrono::time_point<Clock, Duration>& a_tp) {
            using std::chrono::nanoseconds;
            using std::chrono::seconds;
            using std::chrono::duration_cast;

            auto duration = a_tp.time_since_epoch();
            auto sec      = duration_cast<seconds>(duration);
            auto nsec     = duration_cast<nanoseconds>(duration - sec);
            m_tv          = long(sec.count()) * N10e9 + nsec.count();
        }

        std::tuple<int, unsigned, unsigned> to_ymd(bool a_utc = true) const {
            auto tm = to_tm(a_utc);
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
            auto tm = to_tm(a_utc);
            return std::make_tuple(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                                   tm.tm_hour,      tm.tm_min,   tm.tm_sec);
        }

        /// Convert time to "tm" structure
        template <bool UTC>
        struct tm               to_tm()    const {
            struct tm tm;
            time_t s = sec();

            if (UTC) gmtime_r   (&s, &tm);
            else     localtime_r(&s, &tm);

            return tm;
        }

        struct tm               to_tm(bool a_utc) const {
            return a_utc ? to_tm<true>() : to_tm<false>();
        }

        struct timeval          timeval()  const {
            struct timeval tv; copy_to(tv); return tv;
        }
        struct timespec         timespec() const {
            auto pair = split();
            return ::timespec{pair.first, pair.second};
        }
        time_t   sec()                     const { return m_tv / N10e9; }
        long     usec()                    const { long x=m_tv/1000;  return x-(x/N10e6)*N10e6;}
        long     msec()                    const { long x=m_tv/N10e6; return x-(x/1000)*1000;}
        long     nsec()                    const { return m_tv - (m_tv/N10e9)*N10e9; }

        long     microseconds()            const { return m_tv/1000; }
        double   seconds()                 const { return double(m_tv) / N10e9; }
        long     milliseconds()            const { return m_tv / N10e6; }
        long     nanoseconds()             const { return m_tv; }

        void sec (long s)                { m_tv = s * N10e9 + nsec(); }
        void usec(long us)               { m_tv = sec() * N10e9 + us*1000; }
        void microseconds(int64_t us)    { m_tv = us*1000; }
        void milliseconds(size_t  ms)    { m_tv = ms*N10e6; }
        void nanoseconds (int64_t ns)    { m_tv = ns; }
        void nanosec     (int64_t ns)    { m_tv = ns; }
        bool empty()               const { return m_tv == 0; }

        void clear()                     { m_tv = 0; }

        void set(time_t a_sec)           { m_tv = a_sec * N10e9; }
        void set(time_t a_sec, long a_us){ m_tv = a_sec * N10e9 + a_us * 1000; }

        void set(time_val tv, long _s=0, long _us=0) { m_tv = tv.add(_s, _us).m_tv; }

        void set(const struct timeval& tv, long _s=0, long _us=0) {
            m_tv = long(tv.tv_sec + _s)*N10e9 + tv.tv_usec*1000 + _us*1000;
        }
        void set(time_val tv, double interval) { m_tv = tv.m_tv + interval * N10e9; }

        /// Return time split in seconds and nanoseconds
        std::pair<long, long> split() const {
            long s  = m_tv / N10e9;
            long us = m_tv - s * N10e9;
            return std::make_pair(s, us);
        }
        void copy_to(struct timeval& tv) const {
            auto pair = split();
            tv.tv_sec = pair.first; tv.tv_usec = pair.second/1000;
        }

        double  diff(time_val t)      const { return double(m_tv - t.m_tv) / N10e9; }
        int64_t diff_nsec(time_val t) const { return (m_tv - t.m_tv);               }
        int64_t diff_usec(time_val t) const { return (m_tv - t.m_tv) / 1000;        }
        int64_t diff_msec(time_val t) const { return (m_tv - t.m_tv) / N10e6;       }

        time_val& add(long s, long us)      { m_tv += s*N10e9 + us*1000; return *this; }
        time_val  add(long s, long us)const { return time_val(*this, s, us);        }

        void add(double interval)           { m_tv += long(round(interval * 1e9));  }

        time_val& add_sec (long s)          { m_tv += s * N10e9; return *this; }
        time_val  add_sec (long s)    const { return time_val(*this, s);       }
        time_val  add_msec(long ms)   const { return nsecs(m_tv + ms*N10e6);   }
        time_val& add_msec(long ms)         { m_tv += ms*N10e6;  return *this; }

        time_val& add_usec(long us)         { m_tv += us*1000;   return *this; }
        time_val  add_usec(long us)   const { return   nsecs(m_tv + us*1000);  }

        time_val& add_nsec(long ns)         { m_tv += ns;        return *this; }
        time_val  add_nsec(long ns)   const { return   nsecs(m_tv + ns);       }

        void now()                          { m_tv = universal_time().m_tv; }

        time_val now(long add_s)      const {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += add_s;
            return time_val(ts);
        }

        time_val now(long add_s, long add_us) const {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec  += add_s;
            ts.tv_nsec += add_us*1000;
            return time_val(ts);
        }

        static time_val universal_time() {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            return time_val(ts);
        }

        /// Construct a time_val from UTC "y/m/d-H:M:S"
        static time_val universal_time(int year, int month, int day,
                                       int hour, int min,   int sec, int usec = 0) {
            static thread_local int    s_y, s_m, s_d;
            static thread_local time_t s_ymd;

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
            static thread_local int    s_y, s_m, s_d;
            static thread_local time_t s_ymd;

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
            return double((universal_time() - start).m_tv) / 1e9;
        }
        static long now_diff_nsec(time_val start)  { return (universal_time().m_tv - start.m_tv);}
        static long now_diff_usec(time_val start)  { return (universal_time().m_tv - start.m_tv)/1000;}
        static long now_diff_msec(time_val start)  { return now_diff_usec(start)/N10e6;}

        time_val operator- (time_val tv)     const { return nsecs(m_tv - tv.m_tv);     }
        time_val operator+ (time_val tv)     const { return nsecs(m_tv + tv.m_tv);     }

        time_val operator- (nsecs a)         const { return nsecs(m_tv - a.nsec());    }
        time_val operator+ (nsecs a)         const { return nsecs(m_tv + a.nsec());    }
        time_val operator- (usecs a)         const { return nsecs(m_tv - a.nsec());    }
        time_val operator+ (usecs a)         const { return nsecs(m_tv + a.nsec());    }
        time_val operator- (msecs a)         const { return nsecs(m_tv - a.nsec());    }
        time_val operator+ (msecs a)         const { return nsecs(m_tv + a.nsec());    }
        time_val operator- (secs  s)         const { return nsecs(m_tv - s.nsec());    }
        time_val operator+ (secs  s)         const { return nsecs(m_tv + s.nsec());    }

        time_val operator- (double interval) const { return operator-(secs(interval)); }
        time_val operator+ (double interval) const { return operator+(secs(interval)); }

        time_val operator- (const struct timeval& tv) const { return operator-(time_val(tv)); }
        time_val operator+ (const struct timeval& tv) const { return operator+(time_val(tv)); }

        void     operator-= (const struct timeval& tv)      { m_tv -= time_val(tv).m_tv;}
        void     operator-= (time_val tv)                   { m_tv -= tv.m_tv;    }
        void     operator-= (nsecs     v)                   { m_tv -= v.nsec();   }
        void     operator-= (usecs     v)                   { m_tv -= v.nsec();   }
        void     operator-= (msecs     v)                   { m_tv -= v.nsec();   }
        void     operator-= (secs      v)                   { m_tv -= v.nsec();   }
        void     operator+= (time_val tv)                   { m_tv += tv.m_tv;    }
        void     operator+= (nsecs     v)                   { m_tv += v.nsec();   }
        void     operator+= (usecs     v)                   { m_tv += v.nsec();   }
        void     operator+= (msecs     v)                   { m_tv += v.nsec();   }
        void     operator+= (secs      v)                   { m_tv += v.nsec();   }
        void     operator+= (double interval)               { add(interval);      }

        time_val& operator= (const struct timeval& t)       { m_tv = time_val(t).m_tv; return *this; }

        void operator= (boost::posix_time::ptime a_rhs) {
            using namespace boost::posix_time;
            static const ptime epoch(boost::gregorian::date(1970,1,1));
            time_duration diff = a_rhs - epoch;
            m_tv = diff.total_nanoseconds();
        }

        bool operator== (time_val tv) const { return m_tv == tv.m_tv;  }
        bool operator!= (time_val tv) const { return m_tv != tv.m_tv;  }
        bool operator>  (time_val tv) const { return m_tv >  tv.m_tv;  }
        bool operator>= (time_val tv) const { return m_tv >= tv.m_tv;  }
        bool operator<  (time_val tv) const { return m_tv <  tv.m_tv;  }
        bool operator<= (time_val tv) const { return m_tv <= tv.m_tv;  }

        bool operator<  (nsecs a)     const { return m_tv <  a.nsec(); }
        bool operator<  (usecs a)     const { return m_tv <  a.nsec(); }
        bool operator<  (msecs a)     const { return m_tv <  a.nsec(); }
        bool operator<  (secs  a)     const { return m_tv <  a.nsec(); }

        bool operator<= (nsecs a)     const { return m_tv <= a.nsec(); }
        bool operator<= (usecs a)     const { return m_tv <= a.nsec(); }
        bool operator<= (msecs a)     const { return m_tv <= a.nsec(); }
        bool operator<= (secs  a)     const { return m_tv <= a.nsec(); }

        bool operator>  (nsecs a)     const { return m_tv >  a.nsec(); }
        bool operator>  (usecs a)     const { return m_tv >  a.nsec(); }
        bool operator>  (msecs a)     const { return m_tv >  a.nsec(); }
        bool operator>  (secs  a)     const { return m_tv >  a.nsec(); }

        bool operator>= (nsecs a)     const { return m_tv >= a.nsec(); }
        bool operator>= (usecs a)     const { return m_tv >= a.nsec(); }
        bool operator>= (msecs a)     const { return m_tv >= a.nsec(); }
        bool operator>= (secs  a)     const { return m_tv >= a.nsec(); }

        /// Returns true if current time_val has value.
        /// This method is explicit to avoid accidental conversion of time_val type to bool
        /// in assignments such as \code time_t a = time_val() \endcode
        explicit operator bool() const { return m_tv; }

        /// NOTE: Streaming support of time_val is provided by timestamp.hpp
        //friend inline std::ostream& operator<< (std::ostream& out, time_val const& a) {
        //    char buf[64]; a.write(buf, TIME_WITH_USEC);
        //    return out << buf;
        //}

        std::string to_string(stamp_type a_tp, char a_ddelim = '\0',
                              char a_tdelim = ':', char a_ssep = '.') const {
            char buf[64];
            auto p = write(buf, a_tp, a_ddelim, a_tdelim, a_ssep);
            return std::string(buf, p - buf);
        }

        char* write(char* a_buf, stamp_type a_tp,
                    char a_ddelim = '\0', char a_tdelim = ':', char a_ssep = '.') const {
            auto pair = split();
            auto p = ((a_tp > NO_TIMESTAMP) && (a_tp < TIME))
                   ? std::make_pair(write_date(pair.first, a_buf, 0, a_ddelim),stamp_type(a_tp+3))
                   : std::make_pair(a_buf, a_tp);
            return write_time(pair, p.first, p.second, a_tdelim, a_ssep);
        }

        /// Write seconds to buffer.
        /// Time zone conversion to seconds must be done externally.
        /// @param a_eos   when this end-of-string position is greater than 0,
        ///                the buffer is terminated with new line at this position
        /// @param a_delim when other than '\0' this value is used as the delimiter
        char* write_date(char* a_buf, size_t a_eos = 8, char a_delim = '\0') const {
            return write_date(sec(), a_buf, a_eos, a_delim);
        }

        static char* write_date(time_t a_sec, char* a_buf, size_t a_eos = 8,
                                char a_sep = '\0') {
            int y; unsigned m,d;
            std::tie(y,m,d) = from_gregorian_time(a_sec);
            int   n = y / 1000;
            char* p = a_buf;
            *p++ = '0' + n; y -= n*1000; n = y/100;
            *p++ = '0' + n; y -= n*100;  n = y/10;
            *p++ = '0' + n; y -= n*10;
            *p++ = '0' + y; n  = m / 10;
            if (a_sep) *p++ = a_sep;
            *p++ = '0' + n; m -= n*10;
            *p++ = '0' + m; n  = d / 10;
            if (a_sep) *p++ = a_sep;
            *p++ = '0' + n; d -= n*10;
            *p++ = '0' + d;
            *p++ = '-';
            if (a_eos) {
                a_buf[a_eos] = '\0';
                char*  end = a_buf + a_eos;
                return end < p ? end : p;
            }
            return p;
        }

        /// Write time as string.
        /// The string is NULL terminated.
        /// Possible formats:
        ///   HHMMSSsss, HHMMSSssssss, HHMMSS.sss, HHMMSS.ssssss, HH:MM:SS[.ssssss].
        /// @param a_buf   the output buffer.
        /// @param a_type  determines the time formatting.
        ///                Valid values: TIME, TIME_WITH_MSEC, TIME_WITH_USEC.
        /// @param a_delim controls the ':' delimiter ('\0' means no delimiter)
        /// @param a_sep   defines the fractional second separating character ('\0' means - none)
        /// @return pointer past the last written character
        char* write_time(char* a_buf, stamp_type a_tp,
                         char a_delim = ':', char a_sep = '.') const {
            return write_time(split(), a_buf, a_tp, a_delim, a_sep);
        }

        static char* write_time(std::pair<long, long> a_tv, char* a_buf,
                                stamp_type a_tp, char a_delim = ':', char a_sep = '.') {
            return write_time(a_tv.first, a_tv.second, a_buf, a_tp, a_delim, a_sep);
        }

        static char* write_time(long a_sec, long a_ns, char* a_buf,
                                stamp_type a_tp, char a_delim = ':', char a_sep = '.')
        {
            char* p = a_buf;
            if (likely(a_tp != NO_TIMESTAMP)) {
                unsigned h,m,s;
                std::tie(h,m,s) = to_hms(a_sec);

                int n = h / 10;
                *p++  = '0' + n; h -= n*10;
                *p++  = '0' + h; n  = m / 10;
                if (a_delim) *p++   = a_delim;
                *p++  = '0' + n; m -= n*10;
                *p++  = '0' + m; n  = s / 10;
                if (a_delim) *p++   = a_delim;
                *p++  = '0' + n; s -= n*10;
                *p++  = '0' + s;
                switch (a_tp) {
                    case TIME:
                        break;
                    case TIME_WITH_MSEC: {
                        if (a_sep) *p++ = a_sep;
                        size_t msec = a_ns / 1000000;
                        p = detail::itoar(msec, p, 3);
                        break;
                    }
                    case TIME_WITH_USEC: {
                        if (a_sep) *p++ = a_sep;
                        size_t usec = a_ns / 1000;
                        p = detail::itoar(usec, p, 6);
                        break;
                    }
                    default:
                        UTXX_THROW_RUNTIME_ERROR
                            ("time_val::write_time: invalid a_type value: ", a_tp);
                }
            }
            *p = '\0';
            return p;
        }
    };

    /// Same as gettimeofday() call
    inline time_val now_utc() { return time_val::universal_time(); }

    /// Convert time_val to boost::posix_time::ptime
    inline boost::posix_time::ptime
    to_ptime (time_val a_tv) {
        long s, ns;
        std::tie(s,ns) = a_tv.split();
        return boost::posix_time::from_time_t(s)
             + boost::posix_time::microsec(ns / 1000);
    }

    /// Convert time_val to chrono::time_point
    inline std::chrono::time_point<std::chrono::system_clock>
    to_time_point(time_val a_tv) {
        auto pair = a_tv.split();
        return std::chrono::system_clock::from_time_t(pair.first)
             + std::chrono::nanoseconds(pair.second);
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
        double elapsed_nsec()               { return elapsed() * 1000000000L;  }

        double latency_nsec(size_t a_count) { return elapsed_nsec() / a_count; }
        double latency_usec(size_t a_count) { return elapsed_usec() / a_count; }
        double latency_msec(size_t a_count) { return elapsed()*1000 / a_count; }
        double latency_sec (size_t a_count) { return elapsed()      / a_count; }

        /// Given number of iterations \a a_count returns iterations per second
        double speed(size_t a_count)        { return double(a_count)/elapsed();}
    };

} // namespace utxx