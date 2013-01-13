//----------------------------------------------------------------------------
/// \file  config_validator.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of configuration validation class.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-09-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/config_validator.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace utxx {
namespace config {

const char* type_to_string(option_type_t a_type) {
    switch (a_type) {
        case STRING:    return "string";
        case INT:       return "int";
        case BOOL:      return "bool";
        case FLOAT:     return "float";
        case ANONYMOUS: return "anonymous";
        case BRANCH:    return "branch";
        default:        return "undefined";
    }
}

std::string option::to_string() const {
    std::stringstream s;
    s   << "option{name=" << name
        << ",type=" << type_to_string(opt_type);
    if (!description.empty())
        s << ",desc=\"" << description << '"';
    if (name_choices.size()) {
        s << ",names=["; bool l_first = true;
        BOOST_FOREACH(const std::string& v, name_choices) {
            if (!l_first) s << ";";
            l_first = false;
            s << '"' << v << '"';
        }
        s << ']';
    }
    if (value_choices.size()) {
        s << ",values=["; bool l_first = true;
        BOOST_FOREACH(const variant& v, value_choices) {
            if (!l_first) s << ";";
            l_first = false;
            s << value(v);
        }
        s << "]";
    }
    if (!default_value.is_null())
        s << ",default=" << value(default_value);
    if (!min_value.is_null())
        s << (value_type == STRING ? ",min_length=" : ",min=") << value(min_value);
    if (!max_value.is_null())
        s << (value_type == STRING ? ",max_length=" : ",max=") << value(max_value);
    s << ",required="   << (required ? "true" : "false");
    s << ",unique="     << (unique ? "true" : "false");
    if (children.size()) {
        bool l_first = true;
        s << ",children=[";
        BOOST_FOREACH(const typename option_map::value_type& o, children) {
            if (!l_first) s << ",";
            l_first = false;
            s << "\n  " << o.second.to_string();
        }
        s << "\n]";
    }
    s << "}";
    return s.str();
}

} // namespace utxx
} // namespace config
