// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief sparse array - grow only array implementation
 *
 * \author Dmitriy Kargapolov
 * \version 1.0
 * \since 12 May 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_SARRAY_HPP_
#define _UTXX_SARRAY_HPP_

#include <utxx/idxmap.hpp>

namespace utxx {

template <typename Data = char, typename IdxMap = idxmap<1> >
class sarray {
    typedef typename IdxMap::mask_t mask_t;
    typedef typename IdxMap::index_t index_t;

public:
    template<typename U>
    struct rebind { typedef sarray<U, IdxMap> other; };

    typedef typename IdxMap::symbol_t symbol_t;
    typedef typename IdxMap::bad_symbol bad_symbol;
    typedef std::pair<mask_t, index_t> pos_t;

private:
    mask_t m_mask;
    Data m_array[0];
    static IdxMap m_map;

public:
    sarray() : m_mask(0), m_array(0) {}

    bool find(symbol_t a_symbol, pos_t& a_ret) {
        m_map.index(m_mask, a_symbol, a_ret.first, a_ret.second);
        return (a_ret.first & m_mask) != 0;
    }

    // ensure at() called with valid position only
    Data& at(const pos_t& a_pos) {
        return m_array[a_pos.second];
    }

    // ensure at() called with valid position only
    const Data& at(const pos_t& a_pos) const {
        return m_array.at[a_pos.second];
    }
};

template <typename Data, typename IdxMap>
IdxMap sarray<Data, IdxMap>::m_map;

} // namespace utxx

#endif // _UTXX_SARRAY_HPP_
