// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief sparse array - vector based implementation
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

#ifndef _UTXX_SVECTOR_HPP_
#define _UTXX_SVECTOR_HPP_

#include <utxx/idxmap.hpp>
#include <vector>

namespace utxx {

template <typename Data = char, typename IdxMap = idxmap<1>,
          typename Alloc = std::allocator<char> >
class svector {
    typedef typename IdxMap::index_t index_t;
    typedef std::vector<Data, Alloc> array_t;

public:
    typedef typename IdxMap::mask_t mask_t;
    enum { capacity = IdxMap::capacity };

    template<typename U>
    struct rebind { typedef svector<U, IdxMap, Alloc> other; };

    typedef typename IdxMap::symbol_t symbol_t;
    typedef typename IdxMap::bad_symbol bad_symbol;
    typedef std::pair<mask_t, index_t> pos_t;

private:
    mask_t m_mask;
    array_t m_array;
    static IdxMap m_map;

public:
    svector() : m_mask(0) {}

    mask_t mask() const { return m_mask; }

    bool find(symbol_t a_symbol, pos_t& a_ret) {
        m_map.index(m_mask, a_symbol, a_ret.first, a_ret.second);
        return (a_ret.first & m_mask) != 0;
    }

    Data& at(const pos_t& a_pos) {
        return m_array.at(a_pos.second);
    }

    const Data& at(const pos_t& a_pos) const {
        return m_array.at(a_pos.second);
    }

    void insert(const pos_t& a_pos, Data& a_data) {
        m_array.insert(m_array.begin() + a_pos.second, a_data);
        m_mask |= a_pos.first;
    }

    typedef typename array_t::iterator iterator;
    typedef typename array_t::const_iterator const_iterator;

    iterator begin() { return m_array.begin(); }
    const_iterator begin() const { return m_array.begin(); }

    iterator end() { return m_array.end(); }
    const_iterator end() const { return m_array.end(); }

};

template <typename Data, typename IdxMap, typename Alloc>
IdxMap svector<Data, IdxMap, Alloc>::m_map;

} // namespace utxx

#endif // _UTXX_SVECTOR_HPP_
