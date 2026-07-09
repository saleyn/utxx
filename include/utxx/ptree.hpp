//----------------------------------------------------------------------------
/// \file  ptree.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Minimal boost::property_tree::basic_ptree drop-in replacement.
///
/// Implements the subset of the boost::property_tree::basic_ptree API used
/// by utxx::variant_tree without any boost dependency.
//----------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace utxx {
namespace ptree {

//----------------------------------------------------------------------------
// Exceptions
//----------------------------------------------------------------------------

class ptree_error : public std::runtime_error {
public:
    explicit ptree_error(const std::string& msg) : std::runtime_error(msg) {}
};

class ptree_bad_data : public ptree_error {
public:
    explicit ptree_bad_data(const std::string& msg) : ptree_error(msg) {}
};

class ptree_bad_path : public ptree_error {
public:
    explicit ptree_bad_path(const std::string& msg) : ptree_error(msg) {}
};

class file_parser_error : public ptree_error {
    std::string m_filename;
    int         m_line;
public:
    file_parser_error(const std::string& msg,
                      const std::string& filename,
                      int                line)
        : ptree_error(msg + " in " + filename + " line " + std::to_string(line))
        , m_filename(filename)
        , m_line(line)
    {}
    const std::string& filename() const { return m_filename; }
    int                line()     const { return m_line; }
};

//----------------------------------------------------------------------------
// id_translator: identity translator for string paths
//----------------------------------------------------------------------------

template <class String>
struct id_translator {
    using internal_type = String;
    using external_type = String;
    std::optional<String> get_value(const String& v) const { return v; }
    std::optional<String> put_value(const String& v) const { return v; }
};

//----------------------------------------------------------------------------
// string_path: path type built from a string with a separator character
//----------------------------------------------------------------------------

template <class String, class Translator = id_translator<String>>
class string_path {
public:
    using char_type       = typename String::value_type;
    using key_type        = String;
    using translator_type = Translator;

    explicit string_path(char_type sep = char_type('.'))
        : m_sep(sep)
    {}

    string_path(const String& s, char_type sep = char_type('.'))
        : m_str(s), m_sep(sep)
    {}

    string_path(const char_type* s, char_type sep = char_type('.'))
        : m_str(s), m_sep(sep)
    {}

    string_path(const string_path&) = default;
    string_path(string_path&&)      = default;
    string_path& operator=(const string_path&) = default;
    string_path& operator=(string_path&&)      = default;

    bool          empty()     const { return m_str.empty(); }
    char_type     separator() const { return m_sep; }
    const String& dump()      const { return m_str; }

    bool single() const { return m_str.find(m_sep) == String::npos; }

    // Pop and return the first key component, modifying this path in place
    String reduce() {
        auto pos = m_str.find(m_sep);
        if (pos == String::npos) {
            String key(std::move(m_str));
            m_str.clear();
            return key;
        }
        String key = m_str.substr(0, pos);
        m_str = m_str.substr(pos + 1);
        return key;
    }

    string_path& operator/=(const string_path& rhs) {
        if (rhs.empty()) return *this;
        if (!m_str.empty()) m_str += m_sep;
        m_str += rhs.m_str;
        return *this;
    }

    string_path operator/(const string_path& rhs) const {
        string_path t(*this);
        t /= rhs;
        return t;
    }

    bool operator==(const string_path& rhs) const {
        return m_str == rhs.m_str && m_sep == rhs.m_sep;
    }
    bool operator!=(const string_path& rhs) const { return !(*this == rhs); }

private:
    String    m_str;
    char_type m_sep;
};

//----------------------------------------------------------------------------
// translator_between: maps ExternalType <-> InternalType
//----------------------------------------------------------------------------

template <class Internal, class External>
struct translator_between {
    using internal_type = Internal;
    using external_type = External;
    std::optional<external_type> get_value(const internal_type& v) const {
        return static_cast<external_type>(v);
    }
    std::optional<internal_type> put_value(const external_type& v) const {
        return static_cast<internal_type>(v);
    }
};

template <class T>
struct translator_between<T, T> {
    using internal_type = T;
    using external_type = T;
    std::optional<T> get_value(const T& v) const { return v; }
    std::optional<T> put_value(const T& v) const { return v; }
};

//----------------------------------------------------------------------------
// basic_ptree
//
// Children are stored in a std::list<pair<Key,self_type>> preserving
// insertion order.  A sorted multimap<Key,list::iterator> provides
// O(log n) key lookup.
//
// assoc_iterator / const_assoc_iterator wrap the multimap iterator but
// dereference to the list entry (pair<Key,self_type>) so that code
// using el->first / el->second works identically to boost's ptree.
//----------------------------------------------------------------------------

template <class Key, class Data, class KeyCompare = std::less<Key>>
class basic_ptree {
public:
    using key_type    = Key;
    using data_type   = Data;
    using key_compare = KeyCompare;
    using self_type   = basic_ptree<Key, Data, KeyCompare>;
    using path_type   = string_path<Key, id_translator<Key>>;

    using container_type = std::list<std::pair<Key, self_type>>;
    using value_type     = typename container_type::value_type;
    using size_type      = typename container_type::size_type;

    using iterator             = typename container_type::iterator;
    using const_iterator       = typename container_type::const_iterator;
    using reverse_iterator       = typename container_type::reverse_iterator;
    using const_reverse_iterator = typename container_type::const_reverse_iterator;

private:
    using index_type = std::multimap<Key, iterator, KeyCompare>;

    Data           m_data;
    container_type m_children;
    index_type     m_index;

    void rebuild_index() {
        m_index.clear();
        for (auto it = m_children.begin(); it != m_children.end(); ++it)
            m_index.emplace(it->first, it);
    }

public:
    // assoc_iterator wraps the index (multimap) iterator but dereferences
    // to the underlying list entry so el->first/el->second work as expected.
    struct assoc_iterator {
        using idx_t = typename index_type::iterator;
        idx_t idx;

        assoc_iterator() = default;
        explicit assoc_iterator(idx_t i) : idx(i) {}

        value_type&     operator*()  const { return *(idx->second); }
        value_type*     operator->() const { return &*(idx->second); }
        assoc_iterator& operator++()      { ++idx; return *this; }
        assoc_iterator  operator++(int)   { auto t(*this); ++idx; return t; }
        bool operator==(const assoc_iterator& o) const { return idx == o.idx; }
        bool operator!=(const assoc_iterator& o) const { return idx != o.idx; }
    };

    struct const_assoc_iterator {
        using idx_t = typename index_type::const_iterator;
        idx_t idx;

        const_assoc_iterator() = default;
        explicit const_assoc_iterator(idx_t i) : idx(i) {}
        const_assoc_iterator(assoc_iterator ai) : idx(ai.idx) {}

        const value_type& operator*()  const { return *(idx->second); }
        const value_type* operator->() const { return &*(idx->second); }
        const_assoc_iterator& operator++()    { ++idx; return *this; }
        const_assoc_iterator  operator++(int) { auto t(*this); ++idx; return t; }
        bool operator==(const const_assoc_iterator& o) const { return idx == o.idx; }
        bool operator!=(const const_assoc_iterator& o) const { return idx != o.idx; }
        bool operator==(const assoc_iterator& o)       const { return idx == typename index_type::const_iterator(o.idx); }
        bool operator!=(const assoc_iterator& o)       const { return idx != typename index_type::const_iterator(o.idx); }
    };

    //--- Constructors --------------------------------------------------------

    basic_ptree() = default;
    explicit basic_ptree(const Data& d) : m_data(d) {}
    explicit basic_ptree(Data&&      d) : m_data(std::move(d)) {}

    basic_ptree(const basic_ptree& rhs) : m_data(rhs.m_data) {
        for (const auto& kv : rhs.m_children)
            m_children.push_back(kv);
        rebuild_index();
    }

    basic_ptree(basic_ptree&& rhs) noexcept
        : m_data(std::move(rhs.m_data))
        , m_children(std::move(rhs.m_children))
        , m_index(std::move(rhs.m_index))
    {}

    basic_ptree& operator=(const basic_ptree& rhs) {
        if (this != &rhs) { basic_ptree tmp(rhs); swap(tmp); }
        return *this;
    }

    basic_ptree& operator=(basic_ptree&& rhs) noexcept {
        if (this != &rhs) {
            m_data     = std::move(rhs.m_data);
            m_children = std::move(rhs.m_children);
            m_index    = std::move(rhs.m_index);
        }
        return *this;
    }

    bool operator==(const basic_ptree& rhs) const {
        if (m_data != rhs.m_data) return false;
        if (m_children.size() != rhs.m_children.size()) return false;
        auto a = m_children.begin(), ae = m_children.end();
        auto b = rhs.m_children.begin();
        for (; a != ae; ++a, ++b)
            if (a->first != b->first || !(a->second == b->second)) return false;
        return true;
    }
    bool operator!=(const basic_ptree& rhs) const { return !(*this == rhs); }

    //--- Data access ---------------------------------------------------------

    Data&       data()        { return m_data; }
    const Data& data() const  { return m_data; }
    // Direct data access without translation (boost ptree compat)
    Data&       get_value()       { return m_data; }
    const Data& get_value() const { return m_data; }

    template <class T, class Translator>
    T get_value(Translator tr) const {
        return get_value_impl<T>(tr.get_value(m_data));
    }

    // No-translator overload: T must be same as Data (or convertible)
    template <class T>
    T get_value() const {
        return get_value<T>(translator_between<Data, T>());
    }

    template <class T, class Translator>
    void put_value(const T& val, Translator tr) {
        put_value_impl(tr.put_value(val));
    }

    // No-translator overload
    template <class T>
    void put_value(const T& val) {
        put_value(val, translator_between<Data, T>());
    }

    //--- Sequential container ------------------------------------------------

    iterator       begin()  { return m_children.begin(); }
    iterator       end()    { return m_children.end();   }
    const_iterator begin()  const { return m_children.begin(); }
    const_iterator end()    const { return m_children.end();   }
    const_iterator cbegin() const { return m_children.cbegin(); }
    const_iterator cend()   const { return m_children.cend();   }

    size_type size()  const { return m_children.size(); }
    bool      empty() const { return m_children.empty(); }

    void clear() {
        m_children.clear();
        m_index.clear();
        m_data = Data{};
    }

    void swap(basic_ptree& rhs) noexcept {
        using std::swap;
        swap(m_data,     rhs.m_data);
        swap(m_children, rhs.m_children);
        swap(m_index,    rhs.m_index);
    }

    // Insert at end; returns sequential iterator to the new element
    iterator push_back(const value_type& kv) {
        auto it = m_children.insert(m_children.end(), kv);
        m_index.emplace(it->first, it);
        return it;
    }

    iterator push_back(value_type&& kv) {
        auto it = m_children.insert(m_children.end(), std::move(kv));
        m_index.emplace(it->first, it);
        return it;
    }

    // Key-based find — returns assoc_iterator (not_found() if absent), matching boost::ptree API
    assoc_iterator find(const Key& k) {
        return assoc_iterator(m_index.find(k));
    }

    const_assoc_iterator find(const Key& k) const {
        return const_assoc_iterator(m_index.find(k));
    }

    //--- Associative (sorted) iteration ---------------------------------------

    assoc_iterator       ordered_begin()       { return assoc_iterator(m_index.begin()); }
    assoc_iterator       ordered_end()         { return assoc_iterator(m_index.end());   }
    const_assoc_iterator ordered_begin() const { return const_assoc_iterator(m_index.cbegin()); }
    const_assoc_iterator ordered_end()   const { return const_assoc_iterator(m_index.cend());   }

    assoc_iterator       not_found()       { return assoc_iterator(m_index.end()); }
    const_assoc_iterator not_found() const { return const_assoc_iterator(m_index.cend()); }

    //--- Path-based access ---------------------------------------------------

    // Convert anything path-like to a path_type
    static path_type to_path(const path_type& p)               { return p; }
    static path_type to_path(const Key& s)                     { return path_type(s); }
    static path_type to_path(const typename Key::value_type* s){ return path_type(Key(s)); }

    // Returns optional<reference_wrapper<>> because std::optional can't hold refs
    template <class Path>
    std::optional<std::reference_wrapper<self_type>>
    get_child_optional(const Path& path) {
        self_type* p = find_path(this, to_path(path));
        if (!p) return std::nullopt;
        return std::ref(*p);
    }

    template <class Path>
    std::optional<std::reference_wrapper<const self_type>>
    get_child_optional(const Path& path) const {
        const self_type* p = find_path(this, to_path(path));
        if (!p) return std::nullopt;
        return std::cref(*p);
    }

    template <class Path>
    self_type& get_child(const Path& path) {
        auto opt = get_child_optional(path);
        if (!opt) throw ptree_bad_path("path not found");
        return opt->get();
    }

    template <class Path>
    const self_type& get_child(const Path& path) const {
        auto opt = get_child_optional(path);
        if (!opt) throw ptree_bad_path("path not found");
        return opt->get();
    }

    template <class Path, class T>
    T get(const Path& path, const T& default_val) const {
        auto opt = get_child_optional(path);
        if (!opt) return default_val;
        translator_between<Data, T> tr;
        auto vopt = tr.get_value(opt->get().data());
        return vopt ? *vopt : default_val;
    }

    template <class Path, class T, class Translator>
    T get(const Path& path, const T& default_val, Translator tr) const {
        auto opt = get_child_optional(path);
        if (!opt) return default_val;
        auto vopt = tr.get_value(opt->get().data());
        return vopt ? *vopt : default_val;
    }

    template <class Path, class T>
    self_type& put(const Path& path, const T& val) {
        self_type* node = ensure_path(to_path(path));
        translator_between<Data, T> tr;
        auto opt = tr.put_value(val);
        if (!opt) throw ptree_bad_data("put conversion failed");
        node->m_data = *opt;
        return *node;
    }

    template <class Path, class T, class Translator>
    self_type& put(const Path& path, const T& val, Translator tr) {
        self_type* node = ensure_path(to_path(path));
        auto opt = tr.put_value(val);
        if (!opt) throw ptree_bad_data("put conversion failed");
        node->m_data = *opt;
        return *node;
    }

    template <class Path>
    self_type& put_child(const Path& path, const self_type& child) {
        self_type* node = ensure_path(to_path(path));
        *node = child;
        return *node;
    }

private:
    // Helpers: accept either T or std::optional<T> from translator get/put
    template <class T>
    static T get_value_impl(T&& raw) { return std::forward<T>(raw); }
    template <class T>
    static T get_value_impl(std::optional<T>&& opt) {
        if (!opt) throw ptree_bad_data("get_value conversion failed");
        return std::move(*opt);
    }
    template <class T>
    static T get_value_impl(const std::optional<T>& opt) {
        if (!opt) throw ptree_bad_data("get_value conversion failed");
        return *opt;
    }

    template <class T>
    void put_value_impl(T&& raw) { m_data = std::forward<T>(raw); }
    template <class T>
    void put_value_impl(std::optional<T>&& opt) {
        if (!opt) throw ptree_bad_data("put_value conversion failed");
        m_data = std::move(*opt);
    }
    template <class T>
    void put_value_impl(const std::optional<T>& opt) {
        if (!opt) throw ptree_bad_data("put_value conversion failed");
        m_data = *opt;
    }

    template <class PT, class Path>
    static PT* find_path(PT* tree, const Path& path) {
        const auto& str = path.dump();
        if (str.empty()) return tree;
        const auto sep = path.separator();
        auto*       cur = tree;
        const auto* p   = str.c_str();
        const auto* end = p + str.size();
        while (p < end) {
            const auto* q = std::find(p, end, sep);
            auto key = Key(p, q - p);
            auto it  = cur->find(key);
            if (it == cur->not_found()) return nullptr;
            cur = &it->second;
            p   = (q == end) ? end : q + 1;
        }
        return cur;
    }

    template <class Path>
    self_type* ensure_path(const Path& path) {
        const auto& str = path.dump();
        if (str.empty()) return this;
        const auto sep = path.separator();
        auto*       cur = this;
        const auto* p   = str.c_str();
        const auto* end = p + str.size();
        while (p < end) {
            const auto* q = std::find(p, end, sep);
            auto key = Key(p, q - p);
            auto it  = cur->find(key);
            if (it == cur->not_found()) {
                auto ins = cur->m_children.insert(cur->m_children.end(), {key, self_type{}});
                cur->m_index.emplace(ins->first, ins);
                cur = &ins->second;
            } else {
                cur = &it->second;
            }
            p = (q == end) ? end : q + 1;
        }
        return cur;
    }
};

// Convenience alias matching boost::property_tree::ptree
using ptree = basic_ptree<std::string, std::string>;

} // namespace ptree
} // namespace utxx

// Macro replacement for BOOST_PROPERTY_TREE_THROW
#ifndef BOOST_PROPERTY_TREE_THROW
#  define BOOST_PROPERTY_TREE_THROW(x) throw x
#endif
