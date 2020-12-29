//----------------------------------------------------------------------------
/// \file   rate_throttler.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Efficiently calculates the throttling rate over a number of seconds.
///
/// The algorithm implements a variation of token bucket algorithm that
/// doesn't require to add tokens to the bucket on a timer but rather it
/// maintains a circular buffer of tokens with resolution of 1/BucketsPerSec.
/// The basic_rate_throttler::add() function is used to adds items to a bucket
/// associated with the timestamp passed as the first argument to the
/// function.  The throttler::running_sum() returns the total number of
/// items over the given interval of seconds.
/// \note See also this article on a throttling algorithm
/// http://www.devquotes.com/2010/11/24/an-efficient-network-throttling-algorithm.
/// \note Another article on rate limiting
/// http://www.pennedobjects.com/2010/10/better-rate-limiting-with-dot-net.
//----------------------------------------------------------------------------
// Created: 2011-01-20
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

#pragma once

#include <utxx/error.hpp>
#include <utxx/meta.hpp>
#include <utxx/time_val.hpp>
#include <utxx/compiler_hints.hpp>
#include <time.h>

namespace utxx {

/// @brief Throttle given rate over a number of seconds.
/// Implementation uses time spacing reservation algorithm where each
/// allocation of samples reserves a fraction of space in the throttling
/// window. The reservation gets freed as the time goes by.  No more than
/// the "rate()" number of samples are allowed to fit in the "window_msec()"
/// window.
template <typename T = uint32_t>
class basic_time_spacing_throttle {
public:
    basic_time_spacing_throttle(T a_rate, uint32_t a_window_msec = 1000,
                                time_val a_now = now_utc())
        : m_rate        (a_rate)
        , m_window_ns   (long(a_window_msec) * 1000000)
        , m_step_ns     (a_rate == 0 ? 0 : m_window_ns / m_rate)
        , m_next_time   (a_now)
    {
        assert(a_rate >= 0);
    }

    void init(T a_rate, uint32_t a_window_msec = 1000, time_val a_now = now_utc()) {
        new (this) basic_time_spacing_throttle(a_rate, a_window_msec, a_now);
    }

    /// Reset the throttle request counter
    void reset(time_val a_now = now_utc()) {
        m_next_time = a_now;
    }

    /// Add \a a_samples to the throtlle's counter.
    /// @return number of samples that fit in the throttling window. 0 means
    /// that the throttler is fully congested, and more time needs to elapse
    /// before the throttles gets reset to accept more samples.
    T add(T a_samples  = 1, time_val a_now = now_utc()) {
        if (m_rate == 0) return a_samples;
        auto next_time = m_next_time;
        next_time.add_nsec(a_samples * m_step_ns);
        auto now_next  = a_now + nsecs(m_window_ns);
        auto diff      = next_time.nanoseconds() - now_next.nanoseconds();
        if  (diff < -m_window_ns) {
            m_next_time = a_now + nsecs(m_step_ns);
            return a_samples;
        } else if (diff < 0)  {
            // All samples fit the throttling threshold
            m_next_time = next_time;
            return a_samples;
        }

        // Figure out how many samples fit the throttling threshold
        auto n = std::max<T>(0, a_samples - (T(diff) / m_step_ns));
        m_next_time.add_nsec(n * m_step_ns);
        return n;
    }

    T        rate()        const { return m_rate;                }
    long     step_msec()   const { return m_step_ns   / 1000000; }
    long     step_usec()   const { return m_step_ns   / 1000;    }
    long     window_msec() const { return m_window_ns / 1000000; }
    long     window_usec() const { return m_window_ns / 1000;    }
    time_val next_time()   const { return m_next_time;           }

    /// Return the number of available samples given \a a_now current time.
    T        available(time_val a_now = now_utc()) const {
        if (m_rate == 0)
            return std::numeric_limits<T>::max();
        return calc_available(a_now);
    }

    /// Return the number of used samples given \a a_now current time.
    T        used(time_val a_now=now_utc()) const {
        return m_rate==0 ? 0 : m_rate-calc_available(a_now);
    }

    /// Return currently used rate per second.
    double   curr_rate_per_second(time_val a_now=now_utc()) const {
        return m_rate==0 ? 0 : (double)(m_rate-calc_available(a_now))*1'000'000'000/m_window_ns;
    }

private:
    T        m_rate;
    long     m_window_ns;
    long     m_step_ns;
    time_val m_next_time;

    /// Return the number of available samples given \a a_now current time.
    T        calc_available(time_val a_now = now_utc()) const {
        assert(m_rate != 0);
        auto diff = (a_now - m_next_time).nanoseconds();
        auto res  = diff >= 0
                  ? m_rate : T(std::min<T>(m_rate, std::max<T>(0, (m_window_ns+diff) / m_step_ns)));
        assert(res >= 0 && res <= m_rate);
        return res;
    }
};

using time_spacing_throttle = basic_time_spacing_throttle<>;

/**
 * \brief Efficiently calculates the throttling rate over a number of seconds.
 * The algorithm implements a variation of token bucket algorithm that
 * doesn't require to add tokens to the bucket on a timer but rather it
 * maintains a cirtular buffer of tokens with resolution of 1/BucketsPerSec.
 * The basic_rate_throttler::add() function is used to adds items to a bucket
 * associated with the timestamp passed as the first argument to the
 * function.  The throttler::running_sum() returns the total number of
 * items over the given interval of seconds. The items automatically expire
 * when the time moves on the successive invocations of the add() method.
 * @tparam MaxSeconds defines the max number of seconds of data to hold
 *          in the circuling buffer.
 * @tparam BucketsPerSec defines the number of bucket slots per second.
 *          The larger the number the more accurate the running sum will be.
 */
template<size_t MaxSeconds = 16, size_t BucketsPerSec = 2>
class basic_rate_throttler {
public:
    static const size_t s_max_seconds       = upper_power<MaxSeconds,2>::value;
    static const size_t s_buckets_per_sec   = upper_power<BucketsPerSec,2>::value;
    static const size_t s_log_buckets_sec   = log<s_buckets_per_sec,2>::value;
    static const size_t s_bucket_count      = s_max_seconds * s_buckets_per_sec;
    static const size_t s_bucket_mask       = s_bucket_count - 1;

    explicit basic_rate_throttler(int a_interval = 1)
        : m_interval(-1)
    {
        init(a_interval);
    }

    basic_rate_throttler           (const basic_rate_throttler&) = default;
    basic_rate_throttler           (basic_rate_throttler&&)      = default;
    basic_rate_throttler& operator=(const basic_rate_throttler&) = default;

    /// Initialize the internal buffer setting the throttling interval
    /// measured in seconds.
    void init(int a_throttle_interval) {
        if (a_throttle_interval == m_interval)
            return;
        m_interval = a_throttle_interval << s_log_buckets_sec;
        if (a_throttle_interval < 0 ||
            (size_t)a_throttle_interval > s_bucket_count / s_buckets_per_sec)
            throw badarg_error("Invalid throttle interval:", a_throttle_interval);
        reset();
    }

    /// Reset the inturnal circular buffer.
    void reset() {
        memset(m_buckets, 0, sizeof(m_buckets));
        m_last_time     = 0;
        m_sum           = 0;
    }

    /// Return running interval.
    long    interval()     const { return m_interval >> s_log_buckets_sec; }
    /// Return current running sum over the interval.
    long    running_sum()  const { return m_sum; }
    /// Return currently used rate over the interval.
    double  curr_rate()    const { return (double)m_sum / (m_interval >> s_log_buckets_sec); }
    [[deprecated]]
    double  running_avg()  const { return curr_rate(); }

    /// Add \a a_count number of items to the bucket associated with \a a_time.
    /// @param a_time is monotonically increasing time value.
    /// @param a_count is the count to add to the bucket associated with \a a_time.
    /// @return current running sum.
    long   add(time_val a_time, int a_count = 1);

    /// Update current timestamp.
    long   refresh(time_val a_time) { return add(a_time, 0); }

    /// Dump the internal state to stream
    void   dump(std::ostream& out, time_val a_time);
private:
    size_t  m_buckets[s_bucket_count];
    time_t  m_last_time;
    long    m_sum;
    long    m_interval;

    // must be power of two
    BOOST_STATIC_ASSERT((s_bucket_count & (s_bucket_count - 1)) == 0);
};

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
template<size_t MaxSeconds, size_t BucketsPerSec>
long basic_rate_throttler<MaxSeconds, BucketsPerSec>::
add(time_val a_time, int a_count)
{
    time_t l_now = (time_t)(a_time.seconds() * s_buckets_per_sec);
    if (unlikely(!m_last_time))
       m_last_time = l_now;
    int  l_bucket    = l_now & s_bucket_mask;
    long l_time_diff = l_now - m_last_time;
    if (unlikely(l_now < m_last_time)) {
        // Clock was adjusted
        m_buckets[l_bucket] = a_count;
        m_sum = a_count;
    } else if (!l_time_diff) {
        m_sum += a_count;
        m_buckets[l_bucket] += a_count;
    } else if (l_time_diff >= m_interval) {
        int l_start = (l_now - m_interval + 1) & s_bucket_mask;
        int l_end   = l_now & s_bucket_mask;
        for (int i = l_start; i != l_end; i = (i+1) & s_bucket_mask)
            m_buckets[i] = 0;
        m_buckets[l_bucket] = a_count;
        m_sum = a_count;
    } else {
        // Calculate sum of buckets from
        int l_valid_buckets = m_interval - l_time_diff;
        int l_start, l_end;
        if (l_valid_buckets <= (m_interval>>1)) {
            l_start = (l_now - m_interval + 1) & s_bucket_mask;
            l_end   = (m_last_time+1) & s_bucket_mask;
            m_sum = a_count;
            #ifdef UTXX_THROTTLE_DEBUG
            printf("Summing %d through %d\n", l_start, l_end);
            #endif
            for (int i = l_start; i != l_end; i = (i+1) & s_bucket_mask) {
                m_sum += m_buckets[i];
            }
            l_start = l_end;
            l_end   = l_now & s_bucket_mask;
        } else {
            l_start = (m_last_time - m_interval + 1) & s_bucket_mask;
            l_end   = (l_now - m_interval + 1) & s_bucket_mask;
            #ifdef UTXX_THROTTLE_DEBUG
            printf("Subtracting/resetting %d through %d\n", l_start, l_end);
            #endif
            for (int i = l_start; i != l_end; i = (i+1) & s_bucket_mask) {
                m_sum -= m_buckets[i];
                // BOOST_ASSERT(m_sum >= 0);
                if (m_sum < 0) {
                    m_sum = 0;
                    #ifdef UTXX_THROTTLE_DEBUG
                    std::cerr << "ERROR " << __FILE__ << ':' << __LINE__ << std::endl;
                    dump(std::cerr, a_time);
                    #endif
                }
                m_buckets[i] = 0;
            }
            l_start = (m_last_time + 1) & s_bucket_mask;
            l_end   = l_now & s_bucket_mask;
            m_sum += a_count;
        }
        #ifdef UTXX_THROTTLE_DEBUG
        printf("Resetting %d through %d\n", l_start, l_end);
        #endif
        // Reset values in intermediate buckets since there was no activity there
        for (int i = l_start; i != l_end; i = (i+1) & s_bucket_mask)
            m_buckets[i] = 0;
        m_buckets[l_bucket] = a_count;
    }
    m_last_time = l_now;
    #ifdef UTXX_THROTTLE_DEBUG
    dump(std::cout);
    #endif

    return m_sum;
}

template<size_t MaxSeconds, size_t BucketsPerSec>
void basic_rate_throttler<MaxSeconds, BucketsPerSec>::
dump(std::ostream& out, time_val a_time)
{
    time_t l_now    = (time_t)(a_time.seconds() * s_buckets_per_sec);
    size_t l_bucket = l_now & s_bucket_mask;

    char buf[16384];
    std::stringstream s;
    sprintf(buf, "last_time=%ld, last_bucket=%3zu, sum=%ld (interval=%ld)\n",
            m_last_time, l_bucket, m_sum, m_interval);
    s << buf;
    int k = 0;
    size_t n = (l_bucket-m_interval) & s_bucket_mask;
    for (size_t j=0; j < s_bucket_count && k < int(sizeof(buf)); j++) {
        k += snprintf(buf+k, sizeof(buf)-k, "%3zu%c", j, (j == l_bucket || j == n) ? '|' : ' ');
        k += snprintf(buf+k, sizeof(buf)-k, "%s", (j < s_bucket_count-1) ? "" : "\n");
    }
    s << buf;
    k = 0;
    for (size_t j=0; j < s_bucket_count && k < int(sizeof(buf)); j++) {
        k += snprintf(buf+k, sizeof(buf)-k, "%3zu%c", m_buckets[j], (j == l_bucket || j == n) ? '|' : ' ');
        k += snprintf(buf+k, sizeof(buf)-k, "%s", (j < s_bucket_count-1) ? "" : "\n");
    }
    s << buf;
    out << s.str();
}

} // namespace utxx
