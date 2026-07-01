// vim:ts=2:sw=2:et
//----------------------------------------------------------------------------
/// \file  typeinfo.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Runtime type information utilities — standalone copy.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
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

#include <string>
#include <typeinfo>
#if defined(__linux__)
#include <cxxabi.h>
#endif

namespace utxx {

namespace detail {
  inline std::string demangle(const char* a_type_name)
  {
#if defined(_WIN32) || defined(_WIN64)
    return a_type_name;
#else
    char   buf[256];
    size_t sz = sizeof(buf);
    int    error;
    abi::__cxa_demangle(a_type_name, buf, &sz, &error);
    return error < 0 ? "***undefined***" : buf;
#endif
  }
} // namespace detail

template <typename T>
std::string type_to_string()
{ return detail::demangle(typeid(T).name()); }

template <typename T>
std::string type_to_string(const T& t)
{ return detail::demangle(typeid(t).name()); }

template <typename T>
std::string type_to_string(T&& t)
{ return detail::demangle(typeid(t).name()); }

inline std::string demangle(const char* a_str)
{
  char temp[256];
  if (sscanf(a_str, "%*[^(]%*[^_]%255[^)+]", temp) == 1) return detail::demangle(temp);
  if (sscanf(a_str, "%255s", temp) == 1) return temp;
  return a_str;
}

} // namespace utxx
