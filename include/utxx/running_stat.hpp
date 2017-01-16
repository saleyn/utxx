//----------------------------------------------------------------------------
/// \file   running_stat.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// This file implements a class that calculates running mean and
/// standard deviation. 
//----------------------------------------------------------------------------
// Created: 2010-05-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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

#include <stdlib.h>
#include <math.h>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <assert.h>
#include <utxx/detail/running_stat_impl.hpp>
#include <utxx/compiler_hints.hpp>

namespace utxx {

//-----------------------------------------------------------------------------
/// Keep running min/max/average/variance stats.
//-----------------------------------------------------------------------------

/// Running count / sum / min / max statistics
template <typename T, typename CntType = size_t>
class basic_running_sum {
protected:
    CntType m_count;
    T       m_last, m_sum, m_min, m_max;
public:
    basic_running_sum() {
        clear();
    }

    basic_running_sum(const basic_running_sum& rhs) 
        : m_count(rhs.m_count)
        , m_last(rhs.m_last), m_sum(rhs.m_sum)
        , m_min(rhs.m_min),   m_max(rhs.m_max)
    {}

#if __cplusplus >= 201103L
    basic_running_sum(basic_running_sum&& rhs) 
        : m_count(rhs.m_count)
        , m_last(rhs.m_last), m_sum(rhs.m_sum)
        , m_min(rhs.m_min),   m_max(rhs.m_max)
    {}

    basic_running_sum<T, CntType>&
    operator=(basic_running_sum&& a_rhs) {
        m_count = a_rhs.m_count;
        m_last  = a_rhs.m_last;
        m_sum   = a_rhs.m_sum;
        m_min   = a_rhs.m_min;
        m_max   = a_rhs.m_max;
        return *this;
    }
#endif

    /// Reset the internal state.
    void clear() { 
        m_count = 0; m_last = 0; m_sum = 0;
        m_min = std::numeric_limits<T>::max();
        m_max = std::numeric_limits<T>::lowest();
    }

    /// Add a sample measurement.
    inline void add(double x) {
        ++m_count;
        m_last = x;
        m_sum += x;
        if (x > m_max) m_max = x;
        if (x < m_min) m_min = x;
    }

    void operator+= (const basic_running_sum<T, CntType>& a) {
        m_count += a.m_count;
        m_sum   += a.m_sum;
        if (a.m_max > m_max) m_max = a.m_max;
        if (a.m_min < m_min) m_min = a.m_min;
    }

    void operator-= (const basic_running_sum<T, CntType>& a) {
        m_count -= a.count();
        m_sum   -= a.sum();
        //m_min    = a.min();
        //m_max    = a.max();
    }

    /// Number of samples since last invocation of clear().
    CntType     count() const { return m_count;  }
    bool        empty() const { return !m_count; }

    T           last()  const { return m_last; }
    T           sum()   const { return m_sum;  }
    double      mean()  const { return likely(m_count) ? double(m_sum) / m_count : 0.0; }
    T           min()   const { return m_min == std::numeric_limits<T>::max()
                                     ? 0 : m_min; }
    T           max()   const { return m_max == std::numeric_limits<T>::lowest()
                                     ? 0 : m_max; }
};

/// Running variance statistics
template <typename T, typename CntType = size_t>
class basic_running_variance : public basic_running_sum<T, CntType> {
    typedef basic_running_sum<T, CntType> base;
    double  m_var;
public:
    basic_running_variance() {
        clear();
    }

    basic_running_variance(const basic_running_variance& rhs) 
        : base(rhs)
        , m_var(rhs.m_var)
    {}

    /// Reset the internal state.
    void clear() { 
        base::clear();
        m_var = 0.0;
    }

    /// Add a sample measurement.
    inline void add(T x) {
        double old = this->mean();
        base::add(x);
        // See Knuth TAOCP v.2, 3rd ed, p.232
        double diff = x - old;
        if (diff != 0.0)
            m_var += diff * (x - base::mean());
    }

    /// Number of samples since last invocation of clear().
    double   variance()  const { return likely(this->m_count) ? m_var/this->m_count : 0.0; }
    double   deviation() const { return sqrt(variance()); }
};

template <typename T, int N = 0, bool FastMinMax = false>
struct basic_moving_average
    : public detail::minmax_impl<basic_moving_average<T,N,FastMinMax>, T, FastMinMax>
{
    typedef
        detail::minmax_impl<basic_moving_average<T,N,FastMinMax>, T, FastMinMax>
        base;

    friend base;

    explicit basic_moving_average(size_t a_capacity = 0)
        : MASK(N ? N-1 : a_capacity-1)
        , m_data(m_samples)
    {
        #if __cplusplus >= 201103L
        static_assert(!N || (N & (N-1)) == 0, "N must be 0 or power of 2");
        #else
        assert(!N || (N & (N-1)) == 0);
        #endif
        if (!(N ^ a_capacity))
            throw std::logic_error
                ("utxx::basic_moving_average: both static and dynamic capacity is given");
        if (a_capacity) {
            if ((a_capacity & (a_capacity-1)) != 0)
                throw std::invalid_argument
                    ("utxx::basic_moving_average: dynamic capacity must be power of 2");
            m_data = new T[a_capacity];
        }

        clear();
    }

    basic_moving_average(const basic_moving_average& a_rhs)
        : base(a_rhs)
        , MASK  (a_rhs.MASK)
        , m_end (a_rhs.m_end)
        , m_last(a_rhs.m_last)
        , m_sum (a_rhs.m_sum)
    {
        if (a_rhs.m_data == a_rhs.m_samples)
            m_data = m_samples;
        else {
            assert(N == 0);
            m_data = new T[capacity()];
        }

        memcpy(m_data, a_rhs.m_data, sizeof(T)*capacity());
    }

    #if __cplusplus >= 201103L
    basic_moving_average(basic_moving_average&& a_rhs)
        : base(a_rhs)
        , MASK  (a_rhs.MASK)
        , m_end (a_rhs.m_end)
        , m_last(a_rhs.m_last)
        , m_sum (a_rhs.m_sum)
    {
        if (a_rhs.m_data != a_rhs.m_samples) {
            assert(N == 0);
            m_data = a_rhs.m_data;
        } else {
            m_data = m_samples;
            memcpy(m_samples, a_rhs.m_samples, sizeof(T)*capacity());
        }
        // Reset the data pointer so that it's not 
        a_rhs.m_data = nullptr;
        a_rhs.m_sum  = 0;
        a_rhs.m_last = 0;
        a_rhs.m_end  = 0;
    }
    #endif

    ~basic_moving_average() {
        if (m_data != m_samples)
            delete [] m_data;
    }

    void add(T sample) {
        m_last    = sample;
        T& oldest = m_data[m_end & MASK];
        m_sum    += sample;

        if (m_end > MASK)
           m_sum -= oldest;

        this->update_minmax(sample);

        oldest = sample;

        ++m_end;
    }

    void clear() {
        base::clear();
        if (m_data)
            memset(m_data, 0, capacity()*sizeof(T));
        m_sum  = 0;
        m_last = 0;
        m_end  = 0;
    }

    bool     empty()    const { return m_end == 0; }
    /// Capacity of the window
    size_t   capacity() const { return MASK+1; }
    /// Number of windowed items (<= capacity())
    size_t   size()     const { return m_end > MASK  ? capacity() : m_end; }
    /// Total number of samples processed since initialization
    size_t   total()    const { return m_end; }

    /// Access to raw samples
    const T* samples()  const { return m_data; }

    double   mean()     const { return likely(m_end) ? double(m_sum) / size() : 0.0; }
    T        sum()      const { return m_sum;  }
    T        last()     const { return m_last; }

    std::pair<T,T> minmax() const {
        if (unlikely(empty()))
            return std::make_pair
                (std::numeric_limits<T>::max(), std::numeric_limits<T>::min());
        return base::minmax();
    }

    T min() const { return base::min(); }
    T max() const { return base::max(); }

protected:
    template <typename D, typename U, bool F> friend class detail::minmax_impl;

    size_t  begin_idx()        const { return m_end - size(); }
    size_t  end_idx()          const { return m_end;          }
    T       data(size_t a_idx) const { return m_data[a_idx & MASK]; }

    const size_t    MASK;
    size_t          m_end;
    T               m_last;
    T               m_sum;
    T*              m_data;
    T               m_samples[N];
};

/**
 * Calculate a running weighted average of values on a given 
 * windowing interval using exponential decay.
 */
class weighted_average {
    size_t m_sec_interval;  ///< Number of seconds in the averaging window
    size_t m_last_seconds;  ///< Last reported timeval.tv_sec.
    double m_last;          ///< Last reported value.

    double m_last_wavg;     ///< Last weighted average
    double m_denominator;   ///< Equals m_sec_interval*60

    void reset(size_t a_sec_interval) {
        interval(a_sec_interval);
        m_last_seconds = 0;
        m_last = m_last_wavg = 0;
    }

public:
    explicit weighted_average(size_t a_sec_interval = 15u) {
        reset(a_sec_interval);
    }

    /// Obtain weighted average
    double calculate(size_t a_now_sec, double a_value) {
        double alpha    = exp(-(double)(a_now_sec - m_last_seconds)
                        / m_denominator);
        m_last_wavg     = a_value + alpha * (m_last_wavg - a_value);
        m_last          = a_value;
        m_last_seconds  = a_now_sec;
        return m_last_wavg;
    }

    /// Clear internal state
    void clear() { reset(m_sec_interval); }

    double last_value()     const { return m_last;      }
    double last_weighted()  const { return m_last_wavg; }

    /// Get windowing interval in seconds
    size_t interval()       const { return m_sec_interval; }

    /// Set windowing interval in seconds
    void interval(size_t a_sec_interval) {
        if (a_sec_interval == 0)
            throw std::out_of_range("Argument must be > 0!");
        m_sec_interval = a_sec_interval;
        m_denominator  = a_sec_interval * 60;
    }
};

} // namespace utxx
