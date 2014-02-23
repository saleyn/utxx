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
#include <boost/pool/pool_alloc.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/thread_guard.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/concept_check.hpp>
#include <utxx/atomic.hpp>
#include <utxx/synch.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/time_val.hpp>
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

namespace boost {
template<class Signature >
class function;
}

namespace utxx {

#ifdef DEBUG_ASYNC_LOGGER
#   define ASYNC_TRACE(x) do { printf x; fflush(stdout); } while(0)
#else
#   define ASYNC_TRACE(x)
#endif

/// Traits of asynchronous logger
struct multi_file_async_logger_traits {
    typedef boost::fast_pool_allocator<void>    allocator;
    typedef synch::futex                        event_type;
    enum {
          commit_timeout    = 2000  // commit this number of usec
        , write_buf_sz      = 256   // Max size of the buffer for string formatting
    };
};

/// Multi-stream asynchronous message logger
template<typename traits = multi_file_async_logger_traits>
struct basic_multi_file_async_logger {
    /// Command sent to basic_multi_file_async_logger by message producers
    struct command_t;

    /// Stream information associated with a file descriptor
    /// used internally by the async logger
    struct  stream_info;

    /// Internal stream identifier
    class   file_id;

    /// Custom destinations (other than regular files) can keep a pointer
    /// to their state associated with file_id structure
    class   stream_state_base {};

    /// Callback called before writing data to disk. It gives the last opportunity
    /// to rewrite the content written to disk and if necessary to reallocate the
    /// message buffer in a_msg using logger's allocate() and deallocate()
    /// functions.  The returned iovec will be used as the content written to disk.
    typedef boost::function<
        iovec (const std::string& a_category, iovec& a_msg)
    >                                               msg_formatter;

    /// Callback called on writing scattered array of iovec structures to stream
    /// Implementation defaults to writev(3). On success the function number of
    /// bytes written or -1 or error.
    typedef boost::function<
        int (stream_info& a_si, const char** a_categories,
             const iovec* a_data, size_t a_size)
    >                                               msg_writer;

    /// Callback executed when stream needs to be reconnected
    typedef boost::function<int(stream_info& a_si)> stream_reconnecter;

    typedef typename traits::event_type             event_type;
    typedef boost::function<
        int (const std::string& name,
             stream_state_base* state,
             std::string& error)
    >                                               stream_opener;
    typedef synch::posix_event                      close_event_type;
    typedef boost::shared_ptr<close_event_type>     close_event_type_ptr;
    typedef typename traits::allocator::template
        rebind<char>::other                         msg_allocator;

private:
    struct stream_info_eq {
        bool operator()(const stream_info* a, const stream_info* b) { return a == b; }
    };
    struct stream_info_lt {
        bool operator()(const stream_info* a, const stream_info* b) { return a < b; }
    };

    typedef typename traits::allocator::template
        rebind<stream_info*>::other                 ptr_allocator;
    typedef std::set<
        stream_info*, stream_info_lt, ptr_allocator
    >                                               pending_data_streams_set;


    typedef std::vector<stream_info*>               stream_info_vec;
    typedef typename traits::allocator::template
        rebind<command_t>::other                    cmd_allocator;

    boost::mutex                                    m_mutex;
    boost::condition_variable                       m_cond_var;
    boost::shared_ptr<boost::thread>                m_thread;
    cmd_allocator                                   m_cmd_allocator;
    msg_allocator                                   m_msg_allocator;
    volatile command_t*                             m_head;
    bool                                            m_cancel;
    int                                             m_max_queue_size;
    event_type                                      m_event;
    long                                            m_active_count;
    stream_info_vec                                 m_files;
    pending_data_streams_set                        m_pending_data_streams;
    int                                             m_last_version;
    double                                          m_reconnect_sec;

    // Default output writer
    static int writev(stream_info& a_si, const char** a_categories,
                      const iovec* a_iovec, size_t a_sz);

    // Register a file or a stream with the logger
    file_id internal_register_stream(
        const std::string&  a_name,
        msg_writer          a_writer,
        stream_state_base*  a_state,
        int                 a_fd);

    bool internal_update_stream(stream_info* a_si, int a_fd);

    // Invoked by the async thread to flush messages from queue to file
    int  commit(const struct timespec* tsp = NULL);
    // Invoked by the async thread
    void run();
    // Enqueues msg to internal queue
    int  internal_enqueue(command_t* a_cmd);
    // Writes data to internal queue
    int  internal_write(const file_id& a_id, const std::string& a_category,
                        char* a_data, size_t a_sz, bool copied);

    void internal_close();
    void internal_close(stream_info* p, int a_errno = 0);

    command_t* allocate_message(const stream_info* a_si, const std::string& a_category,
                                const char* a_data, size_t a_size)
    {
        command_t* p = m_cmd_allocator.allocate(1);
        ASYNC_TRACE(("Allocated message (category=%s, size=%lu): %p\n",
                     a_category.c_str(), a_size, p));
        new (p) command_t(a_si, a_category, a_data, a_size);
        return p;
    }

    command_t* allocate_command(typename command_t::type_t a_tp, const stream_info* a_si) {
        command_t* p = m_cmd_allocator.allocate(1);
        ASYNC_TRACE(("Allocated command (type=%d): %p\n", a_tp, p));
        new (p) command_t(a_tp, a_si);
        return p;
    }

    void deallocate_command(command_t* a_cmd);

    // Write enqueued messages from a_si->begin() till a_end
    int do_writev_and_free(stream_info* a_si, command_t* a_end,
                           const char* a_categories[],
                           const iovec* a_wr_iov, size_t a_sz);

    bool check_range(int a_fd) const {
        return likely(a_fd >= 0 && (size_t)a_fd < m_files.size());
    }
    bool check_range(const file_id& a_id) const {
        if (!a_id) return false;
        int fd = a_id.fd();
        return check_range(fd) && m_files[fd] && a_id.version() == m_files[fd]->version;
    }
public:
    /// Create instance of this logger
    /// @param a_max_files is the max number of file descriptors
    /// @param a_reconnect_msec is the stream reconnection delay
    /// @param alloc is the message allocator to use
    explicit basic_multi_file_async_logger(
        size_t a_max_files = 1024,
        int    a_reconnect_msec = 5000,
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
    int  start();

    /// Stop asynchronous file writering thread
    void stop();

    /// Returns true if the async logger's thread is running
    bool running() { return !m_cancel && m_thread; }

    /// Start a new log file
    /// @param a_filename is the name of the output file
    /// @param a_append   if true the file is open in append mode
    /// @param a_mode     file permission mode (default 660)
    file_id open_file(const std::string& a_filename,
                      bool      a_append = true,
                      int       a_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

    /// Start a new logging stream
    ///
    /// The logger won't write any data to file but will call \a a_writer
    /// callback on every iovec to be written to stream. It's the caller's
    /// responsibility to perform the actual writing.
    /// @param a_name   is the name of the stream
    /// @param a_writer a callback executed on writing messages to stream
    /// @param a_state  custom state available to the implementation's writer
    /// @param a_fd     is an optional file descriptor associated with the stream
    file_id open_stream(const std::string&  a_name,
                        msg_writer          a_writer,
                        stream_state_base*  a_state = NULL,
                        int                 a_fd = -1);

    /// This callback will be called from within the logger's thread
    /// to format a message prior to writing it to a log file. The formatting
    /// function can modify the content of \a a_data and if needed can reallocate
    /// and rewrite \a a_data and \a a_size to point to some other memory. Use
    /// the allocate() and deallocate() functions for this purpose. You should
    /// call this function immediately after calling open_file() and before
    /// writing any messages to it.
    void set_formatter(file_id& a_id, msg_formatter a_formatter);

    /// This callback will be called from within the logger's thread
    /// to write an array of iovec structures to a log file. You should
    /// call this function immediately after calling open_file() and before
    /// writing any messages to it.
    void set_writer(file_id& a_id, msg_writer a_writer);

    /// Set the size of a batch used to write messages to file using on_write
    /// function. Valid value is between 1 and IOV_MAX.
    /// Call this function immediately after calling open_file() and before
    /// writing any messages to it.
    void set_batch_size(file_id& a_id, size_t a_size);

    /// Set a callback for reconnecting to stream
    void set_reconnect(file_id& a_id, stream_reconnecter a_reconnector);

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
        return check_range(a_id) ? m_files[a_id.fd()]->error : -1;
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
    int write(const file_id& a_id, const std::string& a_category, void* a_data, size_t a_sz);

    /// Write a copy of the string a_data to a file.
    int write(const file_id& a_id, const std::string& a_category, const std::string& a_data);

    /// @return max size of the commit queue
    const int max_queue_size()  const { return m_max_queue_size; }

    const int open_files_count() const { return m_active_count; }

};

/// Default implementation of multi_file_async_logger
typedef basic_multi_file_async_logger<> multi_file_async_logger;

//-----------------------------------------------------------------------------
// Local classes
//-----------------------------------------------------------------------------

template<typename traits>
struct basic_multi_file_async_logger<traits>::
command_t {
    static const int MAX_CAT_LEN = 32;
    enum type_t {
        msg,            // Send data message
        close,          // Close stream
        destroy_stream  // Destroy stream object
    };

    union udata {
        struct {
            mutable iovec       data;
            mutable std::string category;
        } msg;
        struct {
            bool                immediate;
        } close;

        udata()  {}
        ~udata() {}
    };

    const type_t        type;
    const stream_info*  stream;
    udata               args;
    mutable command_t*  next;
    mutable command_t*  prev;

    command_t(const stream_info* a_si, const std::string& a_category,
              const char* a_data, size_t a_size)
        : command_t(msg, a_si)
    {
        set_category(a_category);
        args.msg.data.iov_base = (void*)a_data;
        args.msg.data.iov_len  = a_size;
    }

    command_t(type_t a_type, const stream_info* a_si)
        : type(a_type), stream(a_si), next(NULL), prev(NULL)
    {}

    ~command_t() {
        if (type == msg)
            args.msg.category.~basic_string();
    }

    int fd() const { return stream->fd; }

    void unlink() {
        if (prev) prev->next = next;
        if (next) next->prev = prev;
    }
private:
    void set_category(const std::string& a_category) {
        new (&args.msg.category) std::string(a_category);
    }
} __attribute__((aligned(CL_SIZE)));

template<typename traits>
class basic_multi_file_async_logger<traits>::
file_id {
    stream_info* m_stream;
public:
    file_id() { reset(); }
    file_id(stream_info* a_si) : m_stream(a_si) {}
    bool invalid() const    { return !m_stream || m_stream->fd < 0; }
    int  version() const    { BOOST_ASSERT(m_stream); return m_stream->version; }
    int  fd()      const    { BOOST_ASSERT(m_stream); return m_stream->fd; }

    stream_info*        stream()        { return m_stream; }
    const stream_info*  stream() const  { return m_stream; }

    void                reset()         { m_stream = NULL; }
    bool                operator==(const file_id& a) { return m_stream && a.m_stream; }

    operator            bool()   const  { return !invalid(); }
};

/// Stream information associated with a file descriptor
/// used internally by the async logger
template<typename traits>
class basic_multi_file_async_logger<traits>::
stream_info {
    basic_multi_file_async_logger<traits>*  m_logger;
    // This transient list stores commands that are to be written
    // to the stream represented by this stream_info structure
    command_t*                              m_pending_writes_head;
    command_t*                              m_pending_writes_tail;

    // Time of last reconnect attempt
    time_val                                m_last_reconnect_attempt;
    close_event_type_ptr                    on_close;      // Event to signal on close
    msg_formatter                           on_format;     // "before-write" formatter
    msg_writer                              on_write;      // Message writer functor
    stream_reconnecter                      on_reconnect;  // Stream reconnecter

    template <typename T> friend class basic_multi_file_async_logger;

public:
    std::string          name;
    int                  fd;
    int                  error;
    std::string          error_msg;
    int                  version;       // Version number assigned when file is opened.
    size_t               max_batch_sz;  // Max number of messages to be batched
    stream_state_base*   state;

    explicit stream_info(stream_state_base* a_state = NULL);

    stream_info(basic_multi_file_async_logger<traits>* a_logger,
                const std::string& a_name, int a_fd, int a_version,
                msg_writer a_writer = &basic_multi_file_async_logger<traits>::writev,
                stream_state_base* a_state = NULL);

    ~stream_info() { reset(); }

    static iovec def_on_format(const std::string& a_category, iovec& a_msg) { return a_msg; }

    void reset();

    stream_info* reset(const std::string& a_name, msg_writer a_writer,
                       stream_state_base* a_state, int a_fd);

    void set_error(int a_errno, const char* a_err = NULL);

    /// Push a list of commands to the internal pending queue in reverse order
    ///
    /// The commands are pushed as long as they are destined to this stream.
    /// This method is not thread-safe, it's meant for internal use.
    /// @return number of commands enqueued. Upon return \a a_cmd is updated
    ///         with the first command not belonging to this stream or NULL if no
    ///         such command is found in the list
    int push(const command_t*& a_cmd);

    /// Returns true of internal queue is empty
    bool            pending_queue_empty() const         { return !m_pending_writes_head; }

    command_t*&     pending_writes_head()               { return m_pending_writes_head; }
    const command_t* pending_writes_tail() const        { return m_pending_writes_tail; }
    void            pending_writes_head(command_t* a)   { m_pending_writes_head = a; }
    void            pending_writes_tail(command_t* a)   { m_pending_writes_tail = a; }

    const time_val& last_reconnect_attempt() const      { return m_last_reconnect_attempt; }

    /// Erase single \a item command from the internal queue
    void erase(command_t* item);

    /// Erase commands from \a first till \a end from the internal
    /// queue of pending commands
    void erase(command_t* first, const command_t* end);
};

//-----------------------------------------------------------------------------
// Implementation: stream_info
//-----------------------------------------------------------------------------

template<typename traits>
basic_multi_file_async_logger<traits>::
stream_info::stream_info(stream_state_base* a_state)
    : m_logger(NULL)
    , m_pending_writes_head(NULL), m_pending_writes_tail(NULL)
    , on_format(&stream_info::def_on_format)
    , on_write(&basic_multi_file_async_logger<traits>::writev)
    , fd(-1), error(0), version(0), max_batch_sz(IOV_MAX)
    , state(a_state)
{}

template<typename traits>
basic_multi_file_async_logger<traits>::
stream_info::stream_info(
    basic_multi_file_async_logger<traits>* a_logger,
    const std::string& a_name, int a_fd, int a_version,
    msg_writer a_writer,
    stream_state_base* a_state
)   : m_logger(a_logger)
    , m_pending_writes_head(NULL), m_pending_writes_tail(NULL)
    , on_format(&stream_info::def_on_format)
    , on_write(a_writer)
    , name(a_name), fd(a_fd), error(0)
    , version(a_version), max_batch_sz(IOV_MAX)
    , state(a_state)
{}

template<typename traits>
void basic_multi_file_async_logger<traits>::
stream_info::reset() {
    int lfd = fd;
    fd      = -1;
    error   = 0;
    state   = NULL;
    error_msg.clear();
    if (m_logger) {
        if (!pending_queue_empty()) {
            for (command_t* p = m_pending_writes_head, *next; p; p = next) {
                next = p->next;
                m_logger->deallocate_command(p);
            }
            m_pending_writes_head = m_pending_writes_tail = NULL;
        }
        m_logger->internal_close(this, 0);
        if (m_logger->check_range(lfd))
            m_logger->m_files[lfd] = NULL;
    }
}

template<typename traits>
typename basic_multi_file_async_logger<traits>::stream_info*
basic_multi_file_async_logger<traits>::
stream_info::reset(const std::string& a_name, msg_writer a_writer,
                   stream_state_base* a_state, int a_fd)
{
    name    = a_name;
    fd      = a_fd;
    error   = 0;
    state   = a_state;
    on_write= a_writer;
    return this;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
stream_info::set_error(int a_errno, const char* a_err) {
    if (a_err)
        error_msg = a_err;
    else {
        char buf[128];
        strerror_r(a_errno, buf, sizeof(buf));
        error_msg = buf;
    }
    error = a_errno;
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
stream_info::push(const command_t*& a_cmd) {
    int n = 0;
    // Reverse the list
    command_t* p = const_cast<command_t*>(a_cmd), *last = NULL;
    for (; p && p->stream == this; ++n) {
        p->prev = p->next;
        p->next = last;
        last    = p;
        p       = p->prev; // Former p->next

        ASYNC_TRACE(("  FD[%d]: caching cmd (tp=%d) %p (prev=%p, next=%p)\n",
                        fd, last->type, last, last->prev, last->next));
    }

    if (!last)
        return 0;

    last->prev = m_pending_writes_tail;

    if (!m_pending_writes_head)
        m_pending_writes_head = last;

    if (m_pending_writes_tail)
        m_pending_writes_tail->next = last;

    m_pending_writes_tail = const_cast<command_t*>(a_cmd);

    ASYNC_TRACE(("  FD=%d cache head=%p tail=%p\n", fd,
                    m_pending_writes_head, m_pending_writes_tail));

    a_cmd = p;

    return n;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
stream_info::erase(command_t* item) {
    if (item->prev) item->prev->next = item->next;
    if (item->next) item->next->prev = item->prev;
    m_logger->deallocate_command(item);
}

/// Erase commands from \a first till \a end from the internal
/// queue of pending commands
template<typename traits>
void basic_multi_file_async_logger<traits>::
stream_info::erase(command_t* first, const command_t* end) {
    ASYNC_TRACE(("xxx stream %p purging items [%p .. %p) from queue\n",
                 this, first, end));
    for (command_t* p = first, *next; p != end; p = next) {
        next = p->next;
        m_logger->deallocate_command(p);
    }
    if (end) end->prev = NULL;
}

//-----------------------------------------------------------------------------
// Implementation: basic_multi_file_async_logger
//-----------------------------------------------------------------------------

template<typename traits>
basic_multi_file_async_logger<traits>::
basic_multi_file_async_logger(
    size_t a_max_files, int a_reconnect_msec, const msg_allocator& alloc)
    : m_msg_allocator(alloc), m_head(NULL), m_cancel(false)
    , m_max_queue_size(0)
    , m_event(0)
    , m_active_count(0)
    , m_files(a_max_files, NULL)
    , m_last_version(0)
    , m_reconnect_sec((double)a_reconnect_msec / 1000)
{}

template<typename traits>
inline int basic_multi_file_async_logger<traits>::
writev(stream_info& a_si, const char** a_categories, const iovec* a_iovec, size_t a_sz)
{
    return a_si.fd < 0 ? 0 : ::writev(a_si.fd, a_iovec, a_sz);
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
start() {
    boost::mutex::scoped_lock lock(m_mutex);

    if (m_thread)
        return -1;

    m_event.reset();

    m_head      = NULL;
    m_cancel    = false;

    m_thread = boost::shared_ptr<boost::thread>(
        new boost::thread(
            boost::bind(&basic_multi_file_async_logger<traits>::run, this)));

    m_cond_var.wait(lock);

    return 0;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
stop() {
    if (!running())
        return;

    ASYNC_TRACE(("Stopping async logger (head %p)\n", m_head));

    boost::thread_guard<> g(*m_thread);
    m_cancel = true;
    m_event.signal();
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
run() {
    // Notify the caller that we are ready
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_cond_var.notify_all();
    }

    ASYNC_TRACE(("Started async logging thread (cancel=%s)\n",
        m_cancel ? "true" : "false"));

    static const timespec ts =
        {traits::commit_timeout / 1000, (traits::commit_timeout % 1000) * 1000000 };

    while (1) {
        #ifdef DEBUG_ASYNC_LOGGER
        int rc =
        #endif
        commit(&ts);

        ASYNC_TRACE(( "Async thread commit result: %d (head: %p, cancel=%s)\n",
            rc, m_head, m_cancel ? "true" : "false" ));

        if (!m_head && m_cancel)
            break;
    }

    ASYNC_TRACE(("Logger loop finished - calling close()\n"));
    internal_close();
    ASYNC_TRACE(("Logger notifying all of exiting\n"));

    m_thread.reset();
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
internal_close() {
    ASYNC_TRACE(("Logger is closing\n"));
    boost::mutex::scoped_lock lock(m_mutex);
    for (typename stream_info_vec::iterator it=m_files.begin(), e=m_files.end();
            it != e; ++it)
        internal_close(*it, 0);
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
internal_close(stream_info* a_si, int a_errno) {
    if (!a_si || a_si->fd < 0)
        return;

    ::close(a_si->fd);
    if (a_errno)
        a_si->set_error(a_errno);

    atomic::dec(&m_active_count);
    close_event_type_ptr on_close = a_si->on_close;
    ASYNC_TRACE(("----> close(%p, %d) (fd=%d) %s event_val=%d, use_count=%ld, active=%ld\n",
            a_si, a_si->error, a_si->fd,
            on_close ? "notifying caller" : "will NOT notify caller",
            on_close ? on_close->value() : 0,
            on_close.use_count(), m_active_count));
    if (on_close) {
        on_close->signal();
        a_si->on_close.reset();
    }

    m_files[a_si->fd] = NULL;
    a_si->fd = -1;
    a_si->name.clear();
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
set_formatter(file_id& a_id, msg_formatter a_formatter) {
    BOOST_ASSERT(a_id.stream());
    a_id.stream()->on_format =
        a_formatter ? a_formatter : &stream_info::def_on_format;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
set_writer(file_id& a_id, msg_writer a_writer) {
    BOOST_ASSERT(a_id.stream());
    a_id.stream()->on_write = a_writer ? a_writer : &writev;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
set_batch_size(file_id& a_id, size_t a_size) {
    BOOST_ASSERT(a_id.stream());
    a_id.stream()->max_batch_sz = a_size < IOV_MAX ? a_size : IOV_MAX;
}

template<typename traits>
void basic_multi_file_async_logger<traits>::
set_reconnect(file_id& a_id, stream_reconnecter a_reconnecter) {
    BOOST_ASSERT(a_id.stream());
    a_id.stream()->on_reconnect = a_reconnecter;
}

template<typename traits>
typename basic_multi_file_async_logger<traits>::file_id
basic_multi_file_async_logger<traits>::
open_file(const std::string& a_filename, bool a_append, int a_mode)
{
    int n = ::open(a_filename.c_str(),
                a_append ? O_CREAT|O_APPEND|O_WRONLY|O_LARGEFILE
                         : O_CREAT|O_WRONLY|O_TRUNC|O_LARGEFILE,
                a_mode);
    return internal_register_stream(a_filename, &writev, NULL, n);
}

template<typename traits>
typename basic_multi_file_async_logger<traits>::file_id
basic_multi_file_async_logger<traits>::
open_stream(const std::string&  a_name,
            msg_writer          a_writer,
            stream_state_base*  a_state,
            int                 a_fd)
{
    // Reserve a valid file descriptor by allocating a socket
    int n = a_fd < 0 ? socket(AF_INET, SOCK_DGRAM, 0) : a_fd;
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
    if (!check_range(a_fd)) {
        int e = errno;
        if (a_fd > -1) ::close(a_fd);
        errno = e;
        return file_id();
    }

    stream_info* si =
        new stream_info(this, a_name, a_fd, ++m_last_version, a_writer, a_state);

    internal_update_stream(si, a_fd);

    atomic::inc(&m_active_count);
    return file_id(si);
}

template<typename traits>
bool basic_multi_file_async_logger<traits>::
internal_update_stream(stream_info* a_si, int a_fd) {
    if (!check_range(a_fd))
        return false;

    boost::mutex::scoped_lock lock(m_mutex);

    a_si->fd = a_fd;
    a_si->error = 0;
    a_si->error_msg.clear();

    stream_info* old_si = m_files[a_fd];
    if (old_si && old_si != a_si) {
        command_t* c = allocate_command(command_t::destroy_stream, old_si);
        internal_enqueue(c);
    }

    m_files[a_fd] = a_si;

    return true;
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
close_file(file_id& a_id, bool a_immediate, int a_wait_secs) {
    if (!a_id) return 0;

    stream_info* si = a_id.stream();

    if (!m_thread) {
        si->reset();
        return 0;
    }

    if (!si->on_close)
        si->on_close.reset(new close_event_type());
    close_event_type_ptr ev = si->on_close;

    int event_val = ev ? ev->value() : 0;

    command_t* l_cmd = allocate_command(command_t::destroy_stream, a_id.stream());
    l_cmd->args.close.immediate = a_immediate;
    int n = internal_enqueue(l_cmd);

    if (!n && ev) {
        ASYNC_TRACE(("----> close_file(%d) is waiting for ack secs=%d (event_val={%d,%d})\n",
                     a_id.fd(), a_wait_secs, event_val, ev->value()));
        if (m_thread && si->fd >= 0) {
            if (a_wait_secs < 0)
                n = ev->wait(&event_val);
            else {
                boost::posix_time::ptime l_now(
                    boost::posix_time::microsec_clock::universal_time() +
                    boost::posix_time::seconds(a_wait_secs));
                n = ev->wait(l_now, &event_val);
            }
            ASYNC_TRACE(( "====> close_file(%d) ack received (res=%d, val=%d) (err=%d)\n",
                    a_id.fd(), n, event_val, si->error));
        }
    } else {
        ASYNC_TRACE(( "====> close_file(%d) failed to enqueue cmd or no event (n=%d, %s)\n",
                a_id.fd(), n, (ev ? "true" : "false")));
    }
    a_id.reset();
    return n;
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
internal_enqueue(command_t* a_cmd) {
    BOOST_ASSERT(a_cmd);

    command_t* l_last_head;

    // Replace the head with msg
    do {
        l_last_head = const_cast<command_t*>(m_head);
        a_cmd->next = l_last_head;
    } while( !atomic::cas(&m_head, l_last_head, a_cmd) );

    if (!l_last_head)
        m_event.signal();

    ASYNC_TRACE(("--> internal_enqueue cmd %p (type=%d) - cur head: %p, prev head: %p%s\n",
        a_cmd, a_cmd->type, m_head, l_last_head, !l_last_head ? " (signaled)" : ""));

    return 0;
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
internal_write(const file_id& a_id, const std::string& a_category,
               char* a_data, size_t a_sz, bool copied)
{
    if (unlikely(!a_id.stream() || m_cancel)) {
        if (copied)
            deallocate(a_data, a_sz);
        return -1;
    }

    command_t* p = allocate_message(a_id.stream(), a_category, a_data, a_sz);
    ASYNC_TRACE(("->write(%p, %lu) - %s\n", a_data, a_sz, copied ? "allocated" : "no copy"));
    return internal_enqueue(p);
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
write(const file_id& a_id, const std::string& a_category, void* a_data, size_t a_sz) {
    return internal_write(a_id, a_category, static_cast<char*>(a_data), a_sz, false);
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
write(const file_id& a_id, const std::string& a_category, const std::string& a_data) {
    char* q = allocate(a_data.size());
    memcpy(q, a_data.c_str(), a_data.size());

    return internal_write(a_id, a_category, q, a_data.size(), false);
}


template<typename traits>
int basic_multi_file_async_logger<traits>::
do_writev_and_free(stream_info* a_si, command_t* a_end,
                   const char** a_categories, const iovec* a_vec, size_t a_sz)
{
    int n = a_sz ? a_si->on_write(*a_si, a_categories, a_vec, a_sz) : 0;
    if (n >= 0) {
        // Data was successfully written to stream - adjust internal queue's head/tail
        a_si->erase(a_si->pending_writes_head(), a_end);
        a_si->pending_writes_head(a_end);
        if (!a_end)
            a_si->pending_writes_tail(a_end);
    } else if (!a_si->error) {
            a_si->set_error(errno);
        LOG_ERROR(("Error writing %u messages to stream '%s': %s\n",
                a_sz, a_si->name.c_str(), a_si->error_msg.c_str()));
    }
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
    ASYNC_TRACE(("FD=%d, deallocating command (type=%d) %p\n",
                a_cmd->fd(), a_cmd->type, a_cmd));
    a_cmd->~command_t();
    m_cmd_allocator.deallocate(a_cmd, 1);
}

template<typename traits>
int basic_multi_file_async_logger<traits>::
commit(const struct timespec* tsp)
{
    ASYNC_TRACE(("Committing head: %p\n", m_head));

    int event_val = m_event.value();

    while (!m_head) {
        #ifdef DEBUG_ASYNC_LOGGER
        int n =
        #endif
        m_event.wait(tsp, &event_val);

        ASYNC_TRACE(("    COMMIT awakened (res=%d, val=%d, futex=%d), cancel=%d, head=%p\n",
                     n, event_val, m_event.value(), m_cancel, m_head));

        if (m_cancel && !m_head)
            return 0;
    }

    command_t* l_cur_head;

    // Find current head and reset the old head to be NULL
    do {
        l_cur_head = const_cast<command_t*>( m_head );
    } while( !atomic::cas(&m_head, l_cur_head, static_cast<command_t*>(NULL)) );

    ASYNC_TRACE((" --> cur head: %p, new head: %p\n", l_cur_head, m_head));

    BOOST_ASSERT(l_cur_head);

    int n, count = 0;

    // Place reverse commands in the pending queues of individual streams.
    for(const command_t* p = l_cur_head; p; count += n) {
        stream_info* si = const_cast<stream_info*>(p->stream);
        BOOST_ASSERT(si);

        #ifdef DEBUG_ASYNC_LOGGER
        const command_t* last = p;
        #endif

        // Insert data to the pending list
        // (this function advances p until there is a stream change)
        n = si->push(p);
        // Update the index of fds that have pending data
        m_pending_data_streams.insert(si);
        ASYNC_TRACE(("Set stream %p fd[%d].pending_writes(%p) -> %d, head(%p), next(%p)\n",
                     si, si->fd, last, n, si->pending_writes_head(), p));
    }

    ASYNC_TRACE(("Processed total count: %d.\n", count));

    // Process each fd's pending command queue
    if (m_max_queue_size < count)
        m_max_queue_size = count;

    for(typename pending_data_streams_set::iterator
            it = m_pending_data_streams.begin(), e = m_pending_data_streams.end();
            it != e; ++it)
    {
        stream_info*     si = *it;
        msg_formatter& ffmt = si->on_format;

        // If there was an error on this stream try to reconnect the stream
        if (si->error && si->on_reconnect) {
            time_val now(time_val::universal_time());
            double time_diff = now.diff(si->last_reconnect_attempt());

            if (time_diff > m_reconnect_sec) {
                ASYNC_TRACE(("===> Trying to reconnect stream %p "
                             "(prev reconnect %.3fs ago)\n",
                             si, si->last_reconnect_attempt() ? time_diff : 0.0));

                int fd = si->on_reconnect(*si);

                ASYNC_TRACE(("     Stream %p %s\n",
                             si, fd < 0 ? "not reconnected!"
                                        : "reconnected successfully!"));

                if (fd >= 0 && !internal_update_stream(si, fd))
                    LOG_ERROR(("Logger '%s' failed to register file descriptor %d!",
                        si->name.c_str(), si->fd));

                si->m_last_reconnect_attempt = now;
            }
        }

        ASYNC_TRACE(("Processing commands for stream %p (fd=%d)\n", si, si->fd));

        struct iovec iov [si->max_batch_sz];  // Contains pointers to write
        const  char* cats[si->max_batch_sz];  // List of message categories
        size_t n = 0, sz = 0;

        static const int SI_OK               = 0;
        static const int SI_CLOSE_SCHEDULED  = 1 << 0;
        static const int SI_CLOSE            = 1 << 1 | SI_CLOSE_SCHEDULED;
        static const int SI_DESTROY          = 1 << 2 | SI_CLOSE;

        int status = SI_OK;

        const command_t* p = si->pending_writes_head();
        command_t* end;

        // Process commands in blocks of si->max_batch_sz
        for (; p && !si->error && ((status & SI_CLOSE) != SI_CLOSE); p = end) {
            end = p->next;

            if (p->type == command_t::msg) {
                iov[n]  = ffmt(p->args.msg.category, p->args.msg.data);
                cats[n] = p->args.msg.category.c_str();
                sz     += iov[n].iov_len;
                ASYNC_TRACE(("FD=%d (stream %p) cmd %p (#%lu) next(%p), "
                             "write(%p, %lu) free(%p, %lu)\n",
                             si->fd, si, p, n, p->next, iov[n].iov_base, iov[n].iov_len,
                             p->args.msg.data.iov_base, p->args.msg.data.iov_len));
                if (++n == si->max_batch_sz) {
                    int ec = do_writev_and_free(si, end, cats, iov, n);
                    ASYNC_TRACE(("Written %d bytes to %s\n", ec, si->name.c_str()));
                    if (ec > 0)
                        n = 0;
                }
            } else if (p->type == command_t::close) {
                status |= p->args.close.immediate ? SI_CLOSE : SI_CLOSE_SCHEDULED;
                ASYNC_TRACE(("FD=%d, Command %lu address %p (close)\n", si->fd, n, p));
                si->erase(const_cast<command_t*>(p));
            } else if (p->type == command_t::destroy_stream) {
                status |= SI_DESTROY;
                si->erase(const_cast<command_t*>(p));
            } else {
                ASYNC_TRACE(("Command %p has invalid message type: %d "
                             "(stream=%p, prev=%p, next=%p)\n",
                             p, p->type, p->stream, p->prev, p->next));
                si->erase(const_cast<command_t*>(p));
                BOOST_ASSERT(false); // This should never happen!
            }
        }

        if (si->error) {
            ASYNC_TRACE(("Written total %lu bytes to %p (fd=%d) %s with error: %s\n",
                         sz, si, si->fd, si->name.c_str(), si->error_msg.c_str()));
        } else if (n > 0) {
            #ifdef DEBUG_ASYNC_LOGGER
            int k =
            #endif
            do_writev_and_free(si, end, cats, iov, n);
            ASYNC_TRACE(("Written %d bytes to (fd=%d) %s (total = %lu)\n", k,
                    si->fd, si->name.c_str(), sz));
        } else {
            ASYNC_TRACE(("Written total %lu bytes to (fd=%d) %s\n",
                         sz, si->fd, si->name.c_str()));
        }

        // Close associated file descriptor
        if (si->error || status != SI_OK)
            internal_close(si, si->error);

        bool destroy_si = (status & SI_DESTROY) == SI_DESTROY;

        if (destroy_si || si->pending_queue_empty()) {
            ASYNC_TRACE(("Removing %p (name=%s, fd=%d) from list of pending data streams\n",
                         si, si->name.c_str(), si->fd));
            m_pending_data_streams.erase(si);
        }

        if (destroy_si) {
            ASYNC_TRACE(("Destroying %p stream\n", si));
            delete si;
        }
    }
    return count;
}

} // namespace utxx

#endif // _UTXX_ASYNC_FILE_LOGGER_HPP_
