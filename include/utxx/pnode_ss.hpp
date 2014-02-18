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
 * \tparam Coll type of collection of child nodes
 * \tparam Offset offset type for writing or void
 *
 * The Store and Coll types are themself templates
 */
template<typename Store, typename Data, typename Coll, typename Offset = void>
class pnode_ss {
    // prevent copying
    pnode_ss(const pnode_ss&);
    pnode_ss& operator=(const pnode_ss&);

public:
    typedef typename Store::template rebind<pnode_ss>::other store_t;
    typedef typename store_t::pointer_t ptr_t;
    typedef uint8_t shift_t;

    // sparse storage types
    typedef typename Coll::template rebind<ptr_t>::other sarray_t;
    typedef typename sarray_t::symbol_t symbol_t;

    // metadata to support cross-links writing
    typedef detail::meta<Offset> meta_t;

    // constructor
    pnode_ss() : m_suffix(store_t::null), m_shift(0) {}

    // write node to output store, return store pointer type
    template<typename T, typename F>
    typename T::addr_type write_to_store(const store_t& store, F func,
            typename T::store_type& out) const {

        // encode data payload
        typename T::data_encoder data_encoder;
        data_encoder.store(m_data, store, out);
        const buf_t& data_buff = data_encoder.buff();

        // encode children
        typename T::coll_encoder children_encoder;
        children_encoder.store(m_children, store, func, out);
        const buf_t& coll_buff = children_encoder.buff();

        // save offset to the suffix link
        m_meta.link = data_buff.second;

        // encode empty suffix link (reserve space)
        Offset l_link = 0;
        link_buff.first = &l_link;
        link_buff.second = sizeof(l_link);

        // encode shift
        shift_buff.first = &m_shift;
        shift_buff.second = sizeof(m_shift);

        // store sequence of buffers as single memory chunk, return address,
        // save it as node reference (to populate blue links in the 2nd pass)
        m_meta.node = out.store(data_buff, link_buff, shift_buff, coll_buff);

        return m_meta.node;
    }

    template<typename T, typename F>
    void store_links(const store_t& store, F f, typename T::store_type& out) {
        // first process children
        m_children.foreach_value(f);
        // suffix node reference
        if (m_suffix == store_t::null)
            return;
        pnode_ss *l_ptr = store.template native_pointer<pnode_ss>(m_suffix);
        if (!l_ptr)
            throw std::invalid_argument("bad suffix pointer");
        buf_t l_buf(&l_ptr->m_meta.node, sizeof(m_meta.node));
        // write suffix node offset at specific position
        out.store_at(m_meta.node, m_meta.link, l_buf);
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

    // interim data
    typedef std::pair<const void *, size_t> buf_t;
    mutable buf_t link_buff, shift_buff;
};

} // namespace utxx

#endif // _UTXX_PNODE_SS_HPP_
