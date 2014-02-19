// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief sparse array - vector based implementation
 *
 * This is vector based expandable implementation of sparse array
 * with write-to-file support. Read-only counterpart of this class
 * is utxx::container::detail::sarray.
 *
 * \author Dmitriy Kargapolov
 * \since 12 May 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_CONTAINER_DETAIL_SVECTOR_HPP_
#define _UTXX_CONTAINER_DETAIL_SVECTOR_HPP_

#include <utxx/container/detail/idxmap.hpp>
#include <utxx/container/detail/scollitbase.hpp>
#include <vector>
#include <boost/foreach.hpp>

namespace utxx {
namespace container {
namespace detail {

template <typename Data = char, typename IdxMap = idxmap<1>,
          typename Alloc = std::allocator<char> >
class svector {
public:
    typedef Data data_t;
    typedef typename IdxMap::mask_t mask_t;
    typedef typename IdxMap::symbol_t symbol_t;
    typedef typename IdxMap::bad_symbol bad_symbol;
    enum { MaxMask = IdxMap::maxmask };

    typedef std::pair<const symbol_t, Data> value_type;

private:
    typedef typename IdxMap::index_t index_t;
    typedef std::vector<Data, Alloc> array_t;

    enum { capacity = IdxMap::capacity };

    mask_t m_mask;
    array_t m_array;
    static IdxMap m_map;

public:
    template<typename U>
    struct rebind { typedef svector<U, IdxMap, Alloc> other; };

    typedef typename array_t::const_iterator data_iter_t;

    svector() : m_mask(0) {}

    const mask_t& mask() const { return m_mask; }
    const data_iter_t data() const { return m_array.begin(); }

    // find an element by symbol
    const Data* get(symbol_t a_symbol) const {
        mask_t l_mask; index_t l_index;
        m_map.index(m_mask, a_symbol, l_mask, l_index);
        if ((l_mask & m_mask) != 0)
            return &m_array.at(l_index);
        else
            return 0;
    }

    // find element, if not found - create new element calling
    // functor of type C with no arguments, insert it into the
    // collection and return reference to the new element
    template<typename C> Data& ensure(symbol_t a_symbol, C create) {
        mask_t l_mask; index_t l_index;
        m_map.index(m_mask, a_symbol, l_mask, l_index);
        if ((l_mask & m_mask) == 0) {
            Data l_new = create();
            m_array.insert(m_array.begin() + l_index, l_new);
            m_mask |= l_mask;
        }
        return m_array.at(l_index);
    }

    typedef iterator_base<svector, false> iterator;
    typedef iterator_base<svector, true> const_iterator;

    iterator begin() { return iterator(*this); }
    const_iterator begin() const { return const_iterator(*this); }

    iterator end() { return iterator(); }
    const_iterator end() const { return const_iterator(); }

    // call functor for each value
    template<typename F> void foreach_value(F f) {
        BOOST_FOREACH(const Data& data, m_array) f(data);
    }

    // call functor for each key-value pair
    template<typename F> void foreach_keyval(F f) const {
        for (const_iterator it = begin(), e = end(); it != e; ++it)
            f(it->first, it->second);
    }
};

template <typename Data, typename IdxMap, typename Alloc>
IdxMap svector<Data, IdxMap, Alloc>::m_map;

} // namespace detail
} // namespace container
} // namespace utxx

#endif // _UTXX_CONTAINER_DETAIL_SVECTOR_HPP_
