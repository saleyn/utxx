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
#include <utxx/logger/logger_util.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <vector>

namespace utxx {

//------------------------------------------------------------------------------
const std::string& log_level_to_abbrev(log_level level) noexcept
{
    static const std::string s_levels[] = {
        "T", "D", "I", "N", "W", "E", "F", "A", "L", " "
    };

    switch (level) {
        case LEVEL_TRACE5   :
        case LEVEL_TRACE4   :
        case LEVEL_TRACE3   :
        case LEVEL_TRACE2   :
        case LEVEL_TRACE1   :
        case LEVEL_TRACE    : return s_levels[0];
        case LEVEL_DEBUG    : return s_levels[1];
        case LEVEL_INFO     : return s_levels[2];
        case LEVEL_NOTICE   : return s_levels[3];
        case LEVEL_WARNING  : return s_levels[4];
        case LEVEL_ERROR    : return s_levels[5];
        case LEVEL_FATAL    : return s_levels[6];
        case LEVEL_ALERT    : return s_levels[7];
        case LEVEL_LOG      : return s_levels[8];
        default             : break;
    }
    assert(false);
    return s_levels[9];
}

//------------------------------------------------------------------------------
const std::string& log_level_to_string(log_level lvl, bool merge_trace) noexcept
{
    static const std::string s_traces[] = {
        "TRACE1", "TRACE2", "TRACE3", "TRACE4", "TRACE5"
    };

    static const std::string s_levels[] = {
        "TRACE",   "DEBUG", "INFO",  "NOTICE",
        "WARNING", "ERROR", "FATAL", "ALERT", "LOG", "NONE"
    };

    switch (lvl) {
        case LEVEL_TRACE5   : return merge_trace ? s_levels[0] : s_traces[4];
        case LEVEL_TRACE4   : return merge_trace ? s_levels[0] : s_traces[3];
        case LEVEL_TRACE3   : return merge_trace ? s_levels[0] : s_traces[2];
        case LEVEL_TRACE2   : return merge_trace ? s_levels[0] : s_traces[1];
        case LEVEL_TRACE1   : return merge_trace ? s_levels[0] : s_traces[0];
        case LEVEL_TRACE    : return s_levels[0];
        case LEVEL_DEBUG    : return s_levels[1];
        case LEVEL_INFO     : return s_levels[2];
        case LEVEL_NOTICE   : return s_levels[3];
        case LEVEL_WARNING  : return s_levels[4];
        case LEVEL_ERROR    : return s_levels[5];
        case LEVEL_FATAL    : return s_levels[6];
        case LEVEL_ALERT    : return s_levels[7];
        case LEVEL_LOG      : return s_levels[8];
        default             : break;
    }
    assert(false);
    return s_levels[9];
}

//------------------------------------------------------------------------------
size_t log_level_size(log_level level) noexcept
{
    switch (level) {
        case LEVEL_TRACE5   :
        case LEVEL_TRACE4   :
        case LEVEL_TRACE3   :
        case LEVEL_TRACE2   :
        case LEVEL_TRACE1   :
        case LEVEL_TRACE    :
        case LEVEL_DEBUG    : return 5;
        case LEVEL_INFO     : return 4;
        case LEVEL_NOTICE   : return 6;
        case LEVEL_WARNING  : return 7;
        case LEVEL_ERROR    :
        case LEVEL_FATAL    :
        case LEVEL_ALERT    : return 5;
        case LEVEL_LOG      : return 3;
        default             : break;
    }
    assert(false);
    return 0;
}

//------------------------------------------------------------------------------
std::string log_levels_to_str(uint32_t a_levels) noexcept
{
    std::stringstream s;
    bool l_empty = true;
    for (uint32_t i = 1; i <= LEVEL_LOG; i <<= 1) {
        auto level = i < LEVEL_TRACE ? (i | LEVEL_TRACE) : i;
        if  ((level & a_levels) == level) {
            s << (l_empty ? "" : "|")
              << log_level_to_string(static_cast<log_level>(level), false);
            l_empty = false;
        }
    }
    return s.str();
}

//------------------------------------------------------------------------------
int parse_log_levels(const std::string& a_levels)
    throw(std::runtime_error)
{
    std::vector<std::string> str_levels;
    boost::split(str_levels, a_levels, boost::is_any_of(" |,;"),
        boost::algorithm::token_compress_on);
    int result = LEVEL_NONE;
    for (std::vector<std::string>::iterator it=str_levels.begin();
        it != str_levels.end(); ++it)
        result |= parse_log_level(*it);
    return result;
}

//------------------------------------------------------------------------------
log_level parse_log_level(const std::string& a_level)
    throw(std::runtime_error)
{
    if (a_level.empty()) return LEVEL_NONE;

    auto s = boost::to_upper_copy(a_level);
    if (s == "WIRE")     return LEVEL_DEBUG;  // Backward compatibility
    if (s == "NONE" ||
        s == "FALSE")    return LEVEL_NONE;
    if (s == "TRACE")    return LEVEL_TRACE;
    if (s == "TRACE1")   return log_level(LEVEL_TRACE | LEVEL_TRACE1);
    if (s == "TRACE2")   return log_level(LEVEL_TRACE | LEVEL_TRACE2);
    if (s == "TRACE3")   return log_level(LEVEL_TRACE | LEVEL_TRACE3);
    if (s == "TRACE4")   return log_level(LEVEL_TRACE | LEVEL_TRACE4);
    if (s == "TRACE5")   return log_level(LEVEL_TRACE | LEVEL_TRACE5);
    if (s == "DEBUG")    return LEVEL_DEBUG;
    if (s == "INFO")     return LEVEL_INFO;
    if (s == "NOTICE")   return LEVEL_NOTICE;
    if (s == "WARNING")  return LEVEL_WARNING;
    if (s == "ERROR")    return LEVEL_ERROR;
    if (s == "FATAL")    return LEVEL_FATAL;
    if (s == "ALERT")    return LEVEL_ALERT;
    try   { auto n = std::stoi(s); return as_log_level(n); }
    catch (...) {}
    throw std::runtime_error("Invalid log level: " + a_level);
}

//------------------------------------------------------------------------------
int parse_min_log_level(const std::string& a_level) throw(std::runtime_error) {
    auto l = parse_log_level(a_level);
    return detail::mask_bsf(l);
}

} // namespace utxx