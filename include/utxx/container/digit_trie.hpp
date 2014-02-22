// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief trie preset for digital keys
 *
 * \author Dmitriy Kargapolov
 * \since 19 February 2014
 *
 */

/*
 * Copyright (C) 2014 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_CONTAINER_DIGIT_TRIE_HPP_
#define _UTXX_CONTAINER_DIGIT_TRIE_HPP_

#include <utxx/container/detail/simple_node_store.hpp>
#include <utxx/container/detail/flat_data_store.hpp>
#include <utxx/container/detail/file_store.hpp>
#include <utxx/container/detail/svector.hpp>
#include <utxx/container/detail/sarray.hpp>
#include <utxx/container/detail/pnode.hpp>
#include <utxx/container/detail/pnode_ss.hpp>
#include <utxx/container/detail/pnode_ro.hpp>
#include <utxx/container/detail/pnode_ss_ro.hpp>
#include <utxx/container/detail/default_ptrie_codec.hpp>
#include <utxx/container/ptrie.hpp>
#include <utxx/container/mmap_ptrie.hpp>

namespace utxx {
namespace container {

typedef enum {
    Trie_Normal = 0,
    Trie_AhoCorasick,
    Trie_AhoCorasickExport
} trie_model;

namespace {

namespace ct = utxx::container;
namespace dt = utxx::container::detail;

template<typename Data, trie_model Model, typename AddrType>
struct digit_node;

template<typename Data, typename AddrType>
struct digit_node<Data, Trie_Normal, AddrType> {
    // trie node type
    typedef
        dt::pnode<dt::simple_node_store<>, Data, dt::svector<> >
            type;
    // trie read-only (mmap-ed) node type
    typedef
        dt::pnode_ro<dt::flat_data_store<void, AddrType>, Data, dt::sarray<> >
            type_ro;
};

template<typename Data, typename AddrType>
struct digit_node<Data, Trie_AhoCorasick, AddrType> {
    // trie node (with suffix link) type
    typedef
        dt::pnode_ss<dt::simple_node_store<>, Data, dt::svector<> >
            type;
    // trie read-only (mmap-ed) node type
    typedef
        dt::pnode_ss_ro<dt::flat_data_store<void, AddrType>, Data,
            dt::sarray<> > type_ro;
};

template<typename Data, typename AddrType>
struct digit_node<Data, Trie_AhoCorasickExport, AddrType> {
    // trie node (with suffix link) type, export variant
    typedef
        dt::pnode_ss<dt::simple_node_store<>, Data, dt::svector<>, AddrType>
            type;
    // trie read-only (mmap-ed) node type
    typedef
        dt::pnode_ss_ro<dt::flat_data_store<void, AddrType>, Data,
            dt::sarray<> > type_ro;
};

}

template<
    typename Data, trie_model Model = Trie_Normal, typename AddrType = uint32_t
>
struct digit_trie {

    // trie node type
    typedef typename digit_node<Data, Model, AddrType>::type node_type;

    // trie type
    typedef ct::ptrie<node_type> trie_type;

    // concrete trie store type
    typedef typename trie_type::store_t store_type;

    // key element position type
    typedef typename trie_type::position_t position_type;

    // encoder traits
    template<typename DataCodec, typename TrieCodec = dt::mmap_trie_codec>
    struct encoder_type {

        // encoder protocol types
        typedef AddrType addr_type;
        typedef typename DataCodec::template bind<addr_type>::encoder data_encoder;
        typedef typename dt::sarray<addr_type>::encoder coll_encoder;

        typedef typename TrieCodec::template encoder<addr_type> trie_encoder;

        // not part of encoder protocol - placed here for convenience only
        typedef dt::file_store<addr_type> file_store;
    };
};

template<
    typename DataCodec, trie_model Model = Trie_Normal,
    typename AddrType = uint32_t, typename TrieCodec = dt::mmap_trie_codec
>
struct digit_mmap_trie {

    // node/data address type
    typedef AddrType addr_type;

    // node payload type
    typedef typename DataCodec::template bind<addr_type>::data_type data_type;

    // trie node type
    typedef typename digit_node<data_type, Model, addr_type>::type_ro node_type;

    // root node finder
    typedef typename TrieCodec::template root_finder<addr_type> root_finder;

    // mmap-ed trie type
    typedef ct::mmap_ptrie<node_type, root_finder> trie_type;

    // concrete trie store type
    typedef typename trie_type::store_t store_type;

    // key element position type
    typedef typename trie_type::position_t position_type;
};


} // namespace container
} // namespace utxx

#endif // _UTXX_CONTAINER_DIGIT_TRIE_HPP_
