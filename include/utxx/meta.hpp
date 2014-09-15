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

#include <cstddef>

#if __cplusplus >= 201103L
#  include <type_traits>
#  include <tuple>
#  include <utility>
#endif

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

/// Computes the smallest power of \a base equal or greater than number \a n.
template <size_t N, size_t Base>
class upper_power {
    static const size_t s_log = log<N,Base>::value;
    static const size_t s_pow = pow<Base,s_log>::value;
public:
    static const size_t value = s_pow == N ? N : s_pow*Base;
};

// We need specialisation of "upper_power" for N=0, because the log cannot be
// computed in that case (as required in the generic case above).  The result
// is 0 in this case (the corresp exponent of Base is "-oo")    (XXX: even if
// Base==0):
template<size_t Base>
class upper_power<0, Base> {
public:
     static const size_t value  = 0;
};

template <size_t N>
using upper_power2 = upper_power<N, 2>;

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

// For generic types, directly use the result of the signature of its 'operator()'
template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};

// Specialization for pointers to member function
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
{
    /// Arity is the number of arguments
    enum { arity = sizeof...(Args) };

    typedef ReturnType          result_type;
    typedef std::tuple<Args...> args_tuple;

    template <size_t i>
    struct arg
    {
        // the i-th argument is equivalent to the i-th tuple element of a tuple
        // composed of those arguments.
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
};

//-----------------------------------------------------------------------------
// Evaluation of anything
//-----------------------------------------------------------------------------
// functions, functors, lambdas, etc.
template<
    class F, class... Args,
    class = typename std::enable_if<!std::is_member_function_pointer<F>::value>::type,
    class = typename std::enable_if<!std::is_member_object_pointer<F>::value>::type
    >
auto eval(F&& f, Args&&... args) -> decltype(f(std::forward<Args>(args)...))
{
    return f(std::forward<Args>(args)...);
}

// const member function
template<class R, class C, class... Args>
auto eval(R(C::*f)() const, const C& c, Args&&... args) -> R
{
    return (c.*f)(std::forward<Args>(args)...);
}

template<class R, class C, class... Args>
auto eval(R(C::*f)() const, C& c, Args&&... args) -> R
{
    return (c.*f)(std::forward<Args>(args)...);
}

// non-const member function
template<class R, class C, class... Args>
auto eval(R(C::*f)(), C& c, Args&&... args) -> R
{
    return (c.*f)(std::forward<Args>(args)...);
}

// member object
template<class R, class C>
auto eval(R(C::*m), const C& c) -> const R&
{
    return c.*m;
}

template<class R, class C>
auto eval(R(C::*m), C& c) -> R&
{
    return c.*m;
}

#endif

} // namespace utxx

#endif // _UTXX_META_HPP_

