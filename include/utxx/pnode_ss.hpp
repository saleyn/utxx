// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief s-trie writable node with suffix and shift fields
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

#ifndef _UTXX_PNODE_SS_HPP_
#define _UTXX_PNODE_SS_HPP_

#include <stdint.h>
#include <fstream>
#include <stdexcept>
#include <boost/numeric/conversion/cast.hpp>

namespace utxx {

namespace detail {

/**
 * \brief optional node metadata
 */
template<typename Offset>
struct meta {
    Offset node; // offset of the node written
    Offset link; // offset of the blue link written
    meta() : node(0), link(0) {}
};

template<> struct meta<void> {};

}

/**
 * \brief this class implements node of the trie
 * \tparam Store node store facility
 * \tparam Data node payload type
 * \tparam SArray type of collection of child nodes
 * \tparam Offset offset type for writing or void
 *
 * The Store and SArray types are themself templates
 */
template<typename Store, typename Data, typename SArray, typename Offset = void>
class pnode_ss {
    // prevent copying
    pnode_ss(const pnode_ss&);
    pnode_ss& operator=(const pnode_ss&);

public:
    typedef typename Store::template rebind<pnode_ss>::other store_t;
    typedef typename store_t::pointer_t ptr_t;
    typedef uint8_t shift_t;

    // sparse storage types
    typedef typename SArray::template rebind<ptr_t>::other sarray_t;
    typedef typename sarray_t::symbol_t symbol_t;

    // metadata to support cross-links writing
    typedef detail::meta<Offset> meta_t;

    // constructor
    pnode_ss() : m_suffix(store_t::null), m_shift(0) {}

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

        // save offset to the suffix link
        m_meta.link = boost::numeric_cast<T, std::streamoff>(a_ofs.tellp());

        // reserve space for the suffix link, fill it with zero
        T l_link = 0;
        a_ofs.write((const char *)&l_link, sizeof(l_link));

        // write shift
        a_ofs.write((const char *)&m_shift, sizeof(m_shift));

        // write encoded children reference
        l_children.write_to_file(a_ofs);

        // save node reference (to populate blue links in the 2nd pass)
        m_meta.node = l_ret;

        // return offset of the node encoded
        return l_ret;
    }

    // export cross-links and related info
    template <typename F>
    void write_links(const store_t& a_store, F a_f, std::ofstream& a_ofs) {
        // first process children
        m_children.foreach_value(a_f);
        // suffix node reference
        if (m_suffix == store_t::null)
            return;
        pnode_ss *l_ptr = a_store.template native_pointer<pnode_ss>(m_suffix);
        if (!l_ptr)
            throw std::invalid_argument("bad suffix pointer");
        // write suffix node offset at specific position
        a_ofs.seekp(m_meta.link);
        a_ofs.write((const char *)&l_ptr->m_meta.node, sizeof(m_meta.node));
    }

    // node data payload
    const Data& data() const { return m_data; }
    Data& data() { return m_data; }

    // link to suffix node
    const ptr_t& suffix() const { return m_suffix; }
    ptr_t& suffix() { return m_suffix; }

    // suffix distance
    const shift_t& shift() const { return m_shift; }
    shift_t& shift() { return m_shift; }

    // collection of child nodes
    const sarray_t& children() const { return m_children; }
    sarray_t& children() { return m_children; }

private:
    Data m_data;
    ptr_t m_suffix;
    shift_t m_shift;
    sarray_t m_children;

    mutable meta_t m_meta; ///< node metadata if any
};

} // namespace utxx

#endif // _UTXX_PNODE_SS_HPP_
