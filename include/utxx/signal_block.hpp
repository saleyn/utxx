//----------------------------------------------------------------------------
/// \file   signal_block.hpp
/// \author Peter Simons <simons@cryp.to>
//----------------------------------------------------------------------------
/// \brief Signal blocking class.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Peter Simons <simons@cryp.to>
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2010 Peter Simons <simons@cryp.to>

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

#ifndef _UTXX_SIGNALS_HPP_
#define _UTXX_SIGNALS_HPP_

#include <boost/noncopyable.hpp>
#include <utxx/error.hpp>
#include <signal.h>

namespace utxx
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

} // namespace utxx

#endif // _UTXX_SIGNALS_HPP_
