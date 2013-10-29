// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief s-trie read-only memory-mapped node
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

#ifndef _UTXX_PNODE_RO_HPP_
#define _UTXX_PNODE_RO_HPP_

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
class pnode_ro {
    // prevent copying
    pnode_ro(const pnode_ro&);
    pnode_ro& operator=(const pnode_ro&);

public:
    typedef typename Store::template rebind<pnode_ro>::other store_t;
    typedef typename store_t::pointer_t ptr_t;

    // sparse storage types
    typedef typename SArray::template rebind<ptr_t>::other sarray_t;
    typedef typename sarray_t::symbol_t symbol_t;

    // constructor
    pnode_ro() {}

    // node data payload
    const Data& data() const {
        return *(Data*)b;
    }

    // collection of child nodes
    const sarray_t& children() const {
        return *(sarray_t*)(b + sizeof(Data));
    }

private:
    // pointer to data in format |Data|children|
    char b[sizeof(Data) + sizeof(sarray_t)];
};

} // namespace utxx

#endif // _UTXX_PNODE_RO_HPP_
