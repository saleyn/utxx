//----------------------------------------------------------------------------
/// \file  variant_translator.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file contains a translator between variant and other types
//----------------------------------------------------------------------------
// Created: 2010-07-10
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

#include <utxx/variant.hpp>
#include <utxx/typeinfo.hpp>
#include <boost/property_tree/ptree.hpp>
#include <string>
#include <stdlib.h>

namespace boost {
namespace property_tree {
    // Custom translator that works with utxx::variant instead of std::string.
    // This translator is used to read/write values from files.
    template <>
    struct translator_between<utxx::variant, std::string>
    {
        typedef translator_between<utxx::variant, std::string> type;
        typedef std::string     external_type;
        typedef utxx::variant   internal_type;

        boost::optional<external_type> get_value(const internal_type& value) const {
            return boost::optional<external_type>(value.is_null() ? "" : value.to_str());
        }

        boost::optional<internal_type> put_value(const external_type& value) const {
            if (value.empty())
                return internal_type();
            const char* end = value.c_str() + value.size();
            char* e;
            int base = 10;
            if (value.size() > 1 && value[0] == '0') {
                char c = value[1];
                if (c == 'x')
                    base = 16;
                else if ('0' <= c && c <= '7')
                    base = 8;
            }
            long n = strtol(value.c_str(), &e, base);
            // Note that value.size() can be 1, so we need both tests
            if (e > value.c_str() && e >= end-1) { // is an integer and has been decoded fully
                if (!*e)
                    return boost::optional<internal_type>(n);

                if (e == end-1) {
                    switch (toupper(*e)) {
                        case 'K': n *= 1024; break;
                        case 'M': n *= 1024*1024; break;
                        case 'G': n *= 1024*1024*1024; break;
                        default:
                            return internal_type();
                    }
                    return boost::optional<internal_type>(n);
                }
            }
            double d = strtod(value.c_str(), &e);
            if (!*e && e == end) // is a double and has been decoded fully
                return boost::optional<internal_type>(d);

            if (value == "true" || value == "false")
                return boost::optional<internal_type>(value[0] == 't');
            return boost::optional<internal_type>(value);
        }

        // Custom SCON extension to distinguish between values: 123 and \"123\",
        // with the first being an integer and the second being a string
        boost::optional<internal_type> put_value(const std::string& value, bool is_str) const {
            return is_str ? boost::optional<internal_type>(value) : put_value(value);
        }
    };

    template <>
    struct translator_between<utxx::variant, const char*>
    {
        typedef translator_between<utxx::variant, const char*> type;
        boost::optional<const char*> get_value(const utxx::variant& val) const {
            return val.is_null() ? "" : val.to_str().c_str();
        }
        boost::optional<utxx::variant> put_value(const char* val) const {
            translator_between<utxx::variant, std::string> tr;
            return tr.put_value(std::string(val));
        }
    };

    template <>
    struct translator_between<utxx::variant, bool>
    {
        typedef translator_between<utxx::variant, bool> type;
        boost::optional<bool> get_value(const utxx::variant& val) const { return val.to_bool(); }
        boost::optional<utxx::variant> put_value(bool val) const { return utxx::variant(val); }
    };

    template <>
    struct translator_between<utxx::variant, int>
    {
        typedef translator_between<utxx::variant, int> type;
        boost::optional<int> get_value(const utxx::variant& val) const { return val.to_int(); }
        boost::optional<utxx::variant> put_value(int val) const { return utxx::variant(val); }
    };

    template <>
    struct translator_between<utxx::variant, long>
    {
        typedef translator_between<utxx::variant, long> type;
        boost::optional<long> get_value(const utxx::variant& val) const { return val.to_int(); }
        boost::optional<utxx::variant> put_value(long val) const { return utxx::variant(val); }
    };

    template <>
    struct translator_between<utxx::variant, double>
    {
        typedef translator_between<utxx::variant, double> type;
        boost::optional<double>  get_value(const utxx::variant& val) const { return val.to_float(); }
        boost::optional<utxx::variant> put_value(double val) const { return utxx::variant(val); }
    };

} // namespace property_tree
} // namespace boost

namespace utxx {
namespace detail {

    // Custom translator that works with variant instead of std::string
    // This translator is used to get/put values through explicit get/put calls.
    template <typename Ext>
    struct variant_translator
    {
        typedef Ext external_type;
        typedef variant internal_type;
        typedef variant::valid_types valid_types;

        external_type get_value(const internal_type& value) const {
            return value.get<external_type>();
        }

        template<typename T>
        typename boost::disable_if<
            boost::is_same<
                boost::mpl::end<valid_types>::type,
                boost::mpl::find<variant::valid_types, T>
            >,
            internal_type>::type
        put_value(T value) const { return variant(value); }
    };

    template <typename Ch>
    struct basic_translator_from_string
        : public boost::property_tree::translator_between<
            variant, std::basic_string<Ch>
        >
    {
        typedef
            std::basic_string<Ch>
            string_t;

        typedef
            boost::property_tree::basic_ptree<
                string_t, variant, std::less<string_t>
            > tree;

        typedef
            boost::property_tree::string_path<
                string_t,
                boost::property_tree::id_translator<string_t>
            > path_type;

        boost::optional<variant> operator() (const tree& a_tree) const {
            // Get as string and put it as variant by trying to infer value type
            const string_t& s = a_tree.get_value().to_str();
            return *this->put_value(s);
        }

        const variant& operator() (const path_type& a_path, const variant& value) const {
            return value;
        }
    };

    typedef
        basic_translator_from_string<char>
        translator_from_string;

} // namespace detail
} // namespace utxx
