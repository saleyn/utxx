//----------------------------------------------------------------------------
/// \file   mt_queue.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Producer/consumer queue.
///
/// Based on code from:
/// http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
/// Original version authored by Anthony Williams
/// Modifications by Michael Anderson
/// Modifications by Serge Aleynikov
//----------------------------------------------------------------------------
// Created: 2010-09-15
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

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
#pragma once

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <utxx/compiler_hints.hpp>
#include <deque>
#include <exception>

namespace utxx {

class queue_canceled : public std::exception {};

template<typename Data, typename Alloc = std::allocator<char> >
class concurrent_queue
{
public:
    typedef std::deque<Data, Alloc> queue_type;
private:
    queue_type                  m_queue;
    mutable boost::mutex        m_mutex;
    boost::condition_variable   m_condition_variable;
    bool                        m_is_canceled;

public:
    concurrent_queue()
        : m_queue(), m_mutex()
        , m_condition_variable()
        , m_is_canceled(false)
    {}
    
    void reset() {
        m_queue.clear();
        m_is_canceled = false;
    }

    bool canceled() const { return m_is_canceled; }

    int enqueue(Data const& data, struct timespec* timeout = NULL) {
        try {
            push(data);
            return 0;
        } catch (queue_canceled&) {
            return -1;
        }
    }

    int dequeue(Data& value) {
        try                     { pop(value);   }
        catch (queue_canceled&) { return -1; }
        return 0;
    }

    int dequeue(Data& value, struct timespec* wait_time) {
        if (!wait_time)
            return dequeue(value);
        boost::posix_time::time_duration timeout(
            boost::posix_time::seconds(wait_time->tv_sec) +
            boost::posix_time::microseconds(wait_time->tv_nsec / 1000));
        try                     { return timed_pop(value, timeout) ? 0 : -1; }
        catch (queue_canceled&) { return -1; }
    }

    void push(Data const& data) {
        {
            boost::mutex::scoped_lock lock(m_mutex);
            if (unlikely(m_is_canceled)) throw queue_canceled();
            m_queue.push_back(data);
        }
        m_condition_variable.notify_one();
    }

    bool empty() const {
        boost::mutex::scoped_lock lock(m_mutex);
        if (unlikely(m_is_canceled)) throw queue_canceled();
        return m_queue.empty();
    }

    bool try_pop(Data& popped_value) {
        boost::mutex::scoped_lock lock(m_mutex);
        if (unlikely(m_is_canceled)) throw queue_canceled();
        if(m_queue.empty())
            return false;

        popped_value=m_queue.front();
        m_queue.pop_front();
        return true;
    }

    /// Wait until the queue is non-empty and dequeue
    /// a single item.
    void pop(Data& popped_value) {
        boost::mutex::scoped_lock lock(m_mutex);

        while(m_queue.empty() && !m_is_canceled)
            m_condition_variable.wait(lock);

        if (unlikely(m_is_canceled)) throw queue_canceled();

        popped_value=m_queue.front();
        m_queue.pop_front();
    }

    /// Wait until the queue is non-empty and dequeue
    /// a single item.
    bool timed_pop(Data& popped_value, const boost::posix_time::time_duration& timeout) {
        boost::mutex::scoped_lock lock(m_mutex);

        if (m_queue.empty() && !m_is_canceled)
            if (!m_condition_variable.timed_wait(lock, timeout))
                return false;

        if (unlikely(m_is_canceled)) throw queue_canceled();

        popped_value=m_queue.front();
        m_queue.pop_front();
        return true;
    }

    /// Wait until the queue is non-empty and dequeue
    /// all pending items.
    queue_type pop_all() {
        boost::mutex::scoped_lock lock(m_mutex);

        while(m_queue.empty() && !m_is_canceled)
            m_condition_variable.wait(lock);

        queue_type retval;
        std::swap(retval, m_queue);
        return retval;
    }

    void terminate() {
        {
            boost::mutex::scoped_lock lock(m_mutex);
            if (m_is_canceled) return;
            m_is_canceled = true;
        }
        m_condition_variable.notify_all();
    }
};

} // namespace utxx
