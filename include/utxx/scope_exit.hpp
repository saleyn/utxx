//----------------------------------------------------------------------------
/// \file   scope_exit.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Provides a convenient class that executes a lambda on scope exit
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-07-24
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

#if __cplusplus >= 201103L

#include <functional>
#include <utxx/compiler_hints.hpp>

namespace utxx {

/// Call lambda function on exiting scope
template <typename Lambda>
class on_scope_exit {
    Lambda m_lambda;
    bool   m_disable;

    on_scope_exit() = delete;
public:
    on_scope_exit(const Lambda& a_lambda)
        : m_lambda (a_lambda)
        , m_disable(false)
    {}

    on_scope_exit(Lambda&& a_lambda)
        : m_lambda(std::move(a_lambda))
        , m_disable(false)
    {}

    /// If disabled the lambda won't be called at scope exit
    void disable() { m_disable = true;  }
    void enable()  { m_disable = false; }

    ~on_scope_exit() {
        if (!m_disable)
            m_lambda();
    }
};

/// Call functor on exiting scope
class scope_exit : public on_scope_exit<std::function<void()>> {
    using base = on_scope_exit<std::function<void()>>;
public:
    using base::base;
};

#define UTXX_GLUE2(A,B,C) A##B##C
#define UTXX_GLUE(A,B,C) UTXX_GLUE2(A,B,C)
#define UTXX_SCOPE_EXIT(Fun) \
    auto UTXX_GLUE(__on_exit, __LINE__, _fun) = Fun; \
    utxx::on_scope_exit<decltype(UTXX_GLUE(__on_exit, __LINE__, _fun))> \
        UTXX_GLUE(__on_exit, __LINE__, _var) (UTXX_GLUE(__on_exit, __LINE__, _fun))

} // namespace utxx

#endif
