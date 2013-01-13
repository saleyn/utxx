//----------------------------------------------------------------------------
/// \file  cpu.hpp
//----------------------------------------------------------------------------
/// \brief This file contains some CPU specific informational functions.
/// Most of the code in this file is C++ implementation of some C code found
/// in:
///   http://software.intel.com/en-us/articles/optimal-performance-on-multithreaded-software-with-intel-tools
///   http://software.intel.com/en-us/articles/intel-64-architecture-processor-topology-enumeration
/// copyrighted by Gail Lyons
///
/// \see Memory part 2: CPU caches              (http://lwn.net/Articles/252125)
/// \see Memory part 5: What programmers can do (http://lwn.net/Articles/255364)
//----------------------------------------------------------------------------
// Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-16
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

#ifndef _UTXX_CPU_HPP_
#define _UTXX_CPU_HPP_

#if defined(__x86_64__) || defined(__i386__)
#  include <utxx/detail/cpu_x86.hpp>
#else
#  error Platform not supported!
#endif

#endif // _UTXX_CPU_HPP_
