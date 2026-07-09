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
#include <utxx/variant_tree_fwd.hpp>
#include <utxx/typeinfo.hpp>
#include <string>
#include <stdlib.h>

// Specializations of utxx::ptree::translator_between for variant_tree_data.
// These live in the utxx::ptree namespace to match the pattern used by
// boost::property_tree::translator_between specializations.
namespace utxx {
namespace ptree {

    // Custom translator that works with utxx::variant instead of std::string.
    // This translator is used to read/write values from files.
    template <typename Ch>
    struct translator_between<utxx::basic_variant_tree_data<Ch>, std::basic_string<Ch>>
    {
        using type = translator_between<utxx::basic_variant_tree_data<Ch>, std::basic_string<Ch>>;
        using external_type = std::basic_string<Ch>;
        using internal_type = utxx::basic_variant_tree_data<Ch>;

        explicit translator_between(bool allow_int_suffixes = true)
            : m_allow_int_suffixes(allow_int_suffixes)
        {}

        std::optional<external_type> get_value(const internal_type& value) const {
            return std::optional<external_type>(value.is_null() ? "" : value.to_str());
        }

        std::optional<internal_type> put_value(const external_type& value) const {
            if (value.empty())
                return internal_type();
            const char* end = value.c_str() + value.size();
            int base = 10;
            if (value.size() > 1 && value[0] == '0') {
                char c = value[1];
                if (c == 'x')
                    base = 16;
                else if ('0' <= c && c <= '7')
                    base = 8;
            }
            char* e = nullptr;
            long  n = strtol(value.c_str(), &e, base);
            // Note that value.size() can be 1, so we need both tests
            if (e > value.c_str() && e >= end-1) { // is an integer and has been decoded fully
                if (!*e)
                    return std::optional<internal_type>(internal_type(utxx::variant(n)));

                if (!m_allow_int_suffixes)
                    return std::optional<internal_type>(value);
                else if (e == end-1) {
                    switch (toupper(*e)) {
                        case 'K': n *= 1024;           break;
                        case 'M': n *= 1024*1024;      break;
                        case 'G': n *= 1024*1024*1024; break;
                        default:
                            return std::optional<internal_type>(value);
                    }
                    return std::optional<internal_type>(internal_type(utxx::variant(n)));
                }
            }
            double d = strtod(value.c_str(), &e);
            if (!*e && e == end) // is a double and has been decoded fully
                return std::optional<internal_type>(internal_type(utxx::variant(d)));

            if (value == "true" || value == "false")
                return std::optional<internal_type>(internal_type(utxx::variant(value[0] == 't')));
            return std::optional<internal_type>(internal_type(utxx::variant(value)));
        }

        // Custom SCON extension to distinguish between values: 123 and \"123\",
        // with the first being an integer and the second being a string
        std::optional<internal_type> put_value(const std::string& value, bool is_str) const {
            return is_str ? std::optional<internal_type>(internal_type(utxx::variant(value)))
                          : put_value(value);
        }
    private:
        bool m_allow_int_suffixes;
    };

    template <typename Ch>
    struct translator_between<utxx::basic_variant_tree_data<Ch>, utxx::basic_variant_tree_data<Ch>>
    {
        using external_type = utxx::basic_variant_tree_data<Ch>;
        using internal_type = utxx::basic_variant_tree_data<Ch>;
        using type          = translator_between<internal_type, external_type>;

        std::optional<external_type> get_value(const internal_type& val) const { return val; }
        std::optional<internal_type> put_value(external_type        val) const { return val; }
    };

    template <typename Ch, typename T>
    struct translator_between<utxx::basic_variant_tree_data<Ch>, T>
    {
        using external_type = T;
        using internal_type = utxx::basic_variant_tree_data<Ch>;
        using type          = translator_between<internal_type, external_type>;

        std::optional<external_type> get_value(const internal_type& val) const { return val.value().template to<T>(); }
        std::optional<internal_type> put_value(external_type        val) const { return internal_type(utxx::variant(val)); }
    };

    template <typename Ch>
    struct translator_between<utxx::basic_variant_tree_data<Ch>, utxx::variant>
    {
        using external_type = utxx::variant;
        using internal_type = utxx::basic_variant_tree_data<Ch>;
        using type          = translator_between<internal_type, external_type>;

        template <typename T>
        std::optional<T> get_value(const internal_type& val) const { return val.value().template to<T>(); }

        template <typename T>
        std::optional<internal_type> put_value(const T&             val) const { return internal_type(val); }
        std::optional<internal_type> put_value(const external_type& val) const { return internal_type(val); }
    };

    template <typename Ch>
    struct translator_between<utxx::basic_variant_tree_data<Ch>, std::nullptr_t>
    {
        using external_type = std::nullptr_t;
        using internal_type = utxx::basic_variant_tree_data<Ch>;
        using type          = translator_between<internal_type, external_type>;

        std::optional<external_type> get_value(internal_type& v) const { return v.value().template get<std::nullptr_t>(); }
        std::optional<internal_type> put_value(external_type)    const { return internal_type(); }
    };

} // namespace ptree
} // namespace utxx

namespace utxx {
namespace detail {

    // Custom translator that works with variant instead of std::string
    // This translator is used to get/put values through explicit get/put calls.
    template <typename Ext, typename Ch>
    struct variant_translator
    {
        using external_type = Ext;
        using internal_type = basic_variant_tree_data<Ch>;

        external_type get_value(const internal_type& value) const {
            return value.template get<external_type>();
        }

        template<typename T>
        std::enable_if_t<variant::is_valid_type_v<T>, internal_type>
        put_value(T value) const { return internal_type(variant(value)); }
    };

    template <typename Ch>
    struct basic_translator_from_string
        : public utxx::ptree::translator_between<
            basic_variant_tree_data<Ch>, std::basic_string<Ch>
        >
    {
        using string_t  = std::basic_string<Ch>;
        using tree      = utxx::ptree::basic_ptree<
                            string_t, basic_variant_tree_data<Ch>, std::less<string_t>>;
        using path_type = utxx::ptree::string_path<
                            string_t,
                            utxx::ptree::id_translator<string_t>>;

        std::optional<variant> operator() (const tree& a_tree) const {
            // Get as string and put it as variant by trying to infer value type
            const string_t& s = a_tree.get_value().to_str();
            return *this->put_value(s);
        }

        const basic_variant_tree_data<Ch>& operator() (const path_type& a_path, const basic_variant_tree_data<Ch>& value) const {
            return value;
        }
    };

    using translator_from_string = basic_translator_from_string<char>;

} // namespace detail
} // namespace utxx
