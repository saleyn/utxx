//----------------------------------------------------------------------------
/// \file  variant_tree.hpp
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

#ifndef _UTXX_VARIANT_TREE_HPP_
#define _UTXX_VARIANT_TREE_HPP_

#include <utxx/variant.hpp>
#include <utxx/typeinfo.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/throw_exception.hpp>
#include <string>

class T;
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
            return boost::optional<external_type>(
                value.type() == internal_type::TYPE_NULL ? "" : value.to_string());
        }

        boost::optional<internal_type> put_value(const external_type& value) const {
            if (value.empty())
                return boost::optional<internal_type>(internal_type());
            char* end;
            long n = strtol(value.c_str(), &end, 10);
            if (!*end && end > value.c_str()) // is an integer and has been decoded fully
                return boost::optional<internal_type>(n);

            double d = strtod(value.c_str(), &end);
            if (!*end && end > value.c_str()) // is a double and has been decoded fully
                return boost::optional<internal_type>(d);

            if (value == "true" || value == "false")
                return boost::optional<internal_type>(value[0] == 't');
            return boost::optional<internal_type>(value);
        }
    };

} // namespace property_tree
} // namespace boost

namespace utxx {

namespace detail {

    // Custom translator that works with variant instead of std::string
    // This translator is used to get/put values through explicit get/put calls.
    template <class Ext>
    struct variant_translator
    {
        typedef Ext external_type;
        typedef variant internal_type;

        /*
        typedef boost::mpl::joint_view<variant::int_types,
                boost::mpl::vector<bool,double>
            > valid_non_string_types;
        */
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

} // namespace detail


// TODO: replace variant with basic_variant<Ch>

template <class Ch, class KeyComp = std::less<std::basic_string<Ch> > >
class basic_variant_tree
    : public boost::property_tree::basic_ptree<std::basic_string<Ch>, variant, KeyComp>
{
    typedef basic_variant_tree          self_type;
    typedef boost::property_tree::ptree ptree;
public:
    typedef boost::property_tree::basic_ptree<std::basic_string<Ch>, variant, KeyComp>
                                                            base;
    typedef boost::property_tree::ptree_bad_path            bad_path;
    typedef typename base::key_type                         key_type;
    typedef boost::property_tree::string_path<std::basic_string<Ch>,
                boost::property_tree::id_translator<std::basic_string<Ch> >
            >                                               path_type;
    typedef std::pair<const std::basic_string<Ch>, base>    value_type;
    typedef typename base::iterator                         iterator;
    typedef typename base::const_iterator                   const_iterator;

    class translator_from_string
        : public boost::property_tree::translator_between
        <
            variant, std::basic_string<Ch>
        >
    {
    public:
        boost::optional<variant> operator() (const base& a_tree) const {
            return *this->put_value(a_tree.get_value<std::string>());
        }

        variant operator() (const path_type& a_path, const variant& value) const {
            return value;
        }
    };

    basic_variant_tree() {}

    // Creates node with no children
    basic_variant_tree(const variant& a_data) : base(a_data)
    {}

    // Creates node with no children
    basic_variant_tree(const std::basic_string<Ch>& a_data) : base(variant(a_data))
    {}

    basic_variant_tree(const Ch* a_data) : base(variant(std::basic_string<Ch>(a_data)))
    {}

    basic_variant_tree(const basic_variant_tree& a_rhs) : base(a_rhs)
    {}

    #if __cplusplus >= 201103L
    basic_variant_tree(basic_variant_tree&& a_rhs)  { swap(a_rhs); }
    basic_variant_tree(base&&               a_rhs)  { swap(a_rhs); }
    #endif

/*
    // This cons. is for supporting property_tree's read_{info,xml,ini}() functions.
    basic_variant_tree(ptree& a_rhs, const path_type& a_prefix = path_type()) {
        ptree temp;
        if (!a_prefix.empty())
            temp.add_child(a_prefix, a_rhs);
        else
            temp.swap(a_rhs);

        translate_data(temp.begin(), temp.end(), translator_from_string());
    }
*/
    template <class T>
    T get_value() const {
        using boost::throw_exception;
        boost::optional<T> o = get_value_optional<T>();
        if(o)
            return *o;
        BOOST_PROPERTY_TREE_THROW(boost::property_tree::ptree_bad_data(
            std::string("conversion of data to type \"") +
            typeid(T).name() + "\" failed", base::data()));
    }

    template <class T>
    T get_value(const T& default_value) const {
        return base::get_value(default_value, detail::variant_translator<T>());
    }

    std::basic_string<Ch> get_value(const char* default_value) const {
        return base::get_value(std::basic_string<Ch>(default_value),
            detail::variant_translator<std::basic_string<Ch> >());
    }

    template <class T>
    boost::optional<T> get_value_optional() const {
        return detail::variant_translator<T>().get_value(this->data());
    }

    template <class T>
    void put_value(const T& value) {
        base::put_value(value, detail::variant_translator<T>());
    }

    template <class T>
    T get(const path_type& path) const {
        try {
            return base::get_child(path).BOOST_NESTED_TEMPLATE
                get_value<T>(detail::variant_translator<T>());
        } catch (boost::bad_get& e) {
            std::stringstream s;
            s << "Cannot convert value to type '" << type_to_string<T>() << "'";
            throw bad_path(s.str(), path);
        }
    }

    template <class T>
    T get(const path_type& path, const T& default_value) const {
        try {
            return base::get(path, default_value, detail::variant_translator<T>());
        } catch (boost::bad_get& e) {
            throw bad_path("Wrong or missing value type", path);
        }
    }

    std::string get(const path_type& path, const char* default_value) const {
        return base::get(path, std::string(default_value),
            detail::variant_translator<std::string>());
    }

    template <class T>
    boost::optional<T> get_optional(const path_type& path) const {
        return base::get_optional(path, detail::variant_translator<T>());
    }

    template <class T>
    void put(const path_type& path, const T& value) {
        base::put(path, value, detail::variant_translator<T>());
    }

    template <class T>
    self_type& add(const path_type& path, const T& value) {
        return static_cast<self_type&>(
            base::add(path, value, detail::variant_translator<T>()));
    }

    void swap(basic_variant_tree& rhs) {
        base::swap(static_cast<base&>(rhs));
    }
    void swap(base& rhs) {
        base::swap(rhs);
    }
    void swap(ptree& rhs) {
        base::swap(static_cast<self_type&>(rhs));
    }

    self_type &get_child(const path_type &path) {
        return static_cast<self_type&>(base::get_child(path));
    }

    /** Get the child at the given path, or throw @c ptree_bad_path. */
    const self_type &get_child(const path_type &path) const {
        return static_cast<const self_type&>(base::get_child(path));
    }

    /** Get the child at the given path, or return @p default_value. */
    self_type &get_child(const path_type &path, self_type &default_value) {
        return static_cast<self_type&>(base::get_child(path, default_value));
    }

    /** Get the child at the given path, or return @p default_value. */
    const self_type &get_child(const path_type &path,
                               const self_type &default_value) const {
        return static_cast<const self_type&>(base::get_child(path, default_value));
    }

    /** Get the child at the given path, or return boost::null. */
    boost::optional<self_type &>
    get_child_optional(const path_type &path) {
        boost::optional<base&> o = base::get_child_optional(path);
        if (!o)
            return boost::optional<self_type&>();
        return boost::optional<self_type&>(static_cast<self_type&>(*o));
    }

    /** Get the child at the given path, or return boost::null. */
    boost::optional<const self_type &>
    get_child_optional(const path_type &path) const {
        boost::optional<const base&> o = base::get_child_optional(path);
        if (!o)
            return boost::optional<const self_type&>();
        return boost::optional<const self_type&>(static_cast<const self_type&>(*o));
    }

    self_type &put_child(const path_type &path, const self_type &value) {
        return static_cast<self_type&>(base::put_child(path, value));
    }

    std::basic_ostream<Ch>& dump
    (
        std::basic_ostream<Ch>& out,
        size_t  a_tab_width   = 2,
        bool    a_show_types  = true,
        bool    a_show_braces = false,
        Ch      a_intent_char = ' '
    ) const {
        return dump(out, a_tab_width, a_show_types, a_show_braces, a_intent_char, 0);
    }

    std::basic_string<Ch> to_string
    (
        size_t  a_tab_width     = 2,
        bool    a_with_types    = true,
        bool    a_with_braces   = true
    ) const {
        std::basic_stringstream<Ch> s;
        dump(s, a_tab_width, a_with_types, a_with_braces);
        return s.str();
    }

    /// Merge current tree with \a a_tree by calling \a a_on_update function
    /// for every node in the \a a_tree.  The function accepts two parameters:
    /// and has the following signature:
    ///     variant (const variant_tree::path_type& a_path, const variant& a_data)
    template <typename T>
    void merge(const basic_variant_tree& a_tree, const T& a_on_update) {
        merge("", a_tree, a_on_update);
    }

    void merge(const basic_variant_tree& a_tree) { merge("", a_tree, &update_fun); }

    void merge(const base& a_tree, const path_type& a_prefix = path_type())
    {
        merge(a_prefix, a_tree, translator_from_string());
    }

    /// Execute \a a_on_update function for every node in the tree. The function
    /// should have the following signature:
    ///        void (const path_type& a_path, variant& a_data)
    template <typename T>
    void update(const T& a_on_update) {
        update("", *this, a_on_update);
    }

    /// For internal use
    template <typename T>
    static void translate_data(base& a_tree, const T& a_tr) {
        for (iterator it = a_tree.begin(), e = a_tree.end(); it != e; ++it)
            translate_data(it->second, a_tr);
        a_tree.data() = *a_tr.put_value(a_tree.get_value<std::string>());
    }

private:
    /// For internal use
    template <typename T>
    void translate_data
    (
        typename base::iterator& a_node,
        typename base::iterator& a_end,
        const T& a_tr
    ) {
        for (iterator it = a_node; it != a_end; ++it)
            translate_data(it.begin(), it.end(), a_tr);
        this->put_child(a_node->first, *a_tr.put_value(a_node->second.get_value<std::string>()));
    }

    std::basic_ostream<Ch>& dump
    (
        std::basic_ostream<Ch>& out,
        size_t                  a_tab_width,
        bool                    a_show_types,
        bool                    a_show_braces,
        Ch                      a_indent_char,
        size_t                  a_level
    ) const {
        size_t kwidth = 0;
        for (const_iterator it = this->begin(), e = this->end(); it != e; ++it) {
            size_t n = it->first.size()
                     + (a_show_types ? strlen(it->second.data().type_str()) : 0)
                     + 1;
            kwidth = std::max(n, kwidth);
        }
        for (const_iterator it = this->begin(), e = this->end(); it != e; ++it) {
            out << std::string(a_level*a_tab_width, a_indent_char)
                << it->first;
            if (a_show_types) {
                out << key_type("::")
                    << key_type(it->second.data().type_str())
                    << key_type("()");
            }
            if (!it->second.data().is_null()) {
                int pad = kwidth - it->first.size()
                        - (a_show_types ? strlen(it->second.data().type_str()) : 0);
                out << std::string(pad, Ch(' '))
                << key_type("= ")
                << key_type(it->second.data().is_string() ? "\"" : "")
                << it->second.data().to_string()
                << key_type(it->second.data().is_string() ? "\"" : "");
            }
            if (it->second.size()) {
                if (a_show_braces) out << key_type(" {");
                out << std::endl;
                static_cast<const self_type&>(it->second).dump(
                    out, a_tab_width, a_show_types, a_show_braces,
                    a_indent_char, a_level+1);
                if (a_show_braces) out << key_type("}");
                out << std::endl;
            } else
                out << std::endl;
        }
        return out;
    }

    /// This recursive function will call a_method for every node in the tree
    template<typename T>
    void merge
    (
        const path_type&    a_path,
        const base&         a_tree,
        const T&            a_on_update
    ) {
        for (const_iterator it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
            path_type path = a_path / path_type(it->first);
            merge(path, it->second, a_on_update);
        }
        put(a_path, a_on_update(a_path, a_tree.data()));
    }

    /// This recursive function will call a_method for every node in the tree
    template<typename T>
    static void update
    (
        const path_type&    a_path,
        basic_variant_tree& a_tree,
        const T&            a_on_update
    ){
        variant& v = a_tree.data();
        a_on_update(a_path, v);
        for (iterator it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
            basic_variant_tree::path_type l_path =
                a_path / basic_variant_tree::path_type(it->first);
            update(l_path, static_cast<basic_variant_tree&>(it->second), a_on_update);
        }
    }

    static const variant& update_fun(
        const basic_variant_tree::path_type& a, const variant& v)
    { return v; }
};

/// Path in the tree derived from boost/property_tree/string_path.hpp
typedef basic_variant_tree<char>            variant_tree;
typedef typename variant_tree::path_type    tree_path;

} // namespace utxx

inline utxx::tree_path operator/ (const utxx::tree_path& a, const std::string& s) {
    utxx::tree_path t(a);
    t /= s;
    return t;
}

inline utxx::tree_path operator/ (const utxx::tree_path& a,
    const std::pair<std::string,std::string>& a_option_with_value)
{
    std::string s = a_option_with_value.first + '[' + a_option_with_value.second + ']';
    utxx::tree_path t(a);
    t /= s;
    return t;
}

inline utxx::tree_path operator/ (const utxx::tree_path& a,
    const std::pair<const char*,const char*>& a_option_with_value)
{
    std::string s = std::string(a_option_with_value.first)
                  + '[' + a_option_with_value.second + ']';
    utxx::tree_path t(a);
    t /= s;
    return t;
}

inline utxx::tree_path& operator/ (utxx::tree_path& a, const std::string& s) {
    return a /= s;
}

inline utxx::tree_path& operator/ (utxx::tree_path& a,
    const std::pair<std::string,std::string>& a_option_with_value)
{
    return a /= a_option_with_value.first + '[' + a_option_with_value.second + ']';
}

inline utxx::tree_path& operator/ (utxx::tree_path& a,
    const std::pair<const char*,const char*>& a_option_with_value)
{
    return a /= (std::string(a_option_with_value.first) +
                 '[' + a_option_with_value.second + ']');
}

inline utxx::tree_path operator/ (const std::string& a, const utxx::tree_path& s) {
    utxx::tree_path t(a);
    t /= s;
    return t;
}

#endif // _UTXX_VARIANT_TREE_HPP_
