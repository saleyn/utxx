// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief s-trie writable node
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

#ifndef _UTXX_PNODE_HPP_
#define _UTXX_PNODE_HPP_

#include <fstream>
#include <boost/numeric/conversion/cast.hpp>

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
class pnode {
    // prevent copying
    pnode(const pnode&);
    pnode& operator=(const pnode&);

public:
    typedef typename Store::template rebind<pnode>::other store_t;
    typedef typename store_t::pointer_t ptr_t;

    // sparse storage types
    typedef typename SArray::template rebind<ptr_t>::other sarray_t;
    typedef typename sarray_t::symbol_t symbol_t;

    // constructor
    pnode() {}

    // write node to file
    template <typename T, typename F>
    T write_to_file(const store_t& a_store, F a_f, std::ofstream& a_ofs) const {

        // write data payload, get encoded reference
        typename Data::template ext_header<T> l_data;
        m_data.write_to_file(l_data, a_store, a_ofs);

        // write children encoded, get encoded reference
        typename sarray_t::template ext_header<T> l_children;
        m_children.write_to_file(l_children, a_f, a_ofs);

        // save offset of the node encoded
        T l_ret = boost::numeric_cast<T, std::streamoff>(a_ofs.tellp());

        // write encoded data reference
        l_data.write_to_file(a_ofs);

        // write encoded children reference
        l_children.write_to_file(a_ofs);

        // return offset of the node encoded
        return l_ret;
    }

    // no cross-links to update - empty method
    template <typename F>
    void write_links(const store_t&, F, std::ofstream&) {}

    // node data payload
    const Data& data() const { return m_data; }
    Data& data() { return m_data; }

    // collection of child nodes
    const sarray_t& children() const { return m_children; }
    sarray_t& children() { return m_children; }

private:
    Data m_data;
    sarray_t m_children;
};

} // namespace utxx

#endif // _UTXX_PNODE_HPP_
