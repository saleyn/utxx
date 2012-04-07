//----------------------------------------------------------------------------
/// \file  async_file_logger.cpp
//----------------------------------------------------------------------------
/// \brief Asynchronous file logger
//----------------------------------------------------------------------------
// Copyright (c) 2012 Omnibius, LLC
// Author: Serge Aleynikov <saleyn@gmail.com>
// Created: 2012-03-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of util open-source projects.

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

#ifndef _UTIL_ASYNC_FILE_LOGGER_HPP_
#define _UTIL_ASYNC_FILE_LOGGER_HPP_

#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <util/atomic.hpp>
#include <util/synch.hpp>
#include <util/compiler_hints.hpp>
#include <util/logger.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>

namespace util {

#ifdef DEBUG_ASYNC_LOGGER
#   define ASYNC_TRACE(x) printf x
#else
#   define ASYNC_TRACE(x)
#endif

/// Traits of asynchronous logger
struct multi_file_async_logger_traits {
    typedef std::allocator<void>    allocator;
    typedef synch::futex            event_type;

    enum {
          commit_timeout    = 2000  // commit this number of usec
        , write_buf_sz      = 256   // Max size of the buffer for string formatting
    };
};

/// Asynchronous logger of text messages.
template<typename traits = multi_file_async_logger_traits>
struct basic_multi_file_async_logger {
    struct file_info {
        std::string name;
        int         fd;
        int         error;
        int         version;  // Version number assigned when file is opened.
    };

    class file_id {
        int m_fd;
        int m_version;
    public:
        file_id() : m_fd(-1), m_version(0) {}
        file_id(int a_fd, int a_vsn) : m_fd(a_fd), m_version(a_vsn) {}
        bool invalid()  const { return m_fd < 0; }
        int  version()  const { return m_version; }
        int  fd()       const { return m_fd; }
        bool operator== (const file_id& a) {
            return m_fd == a.m_fd && m_version == a.m_version;
        }
    };

private:
    struct command_t {
        enum type_t { msg, close } type;
        int fd;
        union {
            struct { void* data; size_t size; } msg;
            struct { bool immediate;          } close;
        } args;
        command_t* next;

        command_t(type_t a_type, int a_fd) : type(a_type), fd(a_fd), next(NULL) {}
    };


    typedef typename traits::allocator::template
        rebind<char>::other                 msg_allocator;
    typedef typename traits::allocator::template
        rebind<command_t>::other            cmd_allocator;

    typedef std::vector<file_info>          file_info_vec;
    typedef std::map<uint32_t, command_t*>  pending_cmds_map;

    typedef typename traits::event_type event_type;

    boost::mutex                        m_mutex;
    boost::condition_variable           m_stop_condition;
    boost::shared_ptr<boost::thread>    m_thread;
    cmd_allocator                       m_cmd_allocator;
    msg_allocator                       m_msg_allocator;
    volatile command_t*                 m_head;
    bool                                m_cancel;
    int                                 m_max_queue_size;
    event_type                          m_event;
    int                                 m_active_count;
    file_info_vec                       m_files;
    int                                 m_last_version;

    // Invoked by the async thread to flush messages from queue to file
    int  commit(const struct timespec* tsp = NULL);
    // Invoked by the async thread
    void run(boost::barrier* a_barrier);
    // Enqueues msg to internal queue
    int  internal_enqueue(command_t* msg);

    void close();

    void deallocate_commands(command_t** a_cmds, size_t a_sz) {
        for (size_t i = 0; i < a_sz; i++)
            deallocate_command(a_cmds[i]);
    }

    void deallocate_command(command_t* a_cmd);

    int do_writev_and_free(int a_fd, command_t* a_cmds[],
        const struct iovec* a_iov, size_t a_sz);

public:
    explicit basic_multi_file_async_logger(
        size_t a_max_files = 1024,
        const msg_allocator& alloc = msg_allocator())
        : m_msg_allocator(alloc), m_head(NULL), m_cancel(false)
        , m_max_queue_size(0)
        , m_active_count(0)
        , m_files(a_max_files)
        , m_last_version(0)
    {
        for (size_t i = 0; i < m_files.size(); i++) {
            m_files[i].fd      = -1;
            m_files[i].error   = 0;
            m_files[i].version = 0;
        }
    }

    ~basic_multi_file_async_logger() {
        if (m_thread)
            stop();
    }

    /// Initialize and start asynchronous file writer
    /// @param filename     - name of the file
    /// @param max_msg_size - expected maximum message size (used to allocate
    ///                       the buffer on the stack to format the
    ///                       output string when calling fwrite function)
    /// @param mode         - set file access mode
    int start();

    /// Stop asynchronous file writering thread
    void stop();

    /// Start a new log file
    file_id open_file(const std::string& a_filename, bool a_append = true,
                      int a_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

    /// Close one log file
    int close_file(const file_id& a_id, bool a_immediate = false);

    template <class T>
    T* allocate() {
        return reinterpret_cast<T*>(m_msg_allocator.allocate(sizeof(T)));
    }

    char* allocate(size_t a_sz) {
        return m_msg_allocator.allocate(a_sz);
    }

    /// Formatted write with argumets.  The a_data must have been
    /// previously allocated using the allocate() function.
    /// The logger will implicitely assume deallocation responsibility.
    int write(size_t a_fd, void* a_data, size_t a_sz);

    /// @return max size of the commit queue
    const int max_queue_size()  const { return m_max_queue_size; }

};

//-----------------------------------------------------------------------------
// I m p l e m e n t a t i o n
//-----------------------------------------------------------------------------

template<typename traits>
int basic_multi_file_async_logger<traits>::start()
{
    if (m_thread)
        return -1;

    m_event.reset();

    m_head      = NULL;
    m_cancel    = false;

    boost::barrier l_synch(2);   // Synchronize with calling thread.

    m_thread = boost::shared_ptr<boost::thread>(
        new boost::thread(
            boost::bind(&basic_multi_file_async_logger<traits>::run, this, &l_synch)));

    l_synch.wait();
    return 0;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::stop()
{
    if (!m_thread || m_cancel)
        return;
    m_cancel = true;
    ASYNC_TRACE(("Stopping async logger (head %p)\n", m_head));
    m_event.signal();
    if (m_thread) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_stop_condition.wait(lock);
    }
}

template<typename traits>
void basic_multi_file_async_logger<traits>::run(boost::barrier* a_barrier)
{
    // Wait until the caller is ready
    a_barrier->wait();

    ASYNC_TRACE(("Started async logging thread (cancel=%s)\n",
        m_cancel ? "true" : "false"));

    static const timespec ts =
        {traits::commit_timeout / 1000, (traits::commit_timeout % 1000) * 1000000 };

    while (1) {
        int n = commit(&ts);

        ASYNC_TRACE(( "Async thread commit result: %d (head: %p, cancel=%s)\n",
            n, m_head, m_cancel ? "true" : "false" ));

        if (n || (!m_head && m_cancel))
            break;
    }

    close();
    m_stop_condition.notify_all();
}

template<typename traits>
void basic_multi_file_async_logger<traits>::close() {
    for (typename file_info_vec::iterator it=m_files.begin(), e=m_files.end();
            it != e; ++it)
        if (it->fd >= 0) {
            ::close(it->fd);
            it->error = errno;
        }
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
internal_enqueue(command_t* a_msg) {
    BOOST_ASSERT(a_msg);

    command_t* l_last_head;

    // Replace the head with msg
    do {
        l_last_head = const_cast<command_t*>(m_head);
        a_msg->next = l_last_head;
    } while( !atomic::cas(&m_head, l_last_head, a_msg) );

    if (!l_last_head)
        m_event.signal();

    ASYNC_TRACE(("internal_enqueue - cur head: %p, prev head: %p\n",
        m_head, l_last_head));

    return 0;
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
write(size_t a_fd, void* a_data, size_t a_sz) {
    BOOST_ASSERT(a_fd < m_files.size());
    if (m_cancel || unlikely(m_files[a_fd].error || m_files[a_fd].fd < 0))
        return -1;

    command_t* p = m_cmd_allocator.allocate(1);
    new (p) command_t(command_t::msg, a_fd);
    p->args.msg.data = a_data;
    p->args.msg.size = a_sz;
    return internal_enqueue(p);
}

template<typename traits>
typename basic_multi_file_async_logger<traits>::file_id
basic_multi_file_async_logger<traits>::
open_file(const std::string& a_filename, bool a_append, int a_mode) {
    boost::mutex::scoped_lock lock(m_mutex);
    int n = ::open(a_filename.c_str(),
                a_append ? O_CREAT|O_APPEND|O_WRONLY|O_LARGEFILE
                         : O_CREAT|O_WRONLY|O_TRUNC|O_LARGEFILE,
                a_mode);
    if (n < 0)
        return file_id(n, 0);
    if ((size_t)n > m_files.size()) {
        ::close(n);
        return file_id(-2, 0);
    }
    file_info& f = m_files[n];
    f.name  = a_filename;
    f.fd    = n;
    f.error = 0;
    f.version++;
    m_active_count++;
    m_last_version++;
    return file_id(n, m_last_version);
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
close_file(const file_id& a_id, bool a_immediate) {
    if (a_id.fd >= m_files.size())
        return -1;
    if (m_files[a_id.fd()].version != a_id.version())
        return -2;
    if (m_files[a_id.fd()].fd < 0)
        return 0;
    command_t* l_cmd = m_cmd_allocator.allocate(1);
    new (l_cmd) command_t(command_t::close, a_id.fd());
    l_cmd->args.close.immediate = a_immediate;
    return 0;
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
do_writev_and_free(int a_fd, command_t* a_cmds[], const iovec* a_iov, size_t a_sz) {
    int n = ::writev(a_fd, a_iov, a_sz);
    if (n < 0) {
        m_files[a_fd].error = errno;
        LOG_ERROR(("Error writing data to file %s: (%d) %s\n",
            m_files[a_fd].name.c_str(), n, strerror(m_files[a_fd].error)));
    }
    deallocate_commands(a_cmds, a_sz);
    return n;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
deallocate_command(command_t* a_cmd) {
    switch (a_cmd->type) {
        case command_t::msg:
            m_msg_allocator.deallocate(
                static_cast<char*>(a_cmd->args.msg.data), a_cmd->args.msg.size);
            break;
        default:
            return;
    }
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
commit(const struct timespec* tsp)
{
    ASYNC_TRACE(("Committing head: %p\n", m_head));

    int l_old_val = m_event.value();

    while (!m_head) {
        l_old_val = m_event.wait(tsp, &l_old_val);

        if (m_cancel && !m_head)
            return 0;
    }

    command_t* l_cur_head;

    // Find current head and reset the old head to be NULL
    do {
        l_cur_head = const_cast<command_t*>( m_head );
    } while( !atomic::cas(&m_head, l_cur_head, static_cast<command_t*>(NULL)) );

    ASYNC_TRACE(( " --> cur head: %p, new head: %p\n", l_cur_head, m_head));

    BOOST_ASSERT(l_cur_head);

    command_t* l_next  = l_cur_head;
    int        l_count = 0;

    pending_cmds_map l_queue;

    // Reverse the section of this list and place commands in the queues
    // of individual fds.
    for(command_t* p = l_cur_head; l_next; l_count++, p = l_next) {
        l_next = p->next;
        std::pair<typename pending_cmds_map::iterator, bool> res =
            l_queue.insert(std::make_pair(p->fd, p));
        // If inserted new command for this fd, it's the head of the list
        p->next = res.second ? NULL : res.first->second;
        res.first->second = p;
        ASYNC_TRACE(("Set fd[%d].head(%p)->next(%p)\n", p->fd, p, p->next));
    }
 
    // Process each fd's command queue
    ASYNC_TRACE(("Total (%d).\n", l_count));

    if (m_max_queue_size < l_count)
        m_max_queue_size = l_count;

    for(typename pending_cmds_map::iterator it = l_queue.begin(), e = l_queue.end();
            it != e; ++it) {
        int l_fd     = it->first;
        command_t* p = it->second;

        ASYNC_TRACE(("Processing commands for fd=%d\n", l_fd));

        struct iovec      iov [IOV_MAX];
        struct command_t* cmds[IOV_MAX];
        size_t n = 0, sz = 0;

        BOOST_ASSERT(p);

        do {
            command_t* l_next = p->next;
            switch (p->type) {
                case command_t::msg: {
                    size_t k = p->args.msg.size;
                    iov[n].iov_base = p->args.msg.data;
                    iov[n].iov_len  = k;
                    cmds[n]         = p;
                    ASYNC_TRACE(("Command %lu address %p\n", n, cmds[n]));
                    sz += k;
                    if (++n == IOV_MAX || p == NULL) {
                        k = do_writev_and_free(l_fd, cmds, iov, n);
                        ASYNC_TRACE(("Written %lu bytes to %s\n", k,
                               m_files[l_fd].name.c_str())); 
                        n = 0;
                    }
                    break;
                }
                case command_t::close: {
                    bool l_immediate = p->args.close.immediate;
                    deallocate_command(p);
                    if (l_immediate && (n > 0 || l_next)) {
                        LOG_WARNING((
                            "Requested to close file '%s' immediately "
                            "while unwritten data still exists in memory!",
                            m_files[l_fd].name.c_str()));
                        ::close(l_fd);
                    }
                    m_files[l_fd].error = ECANCELED;
                    break;
                }
                default:
                    BOOST_ASSERT(false);
                    break;
            }        
            p = l_next;
        } while (p);

        if (!m_files[l_fd].error) {
            if (n > 0) {
                n = do_writev_and_free(l_fd, cmds, iov, n);
                ASYNC_TRACE(("Written %lu bytes to (fd=%d) %s (total = %lu)\n", n,
                       l_fd, m_files[l_fd].name.c_str(), sz)); 
            } else {
                ASYNC_TRACE(("Written total %lu bytes to (fd=%d) %s\n", sz,
                       l_fd, m_files[l_fd].name.c_str())); 
            }

            //fsync(l_fd);
        } else
            deallocate_commands(cmds, n);
    }
    return 0;
}

} // namespace util

#endif // _UTIL_ASYNC_FILE_LOGGER_HPP_
