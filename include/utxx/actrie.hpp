// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief generalized variant of aho-corasick trie
 *
 * This is symbol-based trie implementation with additional
 * 'blue' links for each node pointing to the node representing
 * longest possible suffix of the node's string. It can be used
 * for multiple fixed patterns search in the input string, or for
 * more complex calculations using fold method.
 * Thus, it is a generalization of Aho-Corasick algorithm described in
 * http://en.wikipedia.org/wiki/Aho-Corasick_string_matching_algorithm.
 *
 * \author Dmitriy Kargapolov
 * \since 13 September 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_ACTRIE_HPP_
#define _UTXX_ACTRIE_HPP_

#include <fstream>
#include <stdexcept>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/bind.hpp>

namespace utxx {

namespace detail {

/**
 * \brief optional node metadata
 */
template<typename Offset>
struct meta {
    Offset node; ///< offset of the node written
    Offset link; ///< offset of the blue link written
    meta() : node(0), link(0) {}
};

template<> struct meta<void> {};

/**
 * \brief this class implements node of the trie
 * \tparam Store node store facility
 * \tparam Data node payload type
 * \tparam SArray type of collection of child nodes
 * \tparam Offset offset type for writing or void
 *
 * The Store and SArray types are themself templates
 */
template <typename Store, typename Data, typename SArray, typename Offset>
class actrie_node {
    actrie_node(const actrie_node&);
public:
    typedef typename Store::template rebind<actrie_node>::other store_t;
    typedef typename store_t::pointer_t ptr_t;

    // sparse storage types
    typedef typename SArray::template rebind<ptr_t>::other sarray_t;
    typedef typename sarray_t::symbol_t symbol_t;

    typedef meta<Offset> meta_t;

    actrie_node() : m_suffix(store_t::null) {}

    // build path adding missing nodes if needed
    actrie_node *path_to_node(store_t& a_store, const char *a_key) {
        const char *l_ptr = a_key;
        symbol_t l_symbol;
        actrie_node *l_node_ptr = this;
        while ((l_symbol = *l_ptr++) != 0)
            l_node_ptr = l_node_ptr->next_node(a_store, l_symbol);
        return l_node_ptr;
    }

    // store data, overwrite existing data if any
    void store(store_t& a_store, const char *a_key, const Data& a_data) {
        path_to_node(a_store, a_key)->m_data = a_data;
    }

    // update node data using provided merge-functor
    template <typename MergeFunctor, typename DataT>
    void update(store_t& a_store, const char *a_key, const DataT& a_data,
            MergeFunctor& a_merge) {
        actrie_node *l_node_ptr = path_to_node(a_store, a_key);
        a_merge(l_node_ptr->m_data, a_data);
    }

    // calculate blue links
    void make_links(const store_t& a_store, ptr_t a_root,
            const std::string& a_key) {
        // first process children
        m_children.foreach_keyval(boost::bind(&actrie_node::make_link,
            boost::cref(a_store), a_root, _2, boost::cref(a_key), _1));
        // find my nearest suffix
        const char *l_key = a_key.c_str();
        while (*l_key++) {
            m_suffix = find_exact(a_store, a_root, l_key);
            if (m_suffix != store_t::null)
                break;
        }
    }

    // fold through trie nodes following key components
    template <typename A, typename F>
    void fold(const store_t& store, const char *key, A& acc, F proc) {
        symbol_t l_char;
        actrie_node *l_node_ptr = this;
        while ((l_char = *key++) != 0) {
            l_node_ptr = l_node_ptr->read_node(store, l_char);
            if (!l_node_ptr)
                break;
            if (!proc(acc, l_node_ptr->m_data, store, key))
                break;
        }
    }

    // fold through trie nodes following key components and blue links
    template <typename A, typename F>
    void fold_full(const store_t& store, const char *key, A& acc, F proc) {
        symbol_t l_char;
        actrie_node *l_node_ptr = this;

        while ((l_char = *key) != 0) {

            // get child node
            actrie_node *l_child = l_node_ptr->read_node(store, l_char);
            if (l_child) {
                l_node_ptr = l_child;
                ++key;

                // process current node and all suffixes
                actrie_node *l_node = l_node_ptr;
                while ( proc(acc, l_node->m_data, store, key) ) {
                    // get next suffix
                    l_node = l_node->read_suffix(store);
                    if (l_node == 0)
                        break;
                }
                continue;
            }

            // get suffix node
            actrie_node *l_suffix = l_node_ptr->read_suffix(store);

            if (l_suffix == 0)
                // no child, no suffix - advance key
                ++key;
            else
                // switch to suffix found
                l_node_ptr = l_suffix;
        }
    }

    // write node to file - 1st pass
    template <typename T>
    T write_to_file(const store_t& a_store, std::ofstream& a_ofs)
            const {

        // write data payload, get encoded reference
        typename Data::template ext_header<T> l_data;
        m_data.write_to_file(l_data, a_store, a_ofs);

        // write children encoded, get encoded reference
        typename sarray_t::template ext_header<T> l_children;
        m_children.write_to_file(l_children,
            boost::bind(&actrie_node::template write_child<T>, this,
                boost::cref(a_store), _1, _2), a_ofs);

        // save offset of the node encoded
        T l_ret = boost::numeric_cast<T, std::streamoff>(a_ofs.tellp());

        // write encoded data reference
        l_data.write_to_file(a_ofs);

        // save offset to the blue link
        m_meta.link = boost::numeric_cast<T, std::streamoff>(a_ofs.tellp());

        // reserve space for the blue link, fill it with zero
        T l_nil = 0;
        a_ofs.write((const char *)&l_nil, sizeof(l_nil));

        // write encoded children reference
        l_children.write_to_file(a_ofs);

        // save node reference (to populate blue links in the 2nd pass)
        m_meta.node = l_ret;

        // return offset of the node encoded
        return l_ret;
    }

    // write node to file - 2nd pass
    void write_links(const store_t& a_store, std::ofstream& a_ofs) {
        // first process children
        m_children.foreach_value(boost::bind(&actrie_node::write_link,
            boost::cref(a_store), boost::ref(a_ofs), _1));
        // suffix node reference
        if (m_suffix == store_t::null)
            return;
        actrie_node *l_ptr = a_store.
            template native_pointer<actrie_node>(m_suffix);
        if (!l_ptr)
            throw std::invalid_argument("bad store pointer");
        // write suffix node offset at specific position
        a_ofs.seekp(m_meta.link);
        a_ofs.write((const char *)&l_ptr->m_meta.node, sizeof(m_meta.node));
    }

    // should be called before destruction, can't pass a_store to destructor
    void clear(store_t& a_store) {
        m_children.foreach_value(
            boost::bind(&actrie_node::del_child, boost::ref(a_store), _1));
    }

private:
    actrie_node *convert(const store_t& a_store, ptr_t a_pointer) {
        if (a_pointer == store_t::null)
            return 0;
        actrie_node *l_ptr = a_store.
            template native_pointer<actrie_node>(a_pointer);
        if (!l_ptr)
            throw std::invalid_argument("bad store pointer");
        return l_ptr;
    }

    actrie_node *read_suffix(const store_t& a_store) {
        return convert(a_store, m_suffix);
    }

    actrie_node *read_node(const store_t& a_store, symbol_t a_symbol) {
        const ptr_t *l_next_ptr = m_children.get(a_symbol);
        return l_next_ptr ? convert(a_store, *l_next_ptr) : 0;
    }

    actrie_node *next_node(store_t& a_store, symbol_t a_symbol) {
        ptr_t& l_next = m_children.ensure(a_symbol,
            boost::bind(&actrie_node::new_child, boost::ref(a_store)));
        actrie_node *l_ptr = a_store.
            template native_pointer<actrie_node>(l_next);
        if (!l_ptr)
            throw std::invalid_argument("bad store pointer");
        return l_ptr;
    }

    static ptr_t new_child(store_t& a_store) {
        ptr_t l_ptr = a_store.template allocate<actrie_node>();
        if (l_ptr == store_t::null)
            throw std::runtime_error("store allocation error");
        return l_ptr;
    }

    // return reference to node matching a_key exactly or store_t::null
    ptr_t find_exact(const store_t& a_store, ptr_t a_root, const char *a_key) {
        const char *l_ptr = a_key;
        symbol_t l_symbol;
        actrie_node *l_node_ptr;
        const ptr_t *l_next_ptr = &a_root;
        while ((l_symbol = *l_ptr++) != 0) {
            l_node_ptr = a_store.
                template native_pointer<actrie_node>(*l_next_ptr);
            if (!l_node_ptr)
                throw std::invalid_argument("bad store pointer");
            // get child by symbol
            l_next_ptr = l_node_ptr->m_children.get(l_symbol);
            if (l_next_ptr == 0)
                return store_t::null;
        }
        return *l_next_ptr;
    }

    // used by sarray_t writer
    template <typename T>
    T write_child(const store_t& a_store, ptr_t a_child, std::ofstream& a_ofs)
            const {
        actrie_node *l_ptr = a_store.
            template native_pointer<actrie_node>(a_child);
        if (!l_ptr)
            throw std::invalid_argument("bad store pointer");
        return l_ptr->write_to_file<T>(a_store, a_ofs);
    }

public:
    // child clean up - used by clear()
    static void del_child(store_t& a_store, ptr_t a_child) {
        actrie_node *l_ptr = a_store.
            template native_pointer<actrie_node>(a_child);
        if (!l_ptr) return;
        // recursive call to child's children
        l_ptr->clear(a_store);
        // this calls child's destructor
        a_store.template deallocate<actrie_node>(a_child);
    }

private:
    // calculate blue link for a child
    static void make_link(const store_t& a_store, ptr_t a_root, ptr_t a_child,
            const std::string& a_key, symbol_t a_symbol) {
        actrie_node *l_ptr = a_store.
            template native_pointer<actrie_node>(a_child);
        if (!l_ptr) return;
        // recursive call to child's children
        std::string l_key = a_key + a_symbol;
        l_ptr->make_links(a_store, a_root, l_key);
    }

    // write node to file - 2nd pass wrapper for a child
    static void write_link(const store_t& a_store, std::ofstream& a_ofs,
            ptr_t a_child) {
        actrie_node *l_ptr = a_store.
            template native_pointer<actrie_node>(a_child);
        if (!l_ptr) return;
        // recursive call to child's children
        l_ptr->write_links(a_store, a_ofs);
    }

    Data      m_data;      ///< node data payload
    ptr_t     m_suffix;    ///< link to longest possible suffix node
    sarray_t  m_children;  ///< collection of child nodes

    mutable meta_t m_meta; ///< node metadata if any
};

}

// namespace detail

namespace {

// conditional destroy
template<bool> struct node_destructor {
    template<typename T>
    static inline void destroy(typename T::ptr_t, typename T::store_t&);
};

template<> template<typename T> void
node_destructor<false>::destroy(typename T::ptr_t, typename T::store_t&) {
}

template<> template<typename T> void
node_destructor<true>::destroy(typename T::ptr_t n, typename T::store_t& s) {
    T::del_child(s, n);
}

}

/**
 * \brief this class implements the ac-trie
 * \tparam Store node store facility
 * \tparam Data node payload type
 * \tparam SArray type of collection of child nodes
 * \tparam Offset offset type for writing or void
 *
 * The Store and SArray types are themself templates
 */
template <typename Store, typename Data, typename SArray, typename Offset = void>
class actrie {
public:
    typedef detail::actrie_node<Store, Data, SArray, Offset> node_t;
    typedef typename node_t::store_t store_t;
    typedef typename store_t::pointer_t ptr_t;

    actrie() : m_root(make_root()) {}

    actrie(ptr_t a_root) : m_root(get_root(a_root)) {}

    actrie(store_t& a_store)
        : m_store(a_store) , m_root(make_root())
    {}

    actrie(store_t& a_store, ptr_t& a_root)
        : m_store(a_store), m_root(get_root(a_root))
    {}

    ~actrie() {
        node_destructor<store_t::dynamic>::
            template destroy<node_t>(m_root_ptr, m_store);
    }

    const store_t& store() const { return m_store; }

    // store data, overwrite existing data if any
    void store(const char *a_key, const Data& a_data) {
        m_root.store(m_store, a_key, a_data);
    }

    // update node data using provided merge-functor
    template <typename MergeF, typename DataT>
    void update(const char *a_key, const DataT& a_data, MergeF& a_merge) {
        m_root.update(m_store, a_key, a_data, a_merge);
    }

    // calculate blue links
    void make_links() {
        std::string l_key = "";
        m_root.make_links(m_store, m_root_ptr, l_key);
    }

    // fold through trie nodes following key components
    template <typename A, typename F>
    void fold(const char *key, A& acc, F proc) {
        m_root.fold(m_store, key, acc, proc);
    }

    // fold through trie nodes following key components and suffixes
    template <typename A, typename F>
    void fold_full(const char *key, A& acc, F proc) {
        m_root.fold_full(m_store, key, acc, proc);
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

    // default trie header
    template <typename T>
    struct enc_trie {
        T m_root;
        void write_to_file(store_t& a_store, std::ofstream& a_ofs) {
            a_ofs.write((const char *)this, sizeof(*this));
        }
    };

    // write trie to file
    template <typename T>
    void write_to_file(const char *a_fname) {
        ofile l_file(a_fname);
        std::ofstream& l_ofs = l_file.ofs();
        char l_magic = 'A';
        l_ofs.write(&l_magic, sizeof(l_magic));
        enc_trie<T> l_trie;
        // write nodes
        l_trie.m_root = m_root.write_to_file<T>(m_store, l_ofs);
        // save header offset
        T l_hdr = boost::numeric_cast<T, std::streamoff>(l_ofs.tellp());
        // update blue links
        m_root.write_links(m_store, l_ofs);
        // write trie
        l_ofs.seekp(l_hdr);
        l_trie.write_to_file(m_store, l_ofs);
    }

    // aux method for custom writers
    template <typename T>
    T write_root_node(std::ofstream& a_ofs) {
        T ret = m_root.write_to_file<T>(m_store, a_ofs);
        m_root.write_links(m_store, a_ofs);
        return ret;
    }

protected:
    node_t& get_root(ptr_t a_ptr) {
        m_root_ptr = a_ptr;
        if (m_root_ptr == store_t::null)
            throw std::runtime_error("bad store pointer");
        node_t *l_ptr = m_store.
            template native_pointer<node_t>(a_ptr);
        if (l_ptr == NULL)
            throw std::runtime_error("bad store pointer");
        return *l_ptr;
    }

    node_t& make_root() {
        return get_root(m_store.template allocate<node_t>());
    }

    store_t m_store;
    ptr_t m_root_ptr;
    node_t& m_root;
};

} // namespace utxx

#endif // _UTXX_ACTRIE_HPP_
