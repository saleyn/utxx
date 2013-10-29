// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief s-trie read-only memory-mapped node with suffix and shift fields
 *
 * \author Dmitriy Kargapolov
 * \since 03 October 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_PNODE_SS_RO_HPP_
#define _UTXX_PNODE_SS_RO_HPP_

#include <stdint.h>

namespace utxx {

/**
 * \brief this class implements node of the trie
 * \tparam Store node store facility
 * \tparam Data node payload type
 * \tparam SArray type of collection of child nodes
 *
 * The Store and SArray types are themself templates
 */
template <typename Store, typename Data, typename SArray>
class pnode_ss_ro {
    // prevent copying
    pnode_ss_ro(const pnode_ss_ro&);
    pnode_ss_ro& operator=(const pnode_ss_ro&);

public:
    typedef typename Store::template rebind<pnode_ss_ro>::other store_t;
    typedef typename store_t::pointer_t ptr_t;
    typedef uint8_t shift_t;

    // sparse storage types
    typedef typename SArray::template rebind<ptr_t>::other sarray_t;
    typedef typename sarray_t::symbol_t symbol_t;

    // constructor
    pnode_ss_ro() {}

    // node data payload
    const Data& data() const {
        return *(Data*)b;
    }

    // link to suffix node
    const ptr_t& suffix() const {
        return *(ptr_t*)(b + sizeof(Data));
    }

    // suffix distance
    const shift_t& shift() const {
        return *(shift_t*)(b + sizeof(Data) + sizeof(ptr_t));
    }

    // collection of child nodes
    const sarray_t& children() const {
        return *(sarray_t*)(b + sizeof(Data) + sizeof(ptr_t) + sizeof(shift_t));
    }

private:
    // pointer to data in format |Data|suffix|shift|children|
    char b[sizeof(Data) + sizeof(ptr_t) + sizeof(shift_t) + sizeof(sarray_t)];
};

} // namespace utxx

#endif // _UTXX_PNODE_SS_RO_HPP_
