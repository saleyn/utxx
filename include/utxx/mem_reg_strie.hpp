// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief s-trie in flat memory region, data - immediate part of the node
 *
 * \author Dmitriy Kargapolov
 * \version 1.0
 * \since 15 June 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_MEM_REG_STRIE_HPP_
#define _UTXX_MEM_REG_STRIE_HPP_

#include <utxx/strie.hpp>
#include <utxx/sarray.hpp>

namespace utxx {

template <typename Store, typename Data, typename SArray = utxx::sarray<> >
class mem_reg_strie {
protected:
    // default "is data empty" functor
    static bool empty_f(const Data& obj) { return obj.empty(); }

    // default "is data empty" functor with "exact matching" flag
    static bool empty_x_f(const Data& obj, bool ex) { return obj.empty(ex); }

    // strie node holding immediate data
    typedef detail::strie_node<Store, Data, SArray> node_t;

    // node storage type
    typedef typename node_t::store_t node_store_t;

public:
    // data type
    typedef Data data_t;
    // "generic" store offset type
    typedef typename Store::pointer_t offset_t;
    // bad symbol exception handling types
    typedef typename node_t::symbol_t symbol_t;
    typedef typename node_t::bad_symbol bad_symbol;

    // node storage mapped to the memory region provided
    mem_reg_strie(const void *a_mem, offset_t a_len, offset_t a_root)
        : m_node_store(a_mem, a_len)
        , m_root(root(a_root))
    {}

    node_t& root(offset_t a_root) const {
        node_t *l_ptr = m_node_store.native_pointer(a_root);
        if (l_ptr == 0)
            throw std::invalid_argument("mem_reg_strie: bad root offset");
        return *l_ptr;
    }
    
    // fold through trie nodes following key components
    template <typename A, typename F>
    void fold(const char *key, A& acc, F proc) {
        m_root.fold<A, F>(m_node_store, key, acc, proc);
    }

    // lookup data by key, prefix matching only
    template <typename F>
    Data* lookup(const char *a_key, F a_is_empty) {
        return m_root.lookup(m_node_store, a_key, a_is_empty);
    }

    // lookup data by key, prefix matching only, default "data empty" functor
    Data* lookup(const char *a_key) {
        return lookup(a_key, empty_f);
    }

    // lookup data by key, exact matching allowed
    template <typename F>
    Data* lookup_exact(const char *a_key, F a_is_empty) {
        return m_root.lookup_exact(m_node_store, a_key, a_is_empty);
    }

    // lookup data by key, exact matching allowed, default "data empty" functor
    Data* lookup_exact(const char *a_key) {
        return lookup_exact(a_key, empty_x_f);
    }

protected:
    node_store_t m_node_store;
    node_t& m_root;
};

} // namespace utxx

#endif // _UTXX_MEM_REG_STRIE_HPP_
