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

This file is part of the util open-source project.

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

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/scope_exit.hpp>
#include <boost/assert.hpp>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
//#include <sys/mman.h>
#include <string>
#include <string.h>
#include <util/atomic.hpp>
#include <util/meta.hpp>
#include <util/path.hpp>
#include <util/error.hpp>
#include <util/robust_mutex.hpp>

namespace util {

    namespace {
        namespace bip = boost::interprocess;
    }

    namespace detail {

        struct robust_lock : public robust_mutex {
            struct lock_data {
                pthread_mutex_t mutex;
            };
            explicit robust_lock(bool a_destroy_on_exit = false)
                : robust_mutex(a_destroy_on_exit) {}
            void init(lock_data& a_data) { robust_mutex::init(a_data.mutex); }
            void set(lock_data& a_data)  { robust_mutex::set(a_data.mutex);  }
        };

        struct null_lock {
            typedef robust_mutex::make_consistent_functor make_consistent_functor;
            typedef void* native_handle_type;
            typedef boost::unique_lock<null_lock> scoped_lock;
            typedef boost::detail::try_lock_wrapper<null_lock> scoped_try_lock;

            struct lock_data {};

            explicit null_lock(bool a_destroy_on_exit = false) {}
            void init(lock_data&) {}
            void set(lock_data&) {}
            void lock() {}
            void unlock() {}
            bool try_lock() { return true; }
            int  make_consistent() { return 0; }
            void destroy() {}
            native_handle_type native_handle() { return NULL; }
            robust_mutex::make_consistent_functor on_make_consistent;
        };
    }

/// Persistent blob of type T stored in memory mapped file.
template<typename T, typename Lock = detail::robust_lock>
class persist_blob
{
private:
    struct blob_t {
        static const uint32_t s_version = 0xFEAB0001;
        typename Lock::lock_data lock_data;
        uint32_t version;
        T        data;
    } __attribute((aligned( (atomic::cacheline::size) )));

    BOOST_STATIC_ASSERT(sizeof(blob_t) % atomic::cacheline::size == 0);

    blob_t*             m_blob;
    bip::file_mapping   m_file;
    bip::mapped_region  m_region;

    std::string         m_filename;
    Lock                m_lock;

public:
    typedef T                               value_type;
    typedef typename Lock::scoped_lock      scoped_lock;
    typedef typename Lock::scoped_try_lock  scoped_try_lock;

    persist_blob()
        : m_blob(NULL)
        , m_lock(false)
    {}

    ~persist_blob() { close(); }

    /// Default permission mask used for opening a file
    static int default_file_mode() { return S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP; }

    /// @return true if file didn't exist and was created.
    bool init(const char* a_file, const T* a_init_val = NULL,
        bool a_read_only = true, int a_mode = default_file_mode())
        throw (io_error);

    bool is_open() const { return m_blob; }

    void close();

    void reset();

    int  flush()  { return m_blob && m_region.flush() ? 0 : -1; }

    /// Name of the underlying memory mapped file
    const std::string& filename() const { return m_filename; }

    // Use the following get/set functions for concurrent access
    T    get();
    void set(const T& src);

    Lock& get_lock() { return m_lock; }

    // Use the following get/set functions for non-concurrent "dirty" access
    T&       dirty_get()                { BOOST_ASSERT(m_blob); return m_blob->data; }
    const T& dirty_get() const          { BOOST_ASSERT(m_blob); return m_blob->data; }
    void     dirty_set(const T& src)    { BOOST_ASSERT(m_blob); m_blob->data = src;  }

    T*       operator->()               { return dirty_get(); }
    const T* operator->()       const   { return dirty_get(); }

};

template<typename T, typename L>
bool persist_blob<T,L>::init(const char* a_file, const T* a_init_val,
    bool a_read_only, int a_mode) throw (io_error)
{
    BOOST_ASSERT(a_file);

    close();

    bool l_created     = !path::file_exists(a_file);
    bool l_initialized = false;
    {
        int  l_fd;

        if ((l_fd = ::open( a_file, a_read_only ? O_RDONLY : O_CREAT|O_RDWR, a_mode ) ) < 0)
            throw io_error(errno, "Cannot open file ", a_file, " for ",
                a_read_only ? "reading" : "writing");

        BOOST_SCOPE_EXIT_TPL( (&l_fd) ) {
            ::close(l_fd);
        } BOOST_SCOPE_EXIT_END;

        struct stat buf;
        if (::fstat(l_fd, &buf) < 0)
            throw io_error(errno, "Cannot check file size of ", a_file);

        if (buf.st_size == 0) {
            if (::ftruncate(l_fd, sizeof(blob_t)) < 0) {
                int ec = errno;
                close();
                throw io_error(ec, "Cannot set size of file ",
                    a_file, " to ", sizeof(blob_t));
            }
            l_initialized = true;
        } else if (buf.st_size != sizeof(blob_t)) {
            // Something is wrong - blob size on disk must match the blob size.
            close();
            throw io_error("Size of file ", a_file, " has wrong size - likely old version."
                " Delete it and try again!");
        }

    }

    bip::file_mapping  l_shmf(a_file,   a_read_only ? bip::read_only : bip::read_write);
    bip::mapped_region l_region(l_shmf, a_read_only ? bip::read_only : bip::read_write);

    m_blob = reinterpret_cast<blob_t*>(l_region.get_address());

    m_file.swap(l_shmf);
    m_region.swap(l_region);

    /*
    m_blob = reinterpret_cast<blob_t*>(
        ::mmap(0, sizeof(blob_t),
            PROT_READ|PROT_WRITE, MAP_SHARED, m_fd, 0));

    if (m_blob == MAP_FAILED) {
        int ec = errno;
        close();
        throw io_error(ec, "Error mapping file ", a_file, " to memory");
    }
    */
    if (l_initialized) {
        if (a_init_val) {
            m_blob->data = *a_init_val;
        } else
            bzero(m_blob, sizeof(T));
        m_blob->version = blob_t::s_version;
        m_lock.init(m_blob->lock_data);
    } else if (blob_t::s_version != m_blob->version) {
        // Wrong software version
        close();
        throw io_error("Wrong version of data in the file ", a_file,
            " (expected: ", blob_t::s_version, ", got: ", m_blob->version, ')');
    } else {
        m_lock.set(m_blob->lock_data);
    }

    return l_created;
}

template<typename T, typename L>
void persist_blob<T,L>::close() {
    if (m_blob) {
        //::munmap( reinterpret_cast<char *>(m_blob), sizeof(T) );
        m_blob = NULL;
        //m_lock.destroy();
        bip::file_mapping  l_fm;
        bip::mapped_region l_reg;
        m_file.swap(l_fm);
        m_region.swap(l_reg);
    }
}

template<typename T, typename L>
T persist_blob<T,L>::get() {
    BOOST_ASSERT(m_blob);

    scoped_lock g(m_lock);
    return m_blob->data;
}

template<typename T, typename L>
void persist_blob<T,L>::set(const T& src) {
    BOOST_ASSERT(m_blob);
    scoped_lock g(m_lock);
    m_blob->data = src;
}

template<typename T, typename L>
void persist_blob<T,L>::reset() {
    BOOST_ASSERT(m_blob);
    scoped_lock g(m_lock);
    bzero(m_blob->data, sizeof(T));
}

} // namespace util

#endif
