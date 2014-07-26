//----------------------------------------------------------------------------
/// \file verbosity.cpp
//----------------------------------------------------------------------------
/// This file contains verbosty implemenation
//----------------------------------------------------------------------------
// Copyright (c) 2010 Omnibius, LLC
// Author:  Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-06-21
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

#include <utxx/verbosity.hpp>
#include <strings.h>

namespace utxx {

verbose_type verbosity::s_verbose =
    verbosity::parse(verbosity::env());

const char* verbosity::c_str(verbose_type a) {
    switch (a) {
        case VERBOSE_TEST:      return "test";
        case VERBOSE_DEBUG:     return "debug";
        case VERBOSE_INFO:      return "info";
        case VERBOSE_MESSAGE:   return "message";
        case VERBOSE_WIRE:      return "wire";
        case VERBOSE_TRACE:     return "trace";
        default:                return "none";
    }
}

verbose_type verbosity::parse(
    const char* a_verbosity, const char* a_default, bool a_validate)
{
    const char* p = a_verbosity;
    if (!p || p[0] == '\0') p = a_default;
    if (!p || p[0] == '\0') return a_validate ? VERBOSE_INVALID : VERBOSE_NONE;
    int n = atoi(p);
    if      (n == 1 || strncasecmp("test",    p, 4) == 0) return VERBOSE_TEST;
    if      (n == 2 || strncasecmp("debug",   p, 3) == 0) return VERBOSE_DEBUG;
    else if (n == 3 || strncasecmp("info",    p, 4) == 0) return VERBOSE_INFO; 
    else if (n == 4 || strncasecmp("message", p, 4) == 0) return VERBOSE_MESSAGE; 
    else if (n == 5 || strncasecmp("wire",    p, 4) == 0) return VERBOSE_WIRE; 
    else if (n >= 6 || strncasecmp("trace",   p, 4) == 0) return VERBOSE_TRACE; 
    else if (a_validate)                                  return VERBOSE_INVALID;
    else                                                  return VERBOSE_NONE;
}

} // namespace utxx
