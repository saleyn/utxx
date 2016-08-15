//----------------------------------------------------------------------------
/// \file  logger_impl.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of logger back-end implementation registrar.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-04
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
#include <utxx/logger/logger_impl.hpp>

namespace utxx {

void logger_impl_mgr::register_impl(
    const std::string& config_name, std::function<logger_impl*(const char*)>& factory)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    auto it =  m_implementations.find(config_name);
    if  (it == m_implementations.end())
        m_implementations.insert(std::make_pair(config_name, factory));
}

void logger_impl_mgr::unregister_impl(const std::string& config_name)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_implementations.erase(config_name);
}

logger_impl_mgr::impl_callback_t*
logger_impl_mgr::get_impl(const std::string& config_name) {
    std::lock_guard<std::mutex> guard(m_mutex);
    impl_map_t::iterator it = m_implementations.find(config_name);
    return (it != m_implementations.end()) ? &it->second : NULL;
}

} // namespace utxx
