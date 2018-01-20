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


#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <boost/interprocess/sync/file_lock.hpp>
#pragma GCC diagnostic pop

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/filesystem.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/meta.hpp>
#include <utxx/scope_exit.hpp>
#include <utxx/error.hpp>
#include <stdexcept>
#include <fstream>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <cstring>
#include <cstddef>
#include <cstdlib>
#include <atomic>
#include <unistd.h>

namespace utxx {

    namespace        { namespace bip = boost::interprocess; }
    namespace detail { struct empty_data {}; }

    enum class persist_attach_type {
        OPEN_READ_ONLY,     // Open-only
        OPEN_READ_WRITE,    // Open-only
        CREATE_READ_ONLY,   // Create-only
        CREATE_READ_WRITE,  // Create-only
        RECREATE,           // Recreate and open
        READ_WRITE          // Create or open
    };

    template <
        typename    T,
        std::size_t NLocks          = 32,
        typename    Lock            = std::mutex,
        typename    ExtraHeaderData = detail::empty_data>
    struct persist_array {
        struct header : public ExtraHeaderData {
            static const uint32_t s_version = 0xa0b1c2d3;
            uint32_t            version;
            std::atomic<long>   rec_count;
            size_t              max_recs;
            size_t              rec_size;
            size_t              recs_offset;
            Lock                locks[NLocks];
            T                   records[0];
        };

        static const size_t s_locks = NLocks;
    protected:
        typedef persist_array<T, NLocks, Lock, ExtraHeaderData> self_type;

        static_assert((NLocks & (NLocks-1)) == 0, "Must be power of 2");

        static const size_t s_lock_mask = NLocks-1;
        // Shared memory file mapping
        bip::file_mapping   m_file;
        // Shared memory mapped region
        bip::mapped_region  m_region;
        // Name of the memory-mapped file or shm segment
        std::string         m_storage_name;

        // Locks that guard access to internal record structures.
        header* m_header;
        T*      m_begin;
        T*      m_end;

        void check_range(size_t a_id) const {
            size_t n = m_header->max_recs;
            if (likely(a_id < n))
                return;
            throw badarg_error("Invalid record id specified ", a_id, " (max=", n-1, ')');
        }

    public:
        using lock_type   = Lock;
        using scoped_lock = std::lock_guard<Lock>;

        persist_array()
            : m_header(NULL), m_begin(NULL), m_end(NULL)
        {}

#if __cplusplus >= 201103L
        persist_array(persist_array&& a_rhs) {
            *this = std::move(a_rhs);
        }

        void operator=(persist_array&& a_rhs) {
            m_storage_name = std::move(a_rhs.m_storage_name);
            m_header       = a_rhs.m_header;
            m_begin        = a_rhs.m_begin;
            m_end          = a_rhs.m_end;
            m_file  .swap(a_rhs.m_file);
            m_region.swap(a_rhs.m_region);
            a_rhs.m_header = nullptr;
            a_rhs.m_begin  = nullptr;
            a_rhs.m_end    = nullptr;
        }
#endif
        /// Default permission mask used for opening a file
        static int default_file_mode() { return S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP; }

        /// Get total memory size needed to allocate \a a_max_recs
        static size_t total_size(size_t a_max_recs) {
            static const std::size_t s_pack_size = getpagesize();

            auto sz = sizeof(header) + (a_max_recs * sizeof(T));
            sz += (s_pack_size - sz % s_pack_size);
            return sz;
        }

        /// Initialize the storage
        /// @return true if the storage file didn't exist and was created
        bool init(const char* a_filename, size_t a_max_recs, bool a_read_only = false,
            int a_mode = default_file_mode(), void const* a_map_address = nullptr,
            int a_map_options = 0);

        /// Initialize the storage in shared memory.
        /// @param a_segment  the shared memory segment
        /// @param a_name     the name of object in managed shared memory
        /// @param a_flag     if RECREATE  - the existing object will be recreated;
        ///                   if READ_ONLY - will try to attach in read-only mode
        /// @param a_max_recs max capacity of the storage in the number of records
        /// @return true if the shared memory object didn't exist and was created
        bool init(bip::fixed_managed_shared_memory& a_segment,
                  const char* a_name, persist_attach_type a_flag, size_t a_max_recs);

        size_t count()    const { return m_header->rec_count.load(std::memory_order_relaxed); }
        size_t capacity() const { return m_header->max_recs; }

        /// Return internal storage header.
        /// Use only for debugging
        const header& header_data() const { assert(m_header); return *m_header; }

        /// Allocate next record and return its ID.
        /// @return
        size_t allocate_rec() {
            auto error = [this]() {
                throw utxx::runtime_error
                    ("persist_array: Out of storage capacity (", this->m_storage_name, ")!");
            };

            if (unlikely(count() >= capacity()))
                error();
            size_t n = size_t(m_header->rec_count.fetch_add(1, std::memory_order_relaxed));
            if (unlikely(n >= capacity())) {
                m_header->rec_count.store(capacity(), std::memory_order_relaxed);
                error();
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
            BOOST_ASSERT(a_id < m_header->rec_count.load(std::memory_order_relaxed));
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

        /// Add a record to the storage, initialize it using given lambda.
        /// @return ID of the new record
        template <typename InitFun>
        std::pair<T*, size_t> add(const InitFun& a_rec_init) {
            size_t n = allocate_rec();
            scoped_lock guard(get_lock(n));
            T* rec = m_begin+n;
            a_rec_init(n, rec);
            return std::make_pair(rec, n);
        }

        /// @return id of the given object in the storage
        size_t id_of(const T* a_rec) const { return a_rec - m_begin; }

        const T& operator[] (size_t a_id) const {
            check_range(a_id); return *get(a_id);
        }
        T& operator[] (size_t a_id) {
            check_range(a_id); return *get(a_id);
        }

        const T* get(size_t a_rec_id) const {
            return likely(a_rec_id < capacity()) ? m_begin+a_rec_id : NULL;
        }

        T* get(size_t a_rec_id) {
            return likely(a_rec_id < capacity()) ? m_begin+a_rec_id : NULL;
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

        /// Name of the unlerlying storage (either mmap file or shm)
        std::string const& storage_name() const { return m_storage_name; }

        /// Call \a a_visitor for every record.
        /// @param a_visitor functor accepting two arguments:
        ///    (size_t rec_num, T* data)
        /// @param a_min_rec start from this record number
        /// @param a_count print up to this number of records (0 - all)
        /// @return number of records processed.
        template <class Visitor>
        size_t for_each(const Visitor& a_visitor, size_t a_min_rec = 0, size_t a_count = 0) const {
            const T* b = begin() + a_min_rec;
            const T* p = b;
            const T* e = std::min<const T*>(b + (a_count ? a_count : count()), begin() + count());
            if (p >= e)
                return 0;
            for (int i = 0; p != e; ++p, ++i)
                a_visitor(a_min_rec + i, p);
            return e - b;
        }

        template <class Visitor>
        size_t for_each(const Visitor& a_visitor, size_t a_min_rec = 0, size_t a_count = 0) {
            T*       b = begin() + a_min_rec;
            T*       p = b;
            const T* e = std::min<const T*>(p + (a_count ? a_count : count()), begin() + count());
            if (p >= e)
                return 0;
            for (int i = 0; p != e; ++p, ++i)
                a_visitor(a_min_rec + i, p);
            return e - b;
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
    init(const char* a_filename, size_t a_max_recs, bool a_read_only, int a_mode,
         void const* a_map_address, int a_map_options)
    {
        auto sz = total_size(a_max_recs);

        bool l_exists;
        try {
            boost::filesystem::path l_name(a_filename);
            try {
                boost::filesystem::create_directories(l_name.parent_path());
            } catch (boost::system::system_error& e) {
                throw io_error(errno, "Cannot create directory: ",
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

                    if (h.version != header::s_version)
                        throw utxx::runtime_error
                            ("Invalid file format ", a_filename);
                    if (h.rec_size != sizeof(T))
                        throw utxx::runtime_error
                            ("Invalid item size in file ", a_filename,
                             " (expected ", sizeof(T), " got ", h.rec_size, ')');
                    if (h.recs_offset != sizeof(header))
                        throw utxx::runtime_error
                            ("Mismatch in the records offset in ",
                             a_filename, " (expected=",
                             sizeof(header), ", got=", h.recs_offset, ')');
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

                UTXX_SCOPE_EXIT([=]{ ::close(l_fd); });

                bip::file_lock flock(a_filename);
                bip::scoped_lock<bip::file_lock> g_lock(flock);

                header h;
                h.version     = header::s_version;
                h.rec_count.store(0, std::memory_order_release);
                h.max_recs    = a_max_recs;
                h.rec_size    = sizeof(T);
                h.recs_offset = sizeof(header);

                if (::write(l_fd, &h, sizeof(h)) < 0)
                    throw io_error(errno, "Error writing to file ", a_filename);

                if (::ftruncate(l_fd, sz) < 0)
                    throw io_error(errno, "Error setting file ",
                        a_filename, " to size ", sz);

                ::fsync(l_fd);
            }

            auto mode = a_read_only ? bip::read_only : bip::read_write;
            bip::file_mapping  shmf  (a_filename, mode);
            bip::mapped_region region(shmf, mode, 0, sz, a_map_address, a_map_options);

            //Get the address of the mapped region
            void*  addr  = region.get_address();
            #ifndef NDEBUG
            size_t size  = region.get_size();
            assert(addr != nullptr && (a_map_address == nullptr || a_map_address == addr));
            assert(size == sz);
            #endif

            m_file  .swap(shmf);
            m_region.swap(region);
            m_storage_name = a_filename;

            //if (!exists && !a_read_only)
            //    memset(static_cast<char*>(addr) + sizeof(header), 0, size - sizeof(header));
            m_header = static_cast<header*>(addr);
            m_begin  = m_header->records;
            m_end    = m_begin + m_header->max_recs;
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
            throw runtime_error(e.what());
        }

        return !l_exists;
    }

    template <typename T, size_t NLocks, typename Lock, typename Ext>
    bool persist_array<T,NLocks,Lock,Ext>::
    init(bip::fixed_managed_shared_memory& a_segment,
         const char* a_name, persist_attach_type a_flag, size_t a_max_recs)
    {
        std::pair<char*, size_t> fres = a_segment.find<char>(a_name);

        bool found   = fres.first != nullptr;
        bool created = false;
        // Calculate the full object size:
        size_t size  = total_size(a_max_recs);

        // If the segment was found, initialize the pointers and use it
        if (found) {
            switch (a_flag) {
                case persist_attach_type::RECREATE: // Recreate existing object
                    (void)a_segment.destroy<char>(a_name);
                    break;
                case persist_attach_type::READ_WRITE:
                case persist_attach_type::OPEN_READ_ONLY:
                case persist_attach_type::OPEN_READ_WRITE: {
                    //assert(fres.second == 1);
                    m_header = reinterpret_cast<header*>(fres.first);
                    m_begin  = m_header->records;
                    m_end    = m_begin + m_header->max_recs;
                    if (m_header->recs_offset != sizeof(header))
                        throw runtime_error("Mismatch in the records offset in '",
                                            a_name, "' (expected=",
                                            sizeof(header), ", got=",
                                            m_header->recs_offset, ')');
                    BOOST_ASSERT(reinterpret_cast<char*>(m_end) <=
                                 reinterpret_cast<char*>(m_header)+fres.second);
                    created = false;
                    goto INIT_LOCKS;
                }
                default:
                    throw utxx::runtime_error
                        ("persist_array: unsupported opening mode for existing "
                         "shm storage: ", to_underlying(a_flag));
            }
        } else if (a_flag == persist_attach_type::OPEN_READ_ONLY ||
                   a_flag == persist_attach_type::OPEN_READ_WRITE)
            throw utxx::runtime_error
                ("persist_array: cannot open non-existing shared memory array '",
                 a_name, "'");

        assert(a_flag == persist_attach_type::CREATE_READ_ONLY   ||
               a_flag == persist_attach_type::CREATE_READ_WRITE  ||
               a_flag == persist_attach_type::READ_WRITE         ||
               a_flag == persist_attach_type::RECREATE);

        if (a_max_recs == 0)
            throw utxx::badarg_error("persist_array: invalid max number of records!");

        // Otherwise: create a new object:
        try
        {
            // Allocate it as a named array of bytes, zeroing them out with
            // 16-byte alignment:
            static_assert(sizeof(long double) == 16, "sizeof(long double)!=16 ?");
            int    sz16 = (size % 16 == 0) ? (size / 16) : (size / 16 + 1);

            auto*  mem  = a_segment.construct<long double>(a_name)[sz16](0.0L);
            assert(mem != NULL);

            //if (!exists && !a_read_only)
            //    memset(static_cast<char*>(addr) + sizeof(header), 0, size - sizeof(header));
            m_header = reinterpret_cast<header*>(mem);
            m_header->version     = header::s_version;
            m_header->rec_count.store(0, std::memory_order_release);
            m_header->max_recs    = a_max_recs;
            m_header->rec_size    = sizeof(T);
            m_header->recs_offset = sizeof(header);

            m_begin  = m_header->records;
            m_end    = m_begin + a_max_recs;

            m_storage_name = a_name;

            BOOST_ASSERT(reinterpret_cast<char*>(m_end) <=
                         reinterpret_cast<char*>(mem)+size);
            created = true;
        }
        catch (std::exception const& e)
        {
            // Delete the object if it has already been constructed:
            (void)a_segment.destroy<char>(a_name);
            throw utxx::runtime_error
                ("Cannot create a shared memory array '", a_name, "': ", e.what());
        }

    INIT_LOCKS:
        if (a_flag != persist_attach_type::OPEN_READ_ONLY) {
            // If the file is open for writing, initialize the locks
            // since previous program crash might have left locks in inconsistent state
            for (Lock* l = m_header->locks, *e = l + NLocks; l != e; ++l)
                new (l) Lock();
        }
        return created;
    }
} // namespace utxx
