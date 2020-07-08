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
#pragma once

#include <utxx/variant.hpp>
#include <utxx/variant_tree_path.hpp>
#include <utxx/variant_tree_error.hpp>
#include <utxx/variant_tree_fwd.hpp>
#include <utxx/variant_translator.hpp>
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
    struct validator;
}

// TODO: replace variant with basic_variant<Ch>

template <class Ch>
using vt_key = std::basic_string<Ch>;

template <class Ch>
class basic_variant_tree : public basic_variant_tree_base<Ch>
{
    using ptree          = boost::property_tree::ptree;
public:
    using self_type      = basic_variant_tree<Ch>;
    using base           = basic_variant_tree_base<Ch>;
    using key_type       = typename base::key_type;
    using path_type      = basic_tree_path<Ch>;
    using value_type     = typename base::value_type;
    using char_type      = Ch;
    using iterator       = typename base::iterator;
    using const_iterator = typename base::const_iterator;

    basic_variant_tree
    (
        const path_type& a_root_path = path_type(),
        const config::validator* a_validator = nullptr
    )
    {
        this->data().root_path(a_root_path);
        this->data().validator(a_validator);
    }

    // Creates node with no children
    basic_variant_tree(
        const variant& a_data,
        const path_type& a_root_path = path_type(),
        const config::validator* a_validator = nullptr
    )
        : base(a_data)
    {
        this->data().root_path(a_root_path);
        this->data().validator(a_validator);
    }

    // Creates node with no children
    basic_variant_tree(
        const std::basic_string<Ch>& a_data,
        const path_type& a_root_path = path_type(),
        const config::validator* a_validator = nullptr
    )
        : base(variant(a_data))
    {
        this->data().root_path(a_root_path);
        this->data().validator(a_validator);
    }

    basic_variant_tree(
        const Ch* a_data,
        const path_type& a_root_path = path_type(),
        const config::validator* a_validator = nullptr
    )
        : base(variant(std::basic_string<Ch>(a_data)))
    {
        this->data().root_path(a_root_path);
        this->data().validator(a_validator);
    }

    basic_variant_tree(const basic_variant_tree<Ch>& a_rhs)
        : base(a_rhs)
    {
        this->data().root_path(a_rhs.data().root_path());
        this->data().validator(a_rhs.data().validator());
    }

    basic_variant_tree(const basic_variant_tree_base<Ch>& a_rhs, const path_type& a_path=path_type())
        : base(a_path.empty() ? a_rhs : upcast(a_rhs).get_child(a_path))
    {
        auto path = a_path.empty()
                  ? a_rhs.data().root_path()
                  : a_rhs.data().root_path().empty()
                  ? a_path : a_rhs.data().root_path() / a_path;
        this->data().root_path(path);
        this->data().validator(a_rhs.data().validator());
    }

    basic_variant_tree(basic_variant_tree<Ch>&& a_rhs)
        : basic_variant_tree_base<Ch>(std::move(a_rhs))
    {
        this->data().root_path(std::move(a_rhs.root_path()));
        this->data().validator(a_rhs.validator());
    }

    void operator=(basic_variant_tree<Ch> const& a_rhs) {
        this->~basic_variant_tree();
        new (this) basic_variant_tree(a_rhs);
    }

    void operator=(basic_variant_tree<Ch>&& a_rhs) {
        this->~basic_variant_tree();
        new (this) basic_variant_tree(std::move(a_rhs));
    }

    void operator=(basic_variant_tree_base<Ch> const& a_rhs) {
        this->operator=(upcast(a_rhs));
    }

    void operator=(basic_variant_tree_base<Ch>&& a_rhs) {
        this->operator=(reinterpret_cast<self_type&&>(a_rhs));
    }

    static self_type& upcast(basic_variant_tree_base<Ch>& a_rhs) {
        return reinterpret_cast<self_type&>(a_rhs);
    }

    static self_type const& upcast(basic_variant_tree_base<Ch> const& a_rhs) {
        return reinterpret_cast<self_type const&>(a_rhs);
    }

    /// Offset path of this tree from the root configuration
    const path_type& root_path() const { return this->data().root_path();      }
    void root_path(const path_type& p) { this->data().root_path(p);            }
    void root_path(path_type&& p)      { this->data().root_path(std::move(p)); }

    /// Get schema validator
    const config::validator* validator()       const { return this->data().validator(); }
    void validator(const config::validator* a) const { this->data().validator(a);       }

    template <class T>
    T get_value() const {
        boost::optional<T> o = get_value_optional<T>();
        if (!o)
            throw_bad_type<T>(path_type(), this->data());
        return *o;
    }

    template <class T>
    T get_value(const T& default_value) const {
        try { return detail::variant_translator<T,Ch>().get_value(this->data().value()); }
        catch (const boost::bad_get&) { throw_bad_type<T>(path_type(), this->data().value()); }
    }

    std::basic_string<Ch> get_value(const Ch* default_value) const {
        return base::get_value(std::basic_string<Ch>(default_value),
            detail::variant_translator<std::basic_string<Ch>,Ch>());
    }

    template <class T>
    boost::optional<T> get_value_optional() const {
        if (!this->data().is_null()) {
            try { return detail::variant_translator<T,Ch>().get_value(this->data().value()); }
            catch (...) { throw_bad_type<T>(path_type(), this->data().value()); }
        }
        return boost::optional<T>();
    }

    template <class T>
    T get(const path_type& path) const {
        boost::optional<T> r = get_optional<T>(path);
        if (r) return *r;
        if (this->data().validator()) {
            const config::option& o = this->data().validator()->get(path, root_path());
            const auto& v = o.default_subst_value();
            if (v.is_null())
                throw boost::property_tree::ptree_error(
                    utxx::to_string("Path '", (root_path() / path).dump(), "'"
                        " missing required option of type: ",
                        config::type_to_string(o.value_type)));
            else if (!v.is_type<T>())
                throw_bad_type<T>(path, v);
            return v.get<T>();
        }
        throw variant_tree_bad_path("Path not found", path);
    }

    template <class T>
    T get(const path_type& path, const T& default_value) const {
        path_type p(path);
        auto* t = navigate(this, p, nullptr, false);
        if (t)
            try { return  t->base::template get_value<T>(detail::variant_translator<T,Ch>()); }
            catch (...) { throw_bad_type<T>(path, t->data()); }
        return default_value;
    }

    std::basic_string<Ch>
    get(const path_type& path, const Ch* default_value) const {
        return get<std::basic_string<Ch>>(path, std::basic_string<Ch>(default_value));
    }

    template <typename T>
    boost::optional<T> get_optional(const path_type& path) const {
        path_type p(path);
        auto* t = navigate(this, p, nullptr, false);
        if (t && !t->data().is_null()) {
            try { return t->base::template get_value<T>(detail::variant_translator<T,Ch>()); }
            catch (...) { throw_bad_type<T>(path, t->data()); }
        }
        return boost::optional<T>();
    }

    template <class T>
    void put_value(const T& value) {
        this->base::put_value(value, detail::variant_translator<T,Ch>());
    }

    void put_value(const Ch* value) {
        this->base::put_value(value, detail::variant_translator<std::basic_string<Ch>,Ch>());
    }

    template <class T>
    self_type& put(const path_type& path, const T& value) {
        path_type p(path);
        auto* r = navigate(this, p, &value, true);
        BOOST_ASSERT(r);
        return *r;
    }

    self_type& put(const path_type& path, const Ch* value) {
        path_type p(path);
        auto* r = navigate(this, p, value, true);
        BOOST_ASSERT(r);
        return *r;
    }

    template <class T>
    self_type& add(const path_type& path, const T& value) {
        path_type p(path);
        auto* r = navigate(this, p, &value, false);
        BOOST_ASSERT(r);
        return *r;
    }

    void swap(basic_variant_tree& rhs) {
        base::swap(static_cast<base&>(rhs));
    }
    void swap(base&  rhs) { base::swap(rhs); }
    void swap(ptree& rhs) { base::swap(rhs); }

    /**
     * Get the child at given path, where current tree's path of root is \a root.
     *
     * The tree is updated to ensure that \a path exists, and if the
     * schema_validator is defined, a default value for the \a path option
     * is looked up with environment variable substitution.
     * Throw @c ptree_bad_path if such path doesn't exist
     */
    self_type& get_child(const path_type& path) {
        boost::optional<self_type&> r = get_child_optional(path);
        if (r) return *r;
        if (this->data().validator()) {
            const config::option& o = this->data().validator()->get(path, root_path());
            return put(path, o.default_subst_value());
        }
        throw variant_tree_bad_path(
            "Cannot get child - path not found", root_path() / path);
    }

    /**
     * Get the child at the given path, or throw @c ptree_bad_path.
     *
     * If the \a path doesn't exist, and if the
     * schema_validator is not defined, the function throws an error.
     * If the schema_validator is defined, it's consulted to ensure
     * the given \a path represents a valid option.
     */
    const self_type& get_child(const path_type& path) const {
        boost::optional<const self_type&> r = get_child_optional(path);
        if (r) return *r;
        if (this->data().validator())
            // default_value will throw if there's no such path
            return reinterpret_cast<const self_type&>
                (this->data().validator()->def(path, root_path()));
        throw variant_tree_bad_path(
            "Cannot get child - path not found", root_path() / path);
    }

    /** Get the child at the given path, or return @p default_value. */
    self_type& get_child(const path_type& path, self_type& default_value) {
        boost::optional<self_type&> r = get_child_optional(path);
        return r ? *r : default_value;
    }

    /** Get the child at the given path, or return @p default_value. */
    const self_type& get_child(const path_type& path, const self_type& default_value) const {
        boost::optional<const self_type&> r = get_child_optional(path);
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
    boost::optional<const self_type&>
    get_child_optional(const path_type& path) const {
        return this->get_child_optional(this, path);
    }

    boost::optional<self_type&>
    get_child_optional(const path_type& path) {
        return this->get_child_optional(this, path);
    }

    self_type& put_child(const path_type& path, const self_type& value) {
        path_type p(path);
        auto* c = navigate(this, p, nullptr, true);
        if  (!c)  throw variant_tree_bad_path("Path doesn't exist", p);
        c->data().value() = value.data().value();
        c->root_path(path);
        return *c;
    }

    bool exists(const path_type& path) const {
        path_type p(path);
        return !!navigate(this, p, nullptr, false);
    }

    template <class Stream>
    Stream& dump
    (
        Stream& out,
        size_t  a_tab_width   = 2,
        bool    a_show_types  = false,
        bool    a_show_braces = true,
        Ch      a_indent_char = Ch(' '),
        int     a_indent      = 0
    ) const {
        if (!this->data().root_path().empty())
            out << std::basic_string<Ch>(a_indent*a_tab_width, a_indent_char)
                << "[Path: " << this->data().root_path().dump() << ']' << std::endl;
        return dump(out, *this, a_tab_width, a_show_types,
                    a_show_braces, a_indent_char, a_indent);
    }

    /// Pretty print with no arguments for invokation in gdb
    std::basic_string<Ch> to_string() const { return to_string(2); }

    /// Pretty print the tree to string
    std::basic_string<Ch> to_string
    (
        size_t  a_tab_width,
        bool    a_with_types    = false,
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
    void merge(const self_type& a_tree, const T& a_on_update) {
        merge("", a_tree, a_on_update);
    }

    void merge(const self_type& a_tree) { merge("", a_tree, &update_fun); }

    void merge(const self_type& a_tree, const path_type& a_prefix = path_type())
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
    /// @param a_schema if not NULL use this schema validator otherwise
    ///                 use internal validator().
    void validate(const config::custom_validator& a_custom_validator) const {
        validate(nullptr, a_custom_validator);
    }

    /// Validate content of this tree against the custom validator
    /// @param a_schema if not NULL use this schema validator otherwise
    ///                 use internal validator().
    void validate(
        const config::validator*        a_schema           = nullptr,
        const config::custom_validator& a_custom_validator = nullptr
    ) const {
        if (!a_schema && !this->data().validator())
            throw std::runtime_error("Unassigned validator!");
        if (a_schema)
            a_schema->validate(*this, a_custom_validator);
        else
            this->data().validator()->validate(*this, a_custom_validator);
    }

    /// For internal use
    template <typename T>
    static void translate_data(base& a_tree, const T& a_tr) {
        for (iterator it = a_tree.begin(), e = a_tree.end(); it != e; ++it)
            translate_data(it->second, a_tr);
        auto& tree = static_cast<self_type&>(a_tree);
        if (!tree.data().value().is_null())
            tree.data() = *a_tr.put_value(tree.template get_value<std::basic_string<Ch>>());
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
        for (auto it = a_node; it != a_end; ++it)
            translate_data(it.begin(), it.end(), a_tr);
        // Get the value as string and attempt to convert it to variant
        if (!a_node->second.data().value().is_null())
            this->put_child(a_node->first,
                            *a_tr.put_value(a_node->second.template get_value<std::basic_string<Ch>>()));
    }

    // This function won't ever be called, but is needed to make navigate()
    // function to compile
    template <typename T>
    static typename std::enable_if<
        std::is_same<T, std::nullptr_t>::value || (std::is_pointer<T>::value && std::is_const<T>::value),
        const self_type*>
    ::type do_put_value(const self_type* tree, const key_type&, T, bool) {
        return tree;
    }

    static self_type* do_put_value(self_type* tree, const key_type& k, const variant* v, bool put) {
        if (!v)  return tree;
        if (put) { tree->data() = *v; return tree; }
        return add(tree, k, *v);
    }
    static self_type* do_put_value(self_type* tree, const key_type& k, nullptr_t, bool put) {
        if (put) add(tree, k, variant());
        return tree;
    }
    static self_type* do_put_value(self_type* tree, const key_type& k, const Ch* v, bool put) {
        if (!v)  return tree;
        if (put) { tree->base::put_value(std::string(v), detail::variant_translator<std::string,Ch>()); return tree; }
        return add(tree, k, variant(v));
    }
    template <int N>
    static self_type* do_put_value(self_type* tree, const key_type& k, const Ch (&v)[N], bool put) {
        return do_put_value(tree, k, static_cast<const Ch*>(v), put);
    }
    template <typename T>
    static self_type* do_put_value(self_type* tree, const key_type& k, const T* v, bool put) {
        if (!v)  return tree;
        if (put) { tree->base::put_value(*v, detail::variant_translator<T,Ch>()); return tree; }
        return add(tree, k, variant(*v));
    }

    // This function won't ever be called, but is needed to make navigate()
    // function to compile
    static const self_type* add(const self_type* tree, const key_type&, const variant&) {
        return tree;
    }

    static self_type* add(self_type* tree, const key_type& k, const variant& v) {
        return static_cast<self_type*>(&tree->push_back(value_type(k, self_type(v)))->second);
    }

    // Navigate to the path and optionally put given value.
    // If value is NULL and any element of the path doesn't exist, it'll throw.
    // Otherwise each element of the path is created.
    // The path may contain brackets that will be used to match data elements
    // for a given key: "key1[data1].key2.[data3].key3"
    template <typename PTree, typename T = int*>
    static typename std::enable_if<
        std::is_same<self_type, typename remove_cvref<PTree>::type>::value &&
            (std::is_pointer<T>::value || std::is_same<std::nullptr_t, T>::value),
        PTree*>
    ::type navigate(PTree* t, path_type& path, T put_val = nullptr, bool do_put = true)
    {
        using assoc_iter = typename
            std::conditional<
                std::is_const<PTree>::value,
                typename base::const_assoc_iterator,
                typename base::assoc_iterator
            >::type;

        const path_type& root = t->root_path();
        auto*            tree = static_cast<typename std::conditional<
                                            std::is_const<PTree>::value,
                                            const self_type*, self_type*>::type>(t);

        const key_type& pstr  = path.dump();
        const Ch* begin       = pstr.c_str();
        const Ch* p           = begin;
        const Ch* end         = begin + pstr.size();
        const Ch  separator   = path.separator();

        if (path.empty())
            return do_put_value(tree, key_type(), put_val, true);

        if (pstr.size() != 0 && *(end-1) == separator)
            throw variant_tree_bad_path("Invalid path", root / path);

        key_type kp;

        while (true) {
            const Ch* q = std::find(p, end, path.separator());
            const Ch* b = std::find(p, q, Ch('['));
            kp          = key_type(p, b - p);
            key_type dp;

            if (b != q) {
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
            else for(el=tree->ordered_begin(); el != tend; ++el)
                if (el->first == kp || kp.empty())
                    if (el->second.data().is_string() &&
                        el->second.data().to_str() == dp)
                        break;

            PTree* child = static_cast<PTree*>(el == tend ? nullptr : &el->second);

            if (child)          // Child node by kp name was found
                tree = child;
            else if (!do_put)   // Child node not found, and can't mutate the tree
                tree = nullptr;
            else                // Child node not found, add this sub-path
                tree = add(tree, kp, dp.empty() ? variant() : variant(dp));

            if (q == end || !tree) {
                path = path_type(key_type(begin, q - begin), separator);
                break;
            }

            p = q+1;
        }

        if (tree)
           do_put_value(tree, kp, put_val, do_put);

        return tree;
    }

    template <typename PTree>
    static typename std::enable_if<
        std::is_same<self_type, typename remove_cvref<PTree>::type>::value,
        boost::optional<PTree&>
    >::type
    get_child_optional(PTree* tree, const path_type& path) {
        static const boost::optional<PTree&> s_empty;

        path_type p(path);

        BOOST_ASSERT(!p.empty());

        auto*  t = navigate(tree, p, nullptr, false);
        return t ? *t : s_empty;
    }

    template <class Stream>
    static Stream& dump
    (
        Stream&     out,
        const base& a_tree,
        size_t      a_tab_width,
        bool        a_show_types,
        bool        a_show_braces,
        Ch          a_indent_char,
        size_t      a_level
    ) {
        size_t kwidth = 0;
        for (auto it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
            bool simple = detail::is_simple_key(it->first);
            size_t n = detail::create_escapes(it->first).size()
                     + (a_show_types ? strlen(it->second.data().type_str()) : 0)
                     + (simple ? 1 : 3);
            kwidth = std::max(n, kwidth);
        }
        for (auto it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
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
        for (auto it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
            path_type path = a_path / it->first;
            merge(path, it->second, a_on_update);
        }
        put(a_path, a_on_update(a_path, a_tree.data()));
    }

    /// This recursive function will call a_method for every node in the tree
    template<typename T>
    static void update(const path_type& a_path, base& a_tree, const T& a_on_update) {
        variant& v = a_tree.data();
        a_on_update(a_path, v);
        for (auto it = a_tree.begin(), e = a_tree.end(); it != e; ++it) {
            auto l_path = a_path / path_type(it->first);
            update(l_path, it->second, a_on_update);
        }
    }

    static const variant& update_fun(
        const basic_variant_tree::path_type& a, const variant& v)
    { return v; }

    template <typename T>
    void throw_bad_type(const path_type& a_path, const variant& a_data) const {
        throw variant_tree_bad_data(
            utxx::to_string("Path '",
                (this->data().root_path() / a_path).dump(), "' data conversion to type '",
                type_to_string<T>(), "' failed"),
            a_data);
    }
};


} // namespace utxx
