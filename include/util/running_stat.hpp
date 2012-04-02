//----------------------------------------------------------------------------
// running_stat.hpp
//----------------------------------------------------------------------------
/// This file implements a class that calculates running mean and
/// standard deviation. 
//----------------------------------------------------------------------------
// Copyright (c) 2010 Omnibius, LLC
// Author:  Serge Aleynikov <saleyn@mail.com>
// Created: 2010-05-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

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

#ifndef _UTIL_RUNNING_STAT_HPP_
#define _UTIL_RUNNING_STAT_HPP_

#include <stdlib.h>
#include <math.h>
#include <limits>
#include <algorithm>
#include <stdexcept>

/*
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/zip_view.hpp>
#include <boost/mpl/single_view.hpp>
*/

namespace util {

//-----------------------------------------------------------------------------
/// Keep running min/max/average/variance stats.
//-----------------------------------------------------------------------------

/// Basic holder of count / sum / min / max tuple
template <typename CntType = size_t>
class basic_running_sum {
protected:
    CntType m_count;
    double  m_sum, m_min, m_max;
public:
    basic_running_sum() {
        clear();
    }

    basic_running_sum(const basic_running_sum& rhs) 
        : m_count(rhs.m_count), m_sum(rhs.m_sum)
        , m_min(rhs.m_min), m_max(rhs.m_max)
    {}

    /// Reset the internal state.
    void clear() { 
        m_count = 0; m_sum = 0.0; m_sum = 0.0;
        m_min = std::numeric_limits<double>::max();
        m_max = std::numeric_limits<double>::min();
    }

    /// Add a sample measurement.
    inline void add(double x) {
        ++m_count;
        m_sum += x;
        if (x > m_max) m_max = x;
        if (x < m_min) m_min = x;
    }

    void operator+= (const basic_running_sum<CntType>& a) {
        m_count += a.m_count;
        m_sum   += a.m_sum;
        if (a.m_max > m_max) m_max = a.m_max;
        if (a.m_min < m_min) m_min = a.m_min;
    }

    void operator-= (const basic_running_sum<CntType>& a) {
        m_count -= a.count();
        m_sum   -= a.sum();
        //m_min    = a.min();
        //m_max    = a.max();
    }

    /// Number of samples since last invocation of clear().
    CntType     count() const { return m_count; }
    double      sum()   const { return m_sum; }
    double      mean()  const { return m_count > 0 ? m_sum / m_count : 0.0; }
    double      min()   const { return m_min == std::numeric_limits<double>::max()
                                     ? 0.0 : m_min; }
    double      max()   const { return m_max == std::numeric_limits<double>::min()
                                     ? 0.0 : m_max; }
};

   
template <typename CntType = size_t>
class basic_running_variance : public basic_running_sum<CntType> {
    typedef basic_running_sum<CntType> base;
    double m_mean, m_var;
public:
    basic_running_variance() {
        clear();
    }

    basic_running_variance(const basic_running_variance& rhs) 
        : base(rhs)
        , m_mean(rhs.m_mean), m_var(rhs.m_var)
    {}

    /// Reset the internal state.
    void clear() { 
        base::clear();
        m_mean = 0.0; m_var = 0.0;
    }

    /// Add a sample measurement.
    inline void add(double x) {
        base::add(x);
        // See Knuth TAOCP v.2, 3rd ed, p.232
        double old  = m_mean;
        double diff = x - old;
        if (diff != 0.0) {
            m_mean += diff / base::m_count;
            m_var  += (x - old) * (x - m_mean);
        }
    }

    /// Number of samples since last invocation of clear().
    double   mean()      const { return base::m_count > 0 ? m_mean : 0.0; }
    double   variance()  const { return base::m_count > 0 ? m_var/base::m_count : 0.0; }
    double   deviation() const { return sqrt(variance()); }
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
        m_sec_interval = a_sec_interval;
        m_denominator  = a_sec_interval * 60;
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
        m_denominator *= a_sec_interval;
    }
};

/// Running sum statistics for single-threaded use.
typedef basic_running_sum<size_t>      running_sum;
/// Running variance statistics for single-threaded use.
typedef basic_running_variance<size_t> running_variance;

} // namespace util

#endif // _UTIL_RUNNING_STAT_HPP_
