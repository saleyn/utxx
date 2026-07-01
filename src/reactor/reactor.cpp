// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor.cpp
//------------------------------------------------------------------------------
/// \brief Reactor — non-inlined methods
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#include <cassert>
#include <fcntl.h>
#include <reactor/reactor.hpp>
#include <reactor/reactor_misc.hpp>
#include <reactor/reactor_platform.hpp>
#include <regex>
#include <sched.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

namespace utxx {

//------------------------------------------------------------------------------
Reactor::~Reactor()
{
  if (m_own_efd) {
    reactor_destroy(m_epoll_fd);
    m_epoll_fd = -1;
  }
}

//------------------------------------------------------------------------------
void Reactor::SetBuffer(int a_fd, bool a_is_read, dynamic_io_buffer* a_buf)
{
  REACTOR_PRETTY_FUNCTION();
  auto& info = Get(a_fd, REACTOR_SRCX);
  auto& bufp = a_is_read ? info.m_rd_buff_ptr : info.m_wr_buff_ptr;
  auto& buf  = a_is_read ? info.m_rd_buff : info.m_wr_buff;
  buf        = a_buf;
  bufp.reset();
}

//------------------------------------------------------------------------------
void Reactor::ResizeBuffer(int a_fd, bool a_is_read, size_t a_size)
{
  REACTOR_PRETTY_FUNCTION();
  if (!a_size) return;

  auto& info = Get(a_fd, REACTOR_SRCX);
  auto& bufp = a_is_read ? info.m_rd_buff_ptr : info.m_wr_buff_ptr;
  auto& buf  = a_is_read ? info.m_rd_buff : info.m_wr_buff;

  if (buf)
    buf->reserve(a_size);
  else {
    bufp.reset(new dynamic_io_buffer(a_size));
    buf = bufp.get();
  }
}

//------------------------------------------------------------------------------
void Reactor::EPollAdd(int fd, std::string const& a_nm, unsigned a_events, src_info&& a_si)
{
  try {
    reactor_add(m_epoll_fd, fd, a_events, (uintptr_t)fd);
  } catch (...) {
    throw utxx::io_error(
        std::move(a_si), errno,
        utxx::detail::concat("reactor_add failed for '", a_nm, "', fd=", fd));
  }
}

//------------------------------------------------------------------------------
FdInfo* Reactor::Set(
    std::string const& a_name, int a_fd, FdTypeT a_fd_type, uint32_t a_events, src_info&& a_src,
    ErrHandler const& a_on_error, void* a_instance, void* a_opaque, int a_rd_bufsz, int a_wr_bufsz,
    dynamic_io_buffer* a_wr_buf, ReadSizeEstim a_read_sz_fun, TriggerT a_trig)
{
  assert(a_fd >= 0);

  EPollAdd(a_fd, a_name, a_events, std::move(a_src));

  if (a_fd >= int(m_fds.size())) m_fds.resize(a_fd + 64);

  auto p = new FdInfo(
      this, a_name, a_fd, a_fd_type, a_on_error, a_instance, a_opaque, a_rd_bufsz, a_wr_bufsz,
      a_wr_buf, a_read_sz_fun, a_trig);
  m_fds[a_fd].reset(p);
  return p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor::Add(
    std::string const& a_name, int a_fd, RWIOHandler const& a_on_read,
    RWIOHandler const& a_on_write, ErrHandler const& a_on_error, void* a_inst, void* a_opaque,
    int a_rd_bufsz, int a_wr_bufsz, dynamic_io_buffer* a_wr_buf, ReadSizeEstim a_read_at_least,
    TriggerT a_trigger)
{
  REACTOR_PRETTY_FUNCTION();

  if (UNLIKELY(a_fd < 0)) UTXX_THROWX_BADARG_ERROR("invalid fd=", a_fd);

  uint32_t events = REACTOR_EV_IN | REACTOR_EV_ERR | REACTOR_EV_RDHUP |
                    (a_trigger == EDGE_TRIGGERED ? REACTOR_EV_ET : 0u) |
                    (a_on_write ? REACTOR_EV_OUT : 0u);

  UTXX_RLOG(
      this, TRACE5, "adding IO handler '", a_name, "', fd=", a_fd, ", RdBuf=", a_rd_bufsz,
      ", WrBuf=", a_wr_bufsz, ", Events=", EPollEvents(events), ", Opaque=", a_opaque);

  auto p =
      Set(a_name, a_fd, FdTypeT::UNDEFINED, events, REACTOR_SRCX, a_on_error, a_inst, a_opaque,
          a_rd_bufsz, a_wr_bufsz, a_wr_buf, a_read_at_least, a_trigger);
  p->SetHandler(HType::IO, HandlerT::IO{a_on_read, a_on_write});

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor::Add(
    const std::string& a_name, int a_fd, RawIOHandler const& a_on_io, ErrHandler const& a_on_error,
    void* a_opaque, uint32_t a_events, size_t a_rd_bufsz)
{
  REACTOR_PRETTY_FUNCTION();

  if (UNLIKELY(a_fd < 0)) UTXX_THROWX_BADARG_ERROR("invalid fd=", a_fd);

  UTXX_RLOG(
      this, TRACE5, "adding RawIO handler '", a_name, "', fd=", a_fd,
      ", Events=", EPollEvents(a_events), ", Opaque=", a_opaque);

  auto trigger = (a_events & REACTOR_EV_ET) ? EDGE_TRIGGERED : LEVEL_TRIGGERED;
  auto p =
      Set(a_name, a_fd, FdTypeT::UNDEFINED, a_events, REACTOR_SRCX, a_on_error, nullptr, a_opaque,
          a_rd_bufsz, 0, nullptr, nullptr, trigger);
  p->SetHandler(HType::RawIO, a_on_io);

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor::AddFile(
    std::string const& a_name, std::string const& a_filename, FileHandler const& a_on_read,
    ErrHandler const& a_on_error, void* a_instance, void* a_opaque, size_t a_bufsz,
    ReadSizeEstim a_read_at_least, TriggerT a_trigger)
{
  REACTOR_PRETTY_FUNCTION();

  int efd = reactor_eventfd_create();
  if (efd < 0) REACTOR_THROWX_IO_ERROR(errno, "eventfd");

  auto guard = utxx::scope_exit([efd]() { reactor_eventfd_close(efd); });

  auto p =
      Set(a_name, efd, FdTypeT::File, REACTOR_EV_IN | REACTOR_EV_ET | REACTOR_EV_ERR, REACTOR_SRCX,
          a_on_error, a_instance, a_opaque, a_bufsz, 0, nullptr, a_read_at_least, a_trigger);

  p->SetHandler(HType::File, a_on_read);

  UTXX_RLOG(
      this, TRACE5, "adding File reader '", a_name, "' for file ", a_filename,
      ", Opaque=", a_opaque);

  guard.disable();

  auto reader = new AIOReader(efd, a_filename.c_str());
  p->SetFileReader(reader);

  assert(p->RdBuff());
  p->FileReader()->AsyncRead(p->RdBuff()->wr_ptr(), p->RdBuff()->capacity());

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor::AddPipe(
    std::string const& a_name, std::string const& a_command, PipeHandler const& a_on_read,
    ErrHandler const& a_on_error, void* a_opaque, size_t a_rd_bufsz, ReadSizeEstim a_read_at_least)
{
  REACTOR_PRETTY_FUNCTION();

  UTXX_THROWX_RUNTIME_ERROR("not implemented!");

  std::unique_ptr<POpenCmd> exe;
  try {
    exe = std::unique_ptr<POpenCmd>(new POpenCmd(a_command));
  } catch (std::exception& e) {
    UTXX_THROWX_BADARG_ERROR('[', a_name, "] cannot open pipe: ", e.what());
  }

  UTXX_RLOG(
      this, TRACE5, "adding Pipe handler '", a_name, "', fd=", exe->FD(), ", Opaque=", a_opaque);

  auto p =
      Set(a_name, exe->FD(), FdTypeT::Pipe, REACTOR_EV_IN | REACTOR_EV_ET | REACTOR_EV_ERR,
          REACTOR_SRCX, a_on_error, nullptr, a_opaque, a_rd_bufsz, 0, nullptr, nullptr,
          LEVEL_TRIGGERED);
  p->SetHandler(HType::Pipe, a_on_read);
  p->m_exec_cmd.reset(exe.release());

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor::AddUDSListener(
    std::string const& a_name, std::string const& a_file_path, AcceptHandler const& a_on_accept,
    ErrHandler const& a_on_error, void* a_opaque, int a_permissions)
{
  REACTOR_PRETTY_FUNCTION();

  if (utxx::path::file_exists(a_file_path)) {
    if (IsUDSAlive(a_file_path))
      UTXX_THROWX_RUNTIME_ERROR(
          "'", a_name, "' cannot start server - UDS file busy: '", a_file_path, "'");

    if (!utxx::path::file_unlink(a_file_path))
      UTXX_THROWX_RUNTIME_ERROR(
          "'", a_name, "' couldn't remove existing UDS file: '", a_file_path, "'");
  }

  int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd < 0)
    REACTOR_THROWX_IO_ERROR(errno, '[', a_name, "] couldn't create an UDS server socket");

  int on = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  Blocking(listen_fd, false);

  struct sockaddr_un address = {};
  address.sun_family         = AF_UNIX;
  strncpy(address.sun_path, a_file_path.c_str(), sizeof(address.sun_path));
  address.sun_path[sizeof(address.sun_path) - 1] = '\0';

  if (bind(listen_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    REACTOR_THROWX_IO_ERROR(errno, '[', a_name, "] bind of fd=", listen_fd, " failed");

  if (listen(listen_fd, 5) < 0)
    REACTOR_THROWX_IO_ERROR(errno, '[', a_name, "] listen on fd=", listen_fd, " failed");

  unsigned events = REACTOR_EV_IN | REACTOR_EV_ET | REACTOR_EV_ERR;

  UTXX_RLOG(
      this, TRACE5, "adding UDS Listener '", a_name, ", Events=", EPollEvents(events),
      "', fd=", listen_fd, ", Opaque=", a_opaque);

  auto p = Set(a_name, listen_fd, FdTypeT::UNDEFINED, events, REACTOR_SRCX, a_on_error);
  p->SetHandler(HType::Accept, a_on_accept);

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor::AddEvent(
    std::string const& a_name, EventHandler const& a_on_read, ErrHandler const& a_on_error,
    void* a_opaque)
{
  REACTOR_PRETTY_FUNCTION();

  int fd = reactor_eventfd_create();
  if (fd < 0) REACTOR_THROWX_IO_ERROR(errno, "eventfd");

  UTXX_RLOG(this, TRACE5, "adding Event '", a_name, ", fd=", fd, ", Opaque=", a_opaque);

  auto p =
      Set(a_name, fd, FdTypeT::Event, REACTOR_EV_IN | REACTOR_EV_ET | REACTOR_EV_ERR, REACTOR_SRCX,
          a_on_error, nullptr, a_opaque);
  p->SetHandler(HType::Event, a_on_read);

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor::AddTimer(
    std::string const& a_name, uint32_t a_initial_msec, uint32_t a_interval_msec,
    EventHandler a_fun, ErrHandler a_on_error, void* a_opaque)
{
  REACTOR_PRETTY_FUNCTION();

  int fd = reactor_timerfd_create();
  if (fd < 0) REACTOR_THROWX_IO_ERROR(errno, "cannot create timer");

  if (reactor_timerfd_arm(m_epoll_fd, fd, a_initial_msec, a_interval_msec) < 0)
    REACTOR_THROWX_IO_ERROR(errno, "reactor_timerfd_arm");

  UTXX_RLOG(
      this, TRACE5, "adding Timer '", a_name, "', fd=", fd,
      ", initial=", utxx::fixed(double(a_initial_msec) / 1000, 3),
      ", interval=", utxx::fixed(double(a_interval_msec) / 1000, 3), ", Opaque=", a_opaque);

  // On Linux, timerfd is a real fd registered with epoll.
  // On kqueue, reactor_timerfd_create() returns a synthetic ident; the
  // EVFILT_TIMER was already registered by reactor_timerfd_arm() above,
  // so we still call Set() to track the handler but skip EPollAdd for kqueue.
#if defined(REACTOR_OS_KQUEUE)
  if (fd >= int(m_fds.size())) m_fds.resize(fd + 64);
  auto p = new FdInfo(
      this, a_name, fd, FdTypeT::Timer, a_on_error, nullptr, a_opaque, 0, 0, nullptr, nullptr,
      LEVEL_TRIGGERED);
  m_fds[fd].reset(p);
#else
  uint32_t events = REACTOR_EV_IN | REACTOR_EV_ET | REACTOR_EV_ERR;
  auto     p =
      Set(a_name, fd, FdTypeT::Timer, events, REACTOR_SRCX, a_on_error, nullptr, a_opaque, 0, 0,
          nullptr, nullptr, LEVEL_TRIGGERED);
#endif
  p->SetHandler(HType::Timer, a_fun);

  return *p;
}

//------------------------------------------------------------------------------
FdInfo& Reactor::AddSignal(
    std::string const& a_name, const sigset_t& a_mask, SigHandler const& a_fun,
    ErrHandler const& a_on_error, void* a_opaque, int a_sigq_capacity)
{
  REACTOR_PRETTY_FUNCTION();

  int fd = reactor_signalfd_create(m_epoll_fd, a_mask, a_sigq_capacity);
  if (fd < 0)
    REACTOR_THROWX_IO_ERROR(
        errno, "Cannot create signalfd for signals ", utxx::sig_members(a_mask));

  UTXX_RLOG(
      this, TRACE5, "adding Signal handler '", a_name, "', fd=", fd,
      ", signals=", utxx::sig_members(a_mask), ", Opaque=", a_opaque);

#if defined(REACTOR_OS_LINUX)
  auto rd_bufsz = sizeof(signalfd_siginfo) * a_sigq_capacity;
#else
  auto rd_bufsz = sizeof(int) * a_sigq_capacity; // placeholder; not read as signalfd_siginfo
#endif
  auto p =
      Set(a_name, fd, FdTypeT::Signal, REACTOR_EV_IN | REACTOR_EV_ET | REACTOR_EV_ERR, REACTOR_SRCX,
          a_on_error, nullptr, a_opaque, rd_bufsz, 0, nullptr, nullptr, LEVEL_TRIGGERED);
  p->SetHandler(HType::Signal, a_fun);

  return *p;
}

//------------------------------------------------------------------------------
void Reactor::Remove(int& a_fd, bool a_clear_fdinfo)
{
  REACTOR_PRETTY_FUNCTION();

  if (UNLIKELY(a_fd < 0 || !m_fds[a_fd])) return;

  if (UNLIKELY(a_fd >= int(m_fds.size()))) UTXX_THROWX_BADARG_ERROR("invalid fd=", a_fd);

  auto p = m_fds[a_fd].get();
  if (!p) return;

  if (a_clear_fdinfo) CloseFD(a_fd);

  p->FD(-1);
  p->Clear();
}

//------------------------------------------------------------------------------
dynamic_io_buffer* Reactor::SubscribeWrite(int a_fd, bool a_on)
{
  REACTOR_PRETTY_FUNCTION();

  if (UNLIKELY(a_fd < 0 || a_fd >= int(m_fds.size())))
    UTXX_THROWX_BADARG_ERROR("invalid fd=", a_fd);

  auto p = m_fds[a_fd].get();
  if (UNLIKELY(!p)) UTXX_THROWX_BADARG_ERROR("empty fd=", a_fd);

  auto& info = *p;

  if (UNLIKELY(!info.m_wr_buff)) UTXX_THROWX_BADARG_ERROR("write buffer not assigned!");

  uint32_t mask = REACTOR_EV_IN | REACTOR_EV_ET | REACTOR_EV_ERR | REACTOR_EV_RDHUP |
                  (a_on ? REACTOR_EV_OUT : 0u);

  try {
    reactor_mod(m_epoll_fd, a_fd, mask, (uintptr_t)a_fd);
  } catch (...) {
    REACTOR_THROWX_IO_ERROR(errno, "failed to modify events on fd=", a_fd);
  }

  return info.WrBuff();
}

} // namespace utxx
