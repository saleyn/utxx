//----------------------------------------------------------------------------
/// \file  handler_traits.cpp
//----------------------------------------------------------------------------
/// \brief This file implements metaprogramming method signature detection.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

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
#ifndef _UTIL_HANDLER_TRAITS_HPP_
#define _UTIL_HANDLER_TRAITS_HPP_

namespace util {

namespace {
    template <typename T, typename Message, void (T::*)(const Message&)>
    struct signature {
        typedef T type;
    };

    template<typename T, typename Msg, typename M = T>
    struct sign_on_data {
        static const bool value = false;
    };

    template<typename T, typename Msg>
    struct sign_on_data<T, Msg, typename signature<T, Msg, &T::on_data>::type > {
        static const bool value = true;
    };

    template<typename T, typename Msg, typename M = T>
    struct sign_on_packet {
        static const bool value = false;
    };

    template<typename T, typename Msg>
    struct sign_on_packet<T, Msg, typename signature<T, Msg, &T::on_packet>::type > {
        static const bool value = true;
    };

    template<typename T, typename Msg, typename M = T>
    struct sign_on_message {
        static const bool value = false;
    };

    template<typename T, typename Msg>
    struct sign_on_message<T, Msg, typename signature<T, Msg, &T::on_message>::type > {
        static const bool value = true;
    };
}

template <typename Processor, typename Message>
struct has_method {
    static const bool on_data    = sign_on_data   <Processor, Message>::value;
    static const bool on_packet  = sign_on_packet <Processor, Message>::value;
    static const bool on_message = sign_on_message<Processor, Message>::value;
};

} // namespace util

#endif // _UTIL_HANDLER_TRAITS_HPP_
