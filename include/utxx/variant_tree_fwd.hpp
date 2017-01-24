//----------------------------------------------------------------------------
/// \file  variant_tree_fwd.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file contains a tree class that can hold variant values.
//----------------------------------------------------------------------------
// Created: 2010-07-10
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

#include <boost/property_tree/ptree.hpp>

namespace utxx {

    template <class Ch>
    using basic_variant_tree_base =
        boost::property_tree::basic_ptree<
            std::basic_string<Ch>,
            variant,
            std::less<std::basic_string<Ch>>
        >;

    template <typename Ch> class basic_variant_tree;

    typedef basic_variant_tree_base<char> variant_tree_base;
    typedef basic_variant_tree<char>      variant_tree;

} // namespace utxx
