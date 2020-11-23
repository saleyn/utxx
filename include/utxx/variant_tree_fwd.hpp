//----------------------------------------------------------------------------
/// \file  variant_tree_fwd.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file contains a tree class that can hold variant values.
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

#include <boost/property_tree/ptree.hpp>
#include <utxx/variant_tree_path.hpp>

namespace utxx {

    namespace config { struct validator; }

    template <class Ch>
    struct basic_variant_tree_data : public variant {
        using path_t = basic_tree_path<Ch>;

        basic_variant_tree_data()
            : m_schema_validator(nullptr)
        {}

        basic_variant_tree_data(
            const variant&           v,
            const path_t&            r = path_t(),
            const config::validator* validator=nullptr
        )   : variant(v)
            , m_root_path(r)
            , m_schema_validator(validator)
        {}

        basic_variant_tree_data(
            const std::basic_string<Ch>& v,
            const path_t&                r = path_t(),
            const config::validator*     validator=nullptr
        )   : variant(v)
            , m_root_path(r)
            , m_schema_validator(validator)
        {}

        basic_variant_tree_data(
            const char*              v,
            const path_t&            r = path_t(),
            const config::validator* validator=nullptr
        )   : variant(v)
            , m_root_path(r)
            , m_schema_validator(validator)
        {}

        basic_variant_tree_data(const basic_variant_tree_data<Ch>& v)
            : variant(v)
            , m_root_path(v.root_path())
            , m_schema_validator(v.validator())
        {}

        basic_variant_tree_data(basic_variant_tree_data<Ch>&& v)
            : variant(std::move(v.value()))
            , m_root_path(std::move(v.root_path()))
            , m_schema_validator(v.validator())
        {}

        basic_variant_tree_data(
            variant&&                v,
            path_t&&                 r = path_t(),
            const config::validator* validator=nullptr
        )   : variant(std::move(v))
            , m_root_path(std::move(r))
            , m_schema_validator(validator)
        {}

        void  operator=(basic_variant_tree_data<Ch> const& r) {
            *(variant*)this = (variant const&)r;
            m_root_path = r.m_root_path;
            if (r.m_schema_validator)
                m_schema_validator = r.m_schema_validator;
        }

        void  operator=(variant               const& v) { *(variant*)this = v; }
        void  operator=(std::basic_string<Ch> const& v) { *(variant*)this = v; }

        void  operator=(basic_variant_tree_data<Ch>&& r) {
            *(variant*)this = std::move((variant&&)r);
            m_root_path = std::move(r.m_root_path);
            if (r.m_schema_validator)
                m_schema_validator = r.m_schema_validator;
        }

        void  operator=(variant&  r)             { value() = r;                }
        void  operator=(variant&& r)             { value() = std::move(r);     }

        variant&                    value()      { return *(variant*)this;     }
        variant const&              value() const{ return *(variant const*)this;}

        path_t const&  root_path()      const { return m_root_path; }
        void root_path(const path_t& p)       { m_root_path = p;    }
        void root_path(const path_t& p) const { m_root_path = p;    }

        const config::validator*    validator()    const { return m_schema_validator; }
        void validator(const config::validator* p) const { m_schema_validator = p;    }

    private:
        mutable path_t                   m_root_path;
        mutable const config::validator* m_schema_validator;
    };

    template <class Ch>
    using basic_variant_tree_base =
        boost::property_tree::basic_ptree<
            std::basic_string<Ch>,
            basic_variant_tree_data<Ch>,
            std::less<std::basic_string<Ch>>
        >;

    template <typename Ch> class basic_variant_tree;

    using variant_tree_base = basic_variant_tree_base<char>;
    using variant_tree      = basic_variant_tree<char>;

} // namespace utxx
