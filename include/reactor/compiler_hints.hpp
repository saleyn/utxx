// vim:ts=2:sw=2:et
//----------------------------------------------------------------------------
/// \file   compiler_hints.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Branch-prediction hints and helper utilities — standalone copy.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
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

#ifndef NO_HINT_BRANCH_PREDICTION
#ifndef LIKELY
#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#endif
#ifndef UNLIKELY
#define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#endif
#else
#ifndef LIKELY
#define LIKELY(expr) (expr)
#endif
#ifndef UNLIKELY
#define UNLIKELY(expr) (expr)
#endif
#endif

namespace utxx {

#define REACTOR_STRINGIFY(x) #x
#define REACTOR_TOSTRING(x)  REACTOR_STRINGIFY(x)

#ifndef NO_HINT_BRANCH_PREDICTION
inline bool likely(bool expr)
{ return __builtin_expect((expr), 1); }
inline bool unlikely(bool expr)
{ return __builtin_expect((expr), 0); }
#else
inline bool likely(bool expr)
{ return expr; }
inline bool unlikely(bool expr)
{ return expr; }
#endif

template <typename T>
constexpr T& out(T& arg)
{ return arg; }

template <typename T>
constexpr T& inout(T& arg)
{ return arg; }

} // namespace utxx
