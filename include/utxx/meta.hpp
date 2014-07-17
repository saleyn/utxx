//----------------------------------------------------------------------------
/// \file   meta.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file contains some generic metaprogramming functions.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-31
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
#ifndef _UTXX_META_HPP_
#define _UTXX_META_HPP_

#include <boost/static_assert.hpp>

namespace utxx {

template <size_t N, size_t Base>
struct log {
    static const size_t value = 1 + log<N / Base, Base>::value;
};

template <size_t Base> struct log<1, Base> { static const size_t value = 0; };
template <size_t Base> struct log<0, Base>;

template <size_t N, size_t Power>
struct pow {
    static const size_t value = N * pow<N, Power - 1>::value;
};

template <size_t N>     struct pow<N, 0>        { static const size_t value = 1; };
template <size_t Power> struct pow<0, Power>    { static const size_t value = 0; };

/// Computes the power of \a base equal or greater than number \a n.
template <size_t N, size_t Base = 2>
class upper_power {
    static const size_t s_log = log<N,Base>::value;
    static const size_t s_pow = pow<Base,s_log>::value;
public:
    static const size_t value = s_pow == N ? N : s_pow*Base;
};

/// Given the size N and alignment size get the number of padding and aligned 
/// space needed to hold the structure of N bytes.
template<int N, int Size>
class align {
    static const int multiplier = Size / N;
    static const int remainder  = Size % N;
public:
    static const int size       = remainder > 0 ? (multiplier+1) * N : Size;
    static const int padding    = size - Size;
};

#if __cplusplus >= 201103L
/// Convert strongly typed enum to underlying type
/// \code
/// enum class B { B1 = 1, B2 = 2 };
/// std::cout << utxx::to_underlying(B::B2) << std::endl;
/// \endcode
template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

namespace {
    template<int N, char... Chars>
    struct to_int_helper;

    template<int N, char C>
    struct to_int_helper<N, C> {
        static const size_t value = ((size_t)C) << (8*N);
    };

    template <int N, char C, char... Tail>
    struct to_int_helper<N, C, Tail...> {
        static const size_t value =
            ((size_t)C) << (8*N) | to_int_helper<N-1, Tail...>::value;
    };
}

template <char... Chars>
struct to_int {
    static const size_t value = to_int_helper<(sizeof...(Chars))-1, Chars...>::value;
};

#endif

} // namespace utxx

#endif // _UTXX_META_HPP_

