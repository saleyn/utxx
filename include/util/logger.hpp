/**
 * Logging framework
 */
/*-----------------------------------------------------------------------------
 * Copyright (c) 2003-2009 Serge Aleynikov <serge@aleynikov.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *-----------------------------------------------------------------------------
 * Created: 2009-11-25
 * $Id$
 */
#ifndef _UTIL_LOGGER_HPP_
#define _UTIL_LOGGER_HPP_

namespace util { 

class logger;

enum log_level {
      NOLOGGING      = 0
    , LEVEL_NONE     = 0
    , LEVEL_TRACE5   = 1 << 5 | 1 << 0
    , LEVEL_TRACE4   = 1 << 5 | 1 << 1
    , LEVEL_TRACE3   = 1 << 5 | 1 << 2
    , LEVEL_TRACE2   = 1 << 5 | 1 << 3
    , LEVEL_TRACE1   = 1 << 5 | 1 << 4
    , LEVEL_TRACE    = 1 << 5
    , LEVEL_DEBUG    = 1 << 6
    , LEVEL_INFO     = 1 << 7
    , LEVEL_WARNING  = 1 << 8
    , LEVEL_ERROR    = 1 << 9
    , LEVEL_FATAL    = 1 << 10
    , LEVEL_ALERT    = 1 << 11
    , LEVEL_LOG      = 1 << 12
    , LEVEL_NO_DEBUG = LEVEL_INFO | LEVEL_WARNING | LEVEL_ERROR | LEVEL_FATAL | LEVEL_ALERT | LEVEL_LOG
    , LEVEL_NO_TRACE = LEVEL_NO_DEBUG | LEVEL_DEBUG
    , LEVEL_LOG_ALL  = LEVEL_NO_TRACE | LEVEL_TRACE1 | LEVEL_TRACE2 | LEVEL_TRACE3 |
                       LEVEL_TRACE4   | LEVEL_TRACE5
};

} // namespace util

#include <util/logger/logger.hpp>

#endif // _UTIL_LOGGER_HPP_
