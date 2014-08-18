//----------------------------------------------------------------------------
/// \file   running_stat_impl.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// This file implements a class that calculates running mean and
/// standard deviation. 
//----------------------------------------------------------------------------
// Created: 2010-05-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project

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

#ifndef _UTXX_RUNNING_STAT_IMPL_HPP_
#define _UTXX_RUNNING_STAT_IMPL_HPP_

#include <deque>
#include <utxx/compiler_hints.hpp>

#ifdef UTXX_RUNNING_MINMAX_DEBUG
#include <iostream>
#endif

namespace utxx {

namespace detail {
    template<typename Derived, typename T, bool FastMinMax>
    struct minmax_impl;

    // See:
    // http://www.archipel.uqam.ca/309/1/webmaximinalgo.pdf
    template <typename Derived, typename T>
    class minmax_impl<Derived, T, true> {
        std::deque<size_t> m_min_fifo;
        std::deque<size_t> m_max_fifo;
        size_t m_min_idx;
        size_t m_max_idx;

        Derived const* derived_this() const { return static_cast<Derived const*>(this); }

        T      data(size_t a_idx) const { return derived_this()->data(a_idx); }
        size_t end()              const { return derived_this()->end_idx();   }
        #ifdef UTXX_RUNNING_MINMAX_DEBUG
        size_t capacity()         const { return derived_this()->capacity();  }
        #endif
    protected:
        bool outside_window(size_t a_idx) const {
            size_t iend  = end();
            size_t begin = likely(iend > derived_this()->MASK)
                         ? iend - derived_this()->MASK : 0;
            bool   res   = a_idx < begin;
            #ifdef UTXX_RUNNING_MINMAX_DEBUG
            std::cout << "Check outside [" << a_idx << "]: " << (res ? "true" : "false")
                      << std::endl;
            #endif
            return res;
        }

        void update_minmax(T a_sample) {
            if (unlikely(derived_this()->empty())) {
                m_min_idx = m_max_idx = end();
                return;
            }

            size_t prev = end() - 1;

            if (a_sample > data(prev)) { //overshoot
                m_min_fifo.push_back(prev);
                if (outside_window(m_min_fifo.front()))
                    m_min_fifo.pop_front();
                while (!m_max_fifo.empty()) {
                    size_t idx = m_max_fifo.back();
                    if (a_sample <= data(idx)) {
                        if (outside_window(m_max_fifo.front())) {
                            if (m_max_idx == m_max_fifo.front())
                                m_max_idx = end();
                            m_max_fifo.pop_front();
                        }
                        break;
                    }
                    m_max_fifo.pop_back();
                }
            } else {
                m_max_fifo.push_back(prev);
                if (outside_window(m_max_fifo.front()))
                    m_max_fifo.pop_front();
                while (!m_min_fifo.empty()) {
                    size_t idx = m_min_fifo.back();
                    if (a_sample >= data(idx)) {
                        if (outside_window(m_min_fifo.front())) {
                            if (m_min_idx == m_min_fifo.front())
                                m_min_idx = end();
                            m_min_fifo.pop_front();
                        }
                        break;
                    }
                    m_min_fifo.pop_back();
                }
            }

            auto  idx = m_max_fifo.empty() ? m_max_idx : m_max_fifo.front();
            m_max_idx = a_sample > data(idx) ? end() : idx;
            idx       = m_min_fifo.empty() ? m_min_idx : m_min_fifo.front();
            m_min_idx = a_sample < data(idx) ? end() : idx;

            #ifdef UTXX_RUNNING_MINMAX_DEBUG
            std::cout << "========";
            for (size_t i = derived_this()->begin_idx(), e = end(); i < e; ++i)
                std::cout << " [" << i << "]" << data(i);
            std::cout << " [" << end() << "]" << a_sample << std::endl;
            std::cout << "Max: " << max() << " |";
            for (auto i : m_max_fifo) std::cout << " [" << i << "]" << data(i);
            std::cout << std::endl;
            std::cout << "Min: " << min() << " |";
            for (auto i : m_min_fifo) std::cout << " [" << i << "]" << data(i);
            std::cout << std::endl;
            #endif
        }

        T min() const { return data(m_min_idx); }
        T max() const { return data(m_max_idx); }

        std::pair<T,T> minmax() const {
            return std::make_pair(min(), max());
        }
    };

    template <typename Derived, typename T>
    class minmax_impl<Derived, T, false> {
        Derived const* derived_this() const { return static_cast<Derived const*>(this); }
        size_t begin_idx()   const { return derived_this()->begin_idx(); }
        size_t end_idx()     const { return derived_this()->end_idx();   }
        T data(size_t a_idx) const { return derived_this()->data(a_idx); }
        void max();
    protected:

        T min() const {
            T n = std::numeric_limits<T>::max();
            for (size_t i = begin_idx(), e = end_idx(); i != e; ++i) {
                T d = data(i);
                if (d < n) n = d;
            }
            return n;
        }
        T max() const {
            T n = std::numeric_limits<T>::lowest();
            for (size_t i = begin_idx(), e = end_idx(); i != e; ++i) {
                T d = data(i);
                if (d > n) n = d;
            }
            return n;
        }

        std::pair<T,T> minmax() const {
            T mn = std::numeric_limits<T>::max();
            T mx = std::numeric_limits<T>::lowest();
            for (size_t i = begin_idx(), e = end_idx(); i != e; ++i) {
                T d = data(i);
                if (d > mx) mx = d;
                if (d < mn) mn = d;
            }
            return std::make_pair(mn, mx);
        }

        void update_minmax(T sample) {}
    };
}


} // namespace utxx

#endif // _UTXX_RUNNING_STAT_IMPL_HPP_

