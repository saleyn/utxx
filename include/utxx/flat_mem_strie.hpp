// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief s-trie in flat memory region
 *
 * \author Dmitriy Kargapolov
 * \version 1.0
 * \since 16 April 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_FLAT_MEM_STRIE_HPP_
#define _UTXX_FLAT_MEM_STRIE_HPP_

#include <utxx/strie.hpp>
#include <utxx/sarray.hpp>

namespace utxx {

template <typename Store, typename Data, typename SArray = utxx::sarray<> >
class flat_mem_strie {
protected:
    // default "is data empty" functor
    static bool empty_f(const Data& obj) { return obj.empty(); }

    // default "is data empty" functor with "exact matching" flag
    static bool empty_x_f(const Data& obj, bool ex) { return obj.empty(ex); }

    // data storage type
    typedef typename Store::template rebind<Data>::other data_store_t;

    // strie data type is store pointer type
    typedef typename data_store_t::pointer_t data_ptr_t;

    // simple "is data empty" functor based on pointer value only
    static bool ptr_null_f(const data_ptr_t& a_ptr) {
        return a_ptr == data_store_t::null;
    }

    // default processing functor for fold method
    template <typename F>
    struct proc_f {
        data_store_t m_store;
        F m_proc_f;
        proc_f(data_store_t& a_store, F& a_proc_f)
            : m_store(a_store), m_proc_f(a_proc_f)
        {}
        template <typename A>
        bool operator()(A& acc, const data_ptr_t& a_ptr, const char *pos) {
            if (a_ptr == data_store_t::null)
                return true;
            Data *l_ptr = m_store.native_pointer(a_ptr);
            if (l_ptr == 0)
                throw std::invalid_argument("flat_mem_strie: bad data pointer");
            return m_proc_f(acc, *l_ptr, pos);
        }
    };

    // default "is data empty" functor
    template <typename F>
    struct is_empty {
        data_store_t m_store;
        F m_is_empty;
        is_empty(data_store_t& a_store, F& a_is_empty)
            : m_store(a_store), m_is_empty(a_is_empty)
        {}
        bool operator()(const data_ptr_t& a_ptr) {
            if (a_ptr == data_store_t::null)
                return true;
            Data *l_ptr = m_store.native_pointer(a_ptr);
            if (l_ptr == 0)
                throw std::invalid_argument("flat_mem_strie: bad data pointer");
            return m_is_empty(*l_ptr);
        }
    };

    // default "is data empty" functor with "exact matching" flag
    template <typename F>
    struct is_empty_ex {
        data_store_t m_store;
        F m_is_empty;
        is_empty_ex(data_store_t& a_store, F& a_is_empty)
            : m_store(a_store), m_is_empty(a_is_empty)
        {}
        bool operator()(const data_ptr_t& a_ptr, bool a_exact) {
            if (a_ptr == data_store_t::null)
                return true;
            Data *l_ptr = m_store.native_pointer(a_ptr);
            if (l_ptr == 0)
                throw std::invalid_argument("flat_mem_strie: bad data pointer");
            return m_is_empty(*l_ptr, a_exact);
        }
    };

    // strie node holding data store pointer
    typedef detail::strie_node<Store, data_ptr_t, SArray> node_t;

    // node storage type
    typedef typename node_t::store_t node_store_t;

public:
    //  data type
    typedef Data data_t;
    // "generic" store offset type
    typedef typename Store::pointer_t offset_t;
    // bad symbol exception handling types
    typedef typename node_t::symbol_t symbol_t;
    typedef typename node_t::bad_symbol bad_symbol;

    // both data and node storages mapped to the same memory region
    flat_mem_strie(const void *a_mem, offset_t a_len, offset_t a_root)
        : m_node_store(a_mem, a_len)
        , m_data_store(a_mem, a_len)
        , m_root(root(a_root))
    {}

    node_t& root(offset_t a_root) const {
        node_t *l_ptr = m_node_store.native_pointer(a_root);
        if (l_ptr == 0)
            throw std::invalid_argument("flat_mem_strie: bad root offset");
        return *l_ptr;
    }
    
    // fold through trie nodes following key components
    template <typename A, typename F>
    void fold(const char *key, A& acc, F proc) {
        m_root.fold<A>(m_node_store, key, acc, proc_f<F>(m_data_store, proc));
    }

    // lookup data by key, prefix matching only
    template <typename F>
    Data* lookup(const char *a_key, F a_is_empty) {
        data_ptr_t *l_data_ptr = m_root.lookup(m_node_store, a_key,
            is_empty<F>(m_data_store, a_is_empty));
        if (l_data_ptr == 0)
            return 0;
        return m_data_store.native_pointer(*l_data_ptr);
    }

    // lookup data by key, prefix matching only, simple "data empty" functor
    Data* lookup_simple(const char *a_key) {
        data_ptr_t *l_data_ptr = m_root.lookup(m_node_store, a_key, ptr_null_f);
        if (l_data_ptr == 0)
            return 0;
        return m_data_store.native_pointer(*l_data_ptr);
    }

    // lookup data by key, prefix matching only, default "data empty" functor
    Data* lookup(const char *a_key) {
        return lookup(a_key, empty_f);
    }

    // lookup data by key, exact matching allowed
    template <typename F>
    Data* lookup_exact(const char *a_key, F a_is_empty) {
        data_ptr_t *l_data_ptr = m_root.lookup_exact(m_node_store, a_key,
            is_empty_ex<F>(m_data_store, a_is_empty));
        if (l_data_ptr == 0)
            return 0;
        return m_data_store.native_pointer(*l_data_ptr);
    }

    // lookup data by key, exact matching allowed, default "data empty" functor
    Data* lookup_exact(const char *a_key) {
        return lookup_exact(a_key, empty_x_f);
    }

protected:
    node_store_t m_node_store;
    data_store_t m_data_store;
    node_t& m_root;
};

} // namespace utxx

#endif // _UTXX_FLAT_MEM_STRIE_HPP_
