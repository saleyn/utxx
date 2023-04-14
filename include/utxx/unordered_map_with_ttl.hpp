//----------------------------------------------------------------------------
/// \file   unordered_map_with_ttl.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief A Key/Value hashmap with TTL eviction.
///
/// An insert of a Key/Value pair in the map will store the timestamp of the
/// maybe_add.  Additionally a queue of maybe_adds is maintained by this container,
/// which is checked on each insert and the expired Key/Value pairs are
/// evicted from the map.
///
/// For performance reasons it's desirable to call the `refresh()` method
/// every time the caller is idle.
//----------------------------------------------------------------------------
// Copyright (c) 2016 Serge Aleynikov <saleyn@gmail.com>
// Created: 2023-04-14
//----------------------------------------------------------------------------
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

#include <type_traits>
#include <memory>
#include <unordered_map>
#include <list>
#include <cassert>

namespace utxx {
    namespace {
        template <typename T>
        struct val_node {
            val_node() {}
            val_node(T&& v, uint64_t time) : value(v), time(time) {}

            T        value;
            uint64_t time;      // Last maybe_add time
        };

        template <class T>
        struct val_assigner {
            bool operator()(
                [[maybe_unused]] val_node<T>& old,
                [[maybe_unused]] const T&     new_val,
                [[maybe_unused]] uint64_t     time
            ) {
                return false;
            }
        };
    }

    //--------------------------------------------------------------------------
    /// Unordered map with TTL for each key
    /// @tparam K        key type
    /// @tparam T        value type
    /// @tparam Alloc    custom allocator
    ///
    /// Short vectors up to \a MaxItems don't involve memory allocations.
    /// Short vector can be set to have NULL value by using "set_null()" method.
    /// NULL vectors are represented by having size() = -1.
    //--------------------------------------------------------------------------
    template <
        class K,
        class T,
        class Hash = std::hash<K>,
        class KeyEqual  = std::equal_to<K>,
        class ValUpdate = val_assigner<T>,
        class Allocator = std::allocator<std::pair<const K, T>>
    >
    struct unordered_map_with_ttl {
        struct ttl_node {
            uint64_t time;
            K        key;
        };

        using hash_map       = std::unordered_map<K, val_node<T>, Hash, KeyEqual, Allocator>;
        using lru_alloc      = typename std::allocator_traits<Allocator>::template rebind_alloc<ttl_node>;
        using lru_list       = std::list<ttl_node, lru_alloc>;
        using iterator       = typename hash_map::iterator;
        using const_iterator = typename hash_map::const_iterator;

        explicit unordered_map_with_ttl(uint64_t ttl)
            : m_ttl(ttl)
            , m_map()
            , m_lru()
            , m_assign(val_assigner<T>())
        {}

        unordered_map_with_ttl(uint64_t ttl, const Allocator& alloc)
            : m_ttl(ttl)
            , m_map(alloc)
            , m_assign(val_assigner<T>())
        {}

        unordered_map_with_ttl(
            uint64_t         ttl,
            size_t           bucket_count,
            const Hash&      hash  = Hash(),
            const KeyEqual&  equal = KeyEqual(),
            const Allocator& alloc = Allocator()
        )
            : m_ttl(ttl)
            , m_map(bucket_count, hash, equal, alloc)
            , m_lru()
            , m_assign(val_assigner<T>())
        {}

        /// @brief Try to add a given key/value to the map.
        /// @return true if the value was added
        bool try_add(const K& key, T&& value, uint64_t now);

        size_t size() const { return m_map.size(); }

        iterator       begin()         noexcept { return m_map.begin();  }
        const_iterator begin()   const noexcept { return m_map.begin();  }
        const_iterator cbegin()  const noexcept { return m_map.cbegin(); }

        iterator       end()           noexcept { return m_map.end();    }
        const_iterator end()     const noexcept { return m_map.end();    }
        const_iterator cend()    const noexcept { return m_map.cend();   }

        /// @brief Erase the given key from the map
        /// @return true when the key was evicted
        template <typename Key>
        bool erase(Key&& k)                     { return m_map.erase(k) > 0; }

        /// @brief Clear the map
        void clear()                            { m_map.clear(); m_lru.clear(); }

        /// @brief Evict expired key/value pairs
        /// @param now - current timestamp
        /// @return the number of evicted items
        size_t refresh(uint64_t now);

        iterator       find(const K& k)         { return m_map.find(k);  }
        const_iterator find(const K& k)   const { return m_map.find(k);  }

    private:
        uint64_t  m_ttl;
        hash_map  m_map;
        lru_list  m_lru;
        ValUpdate m_assign;
    };

    //--------------------------------------------------------------------------
    // IMPLEMENTATION
    //--------------------------------------------------------------------------

    template <class K, class T, class Hash, class KeyEq, class ValUpdate, class Allocator>
    size_t unordered_map_with_ttl<K,T,Hash,KeyEq,ValUpdate,Allocator>
    ::refresh(uint64_t now)
    {
        assert(now > m_ttl);

        size_t res = 0;
        auto   ttl = now - m_ttl;

        for (auto i = m_lru.begin(); i != m_lru.end() && i->time <= ttl; i = m_lru.begin(), ++res) {
            m_map.erase(i->key);    // Evict expired node
            m_lru.pop_front();
        }

        return res;
    }

    template <class K, class T, class Hash, class KeyEq, class ValUpdate, class Allocator>
    bool unordered_map_with_ttl<K,T,Hash,KeyEq,ValUpdate,Allocator>
    ::try_add(const K& key, T&& value, uint64_t now)
    {
        assert(now > m_ttl);

        auto ttl = now - m_ttl;

        for (auto i = m_lru.begin(); i != m_lru.end() && i->time <= ttl; i = m_lru.begin()) {
            m_map.erase(i->key);    // Evict expired node
            m_lru.pop_front();
        }

        auto it = m_map.find(key);
        auto not_found = it == m_map.end();

        if (not_found)
            m_map.emplace(key, val_node<T>(std::move(value), now));
        else // maybe_add the existing entry
            not_found = m_assign(it->second, std::move(value), now);

        if (not_found)
            m_lru.push_back(ttl_node{now, key});

        return not_found;
    }


} // namespace utxx
