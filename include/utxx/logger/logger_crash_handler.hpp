//----------------------------------------------------------------------------
/// \file  logger_crash_handler.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of crash handler in the logging framework.
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
/* ===========================================================================
* 2011 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
* with no warranties. This code is yours to share, use and modify with no
* strings attached and no restrictions or obligations.
* ============================================================================*/
#pragma once

#include <string>
#include <csignal>

namespace utxx {

namespace detail {
    /// Exit the app by rethrowing a fatal signal, previously caught.
    /// Do not use it elsewhere. It is triggered by the logger after flushing
    /// all queued messages.
    void exit_with_default_sighandler(int a_signo);
} // namespace detail


/// Install signal handler that catches FATAL C-runtime or OS signals.
/// The signals handled by default (if \a a_signals is NULL):
/// * SIGABRT - ABORT (ANSI), abnormal termination
/// * SIGFPE  - Floating point exception (ANSI): http://en.wikipedia.org/wiki/SIGFPE
/// * SIGILL  - ILlegal instruction (ANSI)
/// * SIGSEGV - Segmentation violation i.e. illegal memory reference
/// * SIGTERM - TERMINATION (ANSI)
bool install_sighandler(bool a_install, const sigset_t* a_signals = nullptr);

} // namespace utxx::logger