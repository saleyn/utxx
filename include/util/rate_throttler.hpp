//----------------------------------------------------------------------------
/// \file throttler.hpp
//----------------------------------------------------------------------------
/// \brief Efficiently calculates the throttling rate over a number of seconds.
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
// Copyright (c) 2010 Omnibius, LLC
// Author:  Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-01-20
//----------------------------------------------------------------------------
// License: see attached LICENSE-PROTRADE file for licensing information.
//----------------------------------------------------------------------------

#ifndef _UTIL_THROTTLER_HPP_
#define _UTIL_THROTTLER_HPP_

#include <util/error.hpp>
#include <util/meta.hpp>
#include <util/compiler_hints.hpp>
#include <time.h>

namespace util {

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

    /// Initialize the internal buffer setting the throttling interval
    /// measured in seconds.
    void init(int a_throttle_interval)
        throw(badarg_error)
    {
        if (a_throttle_interval == m_interval)
            return;
        m_interval = a_throttle_interval << s_log_buckets_sec;
        if (a_throttle_interval < 0 || (size_t)a_throttle_interval > s_bucket_count / s_buckets_per_sec)
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
    int     interval()      const { return m_interval >> s_log_buckets_sec; }
    /// Return current running sum over the interval.
    int     running_sum()  const { return m_sum; }
    /// Return current running sum over the interval.
    double  running_avg()  const { return (double)m_sum / (m_interval >> s_log_buckets_sec); }

    /// Add \a a_count number of items to the bucket associated with \a a_time.
    /// @param a_time is monotonically increasing time value.
    /// @param a_count is the count to add to the bucket associated with \a a_time.
    /// @return current running sum.
    size_t add(const timeval& a_time, int a_count = 1);

    /// Update current timestamp.
    size_t refresh(const timeval& a_time) { return add(a_time, 0); }

    /// Dump the internal state to stream
    void dump(std::ostream& out, const timeval& a_time);
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
size_t basic_rate_throttler<MaxSeconds, BucketsPerSec>::
add(const timeval& a_time, int a_count)
{
    time_t l_now = (time_t)( ((double)a_time.tv_sec +
                              (double)a_time.tv_usec / 1000000)
                            * s_buckets_per_sec);
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
            #ifdef THROTTLE_DEBUG
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
            #ifdef THROTTLE_DEBUG
            printf("Subtracting/resetting %d through %d\n", l_start, l_end);
            #endif
            for (int i = l_start; i != l_end; i = (i+1) & s_bucket_mask) {
                m_sum -= m_buckets[i];
                // BOOST_ASSERT(m_sum >= 0);
                if (m_sum < 0) {
                    m_sum = 0;
                    std::cerr << "ERROR " << __FILE__ << ':' << __LINE__ << std::endl;
                    dump(std::cerr, a_time);
                }
                m_buckets[i] = 0;
            }
            l_start = (m_last_time + 1) & s_bucket_mask;
            l_end   = l_now & s_bucket_mask;
            m_sum += a_count;
        }
        #ifdef THROTTLE_DEBUG
        printf("Resetting %d through %d\n", l_start, l_end);
        #endif
        // Reset values in intermediate buckets since there was no activity there
        for (int i = l_start; i != l_end; i = (i+1) & s_bucket_mask)
            m_buckets[i] = 0;
        m_buckets[l_bucket] = a_count;
    }
    m_last_time = l_now;
    #ifdef THROTTLE_DEBUG
    dumpt(std::cout);
    #endif

    return m_sum;
}

template<size_t MaxSeconds, size_t BucketsPerSec>
void basic_rate_throttler<MaxSeconds, BucketsPerSec>::
dump(std::ostream& out, const timeval& a_time)
{
    time_t l_now = (time_t)( ((double)a_time.tv_sec +
                              (double)a_time.tv_usec / 1000000)
                            * s_buckets_per_sec);
    size_t l_bucket = l_now & s_bucket_mask;

    char buf[256];
    std::stringstream s;
    sprintf(buf, "last_time=%ld, last_bucket=%3zu, sum=%ld (interval=%ld)\n",
            m_last_time, l_bucket, m_sum, m_interval);
    s << buf;
    int k = 0;
    size_t n = (l_bucket-m_interval) & s_bucket_mask;
    for (size_t j=0; j < s_bucket_count; j++) {
        k += snprintf(buf+k, sizeof(buf)-k, "%3zu%c", j, (j == l_bucket || j == n) ? '|' : ' ');
        k += snprintf(buf+k, sizeof(buf)-k, "%s", (j < s_bucket_count-1) ? "" : "\n");
    }
    s << buf;
    k = 0;
    for (size_t j=0; j < s_bucket_count; j++) {
        k += snprintf(buf+k, sizeof(buf)-k, "%3zu%c", m_buckets[j], (j == l_bucket || j == n) ? '|' : ' ');
        k += snprintf(buf+k, sizeof(buf)-k, "%s", (j < s_bucket_count-1) ? "" : "\n");
    }
    s << buf;
    out << s.str();
}

} // namespace util

#endif // _UTIL_THROTTLER_HPP_
