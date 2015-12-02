//----------------------------------------------------------------------------
/// \file  os.hpp
//----------------------------------------------------------------------------
/// \brief General purpose functions for interacting with OS
//----------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-11-30
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
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <utxx/compiler_hints.hpp>

namespace utxx {
namespace os   {

/// Return environment variable or given default value
inline const char* getenv(const char* a_name, const char* a_default = "") {
    auto var = ::getenv(a_name);
    return var ? var : a_default;
}

/// Return effective username
inline std::string username() {
    char buf[L_cuserid];
    return unlikely(cuserid(buf) == nullptr) ? "" : buf;
}

} // namespace os
} // namespace utxx
