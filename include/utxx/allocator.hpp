//----------------------------------------------------------------------------
/// \file  variant_tree.hpp
//----------------------------------------------------------------------------
/// \brief Concurrent shared memory allocator.
/// \author: Serge Aleynikov
/// This module implements a concurrent shared memory allocator
/// that can be used for efficient allocation of variable-sized
/// blocks of memory located in a memory mappend file.
///   Since use of memory mapped files involves reservation
/// of a memory region of a given size, that size determines the
/// maximum memory served by the allocator.  The allocator is
/// optimized for speed at the trade off of memory usage. The
/// size of memory requests from the application are rounded
/// up to the closest size-class being a power of 2.
/// Consequently in the worst case it would consume twice the
/// required memory.
///   The allocator should perform well in cases when frequent
/// allocations of short-lived memory are requested mainly of
/// the same type of size-classes.  It is not
/// moving or disturbing memory blocks that are currently
/// used by the application.
///   The <shmem_allocator> class is fully STL-compatible and
/// can be used with standard STL containers. However, those
/// containers would have to guarantee their own thread safety.
///
/// FIXME: in current implementation the allocator is not cache 
/// friendly as it doesn't have processor-specific heaps and 
/// manages all memory in a single heap provided at construction.
//----------------------------------------------------------------------------
// Created: 2010-07-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _SHMEM_ALLOCATOR_HPP_
#define _SHMEM_ALLOCATOR_HPP_

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>
#include <utxx/meta.hpp>
#include <utxx/math.hpp>
#include <utxx/atomic.hpp>

#ifdef ALLOC_TRACE
#  include <iomanip>
#  define TRACEIT(X) { std::stringstream _s; _s << X; std::cerr << _s.str() << std::endl; }
#else
#  define TRACEIT(X) (void*)0
#endif

namespace utxx {
namespace memory {

class shmem_manager {
public:
    enum init_mode {
          TRUNCATE_SHARED_MEMORY
        , ATTACH_SHARED_MEMORY
    };

    /// Initialize shared memory allocator
    /// @param <filename> is the filename of memory-mapped file
    /// @param <sz> is the total memory managed by the allocator
    ///            (it can be 0 if <mode> is not TRUNCATE_SHARED_MEMORY).
    /// @param <mode> is the memory opening mode. If it is
    ///            TRUNCATE_SHARED_MEMORY then the memory is resized to
    ///            <total_mem_size>.
    /// @param <remove_on_destruct> if true then the memory mapped file
    ///            will be deleted on destruction of this instance.
    shmem_manager(const char* filename, size_t sz = 0,
                  init_mode mode = ATTACH_SHARED_MEMORY,
                  bool remove_on_destruct = false)
        throw(std::runtime_error);

    ~shmem_manager();

    /// Reserve <sz> bytes of allocated shared memory region.
    /// @return pointer to reserved memory.  NULL if <sz> is
    ///         larger than remaining available capacity.
    void* reserve(size_t sz);

    /// Total available shared memory
    size_t             available() const { return m_size - m_offset; }
    size_t             size()      const { return m_size; }
    char*              address()   const { return m_address; }
    const std::string& filename()  const { return m_filename; }

    /// True if the shared memory was truncated on construction.
    bool               truncated() const { return m_truncated; }

private:
    size_t          m_size;
    int             m_fd;
    std::string     m_filename;
    init_mode       m_mode;
    char*           m_address;
    bool            m_remove_file;
    size_t          m_offset;
    bool            m_truncated;
};

/**
 * Simple concurrent allocator that manages memory
 * by allocating blocks of power of 2 size-classes.
 * Main memory pool is provided in the constructor.
 * The segment is consulted only when there are no blocks
 * available in the free list for the nearest power of 2
 * size block of the requested allocation.  Otherwise the block
 * of requested size is fetched from the free list.
 * The allocator is safe for concurrent use.
 */
template <int MinSize = 8, int MaxPow2Size = 32>
class pow2_allocator {
public:
    static const unsigned int max_bucket = MaxPow2Size-1;

    typedef struct {
        int   next_free;
        pid_t pid       : 16;  // Pid of the process that allocated this node
        char  size_class: 8;
        bool  allocated : 8;
    } node;

protected:
    // Max object size that allocator can allocate
    static const long max_size = 1 << max_bucket;
    // Min object size that allocator allocates
    static const long min_size = MinSize;
    // Magic version number written in the beginning of base address
    enum { MAGIC = 0xFFDE1234 };

    // Stack used by each size-class specific free list
    class stack {
        int   m_head;
        char* m_base_addr;
        #ifdef ALLOC_STATS
        int   m_push_count;
        #endif
    public:
        stack() : m_head(0) {}
        stack(void* addr) : m_base_addr((char*)addr), m_head(0) {}

        void push(node* nd) {
            BOOST_ASSERT((char*)nd >= m_base_addr);
                // "Wrong address range (base: " << m_base_addr << ", addr: " << nd << ')');
            nd->allocated = false;
            long curr;
            do {
                curr = m_head;
                nd->next_free = curr;
            } while (!atomic::cas(&m_head, curr, (char*)nd-m_base_addr));
            #ifdef ALLOC_STATS
            atomic_add(m_push_count, 1);
            #endif
        }

        node* pop() {
            volatile int curr;
            node* nd;
            do {
                curr = m_head;
                if (curr == 0)
                    return NULL;
                nd = reinterpret_cast<node*>(m_base_addr + curr);
            } while (!atomic::cas(&m_head, curr, nd->next_free));

            nd->allocated = true;
            nd->next_free = 0;
            return nd;
        }

        /// Returns number of items in the stack
        int length() const {
            int len = 0;
            int tmp = m_head;
            while (tmp != 0) {
                len++;
                tmp = reinterpret_cast<node*>(m_base_addr + tmp)->next_free;
            }
            return len;
        }

        #ifdef ALLOC_STATS
        long push_count() const { return m_push_count; }
        #endif
    };

    struct header {
        const unsigned int magic;
        stack  freelist[MaxPow2Size];
        size_t total_size;
        size_t offset;

        header() : magic(pow2_allocator::MAGIC), total_size(0), offset(0) {}
        header(size_t sz)
            : magic(pow2_allocator::MAGIC)
            , total_size(sz-sizeof(header))
            , offset(sizeof(header))
        {
            for (int i=0; i < MaxPow2Size; ++i)
                new (&freelist[i]) stack(this);
        }
    };

    pow2_allocator() {}

private:
    header*         m_header;
    pid_t           m_pid_id;    // PID of the process that created this allocator
    #ifdef ALLOC_STATS
    int             m_mem_hits;
    int             m_pool_hits;
    #endif

    /// Allocate a chunk from the main memory pool given to allocator.
    /// @param <sz> size of a chunk to allocate
    /// @param <size_class> size-class for this chunk
    void* allocate_main_memory(size_t sz, size_t size_class);

public:
    /// Initialize shared memory allocator
    /// @param <total_mem_size> is the total memory managed by the allocator
    pow2_allocator(void* base_addr, size_t total_mem_size, bool initialize=false)
        throw(badarg_error);

    pow2_allocator( const pow2_allocator& a )
        : m_header(a.m_header)
        , m_pid_id(a.m_pid_id)
        #ifdef ALLOC_STATS
        , m_mem_hits(a.m_mem_hits)
        , m_pool_hits(a.m_pool_hits)
        #endif
    {}

    void* allocate(size_t sz);
    void  release(void* p);

    int freelist_size(int bucket) {
        return (bucket < MaxPow2Size) ? m_header->freelist[bucket].length() : -1;
    }

    /// @return size of memory currently used or pooled
    size_t used_memory()        const { return m_header->offset - sizeof(header); }
    /// @return total memory managed by this allocator
    size_t total_memory()       const { return m_header->total_size; }

    /// Determine size of memory pointed to by <p>.
    static size_t size_of(void* p)    { return 1 << ptr_to_node(p)->size_class; }

    /// Get node metadata for a given pointer
    static node* ptr_to_node(void* p) { return reinterpret_cast<node*>(p) - 1; }

    /// @return shared memory base address.
    const void* base_address()  const { return m_header; }
    /// @return memory footprint of the allocator's internal header.
    static const size_t header_size() { return sizeof(header); }

    /// Beginning of addressable range managed by this allocator
    const char* begin() const { return static_cast<const char*>(m_header) + sizeof(header); }
    char*       begin()       { return static_cast<char*>(m_header)       + sizeof(header); }
    /// End of addressable range managed by this allocator
    const char* end()   const { return begin() + m_header->total_size; }

    /// Reclaim all allocated memory blocks owned by process <pid> by
    /// marking them available and moving to the free list.  This method
    /// can be used when implemening an inter-process memory manager, which
    /// detects a death of a process which previously attached to the
    /// shared memory using this allocator.
    void reclaim_resources(pid_t pid);

    /// @return size of space overhead for each allocated chunk.
    static size_t chunk_header_size() { return sizeof(node); }

    #ifdef ALLOC_STATS
    int mem_hits()  const { return m_mem_hits; }
    int pool_hits() const { return m_pool_hits; }
    #endif
};

struct shmem_allocator_policy {
    enum {
          MIN_OBJ_SIZE      = 8
        , MAX_LOG2_OBJ_SIZE = 32
    };
};

/// STL-compartible shared-memory concurrent allocator.
template <typename T, typename Policy = shmem_allocator_policy>
class shmem_allocator
    : public pow2_allocator<
          Policy::MIN_OBJ_SIZE
        , Policy::MAX_LOG2_OBJ_SIZE
      >
{
private:
    typedef pow2_allocator<
          Policy::MIN_OBJ_SIZE
        , Policy::MAX_LOG2_OBJ_SIZE
    > BaseT;
public:
    typedef ::std::size_t    size_type;
    typedef ::std::ptrdiff_t difference_type;
    typedef T*               pointer;
    typedef const T*         const_pointer;
    typedef T&               reference;
    typedef const T&         const_reference;
    typedef T                value_type;

    shmem_allocator() {}

    shmem_allocator(void* base_addr, size_t total_mem_size, bool initialize=false)
        : BaseT(base_addr, total_mem_size, initialize) {}

    /// Type converting allocator constructor
    template <typename U>
    shmem_allocator(const shmem_allocator<U>& a) throw() {}
//        : BaseT( *reinterpret_cast<BaseT*>(&a) )
//    {}

    /// Convert an allocator<Type> to an allocator <Type1>.
    template <typename U>
    struct rebind {
        typedef shmem_allocator<U, Policy> other;
    };

    /// Return address of reference to mutable element.
    pointer address( reference elem ) const { return &elem; }

    /// Return address of reference to const element.
    const_pointer address( const_reference elem ) const { return &elem; }

    /** Allocate an array of count elements.
     * @param <count> # of elements in array.
     * @param <hint> Place where caller thinks allocation should occur.
     * @return Pointer to block of memory.
     */
    pointer allocate(size_type count, const void* hint = 0) {
        (void)hint;  // Ignore the hint.
        void* p = BaseT::allocate(count * sizeof(T));
        return reinterpret_cast< pointer >( p );
    }

    /// Ask allocator to release memory at pointer with size bytes.
    void deallocate(pointer p, size_type size = 0) {
        struct BaseT::node* nd = BaseT::ptr_to_node(p);
        BOOST_ASSERT(size == 0 || size < (1 << nd->size_class));
            // "Invalid size of item (size=" << size << ", expected=" <<
            // nd->size_class << ")");
        BaseT::release(p /* , size*sizeof(T) */);
    }

    /// Calculate max # of elements allocator can handle.
    size_type max_size(void) const throw() {
        const size_type max_bytes = BaseT::total_memory() - BaseT::used_memory();
        const size_type items     = max_bytes / (sizeof(T) + sizeof(typename BaseT::node));
        return items;
    }

    /// Construct an element at the pointer.
    void construct(pointer p, const T& value) {
        // A call to global placement new forces a call to copy constructor.
        ::new(p) T(value);
    }

    /// Destruct the object at pointer.
    void destroy(pointer p) {
        // If the T has no destructor, then use the void cast to suppress
        // a warning from some compilers complain about unreferenced parameter
        (void)p;
        p->~T();
    }
};

//-----------------------------------------------------------------------------

template <typename T, typename Policy>
inline bool operator== (const shmem_allocator<T,Policy>& a, const shmem_allocator<T,Policy>& b)
{
    return a.base_address() == b.base_address()
        && a.total_memory() == b.total_memory();
}

template <typename T, typename Policy>
inline bool operator!= (const shmem_allocator<T,Policy>& a, const shmem_allocator<T,Policy>& b)
{
    return a.base_address() != b.base_address()
        || a.total_memory() != b.total_memory();
}

//-----------------------------------------------------------------------------
// IMPLEMENTATION
//-----------------------------------------------------------------------------
template <int MinSize, int MaxPow2Size>
pow2_allocator<MinSize, MaxPow2Size>
::pow2_allocator(void* base_addr, size_t total_mem_size, bool initialize)
    throw(badarg_error)
    : m_header(static_cast<header*>(base_addr))
    , m_pid_id(::getpid())
    #ifdef ALLOC_STATS
    , m_mem_hits(0);
    , m_pool_hits(0)
    #endif
{
    if (!base_addr)
        throw badarg_error("NULL base address provided!");

    if (total_mem_size <= sizeof(header))
        throw badarg_error("Requested memory is too small");

    if (initialize)
        new (m_header) header(total_mem_size);

    TRACEIT("pow2_allocator(" << this <<
            ") - Allocator constructed (" << m_header <<
            ", initialize=" << initialize << ')');
}

template <int MinSize, int MaxPow2Size>
void* pow2_allocator<MinSize, MaxPow2Size>
::allocate_main_memory(size_t sz, size_t size_class)
{
    size_t curr;
    size_t new_offset;

    do {
        curr = m_header->offset;
        new_offset = curr+sz;
        if (new_offset >= m_header->total_size) {
            TRACEIT("No room to allocate " << sz << '(' << size_class
                    << ") bytes (curr=" << curr << ')');
            return NULL;
        }
    } while(!atomic::cas(&m_header->offset, curr, new_offset));

    node* p       = reinterpret_cast<node*>((char*)m_header + curr);
    p->pid        = m_pid_id;
    p->size_class = size_class;
    p->allocated  = true;
    p->next_free  = 0;
    TRACEIT("Allocated " << std::setw(9) << sz << '[' << size_class
            << "] bytes (offset=" << curr << ", addr=" << (p+1) << ") - not pooled.");
    #ifdef ALLOC_STATS
    atomic_add(m_mem_hits, 1);
    #endif
    return static_cast<void*>(++p);
}

template <int MinSize, int MaxPow2Size>
void* pow2_allocator<MinSize, MaxPow2Size>
::allocate(size_t sz)
{
    size_t alloc_sz = sz + sizeof(node);
    char size_class = (alloc_sz < min_size)
              ? log<min_size, 2>::value
              : math::upper_log2(alloc_sz);
    size_t size = 1 << size_class;

    if (size_class > max_bucket)
        return NULL;

    node* nd = m_header->freelist[size_class].pop();
    if (nd == NULL)
        return allocate_main_memory(size, size_class);

    nd->pid = m_pid_id;

    #ifdef ALLOC_STATS
    atomic_add(m_pool_hits, 1);
    #endif

    TRACEIT("Allocated<" << m_pid_id << '>' << std::setw(9) 
            << size << '/' << sz << " bytes (offset="
            << reinterpret_cast<int>(nd)-reinterpret_cast<int>(m_header)
            << ", addr=" << (nd+1) << ") - from pool[" << (int)size_class << ']');
    return ++nd;
}

template <int MinSize, int MaxPow2Size>
void pow2_allocator<MinSize, MaxPow2Size>
::release(void* p)
{
    if (!p) return;

    node* nd = ptr_to_node(p);
    char size_class = nd->size_class;

    BOOST_ASSERT(size_class < MaxPow2Size);
        // "Memory size (1 << " << size_class << ") is greater then "
        // "(1 << " << MaxPow2Size << ')');

    TRACEIT("Released<" << m_pid_id << "> " << std::setw(9)
            << (1 << size_class) << " bytes to pool[" << (int)size_class
            << "] (offset=" << reinterpret_cast<int>(nd)-reinterpret_cast<int>(m_header)
            << ", addr=" << (nd+1) << ", old_pid=" << nd->pid << ')');
    m_header->freelist[size_class].push(nd);
}

template <int MinSize, int MaxPow2Size>
void pow2_allocator<MinSize, MaxPow2Size>
::reclaim_resources(pid_t pid) {
    // cannot reclaim own resources - we are still alive and may
    // have active references to allocated blocks
    if (pid == m_pid_id)
        return;

    char* p   = m_header->begin();
    char* end = m_header->end();

    while (p < end) {
        node* nd = static_cast<node*>(p);
        if (nd->pid == pid && nd->allocated)
            release(p+sizeof(node));
        p += (1 << nd->size_class);
    }
}

} // namespace memory
} // namespace utxx 

#endif
