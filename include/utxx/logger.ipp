//----------------------------------------------------------------------------
/// \file  logger_impl.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of logger.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-04
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx project.

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
#include <utxx/logger/logger.hpp>
#include <utxx/time_val.hpp>
#include <utxx/convert.hpp>
#include <boost/filesystem/path.hpp>
#include <algorithm>

namespace utxx {

logger_impl::logger_impl()
    : m_log_mgr(NULL), m_bin_sink_id(-1)
{
    for (int i=0; i < NLEVELS; ++i)
        m_msg_sink_id[i] = -1;
}

logger_impl::~logger_impl()
{
    if (m_log_mgr) {
        for (int i=0; i < NLEVELS; ++i)
            if (m_msg_sink_id[i] != -1) {
                log_level level = logger::signal_slot_to_level(i);
                m_log_mgr->remove_msg_logger(level, m_msg_sink_id[i]);
                m_msg_sink_id[i] = -1;
            }
        if (m_bin_sink_id != -1) {
            m_log_mgr->remove_bin_logger(m_bin_sink_id);
            m_bin_sink_id = -1;
        }
    }
}

void logger_impl::add_msg_logger(log_level level, on_msg_delegate_t subscriber)
{
    m_msg_sink_id[logger::level_to_signal_slot(level)] =
        m_log_mgr->add_msg_logger(level, subscriber);
}

void logger_impl::add_bin_logger(on_bin_delegate_t subscriber)
{
    m_bin_sink_id = m_log_mgr->add_bin_logger(subscriber);
}

} // namespace utxx
