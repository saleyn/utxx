//----------------------------------------------------------------------------
/// \file  bitmap.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Implements fast lookup of data by non-uniformly distributed keys,
/// where key space is clustered in groups with possibly large gaps between
/// the groups. E.g. [10,11,12, 50,52,53, 150,151,152].
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-08-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is a part of the utxx open-source project.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _UTXX_CLUSTERED_MAP_HPP_
#define _UTXX_CLUSTERED_MAP_HPP_

#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/int.hpp>
#include <utxx/bitmap.hpp>
#include <map>
#include <functional>

namespace utxx {

struct ascending {};
struct desending {};

template <
    class Key,
    class Data,
    int   LowBits   = 6,
    class SortOrder = ascending
>
class clustered_map {
    BOOST_STATIC_ASSERT(sizeof(Key) <= sizeof(int64_t));
    BOOST_STATIC_ASSERT(LowBits     <= 6);
    typedef bitmap_low<1 << LowBits> bitmap_t;
    static const size_t s_lo_mask = (1 << LowBits) - 1;
    static const size_t s_hi_mask = ~s_lo_mask;
    static const int    s_level2_init_value =
        boost::is_same<SortOrder, ascending>::value ? -1 : bitmap_t::cend;

    struct key_data {
        bitmap_t    index;
        Data        data[1 << LowBits];
    };

    typedef std::map<
        Key,
        key_data,
        typename boost::mpl::if_<
            boost::is_same<SortOrder, ascending>,
            std::less<Key>, std::greater<Key>
        >::type
    > gross_map;

    typedef typename gross_map::iterator l1_iterator;

    gross_map m_map;

    /* Two mru slots are used to cover cases of lookup oscilations
     * between keys on the boundary of adjucent groups to cache
     * slow lookups in the gross_map */
    typename gross_map::iterator m_mru[2];

    bool mru_map_lookup(size_t a_hi, typename gross_map::iterator& a_it) {
        if (!mru_lookup(a_hi, a_it)) {
            a_it = m_map.find(a_hi);
            if (a_it != m_map.end()) {
                m_mru[1] = m_mru[0];
                m_mru[0] = a_it;
                return true;
            }
            return false;
        }
        return true;
    }

    // NB: iterator unchanged if it's not in MRU cache
    bool mru_lookup(size_t a_hi, typename gross_map::iterator& a_it) {
        if (m_mru[0] == m_map.end())
            return false;
        else if (m_mru[0]->first == a_hi)
            a_it = m_mru[0];
        else if (m_mru[1] == m_map.end())
            return false;
        else if (m_mru[1]->first == a_hi) {
            std::swap(m_mru[0], m_mru[1]);
            a_it = m_mru[1];
        } else
            return false;
        return true;
    }

    void update_mru(typename gross_map::iterator& a_it) {
        if (m_mru[0] != a_it && a_it != m_map.end()) {
            m_mru[1] = m_mru[0];
            m_mru[0] = a_it;
        }
    }

    std::pair<bool, Data*> ensure(size_t a_hi, size_t a_lo) {
        typename gross_map::iterator it; // FIXME: should we init it to m_map.end()?
        bool found = mru_map_lookup(a_hi, it);
        if (!found) {
            std::pair<typename gross_map::iterator, bool> res =
                m_map.insert(std::make_pair(a_hi, key_data()));
            it = res.first;
            update_mru(it);
        } else
            found = it->second.index[a_lo];

        it->second.index.set(a_lo);
        return std::make_pair(found, &it->second.data[a_lo]);
    }

public:
    typedef typename gross_map::key_type    key_type;
    typedef typename gross_map::mapped_type mapped_type;
    typedef typename gross_map::value_type  value_type;

    class const_iterator;
    class iterator;

    clustered_map() {
        m_mru[0] = m_map.end();
        m_mru[1] = m_map.end();
    }

    iterator        begin() { return iterator(m_map, m_map.begin()); }
    iterator        end()   { return iterator(m_map, m_map.end(), bitmap_t::cend); }

    const_iterator  begin() const { return const_iterator(m_map, m_map.begin()); }
    const_iterator  end()   const { return const_iterator(m_map, m_map.end(), bitmap_t::cend);   }

    /// Total number of clustered key groups
    size_t group_count() const { return m_map.size(); }
    /// Number of items in the cluster group associated with the \a a_key.
    size_t item_count(Key a_key) const {
        typename gross_map::const_iterator it = m_map.find(a_key & s_hi_mask);
        return it == m_map.end() ? 0 : it->second.index.count();
    }

    /// Return the data pointer associated with the \a a_key entry
    /// in the container. If the \a a_key is not found, return NULL.
    Data* at(Key a_key) {
        typename gross_map::iterator it;
        if (!mru_map_lookup(a_key & s_hi_mask, it))
            return NULL;
        return it->second.index[a_key & s_lo_mask]
            ? &it->second.data[a_key & s_lo_mask]
            : NULL;
    }

    iterator find(Key a_key) {
        typename gross_map::iterator it;
        if (!mru_map_lookup(a_key& s_hi_mask, it))
            return end();
        size_t l2 = a_key & s_lo_mask;
        return it->second.index[l2] ? iterator(m_map, it, l2) : end();
    }

    /// Insert an entry in the container associated with the \a a_key.
    Data& insert(Key a_key) {
        size_t lo = a_key & s_lo_mask;
        std::pair<bool, Data*> t = ensure(a_key & s_hi_mask, lo);
        return *t.second;
    }

    /// Insert \a a_key and \a a_data pair in the container
    void insert(Key a_key, const Data& a_data) {
        size_t lo = a_key & s_lo_mask;
        std::pair<bool, Data*> t = ensure(a_key & s_hi_mask, lo);
        *t.second = a_data;
    }

    /// Return data associated with the \a a_key. If the \a a_key
    /// is not present in the container, it will be inserted.
    Data& operator[] (Key a_key) { return insert(a_key); }

    /// Erase entry pointed by the iterator \a a_it from the container
    bool erase(iterator a_it);

    /// Erase given key from the container
    bool erase(Key a_key) {
        iterator it(m_map, a_key);
        return erase(it);
    }

    /// Clears the container
    void clear() {
        m_map.clear();
        m_mru[0] = m_map.end();
        m_mru[1] = m_map.end();
    }

    /// Returns true when the container is empty
    bool empty() const { return m_map.empty(); }

    template <class Visitor, class State>
    void for_each(Visitor& a_visit, State& a_state) {
        for (iterator it = begin(), e = end(); it != e; ++it)
            a_visit(it.key(), it.data(), a_state);
    }
};

template <class Key, class Data, int LowBits, class SortOrder>
class clustered_map<Key, Data, LowBits, SortOrder>::iterator
{
    gross_map*   m_owner;
    l1_iterator  m_level1;
    int          m_level2;

    int find_first_level2() {
        if (m_level1 == m_owner->end())
            return s_level2_init_value;
        return boost::is_same<SortOrder, ascending>::value
            ? m_level1->second.index.first()
            : m_level1->second.index.last();
    }

    int find_next_level2(int a_level2 = s_level2_init_value) {
        if (m_level1 == m_owner->end())
            return a_level2;
        return boost::is_same<SortOrder, ascending>::value
            ? m_level1->second.index.next(a_level2)
            : m_level1->second.index.prev(a_level2);
    }

    l1_iterator& level1() { return m_level1; }
    int&         level2() { return m_level2; }

    friend class clustered_map<Key, Data, LowBits, SortOrder>;
public:
    using pointer         = Data*;
    using reference       = Data&;
    using const_reference = Data const&;

    iterator() : m_level2(s_level2_init_value) {}

    iterator(gross_map& a_map, Key a_key)
        : m_owner(&a_map)
        , m_level1(a_map.find(a_key & s_hi_mask))
        , m_level2(a_key & s_lo_mask)
    {
        if (m_level1 == a_map.end())
            m_level2 = bitmap_t::cend;
        else if (!m_level1->second.index.is_set(m_level2))
            m_level2 = find_next_level2(m_level2);
    }

    iterator(gross_map& a_map, const l1_iterator& a_level1)
        : m_owner(&a_map)
        , m_level1(a_level1)
        , m_level2(find_first_level2())
    {}

    iterator(gross_map& a_map, const l1_iterator& a_level1, int a_level2)
        : m_owner(&a_map)
        , m_level1(a_level1)
        , m_level2(a_level2)
    {}

    iterator(const iterator& a_rhs)
        : m_owner(a_rhs.m_owner)
        , m_level1(a_rhs.m_level1)
        , m_level2(a_rhs.m_level2)
    {}

    Key key() const {
        BOOST_ASSERT(m_level1 != m_owner->end());
        return m_level1->first | m_level2;
    }

    Data&       data()                      { return m_level1->second.data[m_level2]; } //return this->second.data[m_level2]; }
    const Data& data()              const   { return m_level1->second.data[m_level2]; }

    typename gross_map::iterator&       group()       { return m_level1; }
    typename gross_map::const_iterator& group() const {
        assert(m_owner);
        return const_iterator(*m_owner, m_level1, m_level2);
    }

    size_t      item_count()        const   { return m_level1->index.count(); }
    int         item()              const   { return m_level2; }
    static const int  end_item()            { return bitmap_t::cend; }
    int         first_item_idx()    const   { return find_first_level2(); }
    int         next_item_idx()     const   { return find_next_level2(m_level2); }

    Data* first_item() {
        m_level2 = find_first_level2();
        return m_level2 == end_item() ? NULL : &m_level1->second.data[m_level2];
    }

    Data* next_item() {
        m_level2 = find_next_level2(m_level2);
        return m_level2 == end_item() ? NULL : &m_level1->second.data[m_level2];
    }

    bool operator== (const iterator& a_rhs) const {
        return (m_level1 == a_rhs.m_level1) && item() == a_rhs.item();
    }

    bool operator!= (const iterator& a_rhs) const {
        return (m_level1 != a_rhs.m_level1) || item() != a_rhs.item();
    }

    pointer         operator->() const  { return &m_level1->second.data[m_level2]; }
    reference       operator*()         { return  m_level1->second.data[m_level2]; }
    const_reference operator*()  const  { return  m_level1->second.data[m_level2]; }

    bool find_first_key() {
        m_level1 = m_owner->begin();
        m_level2 = find_first_level2();
        return m_level1 != m_owner->end() && m_level2 != bitmap_t::cend;
    }

    iterator& operator++() {
        int n = m_level2;
        do {
            m_level2 = find_next_level2(n);
            if (m_level2 != bitmap_t::cend)
                return *this;
            ++m_level1;
            n = s_level2_init_value;
        } while (m_level1 != m_owner->end());

        return *this;
    }
};

template <class Key, class Data, int LowBits, class SortOrder>
class clustered_map<Key, Data, LowBits, SortOrder>::const_iterator
    : public clustered_map<Key, Data, LowBits, SortOrder>::iterator
{
public:
    typedef typename clustered_map::iterator base;

    const_iterator() : base() {}
    const_iterator(gross_map& a_map, typename gross_map::const_iterator& a_level1)
        : base(a_map, a_level1)
    {}

    const_iterator(gross_map& a_map, typename gross_map::const_iterator& a_level1, int a_level2)
        : base(a_map, a_level1, a_level2)
    {}

    const_iterator(const const_iterator& a_rhs)
        : base(a_rhs)
    {}
};

template <class Key, class Data, int LowBits, class SortOrder>
bool clustered_map<Key, Data, LowBits, SortOrder>::
erase(iterator a_it) {
    if (m_map.find(a_it.level1()->first) == m_map.end())
        return false;
    if (!a_it.level1()->second.index.is_set(a_it.item()))
        return false;
    a_it.level1()->second.index.clear(a_it.item());
    if (a_it.level1()->second.index.empty()) {
        m_map.erase(a_it.level1());
        if (m_mru[1] == a_it.level1()) m_mru[1] = m_map.end();
        if (m_mru[0] == a_it.level1()) m_mru[0] = m_mru[1];
    }
    return true;
}

} // namespace utxx

#endif // _UTXX_CLUSTERED_MAP_HPP_

