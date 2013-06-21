// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief allocator with memory usage counter
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

#ifndef _UTXX_MEMSTAT_ALLOC_HPP_
#define _UTXX_MEMSTAT_ALLOC_HPP_


#include <memory>
#include <cstdlib>

namespace utxx {

template<typename T, typename C>
struct memstat_alloc : std::allocator<T> {
    typedef typename std::allocator<T>::pointer pointer;
    typedef typename std::allocator<T>::size_type size_type;

    template<typename U>
    struct rebind {
        typedef memstat_alloc<U, C> other;
    };

    memstat_alloc() {}

    template<typename U>
    memstat_alloc(memstat_alloc<U, C> const& u) : std::allocator<T>(u) {}

    pointer allocate(size_type size, std::allocator<void>::const_pointer = 0) {
        size *= sizeof(T);
        void *p = std::malloc(size);
        if (p == 0) throw std::bad_alloc();
        C::inc(size);
        return static_cast<pointer>(p);
    }

    void deallocate(pointer p, size_type size) {
        C::dec(size * sizeof(T));
        std::free(p);
    }
};

} // namespace utxx

#endif // _UTXX_MEMSTAT_ALLOC_HPP_
