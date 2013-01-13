//----------------------------------------------------------------------------
/// \file   robust_mutex.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Robust mutex that can be shared between processes.
//----------------------------------------------------------------------------
// Created: 2009-11-21
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

#ifndef _UTXX_ROBUST_MUTEX_HPP_
#define _UTXX_ROBUST_MUTEX_HPP_

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

namespace utxx {
    class robust_mutex {
    public:
        typedef robust_mutex self_type;
        typedef boost::function<int (robust_mutex&)> make_consistent_functor;

        typedef pthread_mutex_t* native_handle_type;
        typedef boost::unique_lock<robust_mutex> scoped_lock;
        typedef boost::detail::try_lock_wrapper<robust_mutex> scoped_try_lock;

        make_consistent_functor on_make_consistent;

        explicit robust_mutex(bool a_destroy_on_exit = false)
            : m(NULL), m_destroy(a_destroy_on_exit)
        {}

        robust_mutex(pthread_mutex_t& a_mutex,
            bool a_init = false, bool a_destroy_on_exit = false)
        {
            if (a_init)
                init(a_mutex);
            else
                set(a_mutex);
            m_destroy = a_destroy_on_exit;
        }

        void set(pthread_mutex_t& a_mutex) { m = &a_mutex; }

        void init(pthread_mutex_t& a_mutex, pthread_mutexattr_t* a_attr=NULL) {
            m = &a_mutex;
            pthread_mutexattr_t  attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_t* mutex_attr = a_attr ? a_attr : &attr;
            if (pthread_mutexattr_setpshared(mutex_attr, PTHREAD_PROCESS_SHARED) < 0)
                boost::throw_exception(boost::lock_error(errno));
            if (pthread_mutexattr_setrobust_np(mutex_attr, PTHREAD_MUTEX_ROBUST_NP) < 0)
                boost::throw_exception(boost::lock_error(errno));
            if (pthread_mutexattr_setprotocol(mutex_attr, PTHREAD_PRIO_INHERIT) < 0)
                boost::throw_exception(boost::lock_error(errno));
            if (pthread_mutex_init(m, mutex_attr) < 0)
                boost::throw_exception(boost::lock_error(errno));
        }

        ~robust_mutex() {
            if (!m_destroy) return;
            destroy();
        }

        void lock() {
            BOOST_ASSERT(m);
            int res;
            while (1) {
                res = pthread_mutex_lock(m);
                switch (res) {
                    case 0:
                        return;
                    case EINTR:
                        continue;
                    case EOWNERDEAD:
                        res = on_make_consistent
                                ? on_make_consistent(*this)
                                : make_consistent();
                        if (res)
                            boost::throw_exception(boost::lock_error(res));
                        return;
                    default:
                        // If ENOTRECOVERABLE - mutex is not recoverable, must be destroyed.
                        boost::throw_exception(boost::lock_error(res));
                        return;
                }
            }
        }

        void unlock() {
            BOOST_ASSERT(m);
            int ret;
            do {
                ret = pthread_mutex_unlock(m);
            } while (ret == EINTR);
            BOOST_VERIFY(!ret);
        }

        bool try_lock() {
            BOOST_ASSERT(m);
            int res;
            do {
                res = pthread_mutex_trylock(m);
            } while (res == EINTR);
            if(res && (res!=EBUSY))
                boost::throw_exception(boost::lock_error(res));
            return !res;
        }

        int make_consistent() {
            BOOST_ASSERT(m);
            return pthread_mutex_consistent_np(m);
        }

        void destroy() {
            if (!m) return;
            int ret;
            do { ret = pthread_mutex_destroy(m); } while (ret == EINTR);
            m = NULL;
        }

        native_handle_type native_handle() { return m; }

    private:
        pthread_mutex_t* m;
        bool m_destroy;

        robust_mutex(const robust_mutex&);
    };

} // namespace utxx

#endif // _UTXX_ROBUST_MUTEX_HPP_
