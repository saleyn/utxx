//----------------------------------------------------------------------------
/// \file   lock.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Robust mutex that can be shared between processes.
//----------------------------------------------------------------------------
// Created: 2017-07-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/robust_mutex.hpp>

namespace utxx {
    struct robust_lock : public robust_mutex {
        struct lock_data {
            pthread_mutex_t mutex;
        };
        explicit robust_lock(bool a_destroy_on_exit = false)
            : robust_mutex(a_destroy_on_exit) {}
        void init(lock_data& a_data) { robust_mutex::init(a_data.mutex); }
        void set(lock_data& a_data)  { robust_mutex::set(a_data.mutex);  }
    };

    struct null_lock {
        typedef robust_mutex::make_consistent_functor make_consistent_functor;
        typedef void* native_handle_type;
        typedef std::unique_lock<null_lock> scoped_lock;
        typedef std::unique_lock<null_lock> scoped_try_lock;

        struct lock_data {};

        explicit null_lock(bool a_destroy_on_exit = false) {}
        void init(lock_data&) {}
        void set(lock_data&) {}
        void lock() {}
        void unlock() {}
        bool try_lock() { return true; }
        int  make_consistent() { return 0; }
        void destroy() {}
        native_handle_type native_handle() { return NULL; }
        robust_mutex::make_consistent_functor on_make_consistent;
    };

} // namespace utxx
