//----------------------------------------------------------------------------
/// \file   signal_block.hpp
/// \author Serge ALeynikov <saleyn@gmail.com>
/// \author Peter Simons <simons@cryp.to> (signal_block/unblock)
//----------------------------------------------------------------------------
/// \brief Signal blocking class.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Copyright (c) 2010 Peter Simons <simons@cryp.to> (signal_block/unblock)
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2014 Serge Aleynikov

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

#include <boost/noncopyable.hpp>
#include <utxx/error.hpp>
#include <signal.h>

namespace utxx
{

/// Block all POSIX signals in the current scope.
/// \sa signal_unblock
class signal_block : private boost::noncopyable {
    sigset_t m_orig_mask;
    bool     m_restore;

    static sigset_t full()  { sigset_t s; ::sigfillset(&s);  return s; }
    static sigset_t empty() { sigset_t s; ::sigemptyset(&s); return s; }
public:

    explicit signal_block(bool a_block = true, bool a_restore = true)
        : signal_block(a_block ? full() : empty(), a_restore)
    {}

    explicit signal_block(sigset_t const& a_block, bool a_restore = true)
        : m_restore(a_restore)
    {
        if (sigisemptyset(&a_block))
            ::sigemptyset(&m_orig_mask);
        else
            block();
    }

    void block() {
        sigset_t block_all;
        if (::sigfillset(&block_all) < 0)
            UTXX_THROW_IO_ERROR(errno, "sigfillset(3)");
        if (::sigprocmask(SIG_SETMASK, &block_all, &m_orig_mask))
            UTXX_THROW_IO_ERROR(errno, "sigprocmask(2)");
    }

    ~signal_block() {
        if (!sigisemptyset(&m_orig_mask) && m_restore)
            ::sigprocmask(SIG_SETMASK, &m_orig_mask, static_cast<sigset_t*>(0));
    }
};

/// Unblock all POSIX signals in the current scope.
/// \sa signal_block
class signal_unblock : private boost::noncopyable {
    sigset_t m_orig_mask;
    bool     m_restore;
public:
    signal_unblock(bool a_restore = true)
        : m_restore(a_restore)
    {
        sigset_t l_unblock_all;
        if (::sigemptyset(&l_unblock_all) < 0)
            UTXX_THROW_IO_ERROR(errno, "sigfillset(3)");
        if (::sigprocmask(SIG_SETMASK, &l_unblock_all, &m_orig_mask))
            UTXX_THROW_IO_ERROR(errno, "sigprocmask(2)");
    }

    ~signal_unblock() {
        if (m_restore)
            ::sigprocmask(SIG_SETMASK, &m_orig_mask, static_cast<sigset_t*>(0));
    }
};

/// Get a list of all known signal names.
/// @return list of char strings that can be iterated until NULL.
const char** sig_names();

/// Total number of "well-known" signal names in the sig_names() array
static constexpr size_t sig_names_count() { return 64; }

/// Get the name of an OS signal number.
/// @return signal name or "<UNDEFINED>" if the name is not defined.
const char* sig_name(int a_signum);

/// Convert signal set to string
std::string sig_members(const sigset_t& a_set);

/// Initialize a signal set from an argument list
template <class... Signals>
sigset_t sig_init_set(Signals&&... args) {
    sigset_t sset;
    sigemptyset(&sset);
    int sigs[] = { args... };
    for (uint i=0; i < sizeof...(Signals); ++i)
        if (sigaddset(&sset, sigs[i]) < 0)
            UTXX_THROW_IO_ERROR(errno, "Error in sigaddset[", sigs[i], ']');
    return sset;
}

/// Parse a string containing pipe/comma/column/space delimited signal names.
/// The signal names are case insensitive and not required to begin with "SIG".
sigset_t sig_members_parse(const std::string& a_signals, src_info&& a_si);

/// Convert a vector of integer signal numbers to sigset
sigset_t sig_vector_to_set(const std::vector<int>& a_signals);

/// Return a formatted string containing current signals
inline std::string curr_signals_to_str(utxx::src_info&& si = utxx::src_info(), bool decode=false) {
    sigset_t old;
    char res[1024], buf[64];
    if (sigprocmask(SIG_SETMASK, NULL, &old) < 0)
        strcpy(buf, "<error>");
    else
        snprintf(buf, sizeof(buf), "%lx", reinterpret_cast<uint64_t&>(old));

    auto n = snprintf(res, sizeof(res), "%sPID: %d SigMask: %s",
                      si.empty() ? "" : si.to_string("[","] ").c_str(), getpid(), buf);
    if (decode)
      snprintf(res+n, sizeof(res)-n, " %s", utxx::sig_members(old).c_str());

    return res;
}


} // namespace utxx
