//----------------------------------------------------------------------------
/// \file   meta_tuple.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file contains some generic metaprogramming functions.
///
// Content copied from: http://functionalcpp.wordpress.com/page/2
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-01
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

#include <type_traits>

namespace utxx   {
namespace detail {

    template<size_t index>
    struct tuple_eval_helper {
        template<class Func,class TArgs,class... Args>
            static auto evaluate(Func&& f, TArgs&& t_args, Args&&... args)
            -> decltype(tuple_eval_helper<index-1>::evaluate(
                    std::forward<Func>(f),
                    std::forward<TArgs>(t_args),
                    std::get<index>(std::forward<TArgs>(t_args)),
                    std::forward<Args>(args)...
            ))
            {
                return tuple_eval_helper<index-1>::evaluate(
                    std::forward<Func>(f),
                    std::forward<TArgs>(t_args),
                    std::get<index>(std::forward<TArgs>(t_args)),
                    std::forward<Args>(args)...
                );
            }
    };

    template<>
    struct tuple_eval_helper<0> {
        template<class Func,class TArgs,class... Args>
            static auto evaluate(Func&& f, TArgs&& t_args, Args&&... args)
            -> decltype(eval(
                    std::forward<Func>(f),
                    std::get<0>(std::forward<TArgs>(t_args)),
                    std::forward<Args>(args)...
            ))
            {
                return eval(
                    std::forward<Func>(f),
                    std::get<0>(std::forward<TArgs>(t_args)),
                    std::forward<Args>(args)...
                );
            }
    };


} // namespace detail

/// This function allows to apply a tuple of arguments to a function TFunc
///
/// Example:
/// \code
/// int x = tuple_eval(std::plus<int>(),std::forward_as_tuple(1,2));
/// \endcode
template<class Func,class TArgs>
auto tuple_eval(Func&& f, TArgs&& t_args)
-> decltype(tuple_eval_helper<std::tuple_size<
    typename std::remove_reference<TArgs>::type>::value-1>::evaluate(
        std::forward<Func>(f),
        std::forward<TArgs>(t_args)
    ))
{
    return tuple_eval_helper<std::tuple_size<
        typename std::remove_reference<TArgs>::type>::value-1>::evaluate(
            std::forward<Func>(f),
            std::forward<TArgs>(t_args)
        );
}

} // namespace utxx
