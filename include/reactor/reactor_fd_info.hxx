// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_fd_info.hxx
//------------------------------------------------------------------------------
/// \brief FdInfo non-inline implementations (was in ReactorFdInfo.cpp)
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <reactor/reactor.hxx>
#include <reactor/reactor_fd_info.hpp>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/un.h>

namespace utxx {

//------------------------------------------------------------------------------
inline FdInfo::~FdInfo()
{
  if (m_fd >= 0) m_owner->Remove(m_fd);
  Clear();
}

//------------------------------------------------------------------------------
inline void FdInfo::Clear()
{
  if (m_fd != -1) m_owner->Remove(m_fd, false);
  Reset();
}

//------------------------------------------------------------------------------
inline void FdInfo::Reset()
{
  assert(m_fd == -1);

  // clang-format off
  m_handler.Clear();
  m_on_error      = nullptr;
  m_read_at_least = nullptr;
  m_instance      = nullptr;
  m_opaque        = nullptr;
  m_rd_buff_ptr.reset();
  m_rd_buff       = nullptr;
  m_wr_buff_ptr.reset();
  m_wr_buff       = nullptr;
  m_rd_debug      = nullptr;
  m_file_reader.reset();
  m_exec_cmd.reset();
  m_with_pkt_info = false;
  m_sock_src_addr = 0;
  m_sock_src_port = 0;
  m_sock_dst_addr = 0;
  m_sock_dst_port = 0;
  m_sock_if_addr  = 0;
  // clang-format on
}

//------------------------------------------------------------------------------
inline FdInfo::FdInfo(FdInfo&& a_rhs) noexcept
    : m_owner(a_rhs.m_owner), m_name(std::move(a_rhs.m_name)), m_fd(a_rhs.m_fd),
      m_handler(std::move(a_rhs.m_handler)), m_on_error(std::move(a_rhs.m_on_error)),
      m_read_at_least(a_rhs.m_read_at_least), m_instance(a_rhs.m_instance),
      m_opaque(a_rhs.m_opaque), m_rd_buff_ptr(std::move(a_rhs.m_rd_buff_ptr)),
      m_rd_buff(m_rd_buff_ptr.get()), m_wr_buff_ptr(std::move(a_rhs.m_wr_buff_ptr)),
      m_wr_buff(a_rhs.m_wr_buff), m_rd_debug(std::move(a_rhs.m_rd_debug)),
      m_trigger(a_rhs.m_trigger), m_with_pkt_info(a_rhs.m_with_pkt_info),
      m_sock_src_addr(a_rhs.m_sock_src_addr), m_sock_src_port(a_rhs.m_sock_src_port),
      m_sock_dst_addr(a_rhs.m_sock_dst_addr), m_sock_dst_port(a_rhs.m_sock_dst_port),
      m_sock_if_addr(a_rhs.m_sock_if_addr)
{ a_rhs.m_fd = -1; }

//------------------------------------------------------------------------------
inline FdInfo& FdInfo::operator=(FdInfo&& a_rhs)
{
  assert(m_fd == -1);
  if (Type() != HType::UNDEFINED) Clear();

  m_owner         = a_rhs.m_owner;
  m_fd            = a_rhs.m_fd;
  a_rhs.m_fd      = -1;
  m_handler       = std::move(a_rhs.m_handler);
  m_name          = std::move(a_rhs.m_name);
  m_on_error      = std::move(a_rhs.m_on_error);
  m_read_at_least = a_rhs.m_read_at_least;
  m_instance      = a_rhs.m_instance;
  m_opaque        = a_rhs.m_opaque;
  m_rd_buff_ptr   = std::move(a_rhs.m_rd_buff_ptr);
  m_rd_buff       = m_rd_buff_ptr.get();
  m_wr_buff_ptr   = std::move(a_rhs.m_wr_buff_ptr);
  m_wr_buff       = a_rhs.m_wr_buff;
  m_rd_debug      = std::move(a_rhs.m_rd_debug);
  m_file_reader   = std::move(a_rhs.m_file_reader);
  m_exec_cmd      = std::move(a_rhs.m_exec_cmd);
  m_trigger       = a_rhs.m_trigger;
  m_with_pkt_info = a_rhs.m_with_pkt_info;
  m_sock_src_addr = a_rhs.m_sock_src_addr;
  m_sock_src_port = a_rhs.m_sock_src_port;
  m_sock_dst_addr = a_rhs.m_sock_dst_addr;
  m_sock_dst_port = a_rhs.m_sock_dst_port;
  m_sock_if_addr  = a_rhs.m_sock_if_addr;

  return *this;
}

//------------------------------------------------------------------------------
inline void FdInfo::EnableDgramPktInfo(bool a_enable)
{
  REACTOR_PRETTY_FUNCTION();

  if (m_fd_type != FdTypeT::Datagram)
    throw utxx::runtime_error(
        REACTOR_SRC,
        utxx::detail::concat(
            "Cannot ", a_enable ? "enable" : "disable", " PktInfo for a non-UDP socket!"));

  m_with_pkt_info = a_enable;
  int       on    = a_enable;
  socklen_t len   = sizeof(on);

  if (setsockopt(m_fd, SOL_IP, IP_PKTINFO, &on, sizeof(on)) < 0)
    throw utxx::io_error(
        REACTOR_SRC, errno, utxx::detail::concat("Error setting IP_PKTINFO option on ", m_name));

  if (getsockopt(m_fd, SOL_IP, IP_PKTINFO, &on, &len) < 0 || !on)
    throw utxx::io_error(
        REACTOR_SRC, errno,
        utxx::detail::concat("IP_PKTINFO option couldn't be set on ", m_name));

  struct sockaddr_in     si;
  static const socklen_t si_len = sizeof(si);
  m_sock_dst_port = (m_owner && !m_owner->UseGetSockName()) ||
                            getsockname(m_fd, (sockaddr*)&si, const_cast<socklen_t*>(&si_len)) < 0
                      ? 0
                      : si.sin_port;
}

//------------------------------------------------------------------------------
inline void FdInfo::EnablePktTimeStamps(bool a_enable)
{
  REACTOR_PRETTY_FUNCTION();

  if (m_fd_type != FdTypeT::Datagram)
    throw utxx::runtime_error(REACTOR_SRC, "Cannot set pkt time stamps on a non-UDP socket!");

  m_pkt_time_stamps = a_enable;
  int on            = a_enable;

  if (setsockopt(m_fd, SOL_SOCKET, SO_TIMESTAMPNS, &on, sizeof(on)) < 0)
    throw utxx::io_error(
        REACTOR_SRC, errno,
        utxx::detail::concat("Error setting SO_TIMESTAMPNS on fd=", m_fd, ' ', m_name));

  m_ts_wire = time_val{};
}

//------------------------------------------------------------------------------
inline int
FdInfo::ReportError(IOType a_tp, int a_ec, const std::string& a_err, src_info&& a_si, bool a_throw)
{
  REACTOR_PRETTY_FUNCTION();

  auto cleanup = utxx::scope_exit([this]() { Clear(); });

  auto err     = a_ec ? utxx::to_string(a_err, " (", a_ec, ": ", strerror(a_ec), ')') : a_err;

  if (!m_on_error) {
    if (!a_throw) {
      UTXX_RLOG(this, DEBUG, a_si.srcloc(), " UNHANDLED error: ", a_err);
      return -1;
    }
    throw utxx::runtime_error(a_si, m_ident + err);
  }

  m_on_error(*this, a_tp, err, std::move(a_si));
  return -1;
}

//------------------------------------------------------------------------------
inline long FdInfo::HandlePipe(uint32_t a_events)
{
  REACTOR_PRETTY_FUNCTION();

  if (!Reactor::IsReadable(a_events)) return 0;

  assert(m_rd_buff);

  try {
    bool handled;
    int  rc;

    std::tie(rc, handled) = ReadUntilEAgain(
        [this](dynamic_io_buffer& a_buf, int /*a_last_bytes*/) {
          return m_handler.AsPipe()(*this, a_buf);
        },
        m_rd_debug);

    if (handled || rc >= 0) return rc;

    rc = SocketError(m_fd);
    return ReportError(IOType::Read, rc, "error in Pipe handler", REACTOR_SRCX);

  } catch (utxx::runtime_error& e) {
    return ReportError(IOType::UserCode, 0, e.str(), src_info(e.src()));
  } catch (std::exception& e) {
    return ReportError(IOType::UserCode, 0, e.what(), REACTOR_SRCX);
  }
}

//------------------------------------------------------------------------------
inline long FdInfo::HandleFile(uint32_t a_events)
{
  REACTOR_PRETTY_FUNCTION();

  auto rc = HandleEvent(a_events, false);

  if (UNLIKELY(rc <= 0)) return rc;

  auto file = m_file_reader.get();
  assert(file);

  const char* err;
  long        size;
  std::tie(size, err) = file->ReadEvents(rc);

  if (UNLIKELY(size < 0))
    return ReportError(IOType::Read, errno, err, REACTOR_SRCX);
  else if (UNLIKELY(size == 0)) {
    if (file->Remaining() == 0) {
      m_handler.AsFile()(*this, *m_rd_buff);
      return 0;
    }
    return ReportError(
        IOType::Read, 0,
        utxx::to_string("no file input events (remaining: ", file->Remaining(), ')'),
        REACTOR_SRCX);
  }

  m_rd_buff->commit(size);

  bool invoke_cb = true;
  auto got       = m_rd_buff->size();

  if (m_read_at_least) {
    auto p    = m_rd_buff->rd_ptr();
    auto need = m_read_at_least(p, got);
    if (need > (int)got) {
      if (UNLIKELY(need > 100 * 1024 * 1024)) {
        auto e = utxx::to_string("suspicious read size = ", need);
        return ReportError(IOType::Read, EMSGSIZE, e, REACTOR_SRCX);
      }
      m_rd_buff->reserve(need);
      invoke_cb = false;
    }
  }

  if (LIKELY(invoke_cb)) {
    try {
      int cb_rc = m_handler.AsFile()(*this, *m_rd_buff);
      if (UNLIKELY(cb_rc < 0))
        return ReportError(
            IOType::Read, 0,
            utxx::to_string(
                "error processing file data ", file->Filename(), " at pos=", file->Position(),
                " of ", file->Size(), (errno ? ": " : ""), (errno ? strerror(errno) : "")),
            REACTOR_SRCX);
    } catch (utxx::runtime_error& e) {
      return ReportError(IOType::UserCode, 0, e.str(), e.src());
    } catch (std::exception& e) {
      return ReportError(IOType::UserCode, 0, e.what(), REACTOR_SRCX);
    }
  }

  if (UNLIKELY(!file->Remaining())) {
    UTXX_RLOG(this, DEBUG, "file read done (offset=", file->Offset(), ", size=", file->Size(), ')');

    if (m_on_error) m_on_error(*this, IOType::EndOfFile, "end-of-file reached", REACTOR_SRCX);

    rc = got - m_rd_buff->size();
    Clear();
    return rc;
  }

  auto capacity = m_rd_buff->capacity();

  if (UNLIKELY(capacity == 0))
    return ReportError(IOType::UserCode, 0, "not enough buffer capacity", REACTOR_SRC);

  rc = file->AsyncRead(m_rd_buff->wr_ptr(), capacity);

  if (UNLIKELY(rc < 0)) return ReportError(IOType::Read, errno, "error in AsyncRead", REACTOR_SRC);

  return rc;
}

//------------------------------------------------------------------------------
inline long FdInfo::HandleSignal(uint32_t a_events)
{
  REACTOR_PRETTY_FUNCTION();

  if (UNLIKELY(!Reactor::IsReadable(a_events))) return 0;

  assert(m_rd_buff != nullptr);

  try {
    bool handled;
    int  rc;

    std::tie(rc, handled) = ReadUntilEAgain(
        [this](dynamic_io_buffer const& a_buf, int /*a_last_bytes*/) -> int {
#if defined(REACTOR_OS_LINUX)
          auto p = reinterpret_cast<const signalfd_siginfo*>(a_buf.rd_ptr());
          auto e = reinterpret_cast<const signalfd_siginfo*>(a_buf.rd_ptr() + a_buf.size());

          for (auto next = p + 1; next <= e; p = next++)
            m_handler.AsSignal()(*this, p->ssi_signo, p->ssi_code);

          return int(reinterpret_cast<const char*>(p) - a_buf.rd_ptr());
#else
          // On kqueue EVFILT_SIGNAL the signal number is the kevent ident,
          // not delivered via a read; invoke the handler with the fd as signo.
          m_handler.AsSignal()(*this, m_fd, 0);
          return (int)a_buf.size();
#endif
        },
        nullptr);

    if (LIKELY(handled || rc >= 0)) return rc;

    return ReportError(IOType::Read, errno, "error reading from signalfd", REACTOR_SRCX);
  } catch (utxx::runtime_error& e) {
    return ReportError(IOType::UserCode, 0, e.what(), e.src());
  } catch (std::exception& e) {
    return ReportError(IOType::UserCode, 0, e.what(), REACTOR_SRCX);
  }
}

//------------------------------------------------------------------------------
inline long FdInfo::HandleAccept(uint32_t a_events)
{
  REACTOR_PRETTY_FUNCTION();

  if (UNLIKELY(!Reactor::IsReadable(a_events))) return 0;

  struct sockaddr_un addr;
  struct sockaddr*   paddr = (struct sockaddr*)&addr;
  socklen_t          len   = sizeof(addr);
  addr.sun_family          = AF_UNIX;

  while (true) {
    int cli_fd;

    while (UNLIKELY((cli_fd = accept(m_fd, paddr, &len)) < 0 && errno == EINTR));

    if (cli_fd < 0) {
      if (LIKELY(errno == EAGAIN)) break;
      return ReportError(IOType::Accept, errno, "error in accept(2)", REACTOR_SRCX);
    }

    len                -= offsetof(struct sockaddr_un, sun_path);
    addr.sun_path[len]  = 0;

    Blocking(cli_fd, false);

    if (!m_handler.AsAccept()(*this, addr.sun_path, cli_fd)) ::close(cli_fd);
  }

  return 0;
}

} // namespace utxx
