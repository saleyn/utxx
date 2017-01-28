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


#pragma once

#include <stdlib.h>
#include <utxx/compiler_hints.hpp>

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
    static verbose_type s_verbose;
public:
    static verbose_type level() { return s_verbose; }

    static void level(verbose_type a_level) { s_verbose = a_level; }

    /// Returns true if current verbosity level is equal or greater
    /// than \a tp.
    static bool enabled() {
        return unlikely(int(level()) > int(VERBOSE_NONE));
    }

    /// Returns true if current verbosity level is equal or greater
    /// than \a tp.
    static bool enabled(verbose_type tp) {
        return unlikely(int(level()) >= int(tp));
    }

    static const char* c_str(int a) {
        return c_str(static_cast<verbose_type>(a));
    }

    /// Return current verbosity level as a string
    static const char* c_str() { return c_str(s_verbose); }

    static const char* c_str(verbose_type a);

    static const char* env() {
        const char* p = getenv("VERBOSE");
        return p ? p : "";
    }

    template <typename Lambda>
    static void if_enabled(verbose_type a_level, const Lambda& a_fun) {
        if (enabled(a_level))
            a_fun();
    }

    /// Validate if \a a_verbosity is a valid argument and
    /// if so convert it to verbosity level.
    /// @return VERBOSE_INVALID if \a a_verbosity doesn't contain
    ///     a valid verbosity level.
    static verbose_type validate(const char* a_verbosity) {
        return parse(a_verbosity, NULL, true);
    }

    static verbose_type parse(const char* a_verbosity,
        const char* a_default = NULL, bool a_validate = false);
};

} // namespace utxx
