// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief sparse collection iterator base class
 *
 * For use with sarray and svector classes
 *
 * \author Dmitriy Kargapolov
 * \since 18 February 2014
 *
 */

/*
 * Copyright (C) 2014 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_CONTAINER_DETAIL_SCOLLITBASE_HPP_
#define _UTXX_CONTAINER_DETAIL_SCOLLITBASE_HPP_

namespace utxx {
namespace container {
namespace detail {

template<typename T, bool Const> struct type_cast {};
template<typename T> struct type_cast<T, true> { typedef const T type; };
template<typename T> struct type_cast<T, false> { typedef T type; };

// collection iterator base
template<typename Coll, bool Const>
class iterator_base: public std::iterator<
        std::input_iterator_tag, typename type_cast<
            std::pair<const typename Coll::symbol_t, typename Coll::data_t>,
            Const>::type> {

    // iterator essential types
    typedef typename Coll::mask_t mask_t;
    typedef typename Coll::symbol_t key_t;
    typedef typename Coll::data_t data_t;
    typedef typename Coll::data_iter_t data_it_t;
    typedef std::pair<const key_t, data_t> value_type;
    typedef typename type_cast<value_type, Const>::type retval_type;

    // iterator essential data
    typename type_cast<mask_t, Const>::type *msk_ptr;
    data_it_t val_ptr;
    mask_t bit;
    key_t key;
    value_type data;

    enum { MaxMask = Coll::MaxMask };

public:
    // default constructor makes end iterator
    iterator_base() : msk_ptr(0) {}

    // constructor from collection makes pointer to 1st element, if any
    iterator_base(typename type_cast<Coll, Const>::type& a)
            : msk_ptr(&a.mask()) {
        if (*msk_ptr == 0) {
            msk_ptr = 0; return;
        }
        key = '0';
        for (bit=1; bit<MaxMask; ++key, bit<<=1)
            if ((bit & *msk_ptr) != 0)
                break;
        val_ptr = a.data();
        new (&data) value_type(key, *val_ptr);
    }

    iterator_base& operator++() {
        for (bit<<=1, ++key; bit<MaxMask; ++key, bit<<=1)
            if ((bit & *msk_ptr) != 0)
                break;
        if (bit == MaxMask)
            msk_ptr = 0;
        else
            new (&data) value_type(key, *(++val_ptr));
        return *this;
    }

    bool operator==(const iterator_base& rhs) const {
        if (msk_ptr == 0)
            return rhs.msk_ptr == 0;
        if (rhs.msk_ptr == 0)
            return false;
        return bit == rhs.bit;
    }

    bool operator!=(const iterator_base& rhs) const {
        if (msk_ptr == 0)
            return rhs.msk_ptr != 0;
        if (rhs.msk_ptr == 0)
            return true;
        return bit != rhs.bit;
    }

    retval_type& operator*() const { return data; }
    retval_type* operator->() const { return &data; }
};

} // namespace detail
} // namespace container
} // namespace utxx

#endif // _UTXX_CONTAINER_DETAIL_SCOLLITBASE_HPP_
