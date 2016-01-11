//----------------------------------------------------------------------------
/// \file   compiler_hints.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Definition of some types used for optimization.
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

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#pragma once

// Branch prediction optimization (see http://lwn.net/Articles/255364/)

#ifndef NO_HINT_BRANCH_PREDICTION
#  ifndef  LIKELY
#   define LIKELY(expr)    __builtin_expect(!!(expr),1)
#  endif
#  ifndef  UNLIKELY
#   define UNLIKELY(expr)  __builtin_expect(!!(expr),0)
#  endif
#else
#  ifndef  LIKELY
#   define LIKELY(expr)    (expr)
#  endif
#  ifndef  UNLIKELY
#   define UNLIKELY(expr)  (expr)
#  endif
#endif

namespace utxx {

#define UTXX_STRINGIFY(x) #x
#define UTXX_TOSTRING(x)                    UTXX_STRINGIFY(x)
#define UTXX_FILE_SRC_LOCATION __FILE__ ":" UTXX_TOSTRING(__LINE__)

// Though the compiler should optimize this inlined code in the same way as
// when using LIKELY/UNLIKELY macros directly the preference is to use the later
#ifndef NO_HINT_BRANCH_PREDICTION
    inline bool likely(bool expr)   { return __builtin_expect((expr),1); }
    inline bool unlikely(bool expr) { return __builtin_expect((expr),0); }
#else
    inline bool likely(bool expr)   { return expr; }
    inline bool unlikely(bool expr) { return expr; }
#endif

#define UTXX_PP_IIF_0(t, f)      f
#define UTXX_PP_IIF_1(t, f)      t
#define UTXX_PP_IIF_I(bit, t, f) UTXX_PP_IIF_##bit(t, f)
#define UTXX_PP_IIF(bit, t, f)   UTXX_PP_IIF_I(bit, t, f)

/// Evaluate compile-time condition.
/// If the condition is true, call "LIKELY(expr)", else call "UNLIKELY(expr)".
#define UTXX_CHECK(condition, expr) \
    UTXX_PP_IIF(condition, LIKELY(expr), UNLIKELY(expr))

/// A helper function used to signify an "out" argument in a function call
/// \code
///   int i = 10;
///   test("abc", out(i));
/// \endcode
template <typename T>
constexpr T& out(T& arg) { return arg; }

/// A helper function used to signify an "in/out" argument in a function call
/// \code
///   int i = 10;
///   test("abc", inout(i));
/// \endcode
template <typename T>
constexpr T& inout(T& arg) { return arg; }

} // namespace utxx