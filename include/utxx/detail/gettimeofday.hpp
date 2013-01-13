//----------------------------------------------------------------------------
/// \file  gettimeofday.hpp
//----------------------------------------------------------------------------
/// \brief Portable implementation of gettickcount.
/// This file is derived from
/// http://www.suacommunity.com/dictionary/gettimeofday-entry.php
//----------------------------------------------------------------------------
// Author:  Serge Aleynikov
// Created: 2010-07-10
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
#ifndef __GETTIMEOFDAY_HPP_
#define __GETTIMEOFDAY_HPP_

#if defined(__WIN32) || defined(__windows__)

#include <time.h>
#include <windows.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
static const unsigned long long DELTA_EPOCH_IN_MICROSECS = 11644473600000000Ui64;
#else
static const unsigned long long DELTA_EPOCH_IN_MICROSECS = 11644473600000000ULL;
#endif
 
struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};
 
// Definition of a gettimeofday function
 
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    using namespace System;
    using namespace std;
    // Define a structure to receive the current Windows filetime
    FILETIME ft;

    // Initialize the present time to 0 and the timezone to UTC
    unsigned __int64 tmpres = 0;
    static int tzflag = 0;

    if (NULL != tv) {
        GetSystemTimeAsFileTime(&ft);

        // The GetSystemTimeAsFileTime returns the number of 100 nanosecond 
        // intervals since Jan 1, 1601 in a structure. Copy the high bits to 
        // the 64 bit tmpres, shift it left by 32 then or in the low 32 bits.
        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        // Convert to microseconds by dividing by 10
        tmpres /= 10;

        // The Unix epoch starts on Jan 1 1970.  Need to subtract the difference 
        // in seconds from Jan 1 1601.
        tmpres -= DELTA_EPOCH_IN_MICROSECS;

        // Finally change microseconds to seconds and place in the seconds value. 
        // The modulus picks up the microseconds.
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz) {
        if (!tzflag) {
            _tzset();
            tzflag++;
        }

        // Adjust for the timezone west of Greenwich
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}
    
#if 0

// Test routine for calculating a time difference
int main()
{
  struct timeval timediff;
  char dummychar;
 
// Do some interval timing
  gettimeofday(&timediff, NULL);
  double t1=((double)timediff.tv_usec/1000000.0)+timediff.tv_sec;

// Waste a bit of time here
  cout << "Give me a keystroke and press Enter." << endl;
  cin >> dummychar;
  cout << "Thanks." << endl;
 
//Now that you have measured the user's reaction time, display the results in seconds
  gettimeofday(&timediff, NULL);
  double t2=((double)timediff.tv_usec/1000000.0)+timediff.tv_sec;
  cout << t2-t1 << " seconds have elapsed" << endl;
  return 0;
}
    
#endif

#endif // defined(__WIN32) || defined(__windows__)

#endif // __GETTIMEOFDAY_HPP_
