//----------------------------------------------------------------------------
/// \file   enum_helper.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file defines enum stringification declaration macro.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-31
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/punctuation/is_begin_parens.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

// Type value may be in one of three forms:
//      char                // EnumType
//      (char, 0)           // EnumType, DefaultValue
//      (char, Undef, 0)    // EnumType, UndefinedItemName, DefaultValue
//
// The following three macros get the {Type, Name, Value} accordingly:
#define UTXX_ENUM_GET_TYPE(type)       UTXX_ENUM_INTERNAL_1__(0, type)
#define UTXX_ENUM_GET_UNDEF_NAME(type) UTXX_ENUM_INTERNAL_1__(1, type)
#define UTXX_ENUM_GET_UNDEF_VAL(type)  UTXX_ENUM_INTERNAL_1__(2, type)

#define UTXX_ENUM_INTERNAL_1__(N, arg)                                         \
    BOOST_PP_IIF(                                                              \
        BOOST_PP_IS_BEGIN_PARENS(arg),                                         \
        UTXX_ENUM_INTERNAL_2__(N, arg),                                        \
        UTXX_ENUM_INTERNAL_2__(N, (arg, UNDEFINED, 0)))

#define UTXX_ENUM_INTERNAL_2__(N, arg)                                         \
    BOOST_PP_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(arg),2),                 \
        BOOST_PP_TUPLE_ELEM(N, (BOOST_PP_TUPLE_ENUM(arg), _)),                 \
        BOOST_PP_TUPLE_ELEM(N, (BOOST_PP_TUPLE_ELEM(0, arg),                   \
                                UNDEFINED,                                     \
                                BOOST_PP_TUPLE_ELEM(1, arg))))

// Internal macros for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_ENUM_INTERNAL_GET_0__(x, _, val)     BOOST_PP_TUPLE_ELEM(0, val)

// Optionally enable support of BOOST serialization in enums
#ifdef UTXX_ENUM_SUPPORT_SERIALIZATION
# ifndef UTXX__ENUM_FRIEND_SERIALIZATION__
#   include <boost/serialization/access.hpp>

#   define UTXX__ENUM_FRIEND_SERIALIZATION__ \
    friend class boost::serialization::access
# endif
#else
# ifndef UTXX__ENUM_FRIEND_SERIALIZATION__
#   define UTXX__ENUM_FRIEND_SERIALIZATION__
# endif
#endif
