// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief ac-trie in memory mapped region
 *
 * \author Dmitriy Kargapolov
 * \since 24 September 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_MMAP_ACTRIE_HPP_
#define _UTXX_MMAP_ACTRIE_HPP_

#include <utxx/actrie.hpp>
#include <utxx/flat_data_store.hpp>
#include <utxx/sarray.hpp>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace utxx {

namespace { namespace bip = boost::interprocess; }

template <typename Data, typename Offset = int>
class mmap_actrie {
protected:
    typedef actrie<flat_data_store<void, Offset>, Data, sarray<> > trie_t;

public:
    typedef typename trie_t::store_t store_t;
    typedef typename trie_t::ptr_t ptr_t;

protected:
    bip::file_mapping  m_fmap;
    bip::mapped_region m_reg;

    const void *m_addr;  // address of memory region
    size_t      m_size;  // size of memory region
    store_t     m_store; // read-only node and data store
    ptr_t       m_root;  // root position
    trie_t      m_trie;  // underlying actrie

public:
    template <typename F>
    mmap_actrie(const char *fname, F root)
        : m_fmap(fname, bip::read_only)
        , m_reg(m_fmap, bip::read_only)
        , m_addr(m_reg.get_address())
        , m_size(m_reg.get_size())
        , m_store(m_addr, m_size)
        , m_root(root(m_addr, m_size))
        , m_trie(m_store, m_root)
    {}

    // fold through trie nodes following key components
    template <typename A, typename F>
    void fold(const char *key, A& acc, F proc) {
        m_trie.fold(key, acc, proc);
    }

    // fold through trie nodes following key components
    template <typename A, typename F>
    void fold_full(const char *key, A& acc, F proc) {
        m_trie.fold_full(key, acc, proc);
    }
};

} // namespace utxx

#endif // _UTXX_MMAP_ACTRIE_HPP_
