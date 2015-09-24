//----------------------------------------------------------------------------
/// \file   variant.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Variant class that represents a subtype of
/// boost::variant that can hold integer/bool/double/string values.
//----------------------------------------------------------------------------
// Created: 2015-09-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/variant.hpp>
#include <utxx/error.hpp>

namespace utxx {

template<> double variant::to<double>() const {
    switch (type()) {
        case TYPE_NULL:     UTXX_THROW_RUNTIME_ERROR("Unassigned variant value!");
        case TYPE_BOOL:     return to_bool() ? 1.0 : 0.0;
        case TYPE_INT:      return (double)to_int();
        case TYPE_DOUBLE:   return to_double();
        case TYPE_STRING:   return std::atof(c_str());
        default:            UTXX_THROW_RUNTIME_ERROR("Unsupported variant type: ", type());
    }
}

template<> int64_t variant::to<int64_t>() const {
    switch (type()) {
        case TYPE_NULL:     UTXX_THROW_RUNTIME_ERROR("Unassigned variant value!");
        case TYPE_BOOL:     return (int64_t)to_bool();
        case TYPE_INT:      return to_int();
        case TYPE_DOUBLE:   return (int64_t)to_double();
        case TYPE_STRING:   return std::atol(c_str());
        default:            UTXX_THROW_RUNTIME_ERROR("Unsupported variant type: ", type());
    }
}

template<> int variant::to<int>() const {
    return (int)to<int64_t>();
}

template<> uint64_t variant::to<uint64_t>() const {
    return (uint64_t)to<int64_t>();
}

template<> uint32_t variant::to<uint32_t>() const {
    return (uint32_t)to<int64_t>();
}

template<> bool variant::to<bool>() const {
    switch (type()) {
        case TYPE_NULL:     UTXX_THROW_RUNTIME_ERROR("Unassigned variant value!");
        case TYPE_BOOL:     return to_bool();
        case TYPE_INT:      return to_int()    != 0;
        case TYPE_DOUBLE:   return to_double() != 0.0;
        case TYPE_STRING:   {
            auto& s = to_str();
            return s.size() &&
                !(!strcasecmp(s.c_str(), "FALSE") ||
                  !strcasecmp(s.c_str(), "NO")    ||
                  !strcasecmp(s.c_str(), "OFF")   ||
                   strchr("FN0", std::toupper(s.c_str()[0])));
        }
        default:            UTXX_THROW_RUNTIME_ERROR("Unsupported variant type: ", type());
    }
}

template<> std::string variant::to<std::string>() const {
    switch (type()) {
        case TYPE_NULL:     UTXX_THROW_RUNTIME_ERROR("Unassigned variant value!");
        case TYPE_BOOL:     return to_bool() ? "true" : "false";
        case TYPE_INT:      return std::to_string(to_int());
        case TYPE_DOUBLE:   return std::to_string(to_double());
        case TYPE_STRING:   return to_str();
        default:            UTXX_THROW_RUNTIME_ERROR("Unsupported variant type: ", type());
    }
}

} // namespace utxx