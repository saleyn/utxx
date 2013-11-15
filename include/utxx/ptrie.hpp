// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief s-trie with persistency
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

#ifndef _UTXX_PTRIE_HPP_
#define _UTXX_PTRIE_HPP_

#include <fstream>
#include <stdexcept>
#include <boost/bind.hpp>

namespace utxx {

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

template<typename Node>
class ptrie {
public:
    typedef Node node_t;
    typedef typename node_t::store_t store_t;
    typedef typename node_t::symbol_t symbol_t;
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
    template<typename Data>
    void store(const symbol_t *key, const Data& data) {
        path_to_node(key)->data() = data;
    }

    // update node data using provided merge-functor
    template<typename MergeFunctor, typename DataT>
    void update(const symbol_t *key, const DataT& data, MergeFunctor& merge) {
        merge(path_to_node(key)->data(), data);
    }

    // calculate suffix links
    void make_links() {
        foreach(boost::bind(&ptrie::make_link, this, _2, _1));
    }

    // traverse trie
    template<typename F>
    void foreach(F functor) {
        foreach(m_root, "", functor);
    }

    // fold through trie nodes following key components
    template <typename A, typename F>
    void fold(const symbol_t *key, A& acc, F proc) {
        symbol_t c;
        node_t *node = &m_root;
        while ((c = *key++) != 0) {
            node = read_node(node, c);
            if (!node)
                break;
            if (!proc(acc, node->data(), m_store, key))
                break;
        }
    }

    // fold through trie nodes following key components and suffix links
    template <typename A, typename F>
    void fold_full(const symbol_t *key, A& acc, F proc) {
        symbol_t c;
        node_t *node = &m_root;

        while ((c = *key) != 0) {

            // get child node
            node_t *child = read_node(node, c);
            if (child) {
                node = child;
                ++key;

                // process current node and all suffixes
                node_t *suffix = node;
                while ( proc(acc, suffix->data(), m_store, key) ) {
                    // get next suffix
                    suffix = read_suffix(suffix);
                    if (suffix == 0)
                        break;
                }
                continue;
            }

            // get suffix node
            if ((node = read_suffix(node)) == 0) {
                // no child, no suffix - advance key, reset node pointer
                ++key; node = &m_root;
            }
        }
    }

    // write trie to file
    template <typename T>
    void write_to_file(const char *a_fname) {
        ofile l_file(a_fname);
        std::ofstream& l_ofs = l_file.ofs();
        // avoid null store reference by writing a byte
        char l_fake = 'F'; l_ofs.write(&l_fake, sizeof(l_fake));
        // write nodes
        T l_root = write_nodes<T>(l_ofs);
        // write root node reference (default trie header)
        l_ofs.write((const char *)&l_root, sizeof(l_root));
    }

    // write nodes to file, can be used by custom writers
    template <typename T>
    T write_nodes(std::ofstream& a_ofs) {
        // write nodes
        T ret = m_root.write_to_file<T>(m_store, boost::bind(
            &ptrie::template write_child<T>, this, _1, _2), a_ofs);
        // update cross-reference links
        m_root.write_links(m_store, boost::bind(
            &ptrie::write_links, this, boost::ref(a_ofs), _1), a_ofs);
        // restore end-of-file position
        a_ofs.seekp(0, std::ios_base::end);
        // return root node offset
        return ret;
    }

    // RAII-wrapper for std::ofstream
    // can't just rely on std::ofstream destructor due to enabled exceptions
    // which could be thrown while in std::ofstream destructor...
    class ofile {
        std::ofstream m_ofs;
    public:
        ofile(const char *a_fname) {
            m_ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            m_ofs.open(a_fname, std::ofstream::out |
                std::ofstream::binary | std::ofstream::trunc);
        }
        ~ofile() {
            try {
                m_ofs.close();
            } catch (...) {
            }
        }
        std::ofstream& ofs() { return m_ofs; }
    };

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
    node_t *path_to_node(const symbol_t *key) {
        symbol_t c;
        node_t *p_node = &m_root;
        while ((c = *key++) != 0)
            p_node = next_node(p_node, c);
        return p_node;
    }

    // FIXME: use container of symbol_t instead of std::string and array

    // used by foreach
    template<typename F>
    void foreach(node_t& root, const std::string& key, F& fun) {
        root.children().foreach_keyval(boost::bind(&ptrie::template each<F>,
            this, _2, boost::cref(key), _1, boost::ref(fun)));
        fun(key, root, m_store);
    }

    // used by foreach
    template<typename F>
    void each(ptr_t child, const std::string& key, symbol_t symbol, F& fun) {
        std::string next_key = key + symbol;
        foreach(*node_ptr(child), next_key, fun);
    }

    // calculate suffix link for one node
    void make_link(node_t& a_node, const std::string& a_key) {
        // find my nearest suffix
        const symbol_t *l_key = a_key.c_str();
        while (*l_key++) {
            a_node.suffix() = find_exact(l_key);
            if (a_node.suffix() != store_t::null)
                break;
        }
    }

    // return reference to node matching a_key exactly or store_t::null
    ptr_t find_exact(const symbol_t *key) {
        symbol_t c;
        node_t *p_node = &m_root;
        const ptr_t *p_ptr = 0;
        while ((c = *key++) != 0) {
            p_ptr = p_node->children().get(c);
            if (p_ptr == 0)
                break;
            p_node = node_ptr(*p_ptr);
        }
        return p_ptr ? *p_ptr : store_t::null;
    }

    // used by sarray_t writer
    template <typename T>
    T write_child(ptr_t a_child, std::ofstream& a_ofs) const {
        return node_ptr(a_child)->write_to_file<T>(m_store, boost::bind(
            &ptrie::template write_child<T>, this, _1, _2), a_ofs);
    }

    // write node to file - 2nd pass wrapper for a child
    void write_links(std::ofstream& a_ofs, ptr_t a_child) const {
        node_ptr(a_child)->write_links(m_store, boost::bind(
            &ptrie::write_links, this, boost::ref(a_ofs), _1), a_ofs);
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

    // class fields
    store_t  m_store;    // data and node store
      ptr_t  m_root_ptr; // root node store-pointer
     node_t& m_root;     // root node reference
};

} // namespace utxx

#endif // _UTXX_PTRIE_HPP_
