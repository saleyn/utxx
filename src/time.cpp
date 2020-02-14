//----------------------------------------------------------------------------
/// \file  time.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of time functions
//----------------------------------------------------------------------------
// Copyright (c) 2020 Serge Aleynikov <saleyn@gmail.com>
// Created: 2020-02-06
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

#include <utxx/time.hpp>
#include <utxx/error.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace utxx {

//----------------------------------------------------------------------------
long parse_time_to_seconds(const char* a_tm, int a_sz)
{
    int n = a_sz < 0 ? strlen(a_tm) : a_sz;
    if (n < 5 || a_tm[2] != ':') // a_tm[5] != ':')
      return -1;
  
    auto parse = [](const char* p) { return 10*(p[0]-'0') + (p[1]-'0'); };
    auto p = a_tm;
    int  h = parse(p);
    int  m = parse(p+3);
    int  s = 0;
    p  = a_tm+5;
    if (n >= 8 && *p == ':') {
        s = parse(++p); p += 2;
    }
    if (n == (p+2 - a_tm)) {
        if (p[0] == 'p' && p[1] == 'm' && h < 12)
            h += 12;
        else if (p[0] == 'a' && p[1] == 'm' && h < 12) { /* do nothing */ }
        else 
            return -1;
    }
    else if (n == p-a_tm) { /* do nothing */ }
    else
        return -1;
  
    return  h*3600 + m*60 + s;
}

//--------------------------------------------------------------------------
int parse_dow_ref(const char*& a_dow, int a_today_dow, bool a_utc)
{
    auto cmp = [&a_dow](const char* dow, int len, int n) {
        for (int i=0; i < len; ++i)
            if (a_dow[i] == '\0' || dow[i] != tolower(a_dow[i]))
                return -1;
        a_dow += len;
        return n;
    };

	static const char* s_dow[] = {"sun","mon","tue","wed","thu","fri","sat"};
    for (int i=0; i < 7; ++i)
        if (cmp(s_dow[i], 3, i) > -1)
            return i;
    int        n = cmp("today", 5, 0);
    if (n < 0) n = cmp("tod",   3, 0);

    if (n < 0)
        return -1;
    if (a_today_dow > -1)
        return a_today_dow;

    auto now = time(nullptr);
    struct tm tm;
    if (a_utc)
        gmtime_r(&now, &tm);
    else
        localtime_r(&now, &tm);
    return tm.tm_wday;
}

} // namespace utxx
