//----------------------------------------------------------------------------
/// \file  Performance histogram printer of usec latencies.
//----------------------------------------------------------------------------
/// \brief Performance histogram printer.
//
// When building include -lrt
//----------------------------------------------------------------------------
// Copyright (c) 2012 Omnibius, LLC
// Author: Serge Aleynikov <saleyn@gmail.com>
// Created: 2012-03-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of utxx open-source project.

Copyright (C) 2012 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _UTXX_PERF_HISTOGRAM_HPP_
#define _UTXX_PERF_HISTOGRAM_HPP_

#include <string>
#include <cstring>
#include <time.h>
#include <ostream>
#include <iomanip>
#include <boost/concept_check.hpp>

namespace utxx {

struct perf_histogram {
    enum clock_type {
          DEFAULT     = 0
        , REALTIME    = CLOCK_REALTIME
        , MONOTONIC   = CLOCK_MONOTONIC
        , HIGH_RES    = CLOCK_PROCESS_CPUTIME_ID
        , THREAD_SPEC = CLOCK_THREAD_CPUTIME_ID
    };

private:
    enum {
          MIN_RES     = 10
        , MAX_RES     = MIN_RES + 500/25 + 1000/250
        , BUCKETS     = MAX_RES+1
    };

    int                 m_latencies[BUCKETS];
    double              m_min_time, m_max_time, m_sum_time;
    struct timespec     m_last_start;
    long                m_count;
    std::string         m_header;
    clock_type          m_clock_type;

    static int to_bucket(double d) 
    {
        if (d < 0.000001 * MIN_RES) return (int) (d * 1000000.0);
        else if (d < 0.000500)      return MIN_RES + (int)(d * 1000000.0) / 25;
        else if (d > 1.0)           return MIN_RES + 500/25 + 1000/250;
        else                        return MIN_RES + 500/25 + (int)(d * 1000000.0) / 250;
    }

    static int from_bucket(int i)
    {
        if      (i < MIN_RES)       return i;
        else if (i < MIN_RES + 24)  return MIN_RES + (i-MIN_RES)*25;
        else                        return MIN_RES + 500/25 + (i - MIN_RES - 500/25)*250;
    }

public:

    class sample {
        perf_histogram& h;
    public:
        sample(perf_histogram& a_h) : h(a_h) { h.start(); }
        ~sample() { h.stop(); }
    };

    perf_histogram(const std::string& header = std::string(), clock_type ct = DEFAULT)
        : m_header(header)
        , m_clock_type(ct ? ct : MONOTONIC)
    {
        reset();
    }

    /// Total number of samples
    long count() const { return m_count; }

    /// Reset internal statistics counters
    void reset(const char* a_header = NULL, clock_type a_type = DEFAULT) {
        if (a_header) m_header      = a_header;
        if (a_type)   m_clock_type  = a_type;
        m_count   = 0;
        m_min_time = 9999999;
        m_max_time = m_sum_time = 0;
        memset(m_latencies,   0, sizeof(m_latencies));
        memset(&m_last_start, 0, sizeof(struct timespec));
    }

    /// Start a measurement sample
    void start() {
        clock_gettime(m_clock_type, &m_last_start);
    }

    /// Stop the measurement sample started with start().
    void stop() {
        struct timespec now;
        clock_gettime(m_clock_type, &now);

        static const double fraction = 1 / 1000000000.0;
        double diff = (double)(now.tv_sec  - m_last_start.tv_sec)
                    + (double)(now.tv_nsec - m_last_start.tv_nsec) * fraction;
        add(diff);
    }

    /// Add the measurement sample to histogram
    void add(double a_duration_seconds) {
        assert(a_duration_seconds >= 0);
        if (m_min_time > a_duration_seconds) m_min_time = a_duration_seconds;
        if (m_max_time < a_duration_seconds) m_max_time = a_duration_seconds;
        m_sum_time    += a_duration_seconds;
        m_count++;
        int k = to_bucket(a_duration_seconds);
        if (k < BUCKETS)
            m_latencies[k]++;
    }

    /// Add statistics from another histogram
    void operator+= (const perf_histogram& a_rhs) {
        if (!a_rhs.m_count) return;

        if (m_max_time < a_rhs.m_max_time) m_max_time = a_rhs.m_max_time;
        if (m_min_time > a_rhs.m_min_time) m_min_time = a_rhs.m_min_time;
        m_sum_time += a_rhs.m_sum_time;
        m_count   += a_rhs.m_count;

        for (int i = 0; i < BUCKETS; i++)
            m_latencies[i] += a_rhs.m_latencies[i];
    }

    /// Dump a latency report to stdout
    void dump(std::ostream& out, int a_filter = -1) {
        if (m_count == 0) {
            out << "  No data samples" << std::endl;
            return;
        }

        out << m_header.c_str() << std::endl << std::fixed << std::setprecision(6)
            << "  MinTime = " << m_min_time << std::endl
            << "  MaxTime = " << m_max_time << std::endl
            << "  AvgTime = " << m_sum_time / m_count << std::endl;

        double tot = 0;
        for (int i = 0; i < BUCKETS; i++)
            if (m_latencies[i] > 0 && (a_filter < 0 || from_bucket(i) < a_filter)) {
                static const int s_gwidth = 30;
                double pcnt  = 100.0 * (double)m_latencies[i] / m_count;
                int    gauge = (int)(s_gwidth*pcnt/100);
                tot += pcnt;
                out << "    " << std::setw(6) << from_bucket(i) << "us = "
                    << std::setw(9) << m_latencies[i] 
                    << '(' << std::setw(6) << std::setprecision(3)
                    << pcnt << ") (total: "
                    << std::setw(7) << std::setprecision(3)
                    << tot << ") |"
                    << std::string(gauge, '*')
                    << std::string(s_gwidth-gauge, ' ')
                    << '|' << std::endl;
            }
    }
};

} // namespace utxx

#endif  // _UTXX_PERF_HISTOGRAM_HPP_
