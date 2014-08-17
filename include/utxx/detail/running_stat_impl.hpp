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

namespace utxx {

namespace detail {
    template<typename Derived, typename T, bool FastMinMax>
    struct minmax_impl;

    template <typename Derived, typename T>
    class minmax_impl<Derived, T, true> {
        std::deque<size_t> m_min_fifo;
        std::deque<size_t> m_max_fifo;

        Derived const* derived_this() const { return static_cast<Derived const*>(this); }
    protected:
        bool outside_window(size_t a_idx) const {
            return (Derived::m_end - Derived::MASK) > a_idx; 
        }

        void update_minmax(T a_sample) {
            size_t prev = Derived::m_end - 1;

            if (a_sample > Derived::m_data[prev]) { //overshoot
                m_min_fifo.push_back(prev);
                //if (i == width + m_min_fifo.front())
                if (outside_window(m_min_fifo.front()))
                    m_min_fifo.pop_front();
                while (!m_max_fifo.empty()) {
                    size_t idx = m_max_fifo.back();
                    if (a_sample <= Derived::m_data[idx]) {
                        //if (i == width + m_max_fifo.front())
                        if (outside_window(m_max_fifo.front()))
                            m_max_fifo.pop_front();
                        break;
                    }
                    m_max_fifo.pop_back();
                }
            } else {
                m_max_fifo.push_back(prev);
                //if (i == width + m_max_fifo.front())
                if (outside_window(m_max_fifo.front()))
                    m_max_fifo.pop_front();
                while (!m_min_fifo.empty()) {
                    size_t idx = m_min_fifo.back();
                    if (a_sample >= Derived::m_data[idx]) {
                        //if (i == width + m_min_fifo.front())
                        if (outside_window(m_min_fifo.front()))
                            m_min_fifo.pop_front();
                        break;
                    }
                    m_min_fifo.pop_back();
                }
            }
        }

        std::pair<T,T> get_minmax() const {
            return std::make_pair(m_min_fifo.front(), m_max_fifo.front());
        }
    };

    template <typename Derived, typename T>
    class minmax_impl<Derived, T, false> {
        Derived const* derived_this() const { return static_cast<Derived const*>(this); }
    protected:

        std::pair<T,T> get_minmax() const {
            if (derived_this()->empty()) return std::make_pair(0, 0);

            T min = std::numeric_limits<T>::max();
            T max = std::numeric_limits<T>::min();
            for (T const *p=derived_this()->m_data,
                         *e=derived_this()->m_data + derived_this()->size();
                 p != e; ++p)
            {
                if (*p > max) max = *p;
                if (*p < min) min = *p;
            }
            return std::make_pair(min, max);
        }

        void update_minmax(T sample) {}
    };
}


} // namespace utxx

#endif // _UTXX_RUNNING_STAT_IMPL_HPP_

