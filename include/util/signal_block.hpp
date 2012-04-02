/*
 * Copyright (c) 2010 Peter Simons <simons@cryp.to>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTIL_SIGNALS_HPP_
#define _UTIL_SIGNALS_HPP_

#include <boost/noncopyable.hpp>
#include <util/error.hpp>
#include <signal.h>

namespace util
{

/**
 * Block all POSIX signals in the current scope.
 *
 * \sa signal_unblock
 */
class signal_block : private boost::noncopyable {
    sigset_t m_orig_mask;
public:
    signal_block() {
        sigset_t block_all;
        if (::sigfillset(&block_all) < 0)
            throw sys_error(errno, "sigfillset(3)", __FILE__, __LINE__);
        if (::sigprocmask(SIG_SETMASK, &block_all, &m_orig_mask))
            throw sys_error(errno, "sigprocmask(2)", __FILE__, __LINE__);
    }

    ~signal_block() {
        if (::sigprocmask(SIG_SETMASK, &m_orig_mask, static_cast<sigset_t*>(0)))
            throw sys_error(errno, "sigprocmask(2)", __FILE__, __LINE__);
    }
};

/**
 * Unblock all POSIX signals in the current scope.
 *
 * \sa signal_block
 */
class signal_unblock : private boost::noncopyable {
    sigset_t m_orig_mask;
public:
    signal_unblock() {
        sigset_t l_unblock_all;
        if (::sigemptyset(&l_unblock_all) < 0)
            throw sys_error(errno, "sigfillset(3)", __FILE__, __LINE__);
        if (::sigprocmask(SIG_SETMASK, &l_unblock_all, &m_orig_mask))
            throw sys_error(errno, "sigprocmask(2)", __FILE__, __LINE__);
    }

    ~signal_unblock() {
        if (::sigprocmask(SIG_SETMASK, &m_orig_mask, static_cast<sigset_t*>(0)))
            throw sys_error(errno, "sigprocmask(2)", __FILE__, __LINE__);
    }
};

} // namespace util

#endif // _UTIL_SIGNALS_HPP_
