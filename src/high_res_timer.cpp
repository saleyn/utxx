//----------------------------------------------------------------------------
/// \file  high_res_timer.hpp
//----------------------------------------------------------------------------
/// \brief This file contains implementation of high resolution timer.
/// Most of the high-resolution timer code is taken from ACE's 
/// high_res_timer (http://www.cs.wustl.edu/~schmidt/ACE.html)
//----------------------------------------------------------------------------
// ACE Library Copyright (c) 1993-2009 Douglas C. Schmidt
// Author:  Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-28
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in various open-source projects.

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

#include <util/high_res_timer.hpp>
#include <util/cpu.hpp>
#include <boost/thread.hpp>
#include <stdio.h>
#include <unistd.h>

namespace util {

size_t   high_res_timer::s_global_scale_factor = high_res_timer::cpu_frequency();
uint64_t high_res_timer::s_usec_global_scale_factor = 
            high_res_timer::s_global_scale_factor * (uint64_t)USECS_IN_SEC;
bool     high_res_timer::s_calibrated;

unsigned int high_res_timer::high_res_timer::get_cpu_frequency() {
    unsigned int scale_factor = 1;
    FILE *cpuinfo = ::fopen("/proc/cpuinfo", "r");

    if (cpuinfo != NULL) {
        char buf[128];

        while (::fgets(buf, sizeof buf, cpuinfo)) {
            double mhertz = 1;

            if (::sscanf (buf, "cpu MHz : %lf\n", &mhertz) == 1) {
                scale_factor = (unsigned int) (mhertz + 0.5);
                break;
            }
        }
        ::fclose(cpuinfo);
    }
    return scale_factor;
}

size_t high_res_timer::calibrate(uint32_t usec, uint32_t iterations) {
    static boost::mutex m;

    boost::lock_guard<boost::mutex> guard(m);

    const time_val sleep_time(0, usec);
    unsigned long long delta_hrtime  = 0;
    unsigned long long actual_sleeps = 0;

    for (size_t i = 0; i < iterations; ++i) {
        time_val t1;
        t1.now();
        int cpu1 = detail::apic_id();
        const hrtime_t start = detail::get_tick_count();
        ::usleep(sleep_time.microseconds());
        const hrtime_t stop  = detail::get_tick_count();
        time_val actual_delta(abs_time(-t1.sec(), -t1.usec()));
        int cpu2 = detail::apic_id();
        if (cpu1 != cpu2) {
            i--; continue;
        }

        uint64_t delta   = actual_delta.microseconds();
        uint64_t hrdelta = (stop > start) ? (stop - start) : (~start + 1 + stop);
        // Store the sample.
        delta_hrtime  += hrdelta;
        actual_sleeps += delta;
    }

    delta_hrtime  /= iterations;
    actual_sleeps /= iterations;

    // The addition of 5 below rounds instead of truncates.
    s_global_scale_factor       = static_cast<int>(
                                    (double)delta_hrtime / actual_sleeps + .5);
    s_usec_global_scale_factor  = (uint64_t)USECS_IN_SEC * s_global_scale_factor;
    s_calibrated                = true;

    return s_global_scale_factor;
}

} // namespace util
