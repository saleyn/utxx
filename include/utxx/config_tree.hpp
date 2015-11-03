//----------------------------------------------------------------------------
/// \file config_tree.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// Defines config_tree, config_path, and config_error classes for
/// ease of configuration management.
//----------------------------------------------------------------------------
// Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-06-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is a part of utxx open-source project.

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

#include <utxx/variant_tree.hpp>

/// Throw utxx::config_error exception with current source location information
#define UTXX_THROW_CONFIG_ERROR(...) \
    UTXX_SRC_THROW(utxx::config_error, UTXX_SRC, ##__VA_ARGS__)
/// Throw utxx::config_error exception with current source location and cached fun name
#define UTXX_THROWX_CONFIG_ERROR(...) \
    UTXX_SRC_THROW(utxx::config_error, UTXX_SRCX, ##__VA_ARGS__)

namespace utxx {

    /// Container for storing configuration data.
    using config_tree       = variant_tree;
    using config_path       = tree_path;

    using config_error      = variant_tree_error;
    using config_bad_data   = variant_tree_bad_data;
    using config_bad_path   = variant_tree_bad_path;

} // namespace utxx