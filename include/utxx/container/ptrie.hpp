// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief trie with persistency support
 *
 * \author Dmitriy Kargapolov
 * \since 03 October 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_CONTAINER_PTRIE_HPP_
#define _UTXX_CONTAINER_PTRIE_HPP_

#include <stdexcept>
#include <vector>
#include <stdint.h>
#include <boost/bind.hpp>
#include <boost/range.hpp>

namespace utxx {
namespace container {

// direction of traversing the trie
enum dir_t { up, down };

namespace {

// conditional destroy
template<bool> struct trie_destructor {
    template<typename T> static inline void destroy(T&);
};

template<> template<typename T>
void trie_destructor<false>::destroy(T&) {}

template<> template<typename T>
void trie_destructor<true>::destroy(T& trie) { trie.clear(); }

}

// optional const qualifier, used by trie_walker
template<bool Const, typename T> struct const_qual;
template<typename T> struct const_qual<true, T> {
    typedef const T type;
};
template<typename T> struct const_qual<false, T> {
    typedef T type;
};

// trie traversing helper
template<typename Node, bool Const, dir_t Dir, typename Key, typename F>
class trie_walker {
    typedef typename const_qual<Const, Node>::type node_t;
    typedef typename const_qual<Const, typename node_t::store_t>::type store_t;
    typedef typename node_t::symbol_t symbol_t;
    typedef typename store_t::pointer_t ptr_t;

    store_t& m_store; F& m_fun;
    Key m_key;

public:
    trie_walker(store_t& a_store, F& a_fun)
        : m_store(a_store), m_fun(a_fun)
    {}

    trie_walker(trie_walker& a_walker, symbol_t a_sym)
        : m_store(a_walker.m_store), m_fun(a_walker.m_fun)
        , m_key(a_walker.m_key)
    {
        m_key.push_back(a_sym);
    }

    template<typename V>
    void operator()(symbol_t k, V v) {
        if (v == store_t::null)
            throw std::invalid_argument("null store pointer");
        node_t *l_ptr = m_store.template native_pointer<node_t>(v);
        if (!l_ptr)
            throw std::invalid_argument("bad store pointer");
        trie_walker next_level(*this, k);
        next_level.foreach(*l_ptr);
    }

    void foreach(node_t& a_node) {
        if (Dir == up) {
            a_node.children().foreach_keyval(*this);
            m_fun(m_key, a_node, m_store);
        } else {
            m_fun(m_key, a_node, m_store);
            a_node.children().foreach_keyval(*this);
        }
    }
};

namespace {

// generic zero-terminated character sequence
template<typename Char>
class char_cursor_base {
    const Char *ptr;
public:
    char_cursor_base(const Char *key) : ptr(key) {}
    bool has_data() const { return *ptr; }
    const Char& get_data() { return *ptr; }
    void next() { ++ptr; }
};

// cursor implementation class
template<typename KeyT> class cursor_impl;

// all cases of C-style char sequences
template<>
class cursor_impl<char *> : public char_cursor_base<char> {
public:
    cursor_impl(const char *key) : char_cursor_base<char>(key) {}
};
template<>
class cursor_impl<const char *> : public char_cursor_base<char> {
public:
    cursor_impl(const char *key) : char_cursor_base<char>(key) {}
};
template<int N>
class cursor_impl<char[N]> : public char_cursor_base<char> {
public:
    cursor_impl(const char *key) : char_cursor_base<char>(key) {}
};
template<int N>
class cursor_impl<const char[N]> : public char_cursor_base<char> {
public:
    cursor_impl(const char *key) : char_cursor_base<char>(key) {}
};

// all other ranges, containers and fixed-size arrays
template<typename K> class cursor_impl {
    typedef typename boost::range_iterator<const K>::type it_t;
    typedef typename boost::range_value<const K>::type val_t;
    it_t it, end;
public:
    cursor_impl(const K& key)
        : it(boost::begin(key)), end(boost::end(key))
    {}
    bool has_data() const { return it != end; }
    const val_t& get_data() { return *it; }
    void next() { ++it; }
};

}

struct ptrie_traits_default {
    template<typename Key>
    struct cursor { typedef cursor_impl<Key> type; };
    // element position in the key sequence type
    typedef uint32_t position_type;
};

template<typename Node, typename Traits = ptrie_traits_default>
class ptrie {
public:
    typedef Node node_t;
    typedef typename node_t::store_t store_t;
    typedef typename node_t::symbol_t symbol_t;
    typedef Traits traits_t;
    typedef typename traits_t::position_type position_t;
    typedef std::vector<symbol_t> key_t;
    typedef typename key_t::const_iterator key_it_t;
    typedef typename store_t::pointer_t ptr_t;

    // constructor
    ptrie() : m_root(make_root()) {}

    // constructor
    ptrie(ptr_t a_root) : m_root(get_root(a_root)) {}

    // constructor
    ptrie(store_t& a_store)
        : m_store(a_store) , m_root(make_root())
    {}

    // constructor
    ptrie(store_t& a_store, ptr_t& a_root)
        : m_store(a_store), m_root(get_root(a_root))
    {}

    // destructor
    ~ptrie() {
        // optionally destroy nodes
        trie_destructor<store_t::dynamic>::template destroy<ptrie>(*this);
    }

    // access to node store
    const store_t& store() const { return m_store; }
    store_t& store() { return m_store; }

    // destroy hierarchy of nodes starting with root
    void clear() { clear(m_root_ptr); }

    // store data, overwrite existing data if any
    template<typename Key, typename Data>
    void store(const Key& key, const Data& data) {
        path_to_node(key)->data() = data;
    }

    // update node data using provided merge-functor
    template<typename Key, typename MergeFunctor, typename DataT>
    void update(const Key& key, const DataT& data, MergeFunctor& merge) {
        merge(path_to_node(key)->data(), data);
    }

    // calculate suffix links
    void make_links() {
        foreach<up, key_t>(boost::bind(&ptrie::make_link, this, _2, _1));
    }

    // traverse trie
    template<dir_t D, typename Key, typename F>
    void foreach(F functor) {
        trie_walker<node_t, false, D, Key, F> walker(m_store, functor);
        walker.foreach(m_root);
    }

    // traverse const trie
    template<dir_t D, typename Key, typename F>
    void foreach(F functor) const {
        trie_walker<node_t, true, D, Key, F> walker(m_store, functor);
        walker.foreach(m_root);
    }

    // fold through trie nodes following key components
    template<typename Key, typename A, typename F>
    void fold(const Key& key, A& acc, F proc) {
        typename Traits::template cursor<Key>::type cursor(key);
        node_t *node = &m_root;
        position_t k = 0;
        bool has_data = cursor.has_data();
        while (has_data) {
            node = read_node(node, cursor.get_data());
            if (!node)
                break;
            cursor.next();
            has_data = cursor.has_data();
            if (!proc(acc, node->data(), m_store, ++k, has_data))
                break;
        }
    }

    // fold through trie nodes following key components and suffix links
    template<typename Key, typename A, typename F>
    void fold_full(const Key& key, A& acc, F proc) {
        typename Traits::template cursor<Key>::type cursor(key);
        node_t *node = &m_root;
        position_t end = 0;

        position_t begin = 0;
        while (cursor.has_data()) {

            // get child node
            node_t *next_node = read_node(node, cursor.get_data());
            if (next_node) {
                // switch to child node
                node = next_node;
                cursor.next(); ++end;

                // process child node and all it's suffixes
                position_t start = begin;
                while ( proc(acc, next_node->data(), m_store, start, end,
                            cursor.has_data()) ) {
                    // get next suffix
                    node_t *suffix = read_suffix(next_node);
                    if (suffix == 0)
                        break;
                    start += next_node->shift();
                    next_node = suffix;
                }
                continue;
            }

            // get suffix node
            node_t *suffix = read_suffix(node);
            if (suffix == 0) {
                // no child, no suffix
                if (node == &m_root) {
                    cursor.next(); ++begin; ++end;
                } else {
                    node = &m_root; begin = end;
                }
            } else {
                // else switch to suffix node
                begin += node->shift();
                node = suffix;
            }
        }
    }

    // write whole trie to output store
    template<typename Enc, typename Out>
    typename Enc::addr_type store_trie(Enc& enc, Out& out) const {
        typename Enc::trie_encoder encoder(enc);
        encoder.store(boost::bind( &ptrie::template store_nodes<Enc, Out>,
            this, boost::ref(enc), boost::ref(out) ), out);
        return out.store(encoder.buff());
    }

protected:
    // use external root reference
    node_t& get_root(ptr_t a_ptr) {
        m_root_ptr = a_ptr;
        return *node_ptr(m_root_ptr);
    }

    // default root initializer
    node_t& make_root() {
        return get_root(m_store.template allocate<node_t>());
    }

    // node clean up - used by clear()
    void clear(ptr_t a_node) {
        if (a_node == store_t::null)
            return;
        node_t *l_ptr = m_store.
            template native_pointer<node_t>(a_node);
        if (!l_ptr)
            return;
        // recursive call to child's children
        l_ptr->children().foreach_value(
            boost::bind(&ptrie::clear, this, _1));
        // this calls child's destructor
        m_store.template deallocate<node_t>(a_node);
    }

    // get child node pointer, may return null
    node_t *read_node(const node_t *a_node, symbol_t a_symbol) const {
        const ptr_t *l_next_ptr = a_node->children().get(a_symbol);
        return l_next_ptr ? node_ptr_or_null(*l_next_ptr) : 0;
    }

    // get pointer to suffix node, may return null
    node_t *read_suffix(const node_t *a_node) {
        return node_ptr_or_null(a_node->suffix());
    }

    // get child node pointer, create child node if missing
    node_t *next_node(node_t *a_node, symbol_t a_symbol) {
        return node_ptr(a_node->children().ensure(a_symbol,
            boost::bind(&ptrie::new_child, this)));
    }

    // create new child node
    ptr_t new_child() {
        ptr_t l_ptr = m_store.template allocate<node_t>();
        if (l_ptr == store_t::null)
            throw std::runtime_error("store allocation error");
        return l_ptr;
    }

    // build path adding missing nodes if needed
    template<typename Key>
    node_t *path_to_node(const Key& key) {
        typename Traits::template cursor<Key>::type cursor(key);
        node_t *p_node = &m_root;
        while (cursor.has_data()) {
            p_node = next_node(p_node, cursor.get_data());
            cursor.next();
        }
        return p_node;
    }

    // calculate suffix link for one node
    void make_link(node_t& a_node, const key_t& a_key) {
        // find my nearest suffix
        key_it_t it = a_key.begin();
        key_it_t e = a_key.end();
        position_t k = 0;
        while (it != e) {
            a_node.suffix() = find_exact(++it, e); ++k;
            if (a_node.suffix() != store_t::null) {
                a_node.shift() = k;
                break;
            }
        }
    }

    // return reference to node matching a_key exactly or store_t::null
    ptr_t find_exact(key_it_t it, key_it_t e) {
        node_t *p_node = &m_root;
        const ptr_t *p_ptr = 0;
        while (it != e) {
            symbol_t c = *it;
            p_ptr = p_node->children().get(c);
            if (p_ptr == 0)
                break;
            p_node = node_ptr(*p_ptr);
            ++it;
        }
        return p_ptr ? *p_ptr : store_t::null;
    }

    // convert store pointer to native pointer to node or 0
    node_t *node_ptr_or_null(ptr_t a_pointer) const {
        return a_pointer == store_t::null ? 0 : to_native(a_pointer);
    }

    // convert store pointer to native pointer to node
    node_t *node_ptr(ptr_t a_pointer) const {
        if (a_pointer == store_t::null)
            throw std::invalid_argument("null store pointer");
        return to_native(a_pointer);
    }

    // convert to native pointer, no input verification
    node_t *to_native(ptr_t a_pointer) const {
        node_t *l_ptr = m_store.template native_pointer<node_t>(a_pointer);
        if (!l_ptr)
            throw std::invalid_argument("bad store pointer");
        return l_ptr;
    }

    // write trie nodes to output store
    template<typename T, typename Out>
    typename T::addr_type store_nodes(T& enc, Out& out) const {
        // store nodes
        typename T::addr_type ret = m_root.template write_to_store<T, Out>(
            m_store, boost::bind(&ptrie::template store_child<T, Out>, this, _1,
            boost::ref(enc), boost::ref(out)), enc, out);
        // update cross-reference links
        m_root.store_links<T, Out>(m_store, boost::bind(
            &ptrie::store_links<T, Out>, this, _1,
                boost::ref(enc), boost::ref(out)), enc, out);
        // return root node address
        return ret;
    }

    template<typename T, typename Out> typename T::addr_type
    store_child(ptr_t addr, T& enc, Out& out) const {
        return node_ptr(addr)->template write_to_store<T, Out>(m_store,
            boost::bind(&ptrie::template store_child<T, Out>, this, _1,
            boost::ref(enc), boost::ref(out)), enc, out);
    }

    template<typename T, typename Out>
    void store_links(ptr_t addr, T& enc, Out& out) const {
        node_ptr(addr)->template store_links<T, Out>(m_store, boost::bind(
            &ptrie::store_links<T, Out>, this, _1,
                boost::ref(enc), boost::ref(out)), enc, out);
    }

    // class fields
    store_t  m_store;    // data and node store
      ptr_t  m_root_ptr; // root node store-pointer
     node_t& m_root;     // root node reference
};

} // namespace container
} // namespace utxx

#endif // _UTXX_CONTAINER_PTRIE_HPP_
