// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief digit trie composition demo
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

#include <utxx/container/digit_trie.hpp>

namespace ct = utxx::container;

// payload type
typedef std::string data_t;

// digit trie types
typedef ct::digit_trie<data_t> types;

// trie node type
typedef typename types::node_type node_t;

// trie type
typedef typename types::trie_type trie_t;

// concrete trie store type
typedef typename types::store_type store_t;

// key element position type
typedef typename types::position_type pos_t;

// fold functor example
static
bool fun(std::string& acc, const data_t& data, const store_t&, pos_t, bool) {
    if (data.empty())
        return true;
    acc = data;
    std::cout << acc << std::endl;
    return true;
}

// foreach functor example
static void enumerate(const std::string& key, node_t& node, store_t&) {
    std::cout << "'" << key << "' -> '" << node.data() << "'" << std::endl;
}

int main() {
    trie_t trie;

    // store some data
    trie.store("123", "three");
    trie.store("1234", "four");
    trie.store("12345", "five");

    // fold through the key-matching nodes
    std::string ret;
    trie.fold("1234567", ret, fun);
    std::cout << "lookup result: " << (ret.empty() ? "not found" : ret)
        << std::endl;

    // traverse all the nodes
    trie.foreach<ct::up, std::string>(enumerate);
    trie.foreach<ct::down, std::string>(enumerate);

    return 0;
}
