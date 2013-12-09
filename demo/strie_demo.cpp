// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief strie composition demo
 *
 * \author Dmitriy Kargapolov
 * \since 14 November 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <utxx/simple_node_store.hpp>
#include <utxx/svector.hpp>
#include <utxx/pnode.hpp>
#include <utxx/ptrie.hpp>

#include <iostream>

// payload type
typedef std::string data_t;

// trie node type
typedef utxx::pnode<utxx::simple_node_store<>, data_t, utxx::svector<> > node_t;

// trie type
typedef utxx::ptrie<node_t> trie_t;

// concrete trie store type
typedef trie_t::store_t store_t;

// key element position type (default: uint32_t)
typedef typename trie_t::position_t pos_t;

// fold functor example
static bool fun(std::string& acc, const data_t& data, const trie_t::store_t&,
        pos_t, bool) {
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
    trie.foreach<utxx::up, std::string>(enumerate);
    trie.foreach<utxx::down, std::string>(enumerate);

    return 0;
}
