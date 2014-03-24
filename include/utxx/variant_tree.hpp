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
#include <utxx/variant_tree_error.hpp>
#include <utxx/variant_tree_fwd.hpp>
#include <utxx/variant_translator.hpp>
#include <utxx/variant_tree_path.hpp>
#include <utxx/typeinfo.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <utxx/detail/variant_tree_utils.hpp>
#include <utxx/config_validator.hpp>
#include <utxx/string.hpp>
#include <string>

namespace utxx {

namespace config {
    class validator;
}

// TODO: replace variant with basic_variant<Ch>

template <class Ch>
using vt_key = std::basic_string<Ch>;

template <class Ch>
class basic_variant_tree : public basic_variant_tree_base<Ch>
{
    typedef basic_variant_tree          self_type;
    typedef boost::property_tree::ptree ptree;

public:
    typedef basic_variant_tree_base<Ch>                     base;
    typedef typename base::key_type                         key_type;
    typedef basic_tree_path<Ch>                             path_type;
    typedef typename base::value_type                       value_type;
    typedef Ch                                              char_type;
    typedef typename base::iterator                         iterator;
    typedef typename base::const_iterator                   const_iterator;

private:
    path_type                m_root_path;
    const config::validator* m_schema_validator;


    template <typename T>
    static const base* do_put_value(
        const base* tree, const key_type& k, const T* v, bool put)
    {
        return tree;
    }
    template <typename T>
    static base* do_put_value(
        base* tree, const key_type& k, const T* v, bool put)
    {
        BOOST_ASSERT(v);
        return put
            ? &tree->put(k, *v, detail::variant_translator<T>())
            : &tree->add(k, *v, detail::variant_translator<T>());
    }

    // Navigate to the path and optionally put given value.
    // If value is NULL and any element of the path doesn't exist, it'll throw.
    // Otherwise each element of the path is created.
    // The path may contain brackets that will be used to match data elements
    // for a given key: "key1[data1].key2.[data3].key3"
    template <typename PTree, typename T = void>
    static typename boost::mpl::if_<boost::is_const<PTree>, const base*, base*>::type
    navigate(PTree* t, path_type& path, const T* put_val = NULL, bool do_put = true)
    {
        typedef typename
            boost::mpl::if_<boost::is_const<PTree>, const base, base>::type Base;

        typedef typename
            boost::mpl::if_<
                boost::is_const<PTree>,
                typename base::const_assoc_iterator,
                typename base::assoc_iterator
            >::type assoc_iter;

        const path_type& root= t->root_path();
        Base* tree           = &t->to_base();

        const key_type& pstr = path.dump();
        const Ch* begin      = pstr.c_str();
        const Ch* p          = begin;
        const Ch* end        = begin + pstr.size();
        const Ch  separator  = path.separator();

        if (pstr.size() == 0 || *(end-1) == separator)
            throw variant_tree_bad_path(
                    "Invalid path", root / path);

        while (true) {
            const Ch* q = std::find(p, end, path.separator());
            const Ch* b = std::find(p, q, Ch('['));
            key_type kp = key_type(p, b - p);
            key_type dp;

            if (b != end) {
                const Ch* be = std::find(b+1, q, Ch(']'));
                if (be == q)
                    throw variant_tree_bad_path(
                        "Missing closing bracket", root / path);
                if (be == b+1)
                    throw variant_tree_bad_path(
                        "Empty data expression in '[]'", root / path);
                if (be+1 != end && *(be+1) != separator)
                    throw variant_tree_bad_path(
                        "Invalid path", root / path);
                dp = key_type(b+1, be - b - 1);
            }

            // Reached the end of path
            if (q == end && put_val && !dp.empty())
                throw variant_tree_bad_path(
                    "Last subpath cannot end with a '[]' filter", root / path);

            assoc_iter el, tend = tree->not_found();

            if (dp.empty())
                el = tree->find(kp);
            else
                for(el=tree->ordered_begin(); el != tend; ++el)
                    if (el->first == kp || kp.empty())
                        if (el->second.data().is_string() && el->second.data().to_str() == dp)
                            break;

            if (el != tend)
                tree = &el->second;
            else if (put_val) {
                tree = q != end
                     ? do_put_value(tree, kp, &dp,     false)
                     : do_put_value(tree, kp, put_val, do_put);
            } else
                tree = NULL;

            if (q == end || !tree) {
                path = path_type(key_type(begin, q - begin), separator);
                break;
            }

            p = q+1;
        }

        return tree;
    }

    template <typename PTree>
    static boost::optional
    <typename boost::mpl::if_<boost::is_const<PTree>, const base&, base&>::type>
    get_child_optional(PTree* tree, const path_type& path) {
        typedef typename PTree::path_type path_type;
        typedef typename
            boost::mpl::if_<boost::is_const<PTree>, const base, base>::type TBase;

        path_type p(path);

        BOOST_ASSERT(!p.empty());

        TBase* t = navigate(tree, p, (const int*)NULL);
        return t ? *t : boost::optional<TBase&>();
    }

public:
    basic_variant_tree
    (
        const path_type& a_root_path = path_type(),
        const config::validator* a_validator = NULL
    )
        : m_root_path(a_root_path)
        , m_schema_validator(a_validator)
    {}

    // Creates node with no children
    basic_variant_tree
    (
        const variant& a_data,
        const path_type& a_root_path = path_type(),
        const config::validator* a_validator = NULL
    )
        : base(a_data)
        , m_root_path(a_root_path)
        , m_schema_validator(a_validator)
    {}

    // Creates node with no children
    basic_variant_tree(
        const std::basic_string<Ch>& a_data,
        const path_type& a_root_path = path_type(),
        const config::validator* a_validator = NULL
    )
        : base(variant(a_data))
        , m_root_path(a_root_path)
        , m_schema_validator(a_validator)
    {}

    basic_variant_tree
    (
        const Ch* a_data,
        const path_type& a_root_path = path_type(),
        const config::validator* a_validator = NULL
    )
        : base(variant(std::basic_string<Ch>(a_data)))
        , m_root_path(a_root_path)
        , m_schema_validator(a_validator)
    {}

    basic_variant_tree(const basic_variant_tree& a_rhs)
        : base(a_rhs)
        , m_root_path(a_rhs.m_root_path)
        , m_schema_validator(a_rhs.m_schema_validator)
    {}

    basic_variant_tree(const base& a_rhs, const path_type& a_root_path = path_type())
        : base(a_rhs), m_root_path(a_root_path)
    {}

    #if __cplusplus >= 201103L
    basic_variant_tree(basic_variant_tree&& a_rhs)  { swap(a_rhs); }
    basic_variant_tree(base&& a_rhs)
        : m_schema_validator(NULL)
    {
        swap(a_rhs);
    }
    #endif

    /// Offset path of this tree from the root configuration
    const path_type& root_path() const { return m_root_path; }
    void root_path(const path_type& p) { m_root_path = p; }

    /// Get schema validator
    const config::validator* validator() const { return m_schema_validator; }
    void validator(const config::validator* a) { m_schema_validator = a; }

    base&       to_base() { return static_cast<base&>(*this); }
    const base& to_base() const { return static_cast<const base&>(*this); }

    template <class T>
    T get_value() const {
        boost::optional<T> o = get_value_optional<T>();
        if (o) return *o;
        throw variant_tree_bad_data(
            utxx::to_string(
                "Path '", m_root_path.dump(), "' data conversion to type '",
                type_to_string<T>(), "' failed"),
            base::data());
    }

    template <class T>
    T get_value(const T& default_value) const {
        return base::get_value(default_value, detail::variant_translator<T>());
    }

    std::basic_string<Ch> get_value(const Ch* default_value) const {
        return base::get_value(std::basic_string<Ch>(default_value),
            detail::variant_translator<std::basic_string<Ch> >());
    }

    template <class T>
    boost::optional<T> get_value_optional() const {
        return detail::variant_translator<T>().get_value(this->data());
    }

    template <class T>
    T get(const path_type& path) const {
        boost::optional<T> r = get_optional<T>(path);
        if (r) return *r;
        if (m_schema_validator) {
            const base& v = m_schema_validator->default_value(path, m_root_path);
            return v.data().get<T>();
        }
        throw variant_tree_bad_path("Path not found", path);
    }

    template <class T>
    T get(const path_type& path, const T& default_value) const {
        path_type p(path);
        const base* t = navigate(this, p, (const int*)NULL);
        return t ? t->get_value<T>(detail::variant_translator<T>()) : default_value;
    }

    std::basic_string<Ch>
    get(const path_type& path, const char* default_value) const {
        return get<std::basic_string<Ch>>(path, std::basic_string<Ch>(default_value));
    }

    template <class T>
    boost::optional<T> get_optional(const path_type& path) const {
        path_type p(path);
        const base* t = navigate(this, p, (const int*)NULL);
        return t
            ? boost::optional<T>(t->get_value<T>(detail::variant_translator<T>()))
            : boost::optional<T>();
    }

    template <class T>
    void put_value(const T& value) {
        base::put_value(value, detail::variant_translator<T>());
    }

    template <class T>
    base& put(const path_type& path, const T& value) {
        path_type p(path);
        base* r = navigate(this, p, &value);
        BOOST_ASSERT(r);
        return *r;
    }

    template <class T>
    base& add(const path_type& path, const T& value) {
        path_type p(path);
        base* r = navigate(this, p, &value, false);
        BOOST_ASSERT(r);
        return *r;
    }

    void swap(basic_variant_tree& rhs) {
        base::swap(static_cast<base&>(rhs));
        std::swap(m_root_path, rhs.m_root_path);
        std::swap(m_schema_validator, rhs.m_schema_validator);
    }
    void swap(base&  rhs) { base::swap(rhs); }
    void swap(ptree& rhs) { base::swap(rhs); }

    /**
     * Get the child at given path, where current tree's path of root is \a root.
     * Throw @c ptree_bad_path if such path doesn't exist
     */
    base& get_child(const path_type& path) {
        boost::optional<base&> r = get_child_optional(path);
        if (r) return *r;
        if (m_schema_validator) {
            const base& v = m_schema_validator->default_value(path, m_root_path);
            return put(path, v.data());
        }
        throw variant_tree_bad_path(
            "Cannot get child - path not found", m_root_path / path);
    }

    /** Get the child at the given path, or throw @c ptree_bad_path. */
    const base& get_child(const path_type& path) const {
        boost::optional<const base&> r = get_child_optional(path);
        if (r) return *r;
        if (m_schema_validator)
            // default_value will throw if there's no such path
            return m_schema_validator->default_value(path, m_root_path);

        throw variant_tree_bad_path(
            "Cannot get child - path not found", m_root_path / path);
    }

    /** Get the child at the given path, or return @p default_value. */
    base& get_child(const path_type& path, self_type& default_value) {
        boost::optional<base&> r = get_child_optional(path);
        return r ? *r : default_value;
    }

    /** Get the child at the given path, or return @p default_value. */
    const base& get_child(const path_type& path, const self_type& default_value) const {
        boost::optional<const base&> r = get_child_optional(path);
        return r ? *r : default_value;
    }

    /**
     * Get the child at the given path (using provided separator).
     *
     * In the contrast to the native implementation of property_tree,
     * this method allows paths to contain the bracket notation that
     * will also match the data of type string with the bracketed content:
     * @code
     * auto result r = tree.get_child_optional("/one[weather]/two[sunshine]", '/')
     * @endcode
     * @param path      is the path that optionally may contain data
     *                  matches in square brackets
     * @param separator is the separating character to use. Defaults to separator
     *                  of the \a path_with_brackets
     */
    boost::optional<const base&>
    get_child_optional(const path_type& path) const {
        return get_child_optional(this, path);
    }

    boost::optional<base&>
    get_child_optional(const path_type& path) {
        return get_child_optional(this, path);
    }

    base& put_child(const path_type& path, const self_type& value) {
        path_type p(path);
        base* c = navigate(this, p, false);
        if (!c) throw variant_tree_bad_path("Path doesn't exist", p);
        return c->put_child(p, value);
    }

    template <class Stream>
    Stream& dump
    (
        Stream& out,
        size_t  a_tab_width   = 2,
        bool    a_show_types  = true,
        bool    a_show_braces = false,
        Ch      a_indent_char = Ch(' '),
        int     a_indent      = 0
    ) const {
        if (!m_root_path.empty())
            out << std::string(a_indent*a_tab_width, a_indent_char)
                << "[Path: " << m_root_path.dump() << ']' << std::endl;
        return dump(out, to_base(), a_tab_width, a_show_types,
                    a_show_braces, a_indent_char, a_indent);
    }

    /// Pretty print the tree to string
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
        merge(a_prefix, a_tree, detail::basic_translator_from_string<Ch>());
    }

    /// Execute \a a_on_update function for every node in the tree. The function
    /// should have the following signature:
    ///        void (const path_type& a_path, variant& a_data)
    template <typename T>
    void update(const T& a_on_update) {
        update("", *this, a_on_update);
    }

    /// Validate content of this tree against the custom validator
    void validate() const {
        if (!m_schema_validator)
            throw std::runtime_error("Unassigned validator!");
        m_schema_validator->validate(*this);
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
        // Get the value as string and attempt to convert it to variant
        this->put_child(a_node->first,
                        *a_tr.put_value(a_node->second.get_value<std::string>()));
    }

    template <class Stream>
    static Stream& dump
    (
        Stream& out,
        const base& a_tree,
        size_t  a_tab_width,
        bool    a_show_types,
        bool    a_show_braces,
        Ch      a_indent_char,
        size_t  a_level
    ) {
        size_t kwidth = 0;
        for (const_iterator it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
            bool simple = detail::is_simple_key(it->first);
            size_t n = detail::create_escapes(it->first).size()
                     + (a_show_types ? strlen(it->second.data().type_str()) : 0)
                     + (simple ? 1 : 3);
            kwidth = std::max(n, kwidth);
        }
        for (const_iterator it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
            bool simple = detail::is_simple_key(it->first);
            out << std::string(a_level*a_tab_width, a_indent_char)
                << (simple ? "" : "\"")
                << detail::create_escapes(it->first)
                << (simple ? "" : "\"");
            if (a_show_types) {
                out << key_type("::")
                    << key_type(it->second.data().type_str())
                    << key_type("()");
            }
            if (!it->second.data().is_null()) {
                int pad = kwidth - it->first.size()
                        - (a_show_types ? strlen(it->second.data().type_str()) : 0);
                key_type s = key_type(it->second.data().to_string());
                out << std::string(pad, Ch(' '))
                    << key_type(it->second.size() > 0 ? "" : "= ")
                    << key_type(it->second.data().is_string() ? "\"" : "")
                    << (it->second.data().is_string() ? detail::create_escapes(s) : s)
                << key_type(it->second.data().is_string() ? "\"" : "");
            }
            if (it->second.size()) {
                if (a_show_braces) out << key_type(" {");
                out << std::endl;
                dump(out, it->second, a_tab_width, a_show_types, a_show_braces,
                     a_indent_char, a_level+1);
                if (a_show_braces)
                    out << std::string(a_level*a_tab_width, a_indent_char)
                        << key_type("}") << std::endl;
            } else
                out << std::endl;
        }
        return out;
    }

    /// This recursive function will call a_method for every node in the tree
    template<typename T>
    void merge(const path_type& a_path, const base& a_tree, const T& a_on_update) {
        for (const_iterator it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
            path_type path = a_path / path_type(it->first);
            merge(path, it->second, a_on_update);
        }
        put(a_path, a_on_update(a_path, a_tree.data()));
    }

    /// This recursive function will call a_method for every node in the tree
    template<typename T>
    static void update(const path_type& a_path, base& a_tree, const T& a_on_update) {
        variant& v = a_tree.data();
        a_on_update(a_path, v);
        for (iterator it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
            basic_variant_tree::path_type l_path =
                a_path / path_type(it->first);
            update(l_path, it->second, a_on_update);
        }
    }

    static const variant& update_fun(
        const basic_variant_tree::path_type& a, const variant& v)
    { return v; }
};


} // namespace utxx

#endif // _UTXX_VARIANT_TREE_HPP_

