//----------------------------------------------------------------------------
/// \file  persist_blob.cpp
//----------------------------------------------------------------------------
/// \brief Persistent blob stored in memory mapped file
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-1-15
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
vsn 2.1 of the License, or (at your option) any later vsn.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/

#ifndef _UTIL_PERSIST_BLOB_HPP_
#define _UTIL_PERSIST_BLOB_HPP_

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string>
#include <string.h>
#include <boost/assert.hpp>
#include <util/atomic.hpp>
#include <util/meta.hpp>

namespace util {

/// Persistent blob of type T stored in memory mapped file.
template<typename T>
class persist_blob
{
private:
    struct blob_t {
        static const uint32_t s_version = 0xFEAB0001;
        long       vsn1;
        long       vsn2;
        uint32_t   version;
        T          data;
    } __attribute((aligned( (atomic::cacheline::size) )));
    
    BOOST_STATIC_ASSERT(sizeof(blob_t) % atomic::cacheline::size == 0);

    int             m_fd;
    blob_t*         m_blob;
    std::string     m_filename;
    unsigned long   m_read_contentions;
    unsigned long   m_write_contentions;
    
public:
    persist_blob() 
        : m_fd(-1), m_blob(NULL)
        , m_read_contentions(0), m_write_contentions(0) 
    {}

    ~persist_blob() { close(); }

    int  init(const char* a_file, const T* a_init_val = NULL,
        int a_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

    bool is_open() const { return m_blob; }

    void close();

    void reset();

    /// Flush buffered changes to disk
    int  sync()                         { return m_fd ? ::fsync(m_fd) : -1; }

    /// Name of the underlying memory mapped file
    const std::string& filename() const { return m_filename; }

    // Use the following get/set functions for concurrent access
    T    get();
    void set(const T& src);

    uint32_t begin_update() {
        return atomic::add(&m_blob->vsn1, 1)+1;
    }

    bool end_update(uint32_t a_vsn) {
        if (m_blob->vsn1 != a_vsn)
            return false;
        if (m_blob->vsn2 < a_vsn-1)
            m_blob->vsn2 = a_vsn;
        else if (m_blob->vsn2 >= a_vsn)
            return false;
        m_blob->vsn2 = a_vsn;
        atomic::memory_barrier();
        return true;
    }

    // Use the following get/set functions for non-concurrent "dirty" access
    T&   dirty_get()                { BOOST_ASSERT(m_blob); return m_blob->data; }
    void dirty_set(const T& src)    { BOOST_ASSERT(m_blob); m_blob->data = src;  }

    T*   operator& ()               { BOOST_ASSERT(m_blob); return &m_blob->data; }
    T*   operator->()               { return this->operator&(); }

    unsigned long read_contentions()    { return m_read_contentions;  }
    unsigned long write_contentions()   { return m_write_contentions; }
    void          vsn(uint32_t& v1, uint32_t& v2) { v1 = m_blob->vsn1; v2 = m_blob->vsn2; }

};

template<typename T>
int persist_blob<T>::init(const char* a_file, const T* a_init_val, int a_mode) {
    if (!a_file)
        return -1;

    close();

    if ((m_fd = ::open( a_file, O_CREAT|O_RDWR, a_mode ) ) < 0)
        return -1;

    struct stat buf;
    if (::fstat( m_fd, &buf) < 0)
        return -1;

    bool initialize = false;

    if (buf.st_size == 0) {
        if (::ftruncate(m_fd, sizeof(blob_t)) < 0) {
            close();
            return -1;
        }
        initialize = true;
    } else if (buf.st_size != sizeof(blob_t)) {
        // Something is wrong - blob size on disk must match the blob size.
        close();
        return -2;
    }

    m_blob = reinterpret_cast<blob_t*>( 
        ::mmap(0, sizeof(blob_t),
            PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0));

    if (m_blob == MAP_FAILED) {
        close();
        return -1;
    } else if (initialize) {
        if (a_init_val) {
            m_blob->vsn1 = 0;
            m_blob->vsn2 = 0;
            m_blob->data = *a_init_val;
        } else
            bzero(m_blob, sizeof(T));
        m_blob->version = blob_t::s_version;
    } else if (blob_t::s_version != m_blob->version) {
        // Wrong software version
        close();
        return -3;
    }

    return 0;
}

template<typename T>
void persist_blob<T>::close() {
    if (m_blob) {
        ::munmap( reinterpret_cast<char *>(m_blob), sizeof(T) );
        m_blob = NULL;
    }
    if( m_fd > -1 ) {
        ::close(m_fd);
        m_fd = -1;
    }
}

template<typename T>
T persist_blob<T>::get() {
    BOOST_ASSERT(m_blob);
    long v1, v2;
    T    data;
    
    int i = 0;
    do {
        if (i++ > 0)
            ++m_read_contentions;
            
        v2   = m_blob->vsn2;
        atomic::memory_barrier();
        data = m_blob->data;
        v1   = m_blob->vsn1;
        atomic::memory_barrier();
    } while (v1 != v2 || v2 != m_blob->vsn2);

    return data;
}

template<typename T>
void persist_blob<T>::set(const T& src) {
    BOOST_ASSERT(m_blob);
    int  i = 0;
    long v;
    do {
        if (i++ > 0)
            ++m_write_contentions;

        v = atomic::add(&m_blob->vsn1, 1) + 1;
        m_blob->data = src;
    } while (!atomic::cas(&m_blob->vsn2, v-1, v));
}

template<typename T>
void persist_blob<T>::reset() {
    BOOST_ASSERT(m_blob);
    bzero(m_blob->data, sizeof(T));
    m_blob->vsn1  = 0;
    m_blob->vsn2  = 0;
    m_read_contentions  = 0;
    m_write_contentions = 0;
    atomic::memory_barrier();
}

} // namespace util

#endif 
