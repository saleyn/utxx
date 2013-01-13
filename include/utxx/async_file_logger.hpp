//----------------------------------------------------------------------------
/// \file   async_file_logger.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Asynchronous file logger
//----------------------------------------------------------------------------
// Copyright (c) 2012 Omnibius, LLC
// Created: 2012-03-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of utxx open-source project.

Copyright (C) 2012 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _UTXX_ASYNC_FILE_LOGGER_HPP_
#define _UTXX_ASYNC_FILE_LOGGER_HPP_

#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <utxx/atomic.hpp>
#include <utxx/synch.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <stdio.h>
#include <stdarg.h>

namespace utxx {

#ifdef DEBUG_ASYNC_LOGGER
#   define ASYNC_TRACE(x) printf x
#else
#   define ASYNC_TRACE(x)
#endif

template<class Alloc>
class text_msg
    : private std::basic_string< char, std::char_traits<char>, Alloc> {
protected:
    typedef std::basic_string< char, std::char_traits<char>, Alloc> base;

public:
    explicit text_msg(const Alloc& alloc = Alloc()): base(alloc) {}

    explicit text_msg(const char* s, const Alloc& alloc = Alloc())
        : base(s, alloc)
    {}

    explicit text_msg(const std::string& s, const Alloc& alloc = Alloc())
        : base(s, 0, std::string::npos, alloc)
    {}

    bool        empty() const  { return base::empty(); }
    size_t      size()  const  { return base::size();  }
    const char* c_str() const  { return base::c_str(); }

    /*
    void operator= (const std::string& s)   { *static_cast<base*>(this) = s; }
    void operator= (const char* s)          { *static_cast<base*>(this) = s; }
    */
};

/// Traits of asynchronous logger
struct async_logger_traits {
    typedef std::allocator<char>    allocator;
    typedef text_msg<allocator>     msg_type;
    typedef synch::futex            event_type;
    enum {
          commit_timeout    = 2000  // commit this number of usec
        , write_buf_sz      = 256
    };
};

/// Asynchronous logger of text messages.
template<typename traits = async_logger_traits>
class basic_async_logger {
protected:
    class cons: public traits::msg_type {
        typedef typename traits::msg_type  base;
        typedef typename traits::allocator allocator;
        mutable const cons* m_next;

    public:
        template<typename T>
        explicit cons(const T& s, const allocator& alloc = allocator())
            : base(s, alloc), m_next(NULL)
        {}

        const cons* next()              const   { return m_next; }
        void        next(const cons* p) const   { m_next = p; }
    };

    typedef typename traits::allocator  allocator;
    typedef typename traits::event_type event_type;
    typedef cons                        log_msg_type;

    boost::shared_ptr<boost::thread>    m_thread;
    allocator                           m_allocator;
    FILE*                               m_file;
    const volatile log_msg_type*        m_head;
    bool                                m_cancel;
    int                                 m_max_queue_size;
    std::string                         m_filename;
    event_type                          m_event;

    // Invoked by the async thread to flush messages from queue to file
    int  commit(const struct timespec* tsp = NULL);
    // Invoked by the async thread
    void run(boost::barrier* a_barrier);
    // Enqueues msg to internal queue
    int  internal_write(const log_msg_type* msg);
    // Print error message
    void print_error(int, const char* what, int line);

public:
    explicit basic_async_logger(const allocator& alloc = allocator())
        : m_allocator(alloc), m_file(NULL), m_head(NULL), m_cancel(false)
        , m_max_queue_size(0)
    {}

    ~basic_async_logger() {
        if (m_file)
            stop();
    }

    /// Initialize and start asynchronous file writer
    /// @param filename     - name of the file
    /// @param max_msg_size - expected maximum message size (used to allocate
    ///                       the buffer on the stack to format the
    ///                       output string when calling fwrite function)
    /// @param mode         - set file access mode
    int start(const std::string& filename);

    /// Stop asynchronous file writering thread
    void stop();

    /// @return name of the log file
    const std::string&  filename()          const { return m_filename; }
    /// @return max size of the commit queue
    const int           max_queue_size()    const { return m_max_queue_size; }

    /// Callback to be called on file I/O error
    boost::function<void (int, const char*)> on_error;
};

/// Asynchronous text logger
template <class Traits = async_logger_traits>
struct text_file_logger: public basic_async_logger<Traits> {
    typedef basic_async_logger<Traits>  base;
    typedef typename base::log_msg_type log_msg_type;
    typedef typename base::allocator    allocator;

    explicit text_file_logger(const allocator& alloc = allocator()): base(alloc) {}

    /// Formatted write with arguments 
    int fwrite(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        int result = vwrite(fmt, ap);
        va_end(ap);

        return result;
    }

    /// Write string to file
    int write(const char* s) {
        return (!this->m_file || this->m_cancel)
            ? -1 : this->internal_write(new log_msg_type(s, this->m_allocator));
    }

    /// Write \a s to file asynchronously
    template<typename T>
    int write(const T& s) {
        return (!this->m_file || this->m_cancel)
            ? -1 : this->internal_write(new log_msg_type(s, this->m_allocator));
    }

    /// Formatted write with argumets
    int vwrite(const char* fmt, va_list ap) {
        if (!this->m_file || this->m_cancel) return -1;

        char l_buf[async_logger_traits::write_buf_sz];
        vsnprintf(l_buf, sizeof(l_buf)-1, fmt, ap);
        return this->internal_write(new log_msg_type(l_buf, this->m_allocator));
    }

};

//-----------------------------------------------------------------------------
// I m p l e m e n t a t i o n
//-----------------------------------------------------------------------------

template<typename traits>
int basic_async_logger<traits>::start(const std::string& filename)
{
    if (m_file)
        return -1;

    m_event.reset();

    m_filename  = filename;
    m_head      = NULL;
    m_cancel    = false;

    if (!(m_file = fopen(m_filename.c_str(), "a+"))) {
        print_error(-1, strerror(errno), __LINE__);
        return -2;
    }

    boost::barrier l_synch(2);   // Synchronize with calling thread.

    m_thread = boost::shared_ptr<boost::thread>(
        new boost::thread(
            boost::bind(&basic_async_logger<traits>::run, this, &l_synch)));

    l_synch.wait();
    return 0;
}

template<typename traits>
void basic_async_logger<traits>::stop()
{
    if (!m_file)
        return;

    m_cancel = true;
    ASYNC_TRACE(("Stopping async logger (head %p)\n", m_head));
    m_event.signal();
    if (m_thread)
        m_thread->join();
}

template<typename traits>
void basic_async_logger<traits>::run(boost::barrier* a_barrier)
{
    // Wait until the caller is ready
    a_barrier->wait();

    ASYNC_TRACE(("Started async logging thread (cancel=%s)\n",
        m_cancel ? "true" : "false"));

    static const timespec ts =
        {traits::commit_timeout / 1000, (traits::commit_timeout % 1000) * 1000000 };

    while (1) {
        int n = commit(&ts);

        ASYNC_TRACE(( "Async thread result: %d (head: %p, cancel=%s)\n",
            n, m_head, m_cancel ? "true" : "false" ));

        if (n || (!m_head && m_cancel))
            break;
    }

    ::fclose(m_file);
    m_file = NULL;
}

template<typename traits>
int basic_async_logger<traits>::commit(const struct timespec* tsp)
{
    ASYNC_TRACE(("Committing head: %p\n", m_head));

    int l_old_val = m_event.value();

    while (!m_head) {
        l_old_val = m_event.wait(tsp, &l_old_val);

        if (m_cancel && !m_head)
            return 0;
    }

    static const log_msg_type* l_new_head = NULL;
    const        log_msg_type* l_cur_head;

    // Find current head and reset the old head to be NULL
    do {
        l_cur_head = const_cast<const log_msg_type*>( m_head );
    } while( !atomic::cas(&m_head, l_cur_head, l_new_head) );

    ASYNC_TRACE(( " --> cur head: %p, new head: %p\n", l_cur_head, m_head));

    BOOST_ASSERT(l_cur_head);

    int   count = 0;
    const log_msg_type* l_last = NULL;
    const log_msg_type* l_next = l_cur_head;

    // Reverse the section of this list
    for(const log_msg_type* p = l_cur_head; l_next; count++, p = l_next) {
        ASYNC_TRACE(("Set last[%p] -> next[%p]\n", l_next, l_last));
        l_next = p->next();
        p->next(l_last);
        l_last = p;
    }

    BOOST_ASSERT(l_last);
    ASYNC_TRACE(("Total (%d). Sublist's head: %p (%s)\n", count, l_last, l_last->c_str()));

    if (m_max_queue_size < count)
        m_max_queue_size = count;

    l_next = l_last;

    // Write messages in this sublist to disk in the proper order
    for(const log_msg_type* p = l_last; l_next; p = l_next) {
        if (::fwrite(p->c_str(), p->size(), 1, m_file) < 0)
            return -1;
        l_next = p->next();
        p->next(NULL);
        ASYNC_TRACE(("Wrote (%d bytes): %p (next: %p): %s\n",
            p->size(), p, l_next, p->c_str()));
        delete p;
    }

    if (::fflush(m_file) < 0)
        return -2;
/*
    l_next = l_last;

    for(log_msg_type* p = l_last; l_next != NULL; p = l_next) {
        l_next = p->next();
        delete p;
    }
*/
    return 0;
}

template<typename traits>
int basic_async_logger<traits>::internal_write(const log_msg_type* msg) {
    if (!msg)
        return -9;

    const log_msg_type* l_last_head;

    // Replace the head with msg
    do {
        l_last_head = const_cast<const log_msg_type*>(m_head);
        msg->next( l_last_head );
    } while( !atomic::cas(&m_head, l_last_head, msg) );

    if (!l_last_head)
        m_event.signal();

    ASYNC_TRACE(("internal_write - cur head: %p, prev head: %p\n",
        m_head, l_last_head));

    return 0;
}

//-----------------------------------------------------------------------------
template<typename traits>
void basic_async_logger<traits>::print_error(int a_errno, const char* a_what, int a_line) {
    if (on_error)
        on_error(a_errno, a_what);
    else
        std::cerr << "Error " << a_errno << " writing to file \"" << m_filename << "\": "
                  << a_what << " [" << __FILE__ << ':' << a_line << ']' << std::endl;
}

} // namespace utxx

#endif // _UTXX_ASYNC_FILE_LOGGER_HPP_
