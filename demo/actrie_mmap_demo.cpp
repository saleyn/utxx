// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief actrie mmap-ed to file demo
 *
 * \author Dmitriy Kargapolov
 * \since 15 November 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <utxx/container/detail/flat_data_store.hpp>
#include <utxx/container/detail/sarray.hpp>
#include <utxx/container/detail/pnode_ss_ro.hpp>
#include <utxx/container/detail/default_ptrie_codec.hpp>
#include <utxx/container/mmap_ptrie.hpp>

#include "string_codec.hpp"

namespace ct = utxx::container;
namespace dt = utxx::container::detail;

// offset type in external data representation
typedef uint32_t offset_t;

// payload type
typedef string_codec::bind<offset_t>::data_type data_t;

// trie node type
typedef dt::pnode_ss_ro<
        dt::flat_data_store</*Node*/void, offset_t>, data_t, dt::sarray<>
> node_t;

// root node finder
typedef dt::mmap_trie_codec::root_finder<offset_t> root_f;

// trie type (default traits)
typedef ct::mmap_ptrie<node_t, root_f> trie_t;

// concrete trie store type
typedef typename trie_t::store_t store_t;

// key element position type (default: uint32_t)
typedef typename trie_t::position_t pos_t;

// fold functor example
static bool fun(const char *& acc, const data_t& data, const store_t& store,
        pos_t begin, pos_t end, bool has_next) {
    if (data.empty())
        return true;
    acc = data.str(store);
    std::cout << begin << ":" << end << ":" << has_next << ":" << acc << "\n";
    return true;
}

// foreach functor example
static void enumerate(const std::string& key, const node_t& node,
        store_t store) {
    const data_t& data = node.data();
    std::cout << "'" << key << "' -> '" << data.str(store) << "'" << std::endl;
}

int main() {
    trie_t trie("actrie.bin");

    // fold through the key-matching nodes
    const char *ret = 0;
    trie.fold_full("01234567", ret, fun);
    std::cout << "lookup result: " << (ret ? ret : "not found") << std::endl;

    // traverse all the nodes
    trie.foreach<ct::up, std::string>(enumerate);
    trie.foreach<ct::down, std::string>(enumerate);

    return 0;
}
