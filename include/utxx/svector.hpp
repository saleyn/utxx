// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief sparse array - vector based implementation
 *
 * This is vector based expandable implementation of sparse array
 * with write-to-file support. Read-only counterpart of this class
 * is utxx::sarray.
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

#ifndef _UTXX_SVECTOR_HPP_
#define _UTXX_SVECTOR_HPP_

#include <utxx/idxmap.hpp>
#include <vector>
#include <boost/foreach.hpp>
#include <fstream>

namespace utxx {

template <typename Data = char, typename IdxMap = idxmap<1>,
          typename Alloc = std::allocator<char> >
class svector {
    typedef typename IdxMap::mask_t mask_t;
    typedef typename IdxMap::index_t index_t;
    typedef std::vector<Data, Alloc> array_t;

    enum { capacity = IdxMap::capacity };

    mask_t m_mask;
    array_t m_array;
    static IdxMap m_map;

public:
    typedef typename IdxMap::symbol_t symbol_t;
    typedef typename IdxMap::bad_symbol bad_symbol;

    template<typename U>
    struct rebind { typedef svector<U, IdxMap, Alloc> other; };

    svector() : m_mask(0) {}

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

    // call functor for each value
    template<typename F> void foreach_value(F f) {
        BOOST_FOREACH(const Data& data, m_array) f(data);
    }

    // key to key-val functor adapter
    template<typename T, typename F>
    class k2kv {
        const T& a_;
        F& f_;
        typename T::const_iterator i_;
    public:
        k2kv(const T& a, F& f) : a_(a), f_(f) {
            i_ = a_.begin();
        }
        template<typename U>
        void operator()(U k) {
            f_(k, *i_); ++i_;
        }
    };

    // call functor for each key-value pair
    template<typename F> void foreach_keyval(F f) const {
        IdxMap::foreach(m_mask, k2kv<array_t, F>(m_array, f));
    }

    template <typename OffsetType>
    struct ext_header {
        // header data to export
        struct {
            mask_t mask;
            OffsetType children[capacity];
        } data;

        // number of children - not exported directly
        unsigned cnt;

        // write header to file
        void write_to_file(std::ofstream& a_ofs) const {
            if (cnt > capacity)
                throw std::out_of_range("invalid number of node children");
            a_ofs.write((const char *)&data,
                sizeof(data) - (capacity - cnt) * sizeof(OffsetType));
        }
    };

    // write children to file, form header
    template<typename OffsetType, typename WriteChildF>
    void write_to_file(ext_header<OffsetType>& a_ext_header,
            WriteChildF write_child_f, std::ofstream& a_ofs) const {
        // fill mask
        a_ext_header.data.mask = m_mask;
        // write children, fill offsets
        unsigned i = 0;
        BOOST_FOREACH(const Data& data, m_array) {
            if (i == capacity)
                throw std::out_of_range("number of children");
            a_ext_header.data.children[i++] = write_child_f(data, a_ofs);
        }
        // save number of children
        a_ext_header.cnt = i;
    }
};

template <typename Data, typename IdxMap, typename Alloc>
IdxMap svector<Data, IdxMap, Alloc>::m_map;

} // namespace utxx

#endif // _UTXX_SVECTOR_HPP_
