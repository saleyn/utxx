//----------------------------------------------------------------------------
/// \file  alloc_fixed_page.cpp
//----------------------------------------------------------------------------
/// \brief An allocator with aligned paged allocation of size(T) objects.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-15
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#pragma once

#include <new>
#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <utxx/atomic.hpp>
#include <stdlib.h>
#ifdef _ALLOCATOR_MEM_DEBUG
#include <stdio.h>
#endif

namespace utxx {
namespace memory {

/**
 * Implementation of paged allocator that allocates memory in
 * aligned pages of PageSize. A new page is allocated when
 * there is no more room to store an object of size T in a page.
 * This allocator is suited for lazily allocated sparse arrays
 * where the memory is consumed when there's a new slot to be
 * occupied whose content has a long lifetime.  A page is freed
 * when there are no more objects referencing the page. This
 * deallocation method is very weak for cases when some of the
 * objects in the page have short lifetime and some have long
 * lifetime.
 */
template <
      typename T
    , size_t   PageSize  = 64*1024
>
class aligned_page_allocator : public boost::noncopyable {
    struct header {
        static const uint32_t s_magic = 1234567890;
        const uint32_t magic;       ///< Magic version that must match s_magic.
        T*             avail_chunk; ///< Next available chunk on this page
        long           alloc_count; ///< Number of allocated chunks on this page
        header() : magic(s_magic) {}
    };

    typedef std::allocator<T> base;
    static const size_t s_page_mask    = PageSize-1;
    static const size_t s_begin_offset = sizeof(header);
    static const size_t s_end_chunks   = PageSize / sizeof(T);
    static const int    s_max_chunks   = ((PageSize-s_begin_offset) / sizeof(T));

    // Page size must be a power of 2
    BOOST_STATIC_ASSERT((PageSize & (PageSize-1)) == 0);
    BOOST_STATIC_ASSERT(s_begin_offset < PageSize);
    BOOST_STATIC_ASSERT(s_max_chunks > 0);

    header* m_page;

    static header* page_alloc() {
        union {
            unsigned long n;
            void*   pp;
            char*   pc;
            header* p;
        } u;
        #if defined(_WIN32) || defined (_WIN64)
        u.pp = _aligned_malloc(PageSize, PageSize);
        if (!u.pp)
            throw std::bad_alloc();
        #else
        if (posix_memalign(&u.pp, PageSize, PageSize) < 0)
            throw std::bad_alloc();
        #endif
        BOOST_ASSERT((u.n & s_page_mask) == 0);
        new (u.p) header();
        u.p->avail_chunk = reinterpret_cast<T*>(u.pc + s_begin_offset);
        u.p->alloc_count = 0;
        #ifdef _ALLOCATOR_MEM_DEBUG
        printf("Allocated page %p\n", u.p);
        #endif
        return u.p;
    }

    static void page_free(header* p) {
        #ifdef _ALLOCATOR_MEM_DEBUG
        printf("Freeing page %p\n", p);
        #endif

        #if defined(_WIN32) || defined (_WIN64)
        _aligned_free(p);
        #else
        free(p);
        #endif
    }

public:
    typedef T*          pointer;
    typedef T&          reference;
    typedef ptrdiff_t   difference_type;
    typedef const T&    const_reference;
    typedef size_t      size_type;
    typedef T           value_type;

    aligned_page_allocator() : m_page(page_alloc()) {
        #ifdef _ALLOCATOR_MEM_DEBUG
        printf("Page size: %d\n", PageSize);
        #endif

        if (!m_page)
            throw std::bad_alloc();
    }

    ~aligned_page_allocator() {
        if (m_page && (m_page->alloc_count == 0)) {
            page_free(m_page);
            m_page = NULL;
        }
    }

    template <typename U>
    struct rebind {
        typedef aligned_page_allocator<U, PageSize> other;
    };

    pointer allocate(size_type n, const void *hint = 0) {
        if (m_page->avail_chunk >= reinterpret_cast<pointer>(
                reinterpret_cast<char*>(m_page) + PageSize))
            m_page = page_alloc();

        pointer p = m_page->avail_chunk++;
        atomic::inc(&m_page->alloc_count);
        #ifdef _ALLOCATOR_MEM_DEBUG
        printf("  Allocated: %p\n", p);
        #endif
        return p;
    }

    void deallocate(pointer p, size_type n){
        unsigned long addr = reinterpret_cast<unsigned long>(p) & ~s_page_mask;
        header* h = reinterpret_cast<header*>(addr);
        #ifdef _ALLOCATOR_MEM_DEBUG
        printf("  Deallocating %p, page=%p\n", p, h);
        #endif
        BOOST_ASSERT(h->magic == header::s_magic);
        if (atomic::add(&h->alloc_count, -1) == 0 && h != m_page)
            page_free(h);
    }

     void construct(pointer p, const T &val){
         new(p) T(val);
     }

     void destroy(pointer p){
         p->~T();
     }

     const header* address() const { return m_page; }
};

} // namespace memory
} // namespace utxx
