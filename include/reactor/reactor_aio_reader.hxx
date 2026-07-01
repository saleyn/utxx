// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_aio_reader.hxx
//------------------------------------------------------------------------------
/// \brief AIOReader inline implementations
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <assert.h>
#include <fcntl.h>
#include <reactor/compat.hpp>
#include <reactor/reactor_aio_reader.hpp>
#include <reactor/reactor_platform.hpp>
#include <utility>

#if defined(REACTOR_OS_LINUX)
#include <libaio.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>

// Use a direct syscall to avoid sporadic EINVAL/EFAULT with some libaio builds
namespace {
  inline int io_getevents_ex(
    io_context_t ctx, long min_nr, long max_nr, struct io_event* events, struct timespec* timeout)
  {
    return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
  }
} // namespace
#endif

namespace utxx {

//------------------------------------------------------------------------------
inline void AIOReader::Clear()
{
  if (m_efd >= 0) {
    reactor_eventfd_close(m_efd);
    m_efd = -1;
  }
  if (m_file_fd >= 0) {
    ::close(m_file_fd);
    m_file_fd = -1;
  }
#if defined(REACTOR_OS_LINUX)
  if (m_ctx) {
    ::io_destroy(m_ctx);
    m_ctx = nullptr;
  }
#endif
}

//------------------------------------------------------------------------------
inline void AIOReader::Init(int a_efd, const char* a_filename)
{
#if !defined(REACTOR_OS_LINUX)
  (void)a_efd;
  (void)a_filename;
  throw utxx::io_error(REACTOR_SRC, ENOSYS, "AIOReader: not supported on this platform");
#else
  Clear();
  m_efd       = a_efd;
  m_filename  = a_filename;
  m_position  = 0;
  m_offset    = 0;
  m_async_ops = 0;

  if (a_efd < 0) throw utxx::io_error(REACTOR_SRC, errno, "invalid eventfd descriptor");

  int flags = O_RDONLY | O_NONBLOCK | O_LARGEFILE;
  m_file_fd = ::open(a_filename, flags);

  if (m_file_fd < 0)
    throw utxx::io_error(
        REACTOR_SRC, errno, utxx::detail::concat("cannot open ", a_filename));

  auto guard  = utxx::scope_exit([this]() { ::close(m_file_fd); });

  m_file_size = utxx::path::file_size(m_file_fd);
  m_ctx       = nullptr;

  if (io_setup(128, &m_ctx) < 0)
    throw utxx::io_error(
        REACTOR_SRC, errno,
        utxx::detail::concat("failed to do AIO setup on file '", a_filename, "'"));

  m_piocb[0] = &m_iocb[0];

  guard.disable();
#endif
}

//------------------------------------------------------------------------------
inline int AIOReader::AsyncRead(char* a_buf, size_t a_size)
{
#if !defined(REACTOR_OS_LINUX)
  (void)a_buf;
  (void)a_size;
  errno = ENOSYS;
  return -1;
#else
  auto rem = Remaining();
  if (UNLIKELY(rem == 0 || m_async_ops)) {
    errno = rem == 0 ? ENODATA : EALREADY;
    return -1;
  }

  auto sz = std::min<long>(rem, a_size);
  ::io_prep_pread(&m_iocb[0], m_file_fd, a_buf, sz, m_offset);
  ::io_set_eventfd(&m_iocb[0], m_efd);
  m_iocb[0].data = (void*)this;
  int rc         = io_submit(m_ctx, 1, m_piocb);

  if (LIKELY(rc >= 0))
    m_async_ops++;
  else
    errno = -rc;

  m_position = m_offset;
  return rc;
#endif
}

//------------------------------------------------------------------------------
inline int AIOReader::CheckEvents()
{
#if !defined(REACTOR_OS_LINUX)
  errno = ENOSYS;
  return -1;
#else
  int       n;
  eventfd_t events = 0;
  while (UNLIKELY((n = ::eventfd_read(m_efd, &events) < 0) && errno == EINTR));
  return UNLIKELY(n < 0) ? n : (int)events;
#endif
}

//------------------------------------------------------------------------------
inline std::pair<long, const char*> AIOReader::ReadEvents(int a_events)
{
#if !defined(REACTOR_OS_LINUX)
  (void)a_events;
  errno = ENOSYS;
  return {-1L, "AIO not supported"};
#else
  assert(a_events <= 1);

  if (UNLIKELY(a_events == 0)) return {0L, ""};

  static struct timespec tmo = {.tv_sec = 0, .tv_nsec = 0};
  struct io_event        events[1];
  int                    n;

  do {
    n = ::io_getevents_ex(m_ctx, 0, a_events, events, &tmo);
  } while (UNLIKELY(n == -EINTR));

  if (UNLIKELY(n == 0)) return {0, ""};
  if (UNLIKELY(n < 0)) {
    errno = -n;
    return {-1L, "io_getevents()"};
  }

  assert(n <= 1);
  assert(&m_iocb[0] == events[0].obj);

  m_async_ops  = std::max<int>(0, m_async_ops - n);
  long got     = events[0].res;
  m_offset    += got;

  return {got, ""};
#endif
}

} // namespace utxx
