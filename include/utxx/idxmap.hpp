// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief symbol-to-index mapping
 *
 * \author Dmitriy Kargapolov
 * \version 1.0
 * \since 01 April 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_IDXMAP_HPP_
#define _UTXX_IDXMAP_HPP_

#include <stdint.h>
#include <stdexcept>

namespace utxx {

// special table to map symbol and mask to index
// in context of simple trie implementation
template <int Pack>
class idxmap {
    static const int NElem = 1024 / Pack * 10;

public:
    typedef int8_t index_t;
    typedef char symbol_t;
    typedef uint16_t mask_t;
    enum { capacity = 10 };

    class bad_symbol : public std::invalid_argument {
        symbol_t m_symbol;
    public:
        bad_symbol(symbol_t a_symbol)
            : std::invalid_argument("bad symbol")
            , m_symbol(a_symbol)
        {}
        symbol_t symbol() const { return m_symbol; }
    };

    idxmap();
    void index(mask_t a_mask, symbol_t a_symbol, mask_t& a_ret_mask,
        index_t& a_ret_index);

private:
    index_t m_maps[NElem];
};

template <>
idxmap<1>::idxmap() {
    for (mask_t i=0, sm=1u; i<10; ++i, sm<<=1) {
        for (mask_t mask=0; mask<1024; ++mask) {
            // find number of i-th 1 in the mask
            index_t idx = 0;
            for (mask_t m = 1u; m != sm; m <<= 1)
                if ((m & mask) != 0) ++idx;
            m_maps[mask | (i << 10)] = idx;
        }
    }
}

template <>
idxmap<2>::idxmap() {
    for (mask_t i=0, sm=1u; i<10; ++i, sm<<=1) {
        for (mask_t mask=0; mask<1024; ++mask) {
            // find number of i-th 1 in the mask
            index_t idx = 0;
            for (mask_t m = 1u; m != sm; m <<= 1)
                if ((m & mask) != 0) ++idx;
            mask_t k = mask | (i << 10);
            index_t& m = m_maps[k >> 1];
            if ((k & 1) != 0)
                m = (m & 0x0f) | (idx << 4);
            else
                m = (m & 0xf0) | idx;
        }
    }
}

template<>
void idxmap<1>::index(mask_t a_mask, symbol_t a_symbol, mask_t& a_ret_mask,
    index_t& a_ret_index)
{
    if (a_mask > 1023)
        throw std::invalid_argument("bad mask");
    int i = a_symbol - '0';
    if (i < 0 || i > 9)
        throw bad_symbol(a_symbol);
    a_ret_mask = 1 << i;
    a_ret_index = m_maps[a_mask | (i << 10)];
}

template<>
void idxmap<2>::index(mask_t a_mask, symbol_t a_symbol, mask_t& a_ret_mask,
    index_t& a_ret_index)
{
    if (a_mask > 1023)
        throw std::invalid_argument("bad mask");
    int i = a_symbol - '0';
    if (i < 0 || i > 9)
        throw bad_symbol(a_symbol);
    mask_t m = a_mask | (i << 10);
    a_ret_mask = 1 << i;
    if ((m & 1) != 0)
        a_ret_index = (m_maps[m >> 1] & 0xf0) >> 4;
    else
        a_ret_index = m_maps[m >> 1] & 0x0f;
}

} // namespace utxx

#endif // _UTXX_IDXMAP_HPP_
