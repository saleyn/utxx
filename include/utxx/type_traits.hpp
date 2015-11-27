//----------------------------------------------------------------------------
/// \file  type_traits.hpp
//----------------------------------------------------------------------------
/// \brief This file contains static metafunctions
//----------------------------------------------------------------------------
// Copyright (c) 2011 Travis Gocke
// Created: 2011-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****
*
*   Copyright 2011 Travis Gockel
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*  KNOWN ISSUES
*   - If the signature and name match, but the function is private, compilation
*     will fail, complaining about how the function is private.
*
***** END LICENSE BLOCK *****
*/
#pragma once

#include <type_traits>

namespace utxx {
namespace detail {

    template <typename T, typename NameGetter>
    struct has_member_impl {
        typedef char matched_return_type;
        typedef long unmatched_return_type;

        template <typename C>
        static matched_return_type f(typename NameGetter::template get<C>*);

        template <typename C>
        static unmatched_return_type f(...);

    public:
        static const bool value = (sizeof(f<T>(0)) == sizeof(matched_return_type));
    };

} // namespace detail

template <typename T, typename NameGetter>
struct has_member:
    std::integral_constant<bool, detail::has_member_impl<T, NameGetter>::value>
{};

} // namespace utxx