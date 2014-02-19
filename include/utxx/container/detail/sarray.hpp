// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief sparse array - read only implementation
 *
 * This is read-only complement to utxx::container::detail::svector class.
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

#ifndef _UTXX_CONTAINER_DETAIL_SARRAY_HPP_
#define _UTXX_CONTAINER_DETAIL_SARRAY_HPP_

#include <utxx/container/detail/idxmap.hpp>
#include <utxx/container/detail/scollitbase.hpp>

namespace utxx {
namespace container {
namespace detail {

template <typename Data = char, typename IdxMap = idxmap<1> >
class sarray {
public:
    typedef Data data_t;
    typedef const Data *data_iter_t;
    typedef typename IdxMap::mask_t mask_t;
    typedef typename IdxMap::symbol_t symbol_t;
    typedef typename IdxMap::bad_symbol bad_symbol;
    enum { MaxMask = IdxMap::maxmask };

private:
    typedef typename IdxMap::index_t index_t;

    mask_t m_mask;
    Data m_array[0];
    static IdxMap m_map;

public:
    template<typename U>
    struct rebind { typedef sarray<U, IdxMap> other; };

    sarray() : m_mask(0) {}

    const mask_t& mask() const { return m_mask; }
    const data_iter_t data() const { return &m_array[0]; }

    // find an element by symbol
    const Data* get(symbol_t a_symbol) const {
        mask_t l_mask; index_t l_index;
        m_map.index(m_mask, a_symbol, l_mask, l_index);
        if ((l_mask & m_mask) != 0)
            return &m_array[l_index];
        else
            return 0;
    }

    typedef iterator_base<sarray, false> iterator;
    typedef iterator_base<sarray, true> const_iterator;

    iterator begin() { return iterator(*this); }
    const_iterator begin() const { return const_iterator(*this); }

    iterator end() { return iterator(); }
    const_iterator end() const { return const_iterator(); }

    // call functor for each key-value pair
    template<typename F> void foreach_keyval(F f) const {
        for (const_iterator it = begin(), e = end(); it != e; ++it)
            f(it->first, it->second);
    }

    // collection writer preparing data for reading by sarray
    //
    struct encoder {
        typedef std::pair<void *, size_t> buf_t;
        enum { capacity = IdxMap::capacity };

        struct {
            mask_t mask;
            Data elements[capacity];
        } body;

        unsigned cnt;

        encoder() : cnt(0) { body.mask = 0; }

        template<typename K, typename V, typename F, typename S>
        void store_it(K k, V v, F& func, S& out) {
            int i = k - '0';
            if (i < 0 || i > 9)
                throw std::out_of_range("element key");
            body.mask |= 1 << i;
            if (cnt == capacity)
                throw std::out_of_range("number of elements");
            body.elements[cnt++] = func(v);
        }

        template<typename T, typename S, typename F, typename O>
        void store(const T& coll, const S&, F func, O& out) {
            // BOOST_FOREACH(const typename T::value_type& v, coll)
            //     store_it(v.first, v.second, func, out);
            coll.foreach_keyval(ftor<encoder, F, O>(*this, func, out));
            buf.first = &body;
            buf.second = sizeof(body) - (capacity - cnt) * sizeof(Data);
        }

        const buf_t& buff() const { return buf; }

    private:
        template<typename T, typename F, typename O>
        struct ftor {
            ftor(T& ftor, F& f, O& o) : t_(ftor), f_(f), o_(o) {}
            T& t_; F& f_; O& o_;
            template<typename K, typename V>
            void operator()(K k, V v) { t_.store_it(k, v, f_, o_); }
        };

        buf_t buf;
    };

};

template <typename Data, typename IdxMap>
IdxMap sarray<Data, IdxMap>::m_map;

} // namespace detail
} // namespace container
} // namespace utxx

#endif // _UTXX_CONTAINER_DETAIL_SARRAY_HPP_
