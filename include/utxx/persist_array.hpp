//----------------------------------------------------------------------------
/// \file  basic_persist_array.hpp
//----------------------------------------------------------------------------
/// \brief Implementation of persistent array storage class.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Omnibius, LLC
// Author:  Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-25
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

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


#ifndef _UTXX_PERSISTENT_ARRAY_HPP_
#define _UTXX_PERSISTENT_ARRAY_HPP_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <boost/interprocess/sync/file_lock.hpp>
#pragma GCC diagnostic pop

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/static_assert.hpp>
#include <boost/scope_exit.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/atomic.hpp>
#include <utxx/error.hpp>
#include <stdexcept>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstddef>
#include <cstdlib>
#include <unistd.h>

namespace utxx {

    namespace { namespace bip = boost::interprocess; }
    namespace detail { struct empty_data {}; }

    template <
        typename T,
        size_t NLocks = 32,
        typename Lock = boost::mutex,
        typename ExtraHeaderData = detail::empty_data>
    struct persist_array {
        struct header {
            static const uint32_t s_version = 0xa0b1c2d3;
            uint32_t        version;
            volatile long   rec_count;
            size_t          max_recs;
            size_t          rec_size;
            Lock            locks[NLocks];
            ExtraHeaderData extra_header_data;
            T               records[0];
        };

        static const size_t s_locks = NLocks;
    protected:
        typedef persist_array<T, NLocks, Lock, ExtraHeaderData> self_type;

        BOOST_STATIC_ASSERT(!(NLocks & (NLocks - 1))); // must be power of 2

        static const size_t s_lock_mask = NLocks-1;
        // Shared memory file mapping
        bip::file_mapping  m_file;
        // Shared memory mapped region
        bip::mapped_region m_region;

        // Locks that guard access to internal record structures.
        header* m_header;
        T*      m_begin;
        T*      m_end;

        void check_range(size_t a_id) const throw (badarg_error) {
            size_t n = m_header->max_recs;
            if (likely(a_id < n))
                return;
            throw badarg_error("Invalid record id specified ", a_id, " (max=", n-1, ')');
        }

    public:
        typedef Lock                        lock_type;
        typedef typename Lock::scoped_lock  scoped_lock;
        typedef typename Lock::scoped_try_lock  scoped_try_lock;

        BOOST_STATIC_ASSERT((NLocks & (NLocks-1)) == 0); // Must be power of 2.

        persist_array()
            : m_header(NULL), m_begin(NULL), m_end(NULL)
        {}

        /// Default permission mask used for opening a file
        static int default_file_mode() { return S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP; }

        /// Initialize the storage
        /// @return true if the storage file didn't exist and was created
        bool init(const char* a_filename, size_t a_max_recs, bool a_read_only = false,
            int a_mode = default_file_mode()) throw (io_error);

        size_t count()    const { return static_cast<size_t>(m_header->rec_count); }
        size_t capacity() const { return m_header->max_recs; }

        /// Return internal storage header.
        /// Use only for debugging
        const header&           header_data() const { BOOST_ASSERT(m_header); return *m_header; }

        /// Return user-defined custom header data.
        const ExtraHeaderData&  extra_header_data() const { return m_header->extra_header_data; }
        ExtraHeaderData&        extra_header_data() { return m_header->extra_header_data; }

        /// Allocate next record and return its ID.
        /// @return
        size_t allocate_rec() throw(std::runtime_error) {
            size_t n = static_cast<size_t>(atomic::add(&m_header->rec_count,1));
            if (n >= capacity()) {
                atomic::dec(&m_header->rec_count);
                throw std::runtime_error("Out of storage capacity!");
            }
            return n;
        }

        std::pair<T*, size_t> get_next() {
            size_t n = allocate_rec();
            return std::make_pair(get(n), n);
        }

        Lock& get_lock(size_t a_rec_id) {
            return m_header->locks[a_rec_id & s_lock_mask];
        }

        /// Add a record with given ID to the store
        void add(size_t a_id, const T& a_rec) {
            BOOST_ASSERT(a_id < m_header->rec_count);
            scoped_lock guard(get_lock(a_id));
            *get(a_id) = a_rec;
        }

        /// Add a record to the storage and return it's id
        size_t add(const T& a_rec) {
            size_t n = allocate_rec();
            scoped_lock guard(get_lock(n));
            *(m_begin+n) = a_rec;
            return n;
        }

        /// @return id of the given object in the storage
        size_t id_of(const T* a_rec) const { return a_rec - m_begin; }

        const T& operator[] (size_t a_id) const throw(badarg_error) {
            check_range(a_id); return *get(a_id);
        }
        T& operator[] (size_t a_id) throw(badarg_error) {
            check_range(a_id); return *get(a_id);
        }

        const T* get(size_t a_rec_id) const {
            return likely(a_rec_id < m_header->max_recs) ? m_begin+a_rec_id : NULL;
        }

        T* get(size_t a_rec_id) {
            return likely(a_rec_id < m_header->max_recs) ? m_begin+a_rec_id : NULL;
        }

        /// Flush header to disk
        bool flush_header() { return m_region.flush(0, sizeof(header)); }

        /// Flush region of cached records to disk
        bool flush(size_t a_from_rec = 0, size_t a_num_recs = 0) {
            return m_region.flush(a_from_rec, a_num_recs*sizeof(T));
        }

        /// Remove memory mapped file from disk
        void remove() {
            m_file.remove(m_file.get_name());
        }

        const T*    begin() const { return m_begin; }
        const T*    end()   const { return m_end; }
        T*          begin()       { return m_begin; }
        T*          end()         { return m_end; }

        /// Call \a a_visitor for every record.
        /// @param a_visitor functor accepting two arguments:
        ///    (size_t rec_num, T* data)
        /// @param a_min_rec start from this record number
        /// @param a_count print up to this number of records (0 - all)
        /// @return number of records processed.
        template <class Visitor>
        size_t for_each(const Visitor& a_visitor, size_t a_min_rec = 0, size_t a_count = 0) const {
            const T* l_begin = begin() + a_min_rec;
            const T* p = l_begin;
            const T* l_end = begin() + count();
            const T* e = p + (a_count ? a_count : count());
            if (e > l_end) e = l_end;
            if (p >= e)
                return 0;
            for (int i = 0; p != e; ++p, ++i)
                a_visitor(a_min_rec + i, p);
            return e - l_begin;
        }

        std::ostream& dump(std::ostream& out, const std::string& a_prefix="") const {
            for (const T* p = begin(), *e = p + count(); p != e; ++p)
                out << a_prefix << *p << std::endl;
            return out;
        }
    };

    //-------------------------------------------------------------------------
    // Implementation
    //-------------------------------------------------------------------------

    template <typename T, size_t NLocks, typename Lock, typename Ext>
    bool persist_array<T,NLocks,Lock,Ext>::
    init(const char* a_filename, size_t a_max_recs, bool a_read_only, int a_mode)
        throw (io_error)
    {
        static const size_t s_pack_size = getpagesize();

        size_t sz = sizeof(header) + (a_max_recs * sizeof(T));
        sz += (s_pack_size - sz % s_pack_size);

        bool l_exists;
        try {
            boost::filesystem::path l_name(a_filename);
            try {
                boost::filesystem::create_directories(l_name.parent_path());
            } catch (boost::system::system_error& e) {
                throw io_error("Cannot create directory: ",
                    l_name.parent_path(), ": ", e.what());
            }

            {
                std::filebuf f;
                f.open(a_filename, std::ios_base::in | std::ios_base::out
                    | std::ios_base::binary);
                l_exists = f.is_open();

                if (l_exists && !a_read_only) {
                    bip::file_lock flock(a_filename);
                    bip::scoped_lock<bip::file_lock> g_lock(flock);

                    header h;
                    f.sgetn(reinterpret_cast<char*>(&h), sizeof(header));

                    if (h.version != header::s_version) {
                        throw io_error("Invalid file format ", a_filename);
                    }
                    if (h.rec_size != sizeof(T)) {
                        throw io_error("Invalid item size in file ", a_filename,
                            " (expected ", sizeof(T), " got ", h.rec_size, ')');
                    }
                    // Increase the file size if instructed to do so.
                    if (h.max_recs < a_max_recs) {
                        h.max_recs = a_max_recs;
                        f.pubseekoff(0, std::ios_base::beg);
                        f.sputn(reinterpret_cast<char*>(&h), sizeof(header));
                        f.pubseekoff(sz-1, std::ios_base::beg);
                        f.sputc(0);
                    }
                }
            }

            if (!l_exists && !a_read_only) {
                int l_fd = ::open(a_filename, O_RDWR | O_CREAT | O_TRUNC, a_mode);
                if (l_fd < 0)
                    throw io_error(errno, "Error creating file ", a_filename);

                BOOST_SCOPE_EXIT_TPL( (&l_fd) ) {
                    ::close(l_fd);
                } BOOST_SCOPE_EXIT_END;

                bip::file_lock flock(a_filename);
                bip::scoped_lock<bip::file_lock> g_lock(flock);

                header h;
                h.version   = header::s_version;
                h.rec_count = 0;
                h.max_recs  = a_max_recs;
                h.rec_size  = sizeof(T);

                if (::write(l_fd, &h, sizeof(h)) < 0)
                    throw io_error(errno, "Error writing to file ", a_filename);

                if (::ftruncate(l_fd, sz) < 0)
                    throw io_error(errno, "Error setting file ",
                        a_filename, " to size ", sz);

                ::fsync(l_fd);
            }

            bip::file_mapping shmf(a_filename, a_read_only ? bip::read_only : bip::read_write);
            bip::mapped_region region(shmf, a_read_only ? bip::read_only : bip::read_write);

            //Get the address of the mapped region
            void*  addr  = region.get_address();
            size_t size  = region.get_size();

            m_file.swap(shmf);
            m_region.swap(region);

            //if (!exists && !a_read_only)
            //    memset(static_cast<char*>(addr) + sizeof(header), 0, size - sizeof(header));
            m_header = static_cast<header*>(addr);
            m_begin  = m_header->records;
            m_end    = m_begin + a_max_recs;
            BOOST_ASSERT(reinterpret_cast<char*>(m_end) <= static_cast<char*>(addr)+size);

            //if (!l_exists && !a_read_only) {
            if (!a_read_only) {
                bip::file_lock flock(a_filename);
                bip::scoped_lock<bip::file_lock> g_lock(flock);

                // If the file is open for writing, initialize the locks
                // since previous program crash might have left locks in inconsistent state
                for (Lock* l = m_header->locks, *e = l + NLocks; l != e; ++l)
                    new (l) Lock();
            }

        } catch (io_error& e) {
            throw;
        } catch (std::exception& e) {
            throw io_error(e.what());
        }

        return !l_exists;
    }

} // namespace utxx

#endif // _UTXX_PERSISTENT_ARRAY_HPP_
