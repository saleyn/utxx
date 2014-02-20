//----------------------------------------------------------------------------
/// \file   singleton.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Singleton implementation.
//----------------------------------------------------------------------------
// Created: 2009-11-16
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

#ifndef _UTXX_SINGLETON_HPP_
#define _UTXX_SINGLETON_HPP_

namespace utxx {

/// Create one instance of a class on demand.

template <class T>
class singleton : private T {
private:
    singleton()  {}
    ~singleton() {}

public:
    static T& instance() {
        static singleton<T> s_instance;
        return s_instance;
    }
};

} // namespace utxx

#endif // _UTXX_SINGLETON_HPP_
