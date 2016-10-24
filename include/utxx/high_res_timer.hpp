//----------------------------------------------------------------------------
/// \file   high_res_timer.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file contains implementation of high resolution timer.
/// Most of the high-resolution timer code is taken from ACE's 
/// high_res_timer (http://www.cs.wustl.edu/~schmidt/ACE.html)
/// Note: ACE's implementation suffers from inaccurate 
/// time reporting since different CPUs might have slightly 
/// different frequency and also different initial settings due
/// to different power-on times.
//  TODO: Include per-cpu calibration!!!
//----------------------------------------------------------------------------
// ACE Library Copyright (c) 1993-2009 Douglas C. Schmidt
// Created: 2009-11-28
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

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

#include <utxx/detail/get_tick_count.hpp>
#include <utxx/time_val.hpp>
#include <utxx/compiler_hints.hpp>
//#include <utxx/meta.hpp>

namespace utxx {

class high_res_timer {
public:
    // = Initialization method.

    static const unsigned int USECS_IN_SEC = 1000000;

    /**
    * s_global_scale_factor is set to @a gsf.  All high_res_timers use
    * s_global_scale_factor.  This allows applications to set the scale
    * factor just once for all high_res_timers.
    * Depending on the platform its units are number of ticks per
    * microsecond. Use ticks / global_scale_factor() to get the number of usec.
    */
    /// Returns the global_scale_factor.
    static unsigned int global_scale_factor() {
        return s_global_scale_factor;
    }

    static unsigned long long usec_global_scale_factor() { 
        return s_usec_global_scale_factor;
    }

    /**
    * Set (and return, for info) the global scale factor by sleeping
    * for @a usec and counting the number of intervening clock cycles.
    * Average over @a iterations of @a usec each.  On some platforms,
    * such as Pentiums, this is called automatically during the first
    * high_res_timer construction with the default parameter
    * values.  An application can override that by calling calibrate
    * with any desired parameter values _prior_ to constructing the
    * first high_res_timer instance.
    * Beware for platforms that can change the cycle rate on the fly.
    */
    static size_t calibrate(uint32_t usec = 500000, uint32_t iterations = 10);

    high_res_timer()
        : m_start(0), m_end(0), m_total(0), m_start_incr(0), m_last_incr(0)
    {}

    /// Reinitialize the timer.
    void reset () {
        m_start = m_end = m_total = m_start_incr = m_last_incr = 0;
    }

    /// Reset incremental total timing
    void reset_incr() {
        m_total = 0;
    }

    /// Start timing.
    void start()    { m_start = detail::get_tick_count(); }
    /// Stop timing.
    void stop()     { m_end   = detail::get_tick_count(); }

    /// Start incremental timing.
    void start_incr() { m_start_incr = detail::get_tick_count(); }
    /// Stop incremental timing.
    void stop_incr()  { 
        m_last_incr = elapsed_hrtime(detail::get_tick_count(), m_start_incr);
        m_total += m_last_incr;
    }

    /// Set @a tv to the number of microseconds elapsed.
    time_val elapsed_time()       const { return hrtime_to_tv(m_total); }
    /// Reset incremental total timing and return total elapsed time.
    time_val reset_elapsed_time()       { time_val t = elapsed_time(); m_total = 0; return t; }

    /// @return number of nanoseconds elapsed.
    /// Will overflow when measuring more than 194 day's.
    hrtime_t elapsed_nsec() const {
        hrtime_t nsec = elapsed_hrtime(m_end, m_start) * (1024000u / global_scale_factor());
        return nsec >> 10;
    }

    /// @return elapsed (stop - start) time in microseconds.
    hrtime_t elapsed_usec() const {
        hrtime_t elapsed = elapsed_hrtime(m_end, m_start);
        return (hrtime_t) (elapsed / global_scale_factor());
    }

    /// @return the number of microseconds elapsed between all calls
    /// to m_start_incr and stop_incr.
    time_val elapsed_time_incr() const {
        time_val t; t.nanosec(elapsed_nsec_incr()); return t;
    }

    /// @return the number of nanoseconds elapsed between all calls
    /// to m_start_incr and stop_incr.
    hrtime_t elapsed_nsec_incr() const {
        hrtime_t nsec = m_total * (1024000.0 / global_scale_factor());
        return nsec >> 10;
    }

    /// @return the number of nanoseconds elapsed between last 
    /// start_incr() and stop_incr() calls
    hrtime_t last_nsec_incr() const {
        return m_last_incr;
    }

    /// Get the current "time" as the high resolution counter at this time.
    /// FIXME: this is not accurate between CPUs.
    static time_val gettimeofday_hr() {
        return hrtime_to_tv(detail::get_tick_count());
    }

    /// Converts an @a hrt to @a tv using s_global_scale_factor.
    static time_val hrtime_to_tv(const hrtime_t hrt) {
        // The following are based on the units of s_global_scale_factor
        // being 1/microsecond.  Therefore, dividing by it converts
        // clock ticks to microseconds.
        int    sec = (long) (hrt / usec_global_scale_factor());
        hrtime_t t = sec * usec_global_scale_factor();
        int   usec = (long) ((hrt - t) / global_scale_factor());
        return time_val(sec, usec);
    }

    /**
    * This is used to find out the Mhz of the machine for the scale
    * factor.  If there are any problems getting it, we just return 1
    * (the default).
    */
    static unsigned int cpu_frequency() {
        static unsigned int s_scale_factor = get_cpu_frequency();
        return s_scale_factor;
    }

    static unsigned int get_cpu_frequency();

    /// Gets the high-resolution time using <RDTSC>.
    static hrtime_t gettime() { return detail::get_tick_count(); }

    /// Calculate the difference between two hrtime_t values. It is assumed
    /// that the end time is later than start time, so if end is a smaller
    /// value, the time counter has wrapped around.
    static hrtime_t elapsed_hrtime (const hrtime_t end, const hrtime_t start) {
        // Wrapped-around counter diff
        return likely(end > start) ? (end - start) : (~start + 1 + end);
    }

private:
    hrtime_t m_start;       /// Starting time.
    hrtime_t m_end;         /// Ending time.
    hrtime_t m_total;       /// Total elapsed time.
    hrtime_t m_start_incr;  /// Start time of incremental timing.
    hrtime_t m_last_incr;   /// Last incremental timing.

    /// Helps to convert ticks to microseconds.  That is, ticks /
    /// s_global_scale_factor == microseconds.
    static size_t   s_global_scale_factor;
    static uint64_t s_usec_global_scale_factor;
    static bool     s_calibrated;
};

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

} // namespace utxx

#ifdef DEBUG_TIMESTAMP
#include <utxx/../../src/high_res_timer.cpp>
#endif