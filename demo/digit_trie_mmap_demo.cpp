// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief digit trie mmap-ed to file demo
 *
 * \author Dmitriy Kargapolov
 * \since 20 February 2014
 *
 */

/*
 * Copyright (C) 2014 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <utxx/container/digit_trie.hpp>
#include "string_codec.hpp"

namespace ct = utxx::container;

// digit trie types
typedef ct::digit_mmap_trie<string_codec> types;

// trie type
typedef typename types::trie_type trie_t;

// trie node type
typedef typename types::node_type node_t;

// node payload type
typedef typename types::data_type data_t;

// concrete trie store type
typedef typename types::store_type store_t;

// address type in external data representation
typedef types::addr_type addr_t;

// key element position type
typedef typename types::position_type pos_t;

// fold functor example
static bool fun(const char *& acc, const data_t& data, const store_t& store,
        pos_t, bool) {
    if (data.empty())
        return true;
    acc = data.str(store);
    std::cout << acc << std::endl;
    return true;
}

// foreach functor example
static void enumerate(const std::string& key, const node_t& node,
        store_t store) {
    const data_t& data = node.data();
    std::cout << "'" << key << "' -> '" << data.str(store) << "'" << std::endl;
}

int main() {
    trie_t trie("trie.bin");

    // fold through the key-matching nodes
    const char *ret = 0;
    trie.fold("1234567", ret, fun);
    std::cout << "lookup result: " << (ret ? ret : "not found") << std::endl;

    // traverse all the nodes
    trie.foreach<ct::up, std::string>(enumerate);
    trie.foreach<ct::down, std::string>(enumerate);

    return 0;
}
