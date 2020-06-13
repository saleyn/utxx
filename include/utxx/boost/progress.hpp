//----------------------------------------------------------------------------
/// \file  boost_progress.cpp
//----------------------------------------------------------------------------
/// \brief Compatibility file for boost::progress in different boost versions
//----------------------------------------------------------------------------
// Copyright (c) 2020 Serge Aleynikov <saleyn@gmail.com>
// Created: 2020-06-10
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
#pragma once

#include <boost/version.hpp>

// In older boost versions the location and name of the boost
// progress_display was different

#if BOOST_VERSION > 107000

#include <boost/timer/progress_display.hpp>
namespace utxx { using boost_progress = boost::timer::progress_display; }

#else

#include <boost/progress.hpp>
namespace utxx { using boost_progress = boost::progress_display; }

#endif

