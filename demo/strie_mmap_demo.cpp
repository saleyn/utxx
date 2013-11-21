// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief strie mmap-ed to file demo
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

#include <utxx/flat_data_store.hpp>
#include <utxx/sarray.hpp>
#include <utxx/pnode_ro.hpp>
#include <utxx/mmap_ptrie.hpp>

// offset type in external data representation
typedef uint32_t offset_t;

// external string representation - offset in file pointing to actual string
class encoded_string {
    offset_t ptr;

public:
    template<typename Store>
    bool empty() const { return ptr == Store::null; }

    template<typename Store>
    const char *str(const Store& store) const {
        if (empty<Store>())
            return "";
        const char *s = store.template native_pointer<const char>(ptr);
        if (s == NULL)
            throw std::runtime_error("bad store pointer");
        return s;
    }
};

// payload type
typedef encoded_string data_t;

// trie node type
typedef utxx::pnode_ro<
        utxx::flat_data_store</*Node*/void, offset_t>, data_t, utxx::sarray<>
> node_t;

// trie type
typedef utxx::mmap_ptrie<node_t> trie_t;

// concrete trie store type
typedef typename trie_t::store_t store_t;

// get offset of the 1st (root) node of the trie
static offset_t root(const void *m_addr, size_t m_size) {
    size_t s = sizeof(offset_t);
    if (m_size < s)
        throw std::runtime_error("short file");
    return *(const offset_t *) ((const char *) m_addr + m_size - s);
}

// fold functor example
static bool fun(const char *& acc, const data_t& data, const store_t& store,
        const char *ptr) {
    if (data.empty<store_t>())
        return true;
    acc = data.str(store);
    std::cout << acc << std::endl;
    return true;
}

// foreach functor example
static void enumerate(const std::string& key, const node_t& node,
        store_t store) {
    const encoded_string& data = node.data();
    std::cout << "'" << key << "' -> '" << data.str(store) << "'" << std::endl;
}

int main() {
    trie_t trie("trie.bin", root);

    // fold through the key-matching nodes
    const char *ret = 0;
    trie.fold("1234567", ret, fun);
    std::cout << "lookup result: " << (ret ? ret : "not found") << std::endl;

    // traverse all the nodes
    trie.foreach<utxx::up>(enumerate);
    trie.foreach<utxx::down>(enumerate);

    return 0;
}
