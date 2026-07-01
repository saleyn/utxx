// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor.hxx
//------------------------------------------------------------------------------
/// \brief Reactor + FdInfo inline implementations
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <reactor/reactor.hpp>
#include <reactor/reactor_log.hpp>

namespace utxx {

//==============================================================================
// Reactor inline methods
//==============================================================================

inline dynamic_io_buffer* Reactor::RdBuff(int a_fd)
{ return Get(a_fd, REACTOR_SRC).RdBuff(); }

inline dynamic_io_buffer* Reactor::WrBuff(int a_fd)
{ return Get(a_fd, REACTOR_SRC).WrBuff(); }

inline void Reactor::Ident(const std::string& a_ident)
{ m_ident = utxx::to_string('[', a_ident, '@', m_epoll_fd, "] "); }

inline void Reactor::CloseFD(int& a_fd)
{
  if (a_fd < 0) return;
  if (m_epoll_fd >= 0) reactor_del(m_epoll_fd, a_fd);
  (void)close(a_fd);
  a_fd = -1;
}

template <typename StreamT>
inline void Reactor::DefRdDebug(StreamT& out, const FdInfo& a_fi, const char* a_buf, size_t a_sz)
{
  utxx::verbosity::if_enabled(utxx::VERBOSE_WIRE, [&]() {
    out << '[' << a_fi.Name() << ", fd=" << a_fi.FD() << "] <- " << a_sz << " bytes "
        << utxx::to_bin_string(a_buf, std::min(a_sz, 256ul)) << std::endl;
  });
}

template <class... Args>
inline void Reactor::Log(log_level a_level, src_info&& a_sinfo, Args&&... args)
{
  if (m_logger)
    m_logger(a_level, std::move(a_sinfo), utxx::print(std::forward<Args>(args)...));
  else
    DefaultLog(a_level, std::move(a_sinfo), std::forward<Args>(args)...);
}

inline void Reactor::Wait(int a_timeout_msec)
{
  REACTOR_PRETTY_FUNCTION();

  static const int N = 256;
  reactor_event_t  cev[N];

REPEAT:
  int rc = reactor_wait(m_epoll_fd, cev, N, a_timeout_msec);

  if (UNLIKELY(rc < 0)) {
    if (errno == EINTR) goto REPEAT;
    REACTOR_THROWX_IO_ERROR(errno, "reactor_wait failed");
  }

  int i = 0;

  for (reactor_event_t *p = cev, *end = cev + rc; p != end; ++p) {
    int      fd     = reactor_ev_fd(*p);
    uint32_t events = reactor_ev_mask(*p);

    if (UNLIKELY(fd < 0 || fd >= int(m_fds.size()))) {
      UTXX_RLOG(this, DEBUG, "fd=", fd, " not found!");
      continue;
    }

    UTXX_RLOG(
        this, TRACE5, "processing ", ++i, '/', rc, ' ',
        (fd < int(m_fds.size()) && m_fds[fd] ? m_fds[fd]->Name() : "INVALID FD "), "(fd=", fd,
        ", events=", EPollEvents(events), ')');

    FdInfo& info    = *m_fds[fd];

    auto    cleanup = [this, &fd, &info]() {
      UTXX_RLOG(&info, TRACE5, "closing fd ", fd, " on negative return from handler");
      CloseFD(fd);
      info.FD(-1);
      info.Clear();
    };

    rc = 0;

    try {
      if (UNLIKELY(IsError(events))) {
        auto        tp = IsWritable(events) ? IOType::Write : IOType::Read;
        auto        ec = SocketError(fd);
        std::string err;

        if (ec == EAGAIN || ec == EINTR)
          continue;
        else if (tp == IOType::Write && ec == EINPROGRESS) {
          err = "failed to connect to remote address";
          tp  = IOType::Connect;
        } else if (ec == ENOTSOCK)
          continue;
        else
          err = strerror(ec);

        info.ReportError(tp, 0, err, REACTOR_SRCX, false);
        rc = -1;
      } else if (LIKELY(info.m_fd != -1)) {
        rc = info.Handle(events);
      }
    } catch (...) {
      cleanup();
      throw;
    }

    if (UNLIKELY(rc < 0 && errno != EAGAIN)) cleanup();
  }

  if (rc == 0 && m_on_idle) m_on_idle();
}

//==============================================================================
// FdInfo inline methods
//==============================================================================

inline FdInfo::FdInfo(
    Reactor* a_owner, std::string const& a_name, int a_fd, FdTypeT a_fd_type,
    ErrHandler const& a_on_error, void* a_instance, void* a_opaque, int a_rd_bufsz, int a_wr_bufsz,
    dynamic_io_buffer* a_wr_buf, ReadSizeEstim a_read_sz_fun, TriggerT a_trigger)
    : m_owner(a_owner), m_name(a_name), m_fd(a_fd), m_fd_type(FdTypeT::UNDEFINED), m_handler(),
      m_on_error(a_on_error), m_read_at_least(std::move(a_read_sz_fun)), m_instance(a_instance),
      m_opaque(a_opaque), m_rd_buff_ptr(a_rd_bufsz ? new dynamic_io_buffer(a_rd_bufsz) : nullptr),
      m_rd_buff(m_rd_buff_ptr.get()),
      m_wr_buff_ptr(!a_wr_buf && a_wr_bufsz ? new dynamic_io_buffer(a_wr_bufsz) : nullptr),
      m_wr_buff(a_wr_buf ? a_wr_buf : m_wr_buff_ptr.get()), m_trigger(a_trigger),
      m_ident(Ident(a_owner, a_fd, a_name))
{
  int       type;
  socklen_t len = sizeof(type);
  if (m_fd > -1 && a_fd_type == FdTypeT::UNDEFINED &&
      getsockopt(m_fd, SOL_SOCKET, SO_TYPE, &type, &len) == 0) {
    switch (type) {
      case SOCK_STREAM:    m_fd_type = FdTypeT::Stream; break;
      case SOCK_DGRAM:     m_fd_type = FdTypeT::Datagram; break;
      case SOCK_SEQPACKET: m_fd_type = FdTypeT::SeqPacket; break;
      default:             break;
    }
  }
}

inline std::string FdInfo::Ident(Reactor* a_reactor, int a_fd, std::string const& a_name)
{
  if (!a_reactor) return utxx::to_string("[@", a_fd, '(', a_name, ")] ");
  auto s  = a_reactor->Ident();
  auto it = s.find(']');
  if (it != std::string::npos) s.erase(it);
  return utxx::to_string(s, '@', a_fd, '(', a_name, ")] ");
}

template <typename Action>
inline void FdInfo::SetHandler(HType a_type, const Action& a)
{ m_handler = HandlerT(a_type, a); }

inline int FdInfo::Debug() const
{
  assert(m_owner);
  return m_owner->Debug();
}

inline int FdInfo::ReportError(
    IOType a_tp, int a_ec, const std::string& a_err, const src_info& a_si, bool a_throw)
{ return ReportError(a_tp, a_ec, a_err, src_info(a_si), a_throw); }

template <class... Args>
inline void FdInfo::Log(log_level a_level, src_info&& a_si, Args&&... args)
{
  assert(m_owner);
  m_owner->Log(a_level, std::move(a_si), std::forward<Args>(args)...);
}

//------------------------------------------------------------------------------
// Core read loop — also handles packet-info (UDP src/dst/iface addresses)
//------------------------------------------------------------------------------
template <typename Action, typename DebugAction>
inline std::pair<int, bool>
FdInfo::ReadUntilEAgain(Action const& a_action, DebugAction const& a_debug_act)
{
  REACTOR_PRETTY_FUNCTION();

  int                bytes = 0;
  int                got;
  long               space;
  char*              buf;

  struct iovec       iov[1];
  struct msghdr      msg;
  struct sockaddr_in peeraddr;
  char               ctlbuf[256];

  if (m_with_pkt_info) {
    msg.msg_name       = (void*)&peeraddr;
    msg.msg_namelen    = sizeof(peeraddr);
    msg.msg_iov        = iov;
    msg.msg_iovlen     = 1;
    msg.msg_flags      = 0;
    msg.msg_control    = ctlbuf;
    msg.msg_controllen = sizeof(ctlbuf);
  }

  do {
    buf   = m_rd_buff->wr_ptr();
    space = m_rd_buff->capacity();

    if (UNLIKELY(space == 0)) {
      errno  = ENOBUFS;
      int rc = ReportError(IOType::Read, errno, "buffer overflow", REACTOR_SRC);
      return {rc, true};
    }

    do {
      if (m_with_pkt_info) {
        iov[0].iov_base = buf;
        iov[0].iov_len  = space;
        got             = ::recvmsg(m_fd, &msg, 0);
      } else
        got = ::read(m_fd, buf, space);
    } while (UNLIKELY(got < 0 && errno == EINTR));

    if (UNLIKELY(got <= 0)) {
      if (got == 0) return {got, false};
      bool handled = errno == EAGAIN;
      return {got, handled};
    }

    if (m_with_pkt_info) {
      m_sock_src_addr = peeraddr.sin_addr.s_addr;
      m_sock_src_port = peeraddr.sin_port;
      m_sock_if_addr  = 0;
      m_sock_dst_addr = 0;
      int cont        = 1 + m_pkt_time_stamps;
      for (auto cm = CMSG_FIRSTHDR(&msg); cm && cont; cm = CMSG_NXTHDR(&msg, cm)) {
        if (cm->cmsg_level == IPPROTO_IP && cm->cmsg_type == IP_PKTINFO) {
          auto pi         = reinterpret_cast<struct in_pktinfo*>(CMSG_DATA(cm));
          m_sock_if_addr  = pi->ipi_spec_dst.s_addr;
          m_sock_dst_addr = pi->ipi_addr.s_addr;
          cont--;
        } else if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPNS) {
          auto ts = reinterpret_cast<timespec const*>(CMSG_DATA(cm));
          assert(ts != nullptr);
          m_ts_wire = time_val(*ts);
          if (LIKELY(m_pkt_time_stamps)) cont--;
        }
      }
    }

    DebugFunEval::run(a_debug_act, buf, got);

    if (LIKELY(got > 0)) m_rd_buff->commit(got);

    bytes += got;

    if (m_read_at_least) {
      try {
        auto p    = m_rd_buff->rd_ptr();
        auto have = m_rd_buff->size();
        int  need = m_read_at_least(p, have);
        if (need > got) {
          if (UNLIKELY(need > 100 * 1024 * 1024)) {
            errno  = EMSGSIZE;
            auto e = utxx::to_string("suspicious read size = ", need);
            return {ReportError(IOType::Read, errno, e, REACTOR_SRC), true};
          }
          m_rd_buff->reserve(need);
          return {bytes, false};
        }
      } catch (utxx::runtime_error& e) {
        return {ReportError(IOType::Read, 0, e.str(), e.src()), true};
      } catch (std::exception& e) {
        return {ReportError(IOType::Read, 0, e.what(), REACTOR_SRC), true};
      }
    }

    try {
      int consumed = a_action(*m_rd_buff, got);
      if (UNLIKELY(consumed < 0) || !m_rd_buff)
        return {consumed, false};
      else if (LIKELY(consumed > 0))
        m_rd_buff->read_and_crunch(consumed);
    } catch (utxx::runtime_error& e) {
      return {ReportError(IOType::Read, 0, e.str(), e.src()), true};
    } catch (std::exception& e) {
      return {ReportError(IOType::Read, 0, e.what(), REACTOR_SRC), true};
    }

  } while (m_trigger == EDGE_TRIGGERED);

  return {bytes, true};
}

//------------------------------------------------------------------------------
inline long FdInfo::Handle(uint32_t a_events)
{
  REACTOR_PRETTY_FUNCTION();

  switch (Type()) {
    case HType::IO:     return HandleIO(a_events);
    case HType::RawIO:  return HandleRawIO(a_events);
    case HType::Pipe:   return HandlePipe(a_events);
    case HType::File:   return HandleFile(a_events);
    case HType::Event:  return HandleEvent(a_events, true);
    case HType::Timer:  return HandleTimer(a_events);
    case HType::Accept: return HandleAccept(a_events);
    case HType::Signal: return HandleSignal(a_events);
    default:            UTXX_RLOG(this, DEBUG, "fd=", m_fd, " undefined handler type"); return -1;
  }
}

//------------------------------------------------------------------------------
inline long FdInfo::HandleIO(uint32_t a_events)
{
  REACTOR_PRETTY_FUNCTION();

  bool handled;
  int  rc;

  if (Reactor::IsReadable(a_events)) {
    assert(m_rd_buff);
    try {
      std::tie(rc, handled) = ReadUntilEAgain(
          [this](dynamic_io_buffer& a_buf, int /*a_last_bytes*/) {
            return m_handler.AsIO().rh(*this, a_buf);
          },
          m_rd_debug);

      if (handled || rc > 0 || m_fd < 0) return rc;

      rc       = SocketError(m_fd);
      auto err = rc ? strerror(rc) : "connection closed by peer";
      return ReportError(IOType::Read, rc, err, REACTOR_SRCX);

    } catch (utxx::runtime_error& e) {
      return ReportError(IOType::UserCode, 0, e.str(), src_info(e.src()));
    } catch (std::exception& e) {
      return ReportError(IOType::UserCode, 0, e.what(), REACTOR_SRCX);
    }
  } else if (Reactor::IsWritable(a_events)) {
    assert(m_handler.AsIO().wh);
    assert(m_wr_buff);
    try {
      return m_handler.AsIO().wh(*this, *m_wr_buff);
    } catch (utxx::runtime_error& e) {
      return ReportError(IOType::UserCode, 0, e.str(), src_info(e.src()));
    } catch (std::exception& e) {
      return ReportError(IOType::UserCode, 0, e.what(), REACTOR_SRCX);
    }
  } else {
    return 0;
  }

  return ReportError(IOType::AppLogic, 0, "LOGIC ERROR: unhandled branch", REACTOR_SRCX);
}

//------------------------------------------------------------------------------
inline long FdInfo::HandleRawIO(uint32_t a_events)
{
  REACTOR_PRETTY_FUNCTION();

  auto io_type = (a_events & REACTOR_EV_OUT) ? IOType::Write : IOType::Read;

  assert(m_handler.AsRawIO() != nullptr);
  try {
    m_handler.AsRawIO()(*this, io_type, a_events);
  } catch (utxx::runtime_error& e) {
    return ReportError(IOType::Read, 0, e.what(), e.src());
  } catch (std::exception& e) {
    return ReportError(IOType::Read, 0, e.what(), REACTOR_SRCX);
  }
  return 0;
}

//------------------------------------------------------------------------------
inline long FdInfo::HandleEvent(uint32_t a_events, bool a_invoke_handler)
{
  REACTOR_PRETTY_FUNCTION();

  static const char s_err[] = "error reading eventfd";

  if (UNLIKELY(!Reactor::IsReadable(a_events))) return 0;

  int64_t events = 0;
  int     n;

  do {
    n = ::read(m_fd, &events, sizeof(events));
  } while (UNLIKELY(n < 0 && errno == EINTR));

  if (UNLIKELY(n <= 0)) {
    if (n < 0 && errno == EAGAIN) return 0;
    return ReportError(IOType::Read, errno, s_err, REACTOR_SRCX);
  }

  if (a_invoke_handler) {
    try {
      m_handler.AsEvent()(*this, events);
    } catch (utxx::runtime_error& e) {
      return ReportError(IOType::UserCode, 0, e.what(), e.src());
    } catch (std::exception& e) {
      return ReportError(IOType::UserCode, 0, e.what(), REACTOR_SRCX);
    }
  }

  return events;
}

//------------------------------------------------------------------------------
inline long FdInfo::HandleTimer(uint32_t a_events)
{
  REACTOR_PRETTY_FUNCTION();

  if (UNLIKELY(!Reactor::IsReadable(a_events))) return 0;

  uint64_t exp = 0;
  int      got = reactor_timerfd_read(m_fd, exp);

  if (UNLIKELY(got < 0))
    return ReportError(IOType::Read, errno, "error reading from timerfd", REACTOR_SRCX);

  try {
    m_handler.AsTimer()(*this, exp);
  } catch (utxx::runtime_error& e) {
    return ReportError(IOType::UserCode, 0, e.what(), e.src());
  } catch (std::exception& e) {
    return ReportError(IOType::UserCode, 0, e.what(), REACTOR_SRCX);
  }

  return got;
}

} // namespace utxx

#include <reactor/reactor_fd_info.hxx>
#include <reactor/reactor_types.hxx>
