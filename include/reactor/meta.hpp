// vim:ts=2:sw=2:et
//----------------------------------------------------------------------------
/// \file   meta.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Generic metaprogramming utilities — standalone copy.
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

***** END LICENSE BLOCK *****
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

namespace utxx {

template <size_t N, size_t Base>
struct log {
  static const size_t value = 1 + log<N / Base, Base>::value;
};
template <size_t Base>
struct log<1, Base> {
  static const size_t value = 0;
};
template <size_t Base>
struct log<0, Base>;

template <size_t N, size_t Power>
struct pow {
  static const size_t value = N * pow<N, Power - 1>::value;
};
template <size_t N>
struct pow<N, 0> {
  static const size_t value = 1;
};
template <size_t Power>
struct pow<0, Power> {
  static const size_t value = 0;
};

template <size_t N, size_t Base>
class upper_power {
  static const size_t s_log = log<N, Base>::value;
  static const size_t s_pow = pow<Base, s_log>::value;

public:
  static const size_t value = s_pow == N ? N : s_pow * Base;
};
template <size_t Base>
class upper_power<0, Base> {
public:
  static const size_t value = 0;
};

template <size_t N>
using upper_power2 = upper_power<N, 2>;

template <int N, int Size>
class align {
  static const int multiplier = Size / N;
  static const int remainder  = Size % N;

public:
  static const int size    = remainder > 0 ? (multiplier + 1) * N : Size;
  static const int padding = size - Size;
};

template <typename T, typename U>
struct is_same_decayed : std::is_same<typename std::decay<T>::type, U>::type {};

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e)
{ return static_cast<typename std::underlying_type<E>::type>(e); }

template <typename BaseT, typename T, typename R = T>
using if_base_of = typename std::enable_if<std::is_base_of<BaseT, T>::value, R>::type;

template <typename BaseT, typename T, typename R = T>
using if_not_base_of = typename std::enable_if<!std::is_base_of<BaseT, T>::value, R>::type;

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...) const> {
  enum { arity = sizeof...(Args) };
  typedef ReturnType          result_type;
  typedef std::tuple<Args...> args_tuple;
  template <size_t i>
  struct arg {
    typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
  };
};

template <typename... Ts>
struct last_type {};
template <typename T0, typename T1, typename... Ts>
struct last_type<T0, T1, Ts...> : last_type<T1, Ts...> {};
template <typename T0>
struct last_type<T0> {
  using type = T0;
};

template <typename T>
struct remove_cvref {
  using type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
};
template <class T>
using remove_cvr_t = typename remove_cvref<T>::type;

template <typename T, typename U>
static constexpr const bool is_same()
{ return std::is_same<typename remove_cvref<T>::type, typename remove_cvref<U>::type>::value; }

} // namespace utxx
