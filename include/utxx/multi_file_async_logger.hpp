//----------------------------------------------------------------------------
/// \file  multi_file_async_logger.cpp
//----------------------------------------------------------------------------
/// \brief Multi-file asynchronous logger
///
/// The logger logs data to multiple streams asynchronously
/// It is optimized for performance of the producer to ensure minimal latency.
/// The producer of log messages never blocks during submission of a message.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Omnibius, LLC
// Author: Serge Aleynikov <saleyn@gmail.com>
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

#include <utxx/config.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/thread_guard.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <utxx/atomic.hpp>
#include <utxx/synch.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/logger.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>

namespace utxx {

#ifdef DEBUG_ASYNC_LOGGER
#   define ASYNC_TRACE(x) printf x
#else
#   define ASYNC_TRACE(x)
#endif

/// Traits of asynchronous logger
struct multi_file_async_logger_traits {
    typedef std::allocator<void>    allocator;
    typedef synch::futex            event_type;
    /// Callback called before writing data to disk. It gives the last opportunity
    /// to rewrite the content written to disk and if necessary to reallocate the
    /// message buffer in a_msg using logger's allocate() and deallocate()
    /// functions.  The returned iovec will be used as the content written to disk.
    typedef boost::function<iovec (const char* a_category, iovec& a_msg)> msg_formatter;

    /// Callback called on writing scattered array of iovec structures to stream
    /// Implementation defaults to writev(3).
    typedef boost::function<int (int a_fd, const char** a_categories,
                                 const iovec* a_data, size_t a_size)> msg_writer;

    enum {
          commit_timeout    = 2000  // commit this number of usec
        , write_buf_sz      = 256   // Max size of the buffer for string formatting
    };
};

/// Multi-stream asynchronous message logger
template<typename traits = multi_file_async_logger_traits>
struct basic_multi_file_async_logger {
    /// Custom destinations (other than regular files) can keep a pointer
    /// to their state associated with file_id structure
    class stream_state_base {};

    typedef typename traits::event_type             event_type;
    typedef typename traits::msg_formatter          msg_formatter;
    typedef typename traits::msg_writer             msg_writer;
    typedef boost::function<
        int (const std::string& name,
             stream_state_base* state,
             std::string& error)
    >                                               stream_opener;
    typedef synch::posix_event                      close_event_type;
    typedef boost::shared_ptr<close_event_type>     close_event_type_ptr;
    typedef typename traits::allocator::template
        rebind<char>::other                         msg_allocator;

    /// Stream information associated with a file descriptor
    /// used internally by the async logger
    struct stream_info;

    class  file_id;

private:
    struct command_t;

    typedef std::vector<stream_info>        stream_info_vec;
    typedef typename traits::allocator::template
        rebind<command_t>::other            cmd_allocator;
    typedef std::map<uint32_t, command_t*>  pending_cmds_map;

    boost::mutex                            m_mutex;
    boost::shared_ptr<boost::thread>        m_thread;
    cmd_allocator                           m_cmd_allocator;
    msg_allocator                           m_msg_allocator;
    volatile command_t*                     m_head;
    bool                                    m_cancel;
    int                                     m_max_queue_size;
    event_type                              m_event;
    long                                    m_active_count;
    stream_info_vec                         m_files;
    int                                     m_last_version;

    static inline int writev(int a_fi, const char** a_categories,
                             const iovec* a_iovec, size_t a_sz)
    {
        return ::writev(a_fi, a_iovec, a_sz);
    }

    // Register a file or a stream with the logger
    file_id internal_register_stream(
        const std::string&  a_name,
        msg_writer          a_writer,
        stream_state_base*  a_state = NULL,
        int                 a_fd    = -1);

    // Invoked by the async thread to flush messages from queue to file
    int  commit(const struct timespec* tsp = NULL);
    // Invoked by the async thread
    void run(boost::barrier* a_barrier);
    // Enqueues msg to internal queue
    int  internal_enqueue(command_t* msg);

    void close();
    void close(stream_info* p, int a_errno = 0);

    command_t* allocate_command(typename command_t::type_t a_tp, int a_fd) {
        command_t* p = m_cmd_allocator.allocate(1);
        new (p) command_t(a_tp, a_fd);
        return p;
    }

    void deallocate_command_list(int a_fd, command_t* p);

    void deallocate_commands(command_t** a_cmds, size_t a_sz) {
        for (size_t i = 0; i < a_sz; i++)
            deallocate_command(a_cmds[i]);
    }

    void deallocate_command(command_t* a_cmd);

    int do_writev_and_free(stream_info& a_fi, command_t* a_cmds[],
                           const char* a_categories[],
                           const iovec* a_wr_iov, size_t a_sz);

    bool check_range(int a_fd) const {
        return likely(a_fd >= 0 && (size_t)a_fd < m_files.size());
    }
    bool check_range(const file_id& a_file) const {
        return check_range(a_file.fd())
            && a_file.version() == m_files[a_file.fd()].version;
    }
    stream_info* get_info(const file_id& a_file) {
        BOOST_ASSERT(check_range(a_file));
        stream_info* si = &m_files[a_file.fd()];
        return (unlikely(m_cancel || si->error || si->fd < 0)) ? NULL : si;
    }
public:
    explicit basic_multi_file_async_logger(
        size_t a_max_files = 1024,
        const msg_allocator& alloc = msg_allocator());

    ~basic_multi_file_async_logger() {
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

    /// Returns true if the async logger's thread is running
    bool running() { return !m_cancel && m_thread; }

    /// Start a new log file
    file_id open_file(const std::string& a_filename, bool a_append = true,
                      int a_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

    /// Start a new logging stream
    ///
    /// The logger won't write any data to file but will call \a a_writer
    /// callback on every iovec to be written to stream. It's the caller's
    /// responsibility to perform the actual writing.
    /// @param a_name   is the name of the stream
    /// @param a_writer a callback executed on writing messages to stream
    /// @param a_state  custom state available to the implementation's writer
    file_id open_stream(const std::string&  a_name,
                        msg_writer          a_writer,
                        stream_state_base*  a_state);

    /// This callback will be called from within the logger's thread
    /// to format a message prior to writing it to a log file. The formatting
    /// function can modify the content of \a a_data and if needed can reallocate
    /// and rewrite \a a_data and \a a_size to point to some other memory. Use
    /// the allocate() and deallocate() functions for this purpose. You should
    /// call this function immediately after calling open_file() and before
    /// writing any messages to it.
    void set_formatter(const file_id& a_id, const msg_formatter& a_formatter);

    /// This callback will be called from within the logger's thread
    /// to write an array of iovec structures to a log file. You should
    /// call this function immediately after calling open_file() and before
    /// writing any messages to it.
    void set_writer(const file_id& a_id, const msg_writer& a_writer);

    /// Set the size of a batch used to write messages to file using on_write
    /// function. Valid value is between 1 and IOV_MAX.
    /// Call this function immediately after calling open_file() and before
    /// writing any messages to it.
    void set_batch_size(const file_id& a_id, size_t a_size);

    /// Close one log file
    /// @param a_id identifier of the file to be closed. After return the value
    ///             will be reset.
    /// @param a_immediate indicates whether to close file immediately or
    ///             first to write all pending data in the file descriptor's
    ///             queue prior to closing it
    /// @param a_notify_when_closed is the event that, if not NULL, will be
    ///             signaled when the file descriptor is eventually closed.
    /// @param a_wait_secs is the number of seconds to wait until the file is
    ///             closed. -1 means indefinite.
    int close_file(file_id& a_id, bool a_immediate = false,
                   int a_wait_secs = -1);

    /// @return last error of a given file
    int last_error(const file_id& a_id) const {
        return check_range(a_id) ? m_files[a_id].error : -1;
    }

    template <class T>
    T* allocate() {
        T* p = reinterpret_cast<T*>(m_msg_allocator.allocate(sizeof(T)));
        ASYNC_TRACE(("+allocate<T>(%lu) -> %p\n", sizeof(T), p));
        return p;
    }

    char* allocate(size_t a_sz) {
        char* p = m_msg_allocator.allocate(a_sz);
        ASYNC_TRACE(("+allocate(%lu) -> %p\n", a_sz, p));
        return p;
    }

    void deallocate(char* a_data, size_t a_size) {
        ASYNC_TRACE(("-Deallocating msg(%p, %lu)\n", a_data, a_size));
        m_msg_allocator.deallocate(a_data, a_size);
    }

    /// Formatted write with argumets.  The a_data must have been
    /// previously allocated using the allocate() function.
    /// The logger will implicitely own the pointer and
    /// will have the deallocation responsibility.
    int write(const file_id& a_file, const char* a_category, void* a_data, size_t a_sz);

    /// Write a copy of the string a_data to a file.
    int write(const file_id& a_file, const char* a_category, const std::string& a_data);

    /// @return max size of the commit queue
    const int max_queue_size()  const { return m_max_queue_size; }

    const int open_files_count() const { return m_active_count; }

};

//-----------------------------------------------------------------------------
// Local classes
//-----------------------------------------------------------------------------

template<typename traits>
struct basic_multi_file_async_logger<traits>::command_t {
    static const int MAX_CAT_LEN = 32;
    enum type_t { msg, close }  type;
    union udata {
        struct {
            iovec data;
            char* category;
        } msg;
        struct {
            bool immediate;
        } close;
    };

    int         fd;
    udata       args;
    command_t*  next;

    command_t(type_t a_type, int a_fd)
        : type(a_type), fd(a_fd), next(NULL)
    {}

    ~command_t() {
        if (type == msg && args.msg.category) {
            free((void*)args.msg.category);
            args.msg.category = NULL;
        }
    }

    void set_category(const char* a_category) {
        if (a_category) {
            args.msg.category = strdup(a_category);
        } else {
            args.msg.category = NULL;
        }
    }
} __attribute__((aligned(CL_SIZE)));

/// Stream information associated with a file descriptor
/// used internally by the async logger
template<typename traits>
struct basic_multi_file_async_logger<traits>::
stream_info {
    std::string          name;
    int                  fd;
    int                  error;
    std::string          error_msg;
    const int            version;       // Version number assigned when file is opened.
    size_t               max_batch_sz;  // Max number of messages to be batched
    close_event_type_ptr on_close;      // Event to signal on close
    msg_formatter        on_format;     // Message "before-write" formatter
    msg_writer           on_write;      // Message writer functor
    stream_state_base*   state;

    stream_info(stream_state_base* a_state = NULL)
        : fd(-1), error(0), version(0), max_batch_sz(IOV_MAX)
        , on_format(&stream_info::def_on_format)
        , on_write(&basic_multi_file_async_logger<traits>::writev)
        , state(a_state)
    {}

    stream_info(const std::string& a_name, int a_fd, int a_version,
                msg_writer a_writer = &basic_multi_file_async_logger<traits>::writev,
                stream_state_base* a_state = NULL)
        : name(a_name), fd(a_fd), error(0), version(a_version), max_batch_sz(IOV_MAX)
        , on_format(&stream_info::def_on_format)
        , on_write(a_writer)
        , state(a_state)
    {}

    static iovec def_on_format(const char* a_category, iovec& a_msg) { return a_msg; }

    void set_error(int a_errno, const char* a_err = NULL) {
        if (a_err)
            error_msg = a_err;
        else {
            char buf[128];
            strerror_r(a_errno, buf, sizeof(buf));
            error_msg = buf;
        }
        error = a_errno;
    }
};

template<typename traits>
class basic_multi_file_async_logger<traits>::
file_id {
    int m_fd;
    int m_version;
public:
    file_id() { reset(); }
    file_id(int a_fd, int a_vsn) : m_fd(a_fd), m_version(a_vsn) {}
    bool invalid()      const { return m_fd < 0; }
    int  version()      const { return m_version; }
    int  fd()           const { return m_fd; }
    void reset()          { m_fd = -1; m_version = 0; }
    bool operator== (const file_id& a) {
        return m_fd == a.m_fd && m_version == a.m_version;
    }

    operator bool() const { return m_fd >= 0; }
};

//-----------------------------------------------------------------------------
// I m p l e m e n t a t i o n
//-----------------------------------------------------------------------------

template<typename traits>
basic_multi_file_async_logger<traits>::
basic_multi_file_async_logger(size_t a_max_files, const msg_allocator& alloc)
    : m_msg_allocator(alloc), m_head(NULL), m_cancel(false)
    , m_max_queue_size(0)
    , m_active_count(0)
    , m_files(a_max_files)
    , m_last_version(0)
{}

template<typename traits>
int basic_multi_file_async_logger<traits>::
start() {
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
void basic_multi_file_async_logger<traits>::
stop() {
    if (!running())
        return;

    ASYNC_TRACE(("Stopping async logger (head %p)\n", m_head));

    if (!m_thread)
        return;

    boost::thread_guard<> g(*m_thread);
    m_cancel = true;
    m_event.signal();
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
run(boost::barrier* a_barrier) {
    // Wait until the caller is ready
    a_barrier->wait();

    ASYNC_TRACE(("Started async logging thread (cancel=%s)\n",
        m_cancel ? "true" : "false"));

    static const timespec ts =
        {traits::commit_timeout / 1000, (traits::commit_timeout % 1000) * 1000000 };

    while (1) {
        int rc = commit(&ts);

        ASYNC_TRACE(( "Async thread commit result: %d (head: %p, cancel=%s)\n",
            rc, m_head, m_cancel ? "true" : "false" ));

        if (rc || (!m_head && m_cancel))
            break;
    }

    ASYNC_TRACE(("Logger loop finished - calling close()\n"));
    close();
    ASYNC_TRACE(("Logger notifying all of exiting\n"));

    m_thread.reset();
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
close() {
    ASYNC_TRACE(("Logger is closing\n"));
    boost::mutex::scoped_lock lock(m_mutex);
    for (typename stream_info_vec::iterator it=m_files.begin(), e=m_files.end();
            it != e; ++it)
        close(&*it);
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
close(stream_info* a_fi, int a_errno) {
    if (!a_fi || a_fi->fd < 0)
        return;

    a_fi->fd = -1;
    ::close(a_fi->fd);
    if (a_errno)
        a_fi->set_error(a_errno);

    a_fi->name.clear();
    atomic::dec(&m_active_count);
    close_event_type_ptr l_on_close = a_fi->on_close;
    ASYNC_TRACE(("close(%d, %d) %s (use_count=%ld) active=%ld\n",
            a_fi->fd, a_fi->error,
            l_on_close ? "notifying caller" : "will NOT notify caller",
            l_on_close.use_count(), m_active_count));
    if (l_on_close) {
        ASYNC_TRACE(("Signaling on_close(%d) event\n", a_fi->fd));
        l_on_close->signal();
        a_fi->on_close.reset();
    }
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
set_formatter(const file_id& a_id, const msg_formatter& a_formatter) {
    BOOST_ASSERT(check_range(a_id));
    m_files[a_id.fd()].on_format =
        a_formatter ? a_formatter : &stream_info::def_on_format;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
set_writer(const file_id& a_id, const msg_writer& a_writer) {
    BOOST_ASSERT(check_range(a_id));
    m_files[a_id.fd()].on_write = a_writer ? a_writer : &writev;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
set_batch_size(const file_id& a_id, size_t a_size) {
    BOOST_ASSERT(check_range(a_id));
    m_files[a_id.fd()].max_batch_sz = a_size < IOV_MAX ? a_size : IOV_MAX;
}

template<typename traits>
typename basic_multi_file_async_logger<traits>::file_id
basic_multi_file_async_logger<traits>::
open_file(const std::string& a_filename, bool a_append, int a_mode) {
    int n = ::open(a_filename.c_str(),
                a_append ? O_CREAT|O_APPEND|O_WRONLY|O_LARGEFILE
                         : O_CREAT|O_WRONLY|O_TRUNC|O_LARGEFILE,
                a_mode);
    return internal_register_stream(a_filename, &writev, NULL, n);
}

template<typename traits>
typename basic_multi_file_async_logger<traits>::file_id
basic_multi_file_async_logger<traits>::
open_stream(const std::string& a_name,
            msg_writer         a_writer,
            stream_state_base* a_state)
{
    // Reserve a file descriptor by allocating a socket
    int n = socket(AF_INET, SOCK_DGRAM, 0);
    return internal_register_stream(a_name, a_writer, a_state, n);
}

template<typename traits>
typename basic_multi_file_async_logger<traits>::file_id
basic_multi_file_async_logger<traits>::
internal_register_stream(
    const std::string&  a_name,
    msg_writer          a_writer,
    stream_state_base*  a_state,
    int                 a_fd)
{
    if (a_fd < 0)
        return file_id(a_fd, 0);
    if (!check_range(a_fd)) {
        int e = errno;
        ::close(a_fd);
        errno = e;
        return file_id(-2, 0);
    }

    boost::mutex::scoped_lock lock(m_mutex);

    stream_info& f = m_files[a_fd];
    f.~stream_info();
    new (&f) stream_info(a_name, a_fd, ++m_last_version, a_writer, a_state);

    atomic::inc(&m_active_count);
    return file_id(a_fd, f.version);
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
close_file(file_id& a_id, bool a_immediate, int a_wait_secs) {
    if (a_id.fd() < 0) return 0;
    stream_info* si = get_info(a_id);
    if (unlikely(!si || si->fd < 0)) {
        a_id.reset();
        return 0;
    }

    if (!m_thread) {
        close(si, 0);
        a_id.reset();
        return 0;
    }

    if (!si->on_close)
        si->on_close.reset(new close_event_type());
    close_event_type_ptr l_event = si->on_close;

    int l_event_val = l_event ? l_event->value() : 0;

    command_t* l_cmd = allocate_command(command_t::close, a_id.fd());
    l_cmd->args.close.immediate = a_immediate;
    int n = internal_enqueue(l_cmd);

    if (!n && l_event) {
        ASYNC_TRACE(( "close_file(%d) is waiting for ack (%d)\n", a_id.fd(), a_wait_secs));
        if (m_thread && si->fd >= 0) {
            if (a_wait_secs) {
                boost::posix_time::ptime l_now(
                    boost::posix_time::microsec_clock::universal_time() +
                    boost::posix_time::seconds(a_wait_secs));
                n = l_event->wait(l_now, &l_event_val);
            } else {
                n = l_event->wait(&l_event_val);
            }
            ASYNC_TRACE(( "close_file(%d) ack received %d (err=%d)\n",
                    a_id.fd(), n, si->error));
        }
    } else {
        ASYNC_TRACE(( "close_file(%d) failed to enqueue cmd or no event (n=%d, %s)\n",
                a_id.fd(), n, (l_event ? "true" : "false")));
    }
    a_id.reset();
    return n;
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
write(const file_id& a_file, const char* a_category, void* a_data, size_t a_sz) {
    if (unlikely(!get_info(a_file) || m_cancel))
        return -1;

    command_t* p = allocate_command(command_t::msg, a_file.fd());
    ASYNC_TRACE(("->write(%p, %lu) - no copy\n", a_data, a_sz));
    p->set_category(a_category);
    p->args.msg.data.iov_base = a_data;
    p->args.msg.data.iov_len  = a_sz;
    return internal_enqueue(p);
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
write(const file_id& a_file, const char* a_category, const std::string& a_data) {
    if (unlikely(!get_info(a_file) || m_cancel))
        return -1;

    command_t* p = allocate_command(command_t::msg, a_file.fd());

    char* q = allocate(a_data.size());
    memcpy(q, a_data.c_str(), a_data.size());
    p->set_category(a_category);
    p->args.msg.data.iov_base = q;
    p->args.msg.data.iov_len  = a_data.size();
    ASYNC_TRACE(("->write(%p, %lu) - allocated copy\n", q, a_data.size()));
    return internal_enqueue(p);
}


template<typename traits>
int basic_multi_file_async_logger<traits>::
do_writev_and_free(stream_info& a_fi, command_t* a_cmds[],
                   const char** a_categories,
                   const iovec* a_vec, size_t a_sz)
{
    int n = a_sz ? a_fi.on_write(a_fi.fd, a_categories, a_vec, a_sz) : 0;
    if (n < 0) {
        a_fi.set_error(errno);
        LOG_ERROR(("Error writing data to file %s: (%d) %s\n",
                   a_fi.name.c_str(), a_sz, a_sz
                      ? to_bin_string((char*)a_vec[0].iov_base, a_vec[0].iov_len).c_str()
                      : ""));
    }
    deallocate_commands(a_cmds, a_sz);
    return n;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
deallocate_command(command_t* a_cmd) {
    switch (a_cmd->type) {
        case command_t::msg:
            deallocate(
                static_cast<char*>(a_cmd->args.msg.data.iov_base),
                a_cmd->args.msg.data.iov_len);

            break;
        default:
            break;
    }
    a_cmd->~command_t();
    m_cmd_allocator.deallocate(a_cmd, 1);
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
deallocate_command_list(int a_fd, command_t* p) {
    if (!p) return;
    for (command_t* next = p->next; p; p = next) {
        ASYNC_TRACE(("FD=%d, deallocating unprocessed message %p\n", a_fd, p));
        deallocate_command(p);
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
        int l_fd                    = it->first;
        command_t* p                = it->second;
        bool l_fd_close_immediate   = false;
        bool l_fd_pending_close     = false;

        if (!check_range(l_fd)) {
            deallocate_command_list(l_fd, p);
            continue;
        }

        stream_info* si             = &m_files[l_fd];
        msg_formatter& ffmt     = si->on_format;

        ASYNC_TRACE(("Processing commands for fd=%d\n", l_fd));

        struct iovec      iov [si->max_batch_sz];  // Contains pointers to write
        struct command_t* cmds[si->max_batch_sz];
        const  char*      cats[si->max_batch_sz];  // List of message categories
        size_t n = 0, sz = 0;

        BOOST_ASSERT(p);

        do {
            command_t* l_next = p->next;
            switch (p->type) {
                case command_t::msg: {
                    iov[n]  = ffmt(p->args.msg.category, p->args.msg.data);
                    cmds[n] = p;
                    cats[n] = p->args.msg.category;
                    ASYNC_TRACE(("FD=%d, Command %lu address %p write(%p, %lu) free(%p, %lu)\n",
                            l_fd, n, cmds[n], iov[n].iov_base, iov[n].iov_len,
                            p->args.msg.iov_base, p->args.msg.iov_len));
                    sz += iov[n].iov_len;
                    if (++n == si->max_batch_sz || p == NULL) {
                        #ifdef DEBUG_ASYNC_LOGGER
                        int k =
                        #endif
                        do_writev_and_free(*si, cmds, cats, iov, n);
                        ASYNC_TRACE(("Written %d bytes to %s\n", k,
                               si->name.c_str()));
                        n = 0;
                    }
                    break;
                }
                case command_t::close: {
                    l_fd_close_immediate    = p->args.close.immediate;
                    l_fd_pending_close      = !l_fd_close_immediate;
                    ASYNC_TRACE(("FD=%d, Command %lu address %p (close)\n", l_fd, n, p));
                    deallocate_command(p);
                    if (l_fd_close_immediate) {
                        if (n > 0 || l_next)
                            LOG_WARNING((
                                "Requested to close file '%s' immediately "
                                "while unwritten data still exists in memory!",
                                si->name.c_str()));
                        close(si, ECANCELED);
                    }
                    break;
                }
                default:
                    BOOST_ASSERT(false);
                    break;
            }
            p = l_next;
        } while (p && !l_fd_close_immediate && !si->error);

        if (!si->error || l_fd_pending_close) {
            int ec;
            if (n > 0) {
                int k = do_writev_and_free(*si, cmds, cats, iov, n);
                ASYNC_TRACE(("Written %d bytes to (fd=%d) %s (total = %lu)\n", k,
                       l_fd, si->name.c_str(), sz));
                ec = k < 0 ? si->error : 0;
            } else {
                ec = 0;
                ASYNC_TRACE(("Written total %lu bytes to (fd=%d) %s\n", sz,
                       l_fd, si->name.c_str()));
            }

            if (l_fd_pending_close) {
                ASYNC_TRACE(("FD=%d, processing pending close\n", l_fd));
                ec = ECANCELED;
            }

            if (ec)
                close(si, ec);

            //fsync(l_fd);
        } else
            deallocate_commands(cmds, n);

        if (p) {
            BOOST_ASSERT(l_fd_close_immediate || si->error);
            deallocate_command_list(l_fd, p);
        }
    }
    return 0;
}

} // namespace utxx

#endif // _UTXX_ASYNC_FILE_LOGGER_HPP_
