//------------------------------------------------------------------------------
/// \file   logger_util.hpp
/// \author Serge Aleynikov
//------------------------------------------------------------------------------
/// \brief Utility functions for the logging framework.
//------------------------------------------------------------------------------
// Copyright (C) 2003-2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-25
//------------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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
#pragma  once

#include <string>
#include <stdexcept>
#include <cassert>
#include <utxx/logger/logger_enums.hpp>

namespace utxx {

    namespace detail {
        inline int mask_bsf(log_level a_level) {
            auto l = static_cast<uint32_t>(a_level);
            return l ? ~((1u << (__builtin_ffs(l)-1))-1) : 0;
        }
    }

    /// Return log level as a 1-char string
    const std::string& log_level_to_abbrev(log_level level)  noexcept;

    /// Convert a log_level to string
    /// @param level level to convert
    /// @param merge_trace when true all TRACE1-5 levels are returned as "TRACE"
    const std::string& log_level_to_string(log_level level,
                                           bool merge_trace=true)  noexcept;

    inline const char* log_level_to_cstr(log_level level, bool merge_trace=true) noexcept
                       { return log_level_to_string(level, merge_trace).c_str(); }

    size_t             log_level_size  (log_level level)     noexcept;
    std::string        log_levels_to_str(uint32_t a_levels)  noexcept;

    /// Converts a delimited string to a bitmask of corresponding levels.
    /// This method is used for configuration parsing.
    /// @param a_levels delimited log levels (e.g. "DEBUG | INFO | WARNING").
    int parse_log_levels(const std::string& levels);

    /// Converts a string (e.g. "INFO") to the corresponding log level.
    log_level parse_log_level(const std::string& a_level);

    /// Convert a string (e.g. "INFO") to the log levels greater or equal to it.
    int parse_min_log_level(const std::string& a_level);

    inline int def_log_level() { return as_int<LEVEL_INFO>(); }

} // namespace utxx
