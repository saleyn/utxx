//----------------------------------------------------------------------------
/// \file verbosity.hpp
//----------------------------------------------------------------------------
/// This file contains a class verbosty that controls verbosity setting
/// based on the value of the VERBOSE environment variable.
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


#ifndef _UTXX_VERBOSITY_HPP_
#define _UTXX_VERBOSITY_HPP_

#include <stdlib.h>
#include <string.h>

namespace utxx {

enum verbose_type {
      VERBOSE_INVALID = -1
    , VERBOSE_NONE    =  0
    , VERBOSE_TEST
    , VERBOSE_DEBUG
    , VERBOSE_INFO
    , VERBOSE_MESSAGE
    , VERBOSE_WIRE
    , VERBOSE_TRACE
};

/**
 * Parse application verbosity setting
 */
class verbosity {
public:
    static verbose_type level() {
        static verbose_type verbose = parse(env());
        return verbose;
    }

    static const char* c_str(int a) {
        return c_str(static_cast<verbose_type>(a));
    }

    static const char* c_str(verbose_type a) {
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

    static const char* env() {
        const char* p = getenv("VERBOSE");
        return p ? p : "";
    }

    /// Validate if \a a_verbosity is a valid argument and
    /// if so convert it to verbosity level.
    /// @return VERBOSE_INVALID if \a a_verbosity doesn't contain
    ///     a valid verbosity level.
    static verbose_type validate(const char* a_verbosity) {
        return parse(a_verbosity, NULL, true);
    }

    static verbose_type parse(const char* a_verbosity,
        const char* a_default = NULL, bool a_validate = false)
    {
        const char* p = a_verbosity;
        if (!p || p[0] == '\0') p = a_default;
        if (!p || p[0] == '\0') return a_validate ? VERBOSE_INVALID : VERBOSE_NONE;
        int n = atoi(p);
        if      (n == 1 || strncmp("test",    p, 4) == 0) return VERBOSE_TEST;
        if      (n == 2 || strncmp("debug",   p, 3) == 0) return VERBOSE_DEBUG;
        else if (n == 3 || strncmp("info",    p, 4) == 0) return VERBOSE_INFO; 
        else if (n == 4 || strncmp("message", p, 4) == 0) return VERBOSE_MESSAGE; 
        else if (n == 5 || strncmp("wire",    p, 4) == 0) return VERBOSE_WIRE; 
        else if (n >= 6 || strncmp("trace",   p, 4) == 0) return VERBOSE_TRACE; 
        else if (a_validate)                              return VERBOSE_INVALID;
        else                                              return VERBOSE_NONE;
    }
};

} // namespace utxx

#endif // _UTXX_VERBOSITY_HPP_

