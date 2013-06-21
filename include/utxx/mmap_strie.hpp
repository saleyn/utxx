// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief s-trie in memory mapped region
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

#ifndef _UTXX_MMAP_STRIE_HPP_
#define _UTXX_MMAP_STRIE_HPP_

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace utxx {

namespace { namespace bip = boost::interprocess; }

template <typename MemSTrie>
class mmap_strie {
protected:
    typedef MemSTrie trie_t;
    typedef typename trie_t::data_t data_t;
    typedef typename trie_t::offset_t offset_t;

    bip::file_mapping  m_fmap;
    bip::mapped_region m_reg;

    const void *m_addr; // address of memory region
    size_t      m_size; // size of memory region
    trie_t      m_trie;

public:
    template <typename F>
    mmap_strie(const char *fname, F root)
        : m_fmap(fname, bip::read_only)
        , m_reg(m_fmap, bip::read_only)
        , m_addr(m_reg.get_address())
        , m_size(m_reg.get_size())
        , m_trie(m_addr, m_size, root(m_addr, m_size))
    {}

    ~mmap_strie() {}

    // fold through trie nodes following key components
    template <typename A, typename F>
    void fold(const char *key, A& acc, F proc) {
        m_trie.fold(key, acc, proc);
    }

    // lookup data by key, prefix matching only
    template <typename F>
    data_t* lookup(const char *a_key, F is_empty) {
        return m_trie.lookup(a_key, is_empty);
    }

    // lookup data by key, prefix matching only, default "data empty" functor
    data_t* lookup(const char *a_key) {
        return m_trie.lookup(a_key);
    }

    // lookup data by key, prefix matching only, simple "data empty" functor
    data_t* lookup_simple(const char *a_key) {
        return m_trie.lookup_simple(a_key);
    }

    // lookup data by key, exact matching allowed
    template <typename F>
    data_t* lookup_exact(const char *a_key, F is_empty) {
        return m_trie.lookup_exact(a_key, is_empty);
    }

    // lookup data by key, exact matching allowed, default "data empty" functor
    data_t* lookup_exact(const char *a_key) {
        return m_trie.lookup_exact(a_key);
    }
};

} // namespace utxx

#endif // _UTXX_MMAP_STRIE_HPP_
