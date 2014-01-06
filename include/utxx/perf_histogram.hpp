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
          REALTIME      = CLOCK_REALTIME
        , MONOTONIC     = CLOCK_MONOTONIC
        , HIGH_RES      = CLOCK_PROCESS_CPUTIME_ID
        , THREAD_SPEC   = CLOCK_THREAD_CPUTIME_ID
    };

private:
    enum {
          MIN_RES = 10
        , MAX_RES = MIN_RES + 500/25 + 1000/250
        , BUCKETS = MAX_RES+1
    };

    int                 m_latencies[BUCKETS];
    double              m_minTime, m_maxTime, m_sumTime;
    struct timespec     m_last_start;
    int                 m_count;
    const std::string   m_header;
    const clock_type    m_clock_type;

    static int to_bucket(double d) 
    {
        if (d < 0.000001 * MIN_RES) return (int) (d * 1000000.0);
        else if (d < 0.000500)      return MIN_RES + (int)(d * 1000000.0) / 25;
        else if (d > 1.0)           return MIN_RES + 500/25 + 1000/250;
        else                        return MIN_RES + 500/25 + (int)(d * 1000000.0) / 250;
    }

    static int from_bucket(int i)
    {
        if (i < MIN_RES)            return i;
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

    perf_histogram(const std::string& header = std::string(), clock_type ct = HIGH_RES)
        : m_header(header)
        , m_clock_type(ct)
    {
        reset();
    }

    /// Reset internal statistics counters
    void reset() {
        m_count   = 0;
        m_minTime = 9999999;
        m_maxTime = m_sumTime = 0;
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
        if (diff < 0)
            return;
        if (m_minTime > diff) m_minTime = diff;
        if (m_maxTime < diff) m_maxTime = diff;
        m_sumTime += diff;
        m_count++;
        int k = to_bucket(diff);
        if (k < BUCKETS)
            m_latencies[k]++;
    }

    /// Add statistics from another histogram
    void operator+= (const perf_histogram& a_rhs) {
        if (!a_rhs.m_count) return;

        if (m_maxTime < a_rhs.m_maxTime) m_maxTime = a_rhs.m_maxTime;
        if (m_minTime > a_rhs.m_minTime) m_minTime = a_rhs.m_minTime;
        m_sumTime += a_rhs.m_sumTime;
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
            << "  MinTime = " << m_minTime << std::endl
            << "  MaxTime = " << m_maxTime << std::endl
            << "  AvgTime = " << m_sumTime / m_count << std::endl;

        double tot = 0;
        for (int i = 0; i < BUCKETS; i++)
            if (m_latencies[i] > 0 && (a_filter < 0 || from_bucket(i) < a_filter)) {
                double pcnt = 100.0 * (double)m_latencies[i] / m_count;
                tot += pcnt;
                out << "    " << std::setw(6) << from_bucket(i) << "us = "
                    << std::setw(9) << m_latencies[i] 
                    << '(' << std::setw(6) << std::setprecision(3)
                    << pcnt << ") (total: "
                    << std::setw(7) << std::setprecision(3)
                    << tot << ') '
                    << std::string('*', (int)(30*pcnt/100))
                    << std::endl;
            }
    }
};

} // namespace utxx

#endif  // _UTXX_PERF_HISTOGRAM_HPP_
