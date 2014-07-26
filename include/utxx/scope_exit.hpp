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
#ifndef _UTXX_SCOPE_EXIT_HPP_
#define _UTXX_SCOPE_EXIT_HPP_

#if __cplusplus >= 201103L

namespace utxx {

/// \brief Call lambda function on exiting scope
class scope_exit {
    std::function<void()> m_lambda;
    bool                  m_disable;

    scope_exit() = delete;
public:
    scope_exit(const std::function<void()>& a_lambda)
        : m_lambda (a_lambda)
        , m_disable(false)
    {}

    scope_exit(std::function<void()>&& a_lambda)
        : m_lambda(a_lambda)
        , m_disable(false)
    {
        a_lambda = nullptr;
    }

    /// If disabled the lambda won't be called at scope exit
    void disable(bool a_disable) { m_disable = a_disable; }

    ~scope_exit() {
        if (!m_disable)
            m_lambda();
    }
};

} // namespace utxx

#endif

#endif //_UTXX_SCOPE_EXIT_HPP_

