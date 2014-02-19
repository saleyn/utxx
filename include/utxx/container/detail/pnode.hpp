// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief trie writable node
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

#ifndef _UTXX_CONTAINER_DETAIL_PNODE_HPP_
#define _UTXX_CONTAINER_DETAIL_PNODE_HPP_

namespace utxx {
namespace container {
namespace detail {

/**
 * \brief this class implements node of the trie
 * \tparam Store node store facility
 * \tparam Data node payload type
 * \tparam Coll collection of child nodes
 *
 * The Store and Coll types are themself templates
 * defining rebind<T>::other type
 */
template<typename Store, typename Data, typename Coll>
class pnode {
    // prevent copying
    pnode(const pnode&);
    pnode& operator=(const pnode&);

public:
    typedef typename Store::template rebind<pnode>::other store_t;
    typedef typename store_t::pointer_t ptr_t;

    // sparse storage types
    typedef typename Coll::template rebind<ptr_t>::other sarray_t;
    typedef typename sarray_t::symbol_t symbol_t;

    // constructor
    pnode() {}

    // write node to output store, return store pointer type
    template<typename T, typename F>
    typename T::addr_type write_to_store(const store_t& store, F func,
            typename T::store_type& out) const {

        // encode data payload
        typename T::data_encoder data_encoder;
        data_encoder.store(m_data, store, out);

        // encode children
        typename T::coll_encoder children_encoder;
        children_encoder.store(m_children, store, func, out);

        // store sequence of buffers as single memory chunk, return address
        return out.store(data_encoder.buff(), children_encoder.buff());
    }

    // no cross-links to update - empty method
    template <typename T, typename F>
    void store_links(const store_t&, F, typename T::store_type&) {}

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

} // namespace detail
} // namespace container
} // namespace utxx

#endif // _UTXX_CONTAINER_DETAIL_PNODE_HPP_
