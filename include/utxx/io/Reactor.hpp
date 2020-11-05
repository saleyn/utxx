// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  Reactor.hpp
//------------------------------------------------------------------------------
/// \brief I/O multiplexing event reactor
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/compiler_hints.hpp>
#include <utxx/enum.hpp>
#include <utxx/function.hpp>
#include <utxx/buffer.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/print.hpp>
#include <utxx/string.hpp>
#include <utxx/time_val.hpp>
#include <utxx/os.hpp>
#include <sys/epoll.h>
#include <sys/eventfd.h>

//#include <mqt/infra/Util.hpp>
#include <utxx/io/ReactorMisc.hpp>
#include <utxx/io/ReactorTypes.hpp>
#include <utxx/io/ReactorFdInfo.hpp>

namespace utxx {
namespace io   {

//------------------------------------------------------------------------------
/// Reactor - I/O multiplexer
//------------------------------------------------------------------------------
class Reactor {
public:
  using HType        = io::HType;
  using FdInfoVector = std::vector<std::unique_ptr<FdInfo>>;

  /// Create an epoll reactor
  Reactor
  (
    std::string const&  a_ident,
    int                 a_debug    = 0,
    int                 a_epoll_fd = -1,
    int                 a_max_fds  = 128
  )
    : m_own_efd        (a_epoll_fd == -1)
    , m_epoll_fd       (m_own_efd  ? epoll_create1(0) : a_epoll_fd)
    , m_fds            (a_max_fds)
    , m_debug          (a_debug)
    , m_use_getsockname(utxx::os::getenv("HAVE_GETSOCKNAME", 0l))
  {
    Ident(a_ident);
  }

  ~Reactor();

  /// Identifier used as the logging prefix for this component
  const std::string& Ident() const { return m_ident; }
  void               Ident(const std::string& a_ident);

  /// Get debug level
  int  Debug()            const { return m_debug; }
  void Debug(int a_level)       { m_debug = a_level; }

  /// Set idle handler.
  /// It is called after procession all I/O events before reinvoking
  /// epoll_wait().
  void SetIdle(const IdleHandler& a_handler) { m_on_idle = a_handler; }

  /// Set the logging callback.
  /// If not set the debug output will go to the std::cerr
  void SetLogger(const Logger& a_logger) { m_logger = a_logger; }

  /// Use kernel by-pass mode (offered by some networking cards)
  bool UseKBP()           const { return m_use_kbp; }
  void UseKBP(bool v)           { m_use_kbp = v;    }

  //----------------------------------------------------------------------------
  // Tests for event readability/writeability/errors
  //----------------------------------------------------------------------------
  static bool IsError   (int a_ev) { return a_ev & (EPOLLERR|EPOLLHUP); }
  static bool IsReadable(int a_ev) { return a_ev & EPOLLIN;  }
  static bool IsWritable(int a_ev) { return a_ev & EPOLLOUT; }

  /// Add a Unix Domain Socket listener to the epoll set.
  /// @return FD of the listening socket
  FdInfo& AddUDSListener
  (
    std::string   const& a_name,
    std::string   const& a_file_path,
    AcceptHandler const& a_on_accept,
    ErrHandler    const& a_on_error,
    void*                a_opaque = nullptr,
    int                  a_permissions = 0660
  );

  // TODO: Add INET Listener

  /// Add an FD to the epoll set.
  /// This method installs an I/O handling callback for read/write events on
  /// the file descriptor \a a_fd, which is more efficient than using
  /// std::function.  The \a a_instance argument can be used to treat the
  /// \a a_on_read and \a a_on_write handlers as class methods.
  /// @param a_name     the name of this handler
  /// @param a_fd       the file descriptor to add to reactor
  /// @param a_on_read  the callback to be invoked on read activity
  /// @param a_on_write the callback to be invoked on write activity
  /// @param a_on_error the callback to be called on error conditions
  /// @param a_instance the context in which to invoke the callback
  /// @param a_opaque   some user-specific opaque context passed to callbacks
  /// @param a_rd_bufsz size of the internal read buffer
  /// @param a_wr_bufsz size of the internal write buffer
  /// @param a_wr_buf   pointer to external write buffer to use
  /// @param a_read_at_least functor to call to determine the minimum size
  ///                   to read before invoking the a_on_read callback.
  /// @param a_trigger  sets edge/level triggering condition for this descriptor
  FdInfo& Add
  (
    std::string const& a_name,
    int                a_fd,
    RWIOHandler const& a_on_read,
    RWIOHandler const& a_on_write,
    ErrHandler  const& a_on_error,
    void*              a_instance,
    void*              a_opaque   = nullptr,
    int                a_rd_bufsz = 0, // When =/= 0, owned buffer created
    int                a_wr_bufsz = 0, // When =/= 0, owned buffer created
    dynamic_io_buffer* a_wr_buf   = nullptr, // If not NULL, buf is not owned
    ReadSizeEstim      a_read_at_least = nullptr,
    TriggerT           a_trigger  = TriggerT::EDGE_TRIGGERED
  );

  /// Add "raw" handler of FD events.
  /// The user's \a a_on_io callback is responsible to reading/writing from/to
  /// file descriptor on its own.
  /// @return eventfd associated with file descriptor
  FdInfo& Add
  (
    const std::string&  a_name,
    int                 a_fd,
    RawIOHandler const& a_on_io,
    ErrHandler   const& a_on_error,
    void*               a_opaque = nullptr,
    uint32_t            a_events = EPOLLIN | EPOLLRDHUP | EPOLLOUT | EPOLLET,
    size_t              a_rd_bufsz = 0
  );

  /// Add handler of reading input from file
  /// @return eventfd associated with file descriptor
  FdInfo& AddFile
  (
    std::string   const& a_name    ,
    std::string   const& a_filename,
    FileHandler   const& a_on_read ,
    ErrHandler    const& a_on_error,
    void*                a_instance = nullptr,
    void*                a_opaque   = nullptr,
    size_t               a_rd_bufsz = 5242880, // When =/= 0, buffer is created
    ReadSizeEstim        a_read_at_least = nullptr,
    TriggerT             a_trigger       = LEVEL_TRIGGERED
  );

  /// Add handler of reading input from popen().
  /// @return eventfd associated with file descriptor
  FdInfo& AddPipe
  (
    std::string   const& a_name    ,
    std::string   const& a_command ,
    PipeHandler   const& a_on_read ,
    ErrHandler    const& a_on_error,
    void*                a_opaque = nullptr,
    size_t               a_rd_bufsz = 1024*1024, // When =/= 0, buffer is created
    ReadSizeEstim        a_read_at_least = nullptr
  );

  /// Add a handler to eventfd events.
  /// @return eventfd file descriptor
  FdInfo& AddEvent
  (
    std::string   const& a_name    ,
    EventHandler  const& a_on_read ,
    ErrHandler    const& a_on_error,
    void*                a_opaque = nullptr
  );

  /// Add a timer FD to the epoll set.
  /// @return timerfd file descriptor
  FdInfo& AddTimer
  (
    std::string  const& a_name,
    uint32_t            a_initial_msec,
    uint32_t            a_interval_msec,
    EventHandler        a_on_timer,
    ErrHandler          a_on_error,
    void*               a_opaque = nullptr
  );

  /// Add a signal handler to the epoll set.
  /// @param a_mask           specifies the set of signals that the caller
  ///                         to accept via the file descriptor wishes
  /// @param a_fun            defines the callback to be executed on signal
  /// @param a_on_error       error handler
  /// @param a_opaque         custom value to be passed to the callback
  /// @param a_sigq_capacity  buffer capacity holding # of raised signal infos
  /// @return signalfd file descriptor
  FdInfo& AddSignal
  (
    std::string const&    a_name,
    const       sigset_t& a_mask,
    SigHandler  const&    a_fun,
    ErrHandler  const&    a_on_error,
    void*                 a_opaque = nullptr,
    int                   a_sigq_capacity = 8
  );

  /// Remove an FD from the epoll set
  void Remove(int& a_fd, bool a_clear_fdinfo=true);

  /// Connect a file descriptor
  //void Connect(int a_fd);

  /// Wait for events on file descriptors in the current epoll set.
  /// For each fd that has activity invoke the registered handler.
  void Wait(int a_timeout_msec = 0);

  /// Report epoll file descriptor
  int EPollFD() const { return m_epoll_fd; }

  dynamic_io_buffer* RdBuff(int a_fd);
  dynamic_io_buffer* WrBuff(int a_fd);

  /// Turn on/off subscription to EPOLLOUT (write) events
  dynamic_io_buffer* SubscribeWrite(int a_fd, bool a_on);

  /// Set externally managed read/write buffer for the given \a a_fd
  void SetBuffer   (int a_fd, bool a_is_read, dynamic_io_buffer* a_buf);

  /// Resize owned read/write buffer for the given \a a_fd
  void ResizeBuffer(int a_fd, bool a_is_read, size_t a_size);

  /// A default method that can be used as read "debug" handler
  template <typename StreamT>
  static void DefRdDebug(StreamT& out, const FdInfo& a_fi,
                         const char* a_buf, size_t a_sz);

  template <class... Args>
  void Log(utxx::log_level a_level, utxx::src_info&& a_sinfo, Args&&... args);

  /// When environment option "USE_GETSOCKNAME" is set this will return true
  /// and FdInfo's will use the
  bool UseGetSockName() const { return m_use_getsockname; }

private:
  bool            m_own_efd;
  int             m_epoll_fd;
  FdInfoVector    m_fds;
  IdleHandler     m_on_idle;
  Logger          m_logger;
  int             m_debug;
  std::string     m_ident;               ///< Indent prefix (E.g. "[p1@] ")
  bool            m_use_getsockname;     ///< true when set env USE_GETSOCKNAME
  bool            m_use_kbp;             ///< Use kernel by-pass (e.g. Mellanox LibVBA)

  friend class FdInfo;

  void CloseFD(int& a_fd);

  FdInfo* Set
  (
    std::string const&  a_name,
    int                 a_fd,
    FdTypeT             a_fd_type,
    uint32_t            a_events,
    utxx::src_info&&    a_src,
    ErrHandler  const&  a_on_error    = nullptr,
    void*               a_instance    = nullptr,
    void*               a_opaque      = nullptr,
    int                 a_rd_bufsz    = 0,
    int                 a_wr_bufsz    = 0,
    dynamic_io_buffer*  a_wr_buf      = nullptr,
    ReadSizeEstim       a_read_sz_fun = nullptr,
    TriggerT            a_trigger     = EDGE_TRIGGERED
  );

  void Check(int a_fd, utxx::src_info&& a_sinfo) const {
    if (UNLIKELY(a_fd <= 0 || a_fd >= int(m_fds.size())))
      UTXX_SRC_THROW(utxx::badarg_error, a_sinfo, ": invalid fd=", a_fd,
                     ", maxfd=", m_fds.size());
  }

  FdInfo&       Get(int a_fd, utxx::src_info&& a_sinfo)
  { Check(a_fd, std::move(a_sinfo)); return *m_fds[a_fd]; }

  FdInfo const& Get(int a_fd, utxx::src_info&& a_sinfo) const
  { Check(a_fd, std::move(a_sinfo)); return *m_fds[a_fd]; }

  template <typename DebugLambda>
  FdInfo&  DoAdd(int a_fd, uint32_t a_ev, FdInfo&& a_fi, DebugLambda a_fun);

  void EPollAdd(int a_fd, const std::string& a_nm, uint a_ev, utxx::src_info&&);
};

} // namespace io
} // namespace utxx

#include <utxx/io/Reactor.hxx>
