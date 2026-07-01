// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor.hpp
//------------------------------------------------------------------------------
/// \brief I/O multiplexing event reactor
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <reactor/compat.hpp>
#include <reactor/reactor_fd_info.hpp>
#include <reactor/reactor_misc.hpp>
#include <reactor/reactor_platform.hpp>
#include <reactor/reactor_types.hpp>

namespace utxx {

//------------------------------------------------------------------------------
/// Reactor — epoll-based I/O multiplexer
//------------------------------------------------------------------------------
class Reactor {
public:
  using HType        = utxx::HType;
  using FdInfoVector = std::vector<std::unique_ptr<FdInfo>>;

  /// Create the reactor (epoll on Linux, kqueue on macOS/BSD).
  Reactor(
      std::string const& a_ident, int a_debug = 0, reactor_handle_t a_epoll_fd = -1,
      int a_max_fds = 128)
      : m_own_efd(a_epoll_fd == -1), m_epoll_fd(m_own_efd ? reactor_create() : a_epoll_fd),
        m_fds(a_max_fds), m_debug(a_debug),
        m_use_getsockname(utxx::os::getenv("HAVE_GETSOCKNAME", 0l))
  { Ident(a_ident); }

  ~Reactor();

  const std::string& Ident() const { return m_ident; }
  void               Ident(const std::string& a_ident);

  int                Debug() const { return m_debug; }
  void               Debug(int a_level) { m_debug = a_level; }

  void               SetIdle(const IdleHandler& a_handler) { m_on_idle = a_handler; }
  void               SetLogger(const Logger& a_logger) { m_logger = a_logger; }

  bool               UseKBP() const { return m_use_kbp; }
  void               UseKBP(bool v) { m_use_kbp = v; }

  //----------------------------------------------------------------------------
  static bool        IsError(uint32_t a_ev) { return reactor_is_error(a_ev); }
  static bool        IsReadable(uint32_t a_ev) { return reactor_is_readable(a_ev); }
  static bool        IsWritable(uint32_t a_ev) { return reactor_is_writable(a_ev); }

  /// Add a Unix Domain Socket listener to the epoll set.
  FdInfo&            AddUDSListener(
      std::string const& a_name, std::string const& a_file_path, AcceptHandler const& a_on_accept,
      ErrHandler const& a_on_error, void* a_opaque = nullptr, int a_permissions = 0660);

  /// Add an FD (fast I/O handler path).
  FdInfo&
  Add(std::string const& a_name, int a_fd, RWIOHandler const& a_on_read,
      RWIOHandler const& a_on_write, ErrHandler const& a_on_error, void* a_instance,
      void* a_opaque = nullptr, int a_rd_bufsz = 0, int a_wr_bufsz = 0,
      dynamic_io_buffer* a_wr_buf = nullptr, ReadSizeEstim a_read_at_least = nullptr,
      TriggerT a_trigger = TriggerT::EDGE_TRIGGERED);

  /// Add a "raw" (user-reads-data) FD.
  FdInfo&
          Add(const std::string& a_name, int a_fd, RawIOHandler const& a_on_io,
              ErrHandler const& a_on_error, void* a_opaque = nullptr,
              uint32_t a_events = REACTOR_EV_IN | REACTOR_EV_RDHUP | REACTOR_EV_OUT | REACTOR_EV_ET,
              size_t   a_rd_bufsz = 0);

  /// Add a file handler.
  FdInfo& AddFile(
      std::string const& a_name, std::string const& a_filename, FileHandler const& a_on_read,
      ErrHandler const& a_on_error, void* a_instance = nullptr, void* a_opaque = nullptr,
      size_t a_rd_bufsz = 5242880, ReadSizeEstim a_read_at_least = nullptr,
      TriggerT a_trigger = LEVEL_TRIGGERED);

  /// Add a pipe (popen) handler.
  FdInfo& AddPipe(
      std::string const& a_name, std::string const& a_command, PipeHandler const& a_on_read,
      ErrHandler const& a_on_error, void* a_opaque = nullptr, size_t a_rd_bufsz = 1024 * 1024,
      ReadSizeEstim a_read_at_least = nullptr);

  /// Add an eventfd handler.
  FdInfo& AddEvent(
      std::string const& a_name, EventHandler const& a_on_read, ErrHandler const& a_on_error,
      void* a_opaque = nullptr);

  /// Add a timerfd handler.
  FdInfo& AddTimer(
      std::string const& a_name, uint32_t a_initial_msec, uint32_t a_interval_msec,
      EventHandler a_on_timer, ErrHandler a_on_error, void* a_opaque = nullptr);

  /// Add a signalfd handler.
  FdInfo& AddSignal(
      std::string const& a_name, const sigset_t& a_mask, SigHandler const& a_fun,
      ErrHandler const& a_on_error, void* a_opaque = nullptr, int a_sigq_capacity = 8);

  void               Remove(int& a_fd, bool a_clear_fdinfo = true);

  /// Process epoll events; invoke registered handlers.
  void               Wait(int a_timeout_msec = 0);

  reactor_handle_t   EPollFD() const { return m_epoll_fd; }

  dynamic_io_buffer* RdBuff(int a_fd);
  dynamic_io_buffer* WrBuff(int a_fd);

  dynamic_io_buffer* SubscribeWrite(int a_fd, bool a_on);
  void               SetBuffer(int a_fd, bool a_is_read, dynamic_io_buffer* a_buf);
  void               ResizeBuffer(int a_fd, bool a_is_read, size_t a_size);

  template <typename StreamT>
  static void DefRdDebug(StreamT& out, const FdInfo& a_fi, const char* a_buf, size_t a_sz);

  template <class... Args>
  void    Log(log_level a_level, src_info&& a_sinfo, Args&&... args);

  bool    UseGetSockName() const { return m_use_getsockname; }

  FdInfo& Get(int a_fd, src_info&& a_sinfo)
  {
    Check(a_fd, std::move(a_sinfo));
    return *m_fds[a_fd];
  }
  FdInfo const& Get(int a_fd, src_info&& a_sinfo) const
  {
    Check(a_fd, std::move(a_sinfo));
    return *m_fds[a_fd];
  }

private:
  bool             m_own_efd;
  reactor_handle_t m_epoll_fd;
  FdInfoVector     m_fds;
  IdleHandler      m_on_idle;
  Logger           m_logger;
  int              m_debug;
  std::string      m_ident;
  bool             m_use_getsockname;
  bool             m_use_kbp = false;

  friend class FdInfo;

  void CloseFD(int& a_fd);

  FdInfo*
  Set(std::string const& a_name, int a_fd, FdTypeT a_fd_type, uint32_t a_events, src_info&& a_src,
      ErrHandler const& a_on_error = nullptr, void* a_instance = nullptr, void* a_opaque = nullptr,
      int a_rd_bufsz = 0, int a_wr_bufsz = 0, dynamic_io_buffer* a_wr_buf = nullptr,
      ReadSizeEstim a_read_sz_fun = nullptr, TriggerT a_trigger = EDGE_TRIGGERED);

  void Check(int a_fd, src_info&& a_sinfo) const
  {
    if (UNLIKELY(a_fd <= 0 || a_fd >= int(m_fds.size())))
      REACTOR_SRC_THROW(
          utxx::badarg_error, a_sinfo, ": invalid fd=", a_fd, ", maxfd=", m_fds.size());
  }

  template <typename DebugLambda>
  FdInfo& DoAdd(int a_fd, uint32_t a_ev, FdInfo&& a_fi, DebugLambda a_fun);

  void    EPollAdd(int a_fd, const std::string& a_nm, unsigned a_ev, src_info&&);
};

} // namespace utxx

#include <reactor/reactor.hxx>
