// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief sparse array - read only implementation
 *
 * This is read-only complement to utxx::svector class.
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

#ifndef _UTXX_SARRAY_HPP_
#define _UTXX_SARRAY_HPP_

#include <utxx/idxmap.hpp>

namespace utxx {

template <typename Data = char, typename IdxMap = idxmap<1> >
class sarray {
    typedef typename IdxMap::mask_t mask_t;
    typedef typename IdxMap::index_t index_t;

    mask_t m_mask;
    Data m_array[0];
    static IdxMap m_map;

public:
    typedef typename IdxMap::symbol_t symbol_t;
    typedef typename IdxMap::bad_symbol bad_symbol;

    template<typename U>
    struct rebind { typedef sarray<U, IdxMap> other; };

    sarray() : m_mask(0) {}

    // find an element by symbol
    const Data* get(symbol_t a_symbol) const {
        mask_t l_mask; index_t l_index;
        m_map.index(m_mask, a_symbol, l_mask, l_index);
        if ((l_mask & m_mask) != 0)
            return &m_array[l_index];
        else
            return 0;
    }

    // key to key-val functor adapter
    template<typename T, typename F>
    class k2kv {
        const T *a_;
        const F& f_;
        index_t i_;
    public:
        k2kv(const T *a, const F& f) : a_(a), f_(f) {
            i_ = 0;
        }
        template<typename U>
        void operator()(U k) {
            f_(k, a_[i_++]);
        }
    };

    // call functor for each key-value pair
    template<typename F> void foreach_keyval(F f) const {
        IdxMap::foreach(m_mask, k2kv<Data, F>(m_array, f));
    }
};

template <typename Data, typename IdxMap>
IdxMap sarray<Data, IdxMap>::m_map;

} // namespace utxx

#endif // _UTXX_SARRAY_HPP_
