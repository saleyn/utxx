//----------------------------------------------------------------------------
/// \file   logger_impl.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Supplementary classes for the <logger> class.
//----------------------------------------------------------------------------
// Copyright (C) 2003-2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-25
//----------------------------------------------------------------------------
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
#ifndef _UTXX_LOGGER_IMPL_HPP_
#define _UTXX_LOGGER_IMPL_HPP_

#include <stdarg.h>
#include <stdio.h>
#include <boost/thread/mutex.hpp>
#include <utxx/delegate.hpp>
#include <utxx/event.hpp>
#include <utxx/timestamp.hpp>
#include <utxx/meta.hpp>
#include <utxx/error.hpp>
#include <utxx/logger/logger_enums.hpp>
#include <utxx/logger.hpp>
#include <utxx/path.hpp>
#include <utxx/convert.hpp>
#include <utxx/variant_tree.hpp>
#include <utxx/print.hpp>
#include <unordered_map>
#include <vector>

namespace utxx {

/// Log implementation registrar. It handles registration of
/// logging back-ends, so that they can be instantiated automatically
/// based on configuration information. The manager contains a list
/// of logger backend creation functions mapped by name.
struct logger_impl_mgr {
    typedef std::function<logger_impl*(const char*)>         impl_callback_t;
    typedef std::unordered_map<std::string, impl_callback_t> impl_map_t;

    static logger_impl_mgr& instance() {
        static logger_impl_mgr s_instance;
        return s_instance;
    }

    void register_impl       (const char* config_name, impl_callback_t& factory);
    void unregister_impl     (const char* config_name);
    impl_callback_t* get_impl(const char* config_name);

    /// A static instance of the registrar must be created by
    /// each back-end in order to be automatically registered with
    /// the implementation manager.
    struct registrar {
        registrar(const char* config_name, impl_callback_t& factory)
            : name(config_name)
        {
            instance().register_impl(config_name, factory);
        }
        ~registrar() {
            instance().unregister_impl(name);
        }
    private:
        const char* name;
    };

    impl_map_t& implementations()  { return m_implementations; }
    std::mutex& mutex()            { return m_mutex; }

private:
    impl_map_t m_implementations;
    std::mutex m_mutex;
};

} // namespace utxx

#endif  // _UTXX_LOGGER_IMPL_HPP_
