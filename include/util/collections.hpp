/**
 * \file
 * \brief Collections container with merge iterator.
 *
 * \author Dmitriy Kargapolov <dmitriy dot kargapolov at gmail dot com>
 * \since 30 Jul 2012
 *
 * Copyright (C) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTIL_COLLECTIONS_HPP_
#define _UTIL_COLLECTIONS_HPP_

#include <list>

namespace util {

/// Container for multiple collections.
template<typename Collection>
class collections {
    typedef Collection coll_t;
    typedef typename coll_t::value_type value_t;
    typedef typename coll_t::iterator in_it_t;

    typedef std::list<coll_t> colls_t;
    typedef typename colls_t::iterator it_t;

    /// Underlying collection object reference.
    colls_t m_colls;

public:
    /// Add new collection to the container.
    template<typename T>
    void add(const T& a_elem) {
        m_colls.push_back(a_elem);
    }

    /// Merge iterator class.
    class iterator: public std::iterator<std::input_iterator_tag, value_t> {
        typedef std::pair<in_it_t, in_it_t> it_pair_t;
        typedef std::list<it_pair_t> iters_t;
        typedef typename iters_t::iterator it_it_t;
        iters_t m_iters;
        bool m_end;

        static
        bool compare(const it_pair_t& lhs, const it_pair_t& rhs) {
            if (rhs.first == rhs.second)
                return true;
            if (lhs.first == lhs.second)
                return false;
            return coll_t::compare(*lhs.first, *rhs.first);
        }

        bool reached_end() const {
            return m_iters.begin()->first == m_iters.begin()->second;
        }

        in_it_t& current() {
            return m_iters.begin()->first;
        }

        void getnext() {
            it_it_t me = m_iters.begin();
            it_it_t end = m_iters.end();
            it_it_t next = me;
            ++next;
            if (next == end)
                return;
            it_pair_t l_me = *me;
            if (compare(*next, l_me))
                m_iters.pop_front();
            else
                return;
            while (true) {
                ++next;
                if (next == end) {
                    m_iters.push_back(l_me);
                    break;
                }
                if (compare(*next, l_me))
                    continue;
                m_iters.insert(next, l_me);
                break;
            }
        }

    public:
        iterator() :
                m_end(true) {
        }
        iterator(colls_t& a_colls) :
                m_end(false) {
            for (it_t it = a_colls.begin(), e = a_colls.end(); it != e;
                    ++it)
                m_iters.push_back(it_pair_t(it->begin(), it->end()));
            m_iters.sort(compare);
        }

        iterator& operator++() {
            current().operator++();
            getnext();
            return *this;
        }

        iterator operator++(int) {
            iterator tmp(*this);
            operator++();
            return tmp;
        }

        bool operator==(const iterator& rhs) const {
            if (m_end || reached_end())
                return (rhs.m_end || rhs.reached_end());
            if (rhs.m_end || rhs.reached_end())
                return false;
            return m_iters == rhs.m_iters;
        }

        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }

        value_t& operator*() {
            return current().operator*();
        }

        value_t* operator->() {
            return current().operator->();
        }

    };

    typedef const iterator const_iterator;

    /// Beginning of merged sequence.
    iterator begin() {
        return iterator(m_colls);
    }
    const_iterator begin() const {
        return iterator(m_colls);
    }

    /// End of merged sequence.
    iterator end() {
        return iterator();
    }
    const_iterator end() const {
        return iterator();
    }
};

} // namespace util

#endif // _UTIL_COLLECTIONS_HPP_
