//------------------------------------------------------------------------------
/// \file   dynamic_config.hpp
/// \author Serge Aleynikov
//------------------------------------------------------------------------------
/// \brief Persistent container for dynamic configuration of parameters
///
/// Allows to maintain a set of persistent parameters for a process that can be
/// updated by other processes in real-time.
//------------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-08-15
//------------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2016 Serge Aleynikov <saleyn@gmail.com>

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
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <type_traits>
#include <string.h>
#include <utxx/nchar.hpp>
#include <utxx/hashmap.hpp>
#include <utxx/persist_blob.hpp>

namespace utxx {

    enum class dparam_t : uint16_t {
        UNDEFINED,
        LONG,
        BOOL,
        DOUBLE,
        STR
    };

    using dparam_str_t = nchar<56>;

    struct dynamic_param;

    //--------------------------------------------------------------------------
    // Internal helpers
    //--------------------------------------------------------------------------
    namespace {
        struct helpers {
            size_t operator()(const char* data) const {
                return detail::hsieh_hash(data, strlen(data));
            }
            bool operator()(const char* s1, const char* s2) const {
                return strcmp(s1, s2) == 0;
            }
        };

        struct helperp {
            size_t operator()(const void* data) const {
                return reinterpret_cast<uint64_t>(data);
            }
            bool operator()(const void* p1, const void* p2) const {
                return p1 == p2;
            }
        };
    }

    //--------------------------------------------------------------------------
    /// An instance of dynamically changing parameter
    //--------------------------------------------------------------------------
    template <int MaxParams = 256>
    struct dynamic_config {
        static const int EXCEEDED_CAPACITY = -2;

        explicit
        dynamic_config(const std::string& a_file = std::string())
            : m_last_seen_count(0)
        {
            if (!a_file.empty())
                init(a_file);
        }

        ~dynamic_config() { m_storage.close(); }

        /// @return true if file didn't exist and was created.
        bool init(const std::string& a_filename) {
            if (a_filename.empty())
                UTXX_THROW_BADARG_ERROR("Invalid filename");

            auto   res = m_storage.init(a_filename.c_str(), nullptr, false);
            if (res)
                new (&m_mutex) robust_mutex(m_storage->m_mutex, true);
            else
                m_mutex.set(m_storage->m_mutex);

            update();

            return res;
        }

        void close() {
            m_storage.close();
            m_last_seen_count = 0;
            m_by_name.clear();
            m_by_addr.clear();
        }

        size_t count() const {
            auto   pcount = (std::atomic<size_t>*)&m_storage->m_count;
            return pcount->load(std::memory_order_relaxed);
        }

        /// Update parameter name lookup maps
        void update() { update(true); }

        /// Get index of a given parameter's address
        /// @return -1 if parameter doesn't exist
        int index(const void* p) const {
            auto   it =  m_by_addr.find(p);
            return it == m_by_addr.end() ? -1 : it->second;
        }

        /// Get name of a given parameter's address
        /// @return null if parameter doesn't exist
        const char* name(const void* p) const {
            auto   it =  m_by_addr.find(p);
            return it == m_by_addr.end() ? nullptr : m_storage->name(it->second);
        }

        template <class T>
        typename std::enable_if<
            std::is_same<T, long>::value        ||
            std::is_same<T, bool>::value        ||
            std::is_same<T, double>::value      ||
            std::is_same<T, dparam_str_t>::value,
            typename std::add_lvalue_reference<T>::type>::type
        bind(const char* a_name) {
            lock_guard_t g(m_mutex);
            update(false);

            static T*   dummy;
            auto tp  =  type(dummy);
            auto res =  add(a_name, tp);
            if  (res == EXCEEDED_CAPACITY)
                UTXX_THROW_RUNTIME_ERROR("Too many parameters (count=",
                                         m_storage->m_count, ')');
            return *reinterpret_cast<T*>(m_storage->data(res));
        }

    private:
        using mutex_t        = pthread_mutex_t;
        using lock_guard_t   = std::lock_guard<robust_mutex>;
        using unique_guard_t = std::unique_lock<robust_mutex>;

        struct storage;

        using hash_map_t     = std::unordered_map<const char*, int, helpers, helpers>;
        using hash_map_rev_t = std::unordered_map<const void*, int, helperp, helperp>;
        using storage_t      = persist_blob<storage>;

    private:
        size_t         m_last_seen_count;
        hash_map_t     m_by_name;
        hash_map_rev_t m_by_addr;
        storage_t      m_storage;
        robust_mutex   m_mutex;

        storage* data() { return &m_storage.dirty_get(); }

        static dparam_t type(long*)          { return dparam_t::LONG;   }
        static dparam_t type(bool*)          { return dparam_t::BOOL;   }
        static dparam_t type(double*)        { return dparam_t::DOUBLE; }
        static dparam_t type(dparam_str_t*)  { return dparam_t::STR;    }
        template <class T>
        static dparam_t type(T*);

        static std::string to_name(const char* a_name) {
            int n = std::min<int>(sizeof(typename storage::name_t)-1, strlen(a_name));
            return  std::string(a_name, n);
        }

        /// Add a parameter to storage
        int add(const char* a_name, dparam_t a_tp) {
            auto nm = to_name(a_name);

            //lock_guard_t g(m_mutex);
            auto it = m_by_name.find(nm.c_str());
            if (it != m_by_name.end()) {
                auto  idx = it->second;
                auto& prm = m_storage.dirty_get().get(idx);
                return (prm.type() == a_tp) ? idx : EXCEEDED_CAPACITY;
            }

            auto idx = m_storage->add(nm.c_str(), a_tp);
            if (idx < 0)
                return idx;

            auto name = m_storage->name(idx);
            auto data = m_storage->data(idx);

            m_by_name.emplace(std::make_pair(name, idx));
            m_by_addr.emplace(std::make_pair(data, idx));

            return idx;
        }

        void update(bool a_with_lock) {
            auto n =  count();
            if  (n == m_last_seen_count)
                return;

            unique_guard_t g(m_mutex, std::defer_lock_t{});
            if (a_with_lock) g.lock();

            for (auto i = m_last_seen_count; i < m_storage->m_count; ++i) {
                const char* p = m_storage->name(i);
                m_by_name.emplace(std::make_pair(p, i));
                m_by_addr.emplace(std::make_pair(p, i));
            }
        }
    };

    //--------------------------------------------------------------------------
    /// An instance of dynamically changing parameter
    //--------------------------------------------------------------------------
    struct dynamic_param {
        using str_t = dparam_str_t;

        explicit dynamic_param(uint idx, long        data)
            : m_type(dparam_t::LONG),   m_idx(idx), m_size(sizeof(data)) { set(data); }

        explicit dynamic_param(uint idx, bool        data)
            : m_type(dparam_t::BOOL),   m_idx(idx), m_size(sizeof(data)) { set(data); }

        explicit dynamic_param(uint idx, double      data)
            : m_type(dparam_t::DOUBLE), m_idx(idx), m_size(sizeof(data)) { set(data); }

        explicit dynamic_param(uint idx, std::string const& data)
            : m_type(dparam_t::STR),    m_idx(idx)                       { set(data); }

        explicit dynamic_param(uint idx, char const* data)
            : m_type(dparam_t::STR),    m_idx(idx)                       { set(data); }

        dynamic_param() : m_type(dparam_t::UNDEFINED), m_idx(-1), m_size(0)
        {
            static_assert(sizeof(dynamic_param) == 64, "Invalid size");
            m_data.fill('\0');
        }

        dynamic_param(dparam_t a_tp, int a_idx = 0) : m_type(a_tp), m_idx(a_idx)
        {
            switch (a_tp) {
                case dparam_t::LONG:   m_size = sizeof(long);   break;
                case dparam_t::BOOL:   m_size = sizeof(bool);   break;
                case dparam_t::DOUBLE: m_size = sizeof(double); break;
                default:               m_size = 0;              break;
            }
            memset(m_data.data(), 0, sizeof(m_data));
        }

        dynamic_param(dynamic_param const& a) { *this = a;            }
        dynamic_param(dynamic_param&& a)      { *this = std::move(a); }

        void operator=(dynamic_param&& a) {
            m_type = a.m_type;
            m_idx  = a.m_idx;
            m_size = a.m_size;
            memcpy(m_data.data(), a.m_data.data(), sizeof(m_data));
        }

        void operator=(dynamic_param const& a) {
            m_type = a.m_type;
            m_idx  = a.m_idx;
            m_size = a.m_size;
            memcpy(m_data.data(), a.m_data.data(), sizeof(m_data));
        }

        dparam_t type()      const { return m_type; }
        uint16_t index()     const { return m_idx;  }
        uint32_t size()      const { return m_size; }

        long     to_long()   const { assert(m_type == dparam_t::LONG);   return *(long*)  m_data.data(); }
        long     to_bool()   const { assert(m_type == dparam_t::BOOL);   return *(bool*)  m_data.data(); }
        long     to_double() const { assert(m_type == dparam_t::DOUBLE); return *(double*)m_data.data(); }
        const char* to_str() const { assert(m_type == dparam_t::STR);    return m_data.data(); }
        void*       to_addr()      { return m_data.data(); }
        const void* to_addr()const { return m_data.data(); }

        void atomic_set(long data, std::memory_order ord = std::memory_order_relaxed) {
            assert(m_type == dparam_t::LONG);
            auto* v = (std::atomic<long>*)m_data.data();
            v->store(data, ord);
        }

        void atomic_add(long data, std::memory_order ord = std::memory_order_relaxed) {
            assert(m_type == dparam_t::LONG);
            auto* v = (std::atomic<long>*)m_data.data();
            v->fetch_add(data, ord);
        }

        void atomic_set(bool data, std::memory_order ord = std::memory_order_relaxed) {
            assert(m_type == dparam_t::BOOL);
            auto* v = (std::atomic<bool>*)m_data.data();
            v->store(data, ord);
        }

        void atomic_set(double data, std::memory_order ord = std::memory_order_relaxed) {
            assert(m_type == dparam_t::DOUBLE);
            auto* v = (std::atomic<double>*)m_data.data();
            v->store(data, ord);
        }

        void set(long a_data) { assert(m_type == dparam_t::LONG); *((long*)m_data.data()) = a_data; }
        void set(bool a_data) { assert(m_type == dparam_t::BOOL); *((bool*)m_data.data()) = a_data; }
        void set(double a_dt) { assert(m_type == dparam_t::BOOL); *((double*)m_data.data()) = a_dt; }

        void set(const std::string& a_data) { set(a_data.c_str(), a_data.size()); }

        void set(const char* a_data, size_t a_size) {
            m_size = std::min(sizeof(m_data)-1, a_size);
            memcpy(m_data.data(), a_data, m_size);
            m_data.data()[m_size] = '\0';
        }

    private:
        dparam_t    m_type;
        uint16_t    m_idx;
        uint32_t    m_size;
        str_t       m_data;
    };

    template <int MaxParams>
    struct dynamic_config<MaxParams>::storage {
        typedef char name_t[96];

        friend struct dynamic_config<MaxParams>;

        /// NOTE: Must be called under lock!
        int add(const char* a_name, dparam_t a_tp) {
            auto n = m_count;
            if  (n+1 > MaxParams)
                return -1;
            auto sz = std::min<int>(strlen(a_name), sizeof(name_t)-1);
            memcpy(m_names[n], a_name, sz);
            m_names[n][sz] = '\0';

            m_count = n+1;
            auto* p = &get(n);
            new (p) dynamic_param(a_tp, n);

            return n;
        }

        const char* name(uint a_idx) const {
            assert(a_idx < MaxParams);
            return m_names[a_idx];
        }

        dynamic_param& get(uint a_idx) {
            assert(a_idx < MaxParams);
            auto p = reinterpret_cast<dynamic_param*>(m_data);
            return *(p + a_idx);
        }

        void* data(uint a_idx) {
            assert(a_idx < MaxParams);
            return (reinterpret_cast<dynamic_param*>(m_data)+a_idx)->to_addr();
        }
    private:
        mutex_t m_mutex;
        size_t  m_count;
        name_t  m_names[MaxParams];
        char    m_data[0];
    };

} // namespace utxx