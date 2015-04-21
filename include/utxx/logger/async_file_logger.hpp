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
#pragma once

#include <utxx/atomic.hpp>
#include <utxx/synch.hpp>
#include <iostream>
#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace utxx {

#ifdef DEBUG_ASYNC_LOGGER
#   define ASYNC_TRACE(...) printf(__VA_ARGS__);
#else
#   define ASYNC_TRACE(...)
#endif

//-----------------------------------------------------------------------------
/// Traits of asynchronous logger using file stream writing
//-----------------------------------------------------------------------------
struct async_file_logger_traits {
    using allocator  = std::allocator<char>;
    using event_type = futex;
    using file_type  = FILE*;
    static constexpr const file_type s_null_file_type = nullptr;

    //-------------------------------------------------------------------------
    /// Represents a text message to be logged by the logger
    //-------------------------------------------------------------------------
    class msg_type {
        size_t m_size;
        char*  m_data;
    public:
        explicit msg_type(const char* a_data, size_t a_size)
            : m_size(a_size)
            , m_data(const_cast<char*>(a_data))
        {}

        bool        empty() const { return m_size; }
        size_t      size()  const { return m_size; }
        const char* data()  const { return m_data; }
        char*       data()        { return m_data; }
    };

    static file_type file_open(const std::string& a_filename) {
        return fopen(a_filename.c_str(), "a+");
    };

    static int file_write(file_type a_fd, const char* a_data, size_t a_sz) {
        return fwrite(a_data, a_sz, 1, a_fd);
    };

    static int file_close(file_type a_fd) { return fclose(a_fd); }
    static int file_flush(file_type a_fd) { return fflush(a_fd); }

    enum {
          commit_timeout = 1000  // commit interval in usecs
        , write_buf_sz   = 256
    };
};

//-----------------------------------------------------------------------------
/// Traits of asynchronous logger using direct file I/O writing
//-----------------------------------------------------------------------------
struct async_fd_logger_traits : public async_file_logger_traits {
    using file_type  = int;
    static constexpr const file_type s_null_file_type = -1;

    static file_type file_open(const std::string& a_filename) {
        return ::open(a_filename.c_str(), O_CREAT | O_APPEND | O_RDWR);
    };

    static int file_write(file_type a_fd, const char* a_data, size_t a_sz) {
        return ::write(a_fd, a_data, a_sz);
    };

    static int file_close(file_type a_fd) { return ::close(a_fd); }
    static int file_flush(file_type a_fd) { return 0; }

    enum {
          commit_timeout = 1000  // commit interval in usecs
        , write_buf_sz   = 256
    };
};

//-----------------------------------------------------------------------------
/// Asynchronous logger of text messages.
//-----------------------------------------------------------------------------
template<typename traits = async_file_logger_traits>
class basic_async_logger {
protected:
    class cons: public traits::msg_type {
        using base      = typename traits::msg_type;
        using allocator = typename traits::allocator;
    public:
        cons(const char* a_data, size_t a_sz) : base(a_data, a_sz) {}

        cons* next()        { return m_next; }
        void  next(cons* p) { m_next = p;    }
    private:
        cons* m_next = nullptr;
    };

    using allocator    = typename traits::allocator;
    using event_type   = typename traits::event_type;
    using log_msg_type = cons;
    using file_type    = typename traits::file_type;

    std::unique_ptr<std::thread> m_thread;
    allocator                    m_allocator;
    file_type                    m_file;
    std::atomic<log_msg_type*>   m_head;
    bool                         m_cancel;
    int                          m_max_queue_size;
    std::string                  m_filename;
    event_type                   m_event;
    bool                         m_notify_immediate;

    // Invoked by the async thread to flush messages from queue to file
    int  commit(const struct timespec* tsp = NULL);
    // Invoked by the async thread
    void run(std::condition_variable* a_cv);
    // Print error message
    void print_error(int, const char* what, int line);

public:
    explicit basic_async_logger(const allocator& alloc = allocator())
        : m_allocator(alloc)
        , m_file(traits::s_null_file_type)
        , m_head(nullptr)
        , m_cancel(false)
        , m_max_queue_size(0)
        , m_notify_immediate(true)
    {}

    ~basic_async_logger() {
        if (m_file != traits::s_null_file_type)
            stop();
    }

    /// Initialize and start asynchronous file writer
    /// @param a_filename           - name of the file
    /// @param a_notify_immediate   - whether to notify the I/O thread on write
    int  start(const std::string& filename, bool a_notify_immediate = true);

    /// Stop asynchronous file writering thread
    void stop();

    /// @return name of the log file
    const std::string&  filename()         const { return m_filename;         }
    /// @return max size of the commit queue
    const int           max_queue_size()   const { return m_max_queue_size;   }
    /// @return indicates if async thread must be notified immediately
    /// when message is written to queue
    int                 notify_immediate() const { return m_notify_immediate; }

    /// Allocate a message of size \a a_sz.
    /// The content of the message is accessible via its data() property
    log_msg_type* allocate(size_t a_sz);
    /// Deallocate an object previously allocated by call to allocate().
    void  deallocate(log_msg_type* a_msg);

    /// Write a message to the logger by making of copy of
    int   write_copy(const void* a_data, size_t a_sz);
    // Enqueues msg to internal queue
    int   write(log_msg_type* msg);

    /// Callback to be called on file I/O error
    std::function<void (int, const char*)> on_error;
};

//-----------------------------------------------------------------------------
/// Asynchronous text logger
//-----------------------------------------------------------------------------
template <class Traits = async_file_logger_traits>
struct text_file_logger: public basic_async_logger<Traits> {
    using base          = basic_async_logger<Traits>;
    using log_msg_type  = typename base::log_msg_type;
    using allocator     = typename base::allocator;

    explicit text_file_logger(const allocator& alloc = allocator())
        : base(alloc)
    {}

    /// Formatted write with arguments 
    int fwrite(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        int result = vwrite(fmt, ap);
        va_end(ap);

        return result;
    }

    /// Write string to file
    int write(const char* a) {
        return (!this->m_file || this->m_cancel)
            ? -1 : this->write_copy((void*)a, strlen(a));
    }

    /// Write \a s to file asynchronously
    int write(const std::string& a) {
        return (!this->m_file || this->m_cancel)
            ? -1 : this->write_copy((void*)a.c_str(), a.size());
    }

    /// Formatted write with argumets
    int vwrite(const char* fmt, va_list ap) {
        if (!this->m_file || this->m_cancel)
            return -1;

        char buf[Traits::write_buf_sz];
        int n = vsnprintf(buf, sizeof(buf)-1, fmt, ap);
        return this->write_copy(buf, n);
    }
};

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

template<typename traits>
int basic_async_logger<traits>::
start(const std::string& a_filename, bool a_notify_immediate)
{
    if (m_file)
        return -1;

    m_event.reset();

    m_filename          = a_filename;
    m_head              = nullptr;
    m_cancel            = false;
    m_notify_immediate  = a_notify_immediate;

    if (!(m_file = traits::file_open(m_filename))) {
        print_error(-1, strerror(errno), __LINE__);
        return -2;
    }

    std::condition_variable      cv;
    std::mutex                   mtx;
    std::unique_lock<std::mutex> guard(mtx);

    m_thread.reset(new std::thread([this, &cv]() { run(&cv); }));

    cv.wait(guard);
    return 0;
}

//-----------------------------------------------------------------------------
template<typename traits>
void basic_async_logger<traits>::stop()
{
    if (m_file == traits::s_null_file_type)
        return;

    m_cancel = true;
    ASYNC_TRACE("Stopping async logger (head %p)\n", m_head);
    m_event.signal();
    if (m_thread) {
        m_thread->join();
        m_thread.reset();
    }
}

//-----------------------------------------------------------------------------
template<typename traits>
void basic_async_logger<traits>::run(std::condition_variable* a_cv)
{
    // Notify the caller this thread is ready
    a_cv->notify_one();

    ASYNC_TRACE("Started async logging thread (cancel=%s)\n",
        m_cancel ? "true" : "false");

    static const timespec ts{
        traits::commit_timeout / 1000,
       (traits::commit_timeout % 1000) * 1000000
    };

    while (true) {
        int n = commit(&ts);

        ASYNC_TRACE("Async thread result: %d (head=%p, cancel=%s)\n",
            n, m_head, m_cancel ? "true" : "false" );

        if (n || (!m_head && m_cancel))
            break;
    }

    traits::file_close(m_file);
    m_file = traits::s_null_file_type;
}

//-----------------------------------------------------------------------------
template<typename traits>
int basic_async_logger<traits>::commit(const struct timespec* tsp)
{
    ASYNC_TRACE("Committing head: %p\n", m_head);

    int old_val = m_event.value();

    while (!m_head.load(std::memory_order_relaxed)) {
        m_event.wait(tsp, &old_val);

        if (m_cancel && !m_head.load(std::memory_order_relaxed))
            return 0;
    }

    log_msg_type* cur_head;

    do {
        cur_head = m_head.load(std::memory_order_relaxed);
    } while (!m_head.compare_exchange_weak(cur_head, nullptr,
                                           std::memory_order_release,
                                           std::memory_order_relaxed));

    ASYNC_TRACE(" --> cur head: %p, new head: %p\n", cur_head, m_head);

    assert(cur_head);

    int   count = 0;
    log_msg_type* last = nullptr;
    log_msg_type* next = cur_head;

    // Reverse the section of this list
    for (auto p = cur_head; next; count++, p = next) {
        ASYNC_TRACE("Set last[%p] -> next[%p]\n", next, last);
        next = p->next();
        p->next(last);
        last = p;
    }

    assert(last);
    ASYNC_TRACE("Total (%d). Sublist's head: %p (%s)\n",
                count, last, last->c_str());

    if (m_max_queue_size < count)
        m_max_queue_size = count;

    next = last;

    for (auto p = last; next; p = next) {
        if (traits::file_write(m_file, p->data(), p->size()) < 0)
            return -1;
        next = p->next();
        p->next(nullptr);
        ASYNC_TRACE("Wrote (%ld bytes): %p (next: %p): %s\n",
                    p->size(), p, next, p->c_str());
        deallocate(p);
    }

    if (traits::file_flush(m_file) < 0)
        return -2;

    return 0;
}

//-----------------------------------------------------------------------------
template<typename traits>
inline typename basic_async_logger<traits>::log_msg_type*
basic_async_logger<traits>::
allocate(size_t  a_sz)
{
    auto   size  = sizeof(log_msg_type) + a_sz;
    char*  p     = m_allocator.allocate(size);
    char*  data  = p + sizeof(log_msg_type);
    auto   q     = reinterpret_cast<log_msg_type*>(p);
    new   (q) log_msg_type(data, a_sz);
    return q;
}

//-----------------------------------------------------------------------------
template<typename traits>
inline void basic_async_logger<traits>::
deallocate(log_msg_type* a_msg)
{
    auto   size  = sizeof(log_msg_type) + a_msg->size();
    a_msg->~log_msg_type();
    m_allocator.deallocate(reinterpret_cast<char*>(a_msg), size);
}

//-----------------------------------------------------------------------------
template<typename traits>
inline int basic_async_logger<traits>::
write_copy(const void* a_data, size_t a_sz)
{
    auto msg = allocate(a_sz);
    memcpy(msg->data(), a_data, a_sz);
    return write(msg);
}

//-----------------------------------------------------------------------------
template<typename traits>
inline int basic_async_logger<traits>::write(log_msg_type* msg)
{
    if (!msg)
        return -3;

    int n = msg->size();

    log_msg_type* last_head;

    do {
        last_head = m_head.load(std::memory_order_relaxed);
        msg->next(last_head);
    } while (!m_head.compare_exchange_weak(last_head, msg,
                                           std::memory_order_release,
                                           std::memory_order_relaxed));
    if (!last_head && m_notify_immediate)
        m_event.signal();

    ASYNC_TRACE("write - cur head: %p, prev head: %p\n",
                m_head, last_head);
    return n;
}

//-----------------------------------------------------------------------------
template<typename traits>
void basic_async_logger<traits>::
print_error(int a_errno, const char* a_what, int a_line) {
    if (on_error)
        on_error(a_errno, a_what);
    else
        std::cerr << "Error " << a_errno << " writing to file \""  << m_filename
                  << "\": "   << a_what << " [" << __FILE__ << ':' << a_line
                  << ']'      << std::endl;
}

} // namespace utxx
