// vim:ts=2:sw=2:et
//----------------------------------------------------------------------------
/// \file   buffer.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Buffer classes for I/O operations — standalone copy with no
///        boost or utxx dependencies.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

***** END LICENSE BLOCK *****
*/
#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits.h>
#include <memory>
#include <stdexcept>

namespace utxx {

namespace detail {
  template <class Alloc>
  class basic_dynamic_io_buffer;
}

/**
 * \brief Basic buffer providing stack space for data up to N bytes and
 *        heap space when reallocation of space over N bytes is needed.
 * A typical use of the buffer is for I/O operations that require tracking
 * of produced and consumed space.
 */
template <int N, typename Alloc = std::allocator<char>>
class basic_io_buffer : private std::allocator_traits<Alloc>::template rebind_alloc<char> {
  using alloc_t = typename std::allocator_traits<Alloc>::template rebind_alloc<char>;

  // Non-copyable
  basic_io_buffer(const basic_io_buffer&)            = delete;
  basic_io_buffer& operator=(const basic_io_buffer&) = delete;

protected:
  char*  m_begin;
  char*  m_end;
  char*  m_rd_ptr;
  char*  m_wr_ptr;
  size_t m_wr_lwm;
  char   m_data[N > 0 ? N : 1];

  void   repoint(basic_io_buffer&& a_rhs)
  {
    if (m_begin != m_data && m_end > m_begin) alloc_t::deallocate(m_begin, max_size());

    if (a_rhs.allocated()) {
      m_rd_ptr = a_rhs.m_rd_ptr;
      m_wr_ptr = a_rhs.m_wr_ptr;
      m_begin  = a_rhs.m_begin;
      m_end    = m_begin + a_rhs.max_size();
      assert(m_end == a_rhs.m_end);
      crunch();
    } else {
      auto new_sz = a_rhs.size();
      if (N < (int)new_sz) {
        m_begin = alloc_t::allocate(new_sz);
        m_end   = m_begin + new_sz;
      } else {
        m_begin = m_data;
        m_end   = m_begin + N;
      }
      memcpy(m_begin, a_rhs.m_rd_ptr, a_rhs.size());
      m_rd_ptr = m_begin + (a_rhs.m_rd_ptr - a_rhs.m_begin);
      m_wr_ptr = m_begin + (a_rhs.m_wr_ptr - a_rhs.m_begin);
    }
    m_wr_lwm      = a_rhs.m_wr_lwm;
    a_rhs.m_begin = a_rhs.m_data;
    a_rhs.m_end   = a_rhs.m_begin + N;
    a_rhs.deallocate();
  }

public:
  explicit basic_io_buffer(size_t a_lwm = LONG_MAX, const Alloc& a_alloc = Alloc())
      : alloc_t(reinterpret_cast<const alloc_t&>(a_alloc)), m_begin(m_data), m_end(m_data + N),
        m_rd_ptr(m_data), m_wr_ptr(m_data), m_wr_lwm(a_lwm)
  {}

  basic_io_buffer(basic_io_buffer&& a_rhs)
      : alloc_t(std::move(static_cast<alloc_t&&>(a_rhs))), m_begin(m_data), m_end(m_data + N)
  { repoint(std::forward<basic_io_buffer&&>(a_rhs)); }

  void operator=(basic_io_buffer&& a_rhs)
  {
    static_cast<alloc_t*>(this)->operator=(std::move(static_cast<alloc_t&&>(a_rhs)));
    repoint(std::forward<basic_io_buffer&&>(a_rhs));
  }

  ~basic_io_buffer() { deallocate(); }

  /// Reset read/write pointers. Any unread content will be lost.
  void reset() { m_rd_ptr = m_wr_ptr = m_begin; }

  /// Deallocate any heap space occupied by the buffer.
  void deallocate()
  {
    if (m_begin != m_data) {
      if (m_end > m_begin) alloc_t::deallocate(m_begin, max_size());
      m_begin = m_data;
      m_end   = m_begin + N;
    }
    reset();
  }

  /// Ensure there's enough total space in the buffer to hold \a n bytes.
  void reserve(size_t n)
  {
    if (n <= capacity()) return;
    size_t rd_offset = m_rd_ptr - m_begin;
    size_t wr_offset = m_wr_ptr - m_begin;
    auto   dirty_sz  = wr_offset - rd_offset;
    char*  old_begin = m_begin;
    auto   old_sz    = max_size();
    auto   new_sz    = dirty_sz + n;
    m_begin          = alloc_t::allocate(new_sz);
    m_end            = m_begin + new_sz;
    if (dirty_sz > 0) memcpy(m_begin, old_begin + rd_offset, dirty_sz);
    m_rd_ptr = m_begin;
    m_wr_ptr = m_begin + dirty_sz;
    if (old_begin != m_data) alloc_t::deallocate(old_begin, old_sz);
  }

  // clang-format off
  /// Address of internal data buffer
  const char* address()   const { return m_begin; }
  /// Max number of bytes the buffer can hold.
  size_t      max_size()  const { return m_end    - m_begin;  }
  /// Current number of bytes available to read.
  size_t      size()      const { return m_wr_ptr - m_rd_ptr; }
  /// Current number of bytes available to write.
  size_t      capacity()  const { return m_end    - m_wr_ptr; }
  /// Returns true if there's no data in the buffer
  bool        empty()     const { return m_rd_ptr == m_wr_ptr; }
  /// Get the low memory watermark indicating when the buffer should be
  /// automatically crunched by the read() call.
  size_t      wr_lwm()    const { return m_wr_lwm; }

  bool is_low_space()     const {
    return m_wr_ptr >= m_end - (m_wr_lwm == (size_t)LONG_MAX ? 0 : m_wr_lwm);
  }

  // clang-format off
  /// Current read pointer.
  const char* rd_ptr()    const { return m_rd_ptr; }
  char*       rd_ptr()          { return m_rd_ptr; }

  /// Current write pointer.
  const char* wr_ptr()    const { return m_wr_ptr; }
  char*       wr_ptr()          { return m_wr_ptr; }
  void        wr_ptr(char* ptr) { assert(ptr >= m_rd_ptr); m_wr_ptr = ptr; }

  /// End of buffer space.
  const char* end()       const { return m_end; }

  /// Returns true if the buffer space was dynamically allocated.
  bool        allocated() const { return m_begin != m_data; }
  // clang-format on

  /// Cast this buffer to dynamic_io_buffer
  detail::basic_dynamic_io_buffer<Alloc>&       to_dynamic();
  const detail::basic_dynamic_io_buffer<Alloc>& to_dynamic() const;

  void                                          wr_lwm(size_t a_lwm)
  {
    if (a_lwm > max_size()) throw std::runtime_error("Low watermark too large");
    m_wr_lwm = a_lwm;
  }

  /// Read \a n bytes from the buffer and increment rd_ptr() by \a n.
  /// @returns NULL if there is not enough data; otherwise the pre-increment rd_ptr().
  char* read(int n)
  {
    if ((size_t)n > size()) return nullptr;
    char* p = m_rd_ptr;
    if (n) {
      m_rd_ptr += n;
      assert(m_rd_ptr <= m_wr_ptr);
    }
    return p;
  }

  /// Discard \a n bytes by advancing rd_ptr() by \a n.
  void discard(unsigned n)
  {
    m_rd_ptr += n;
    assert(m_rd_ptr <= m_wr_ptr);
  }

  /// Read \a n bytes and crunch the buffer when capacity falls below wr_lwm().
  /// @returns -1 if not enough data.
  int read_and_crunch(int n)
  {
    if (read(n) == nullptr) return -1;
    if (capacity() < m_wr_lwm) crunch();
    assert(m_rd_ptr <= m_wr_ptr);
    return n;
  }

  /// Write \a n bytes from \a a_src into the buffer.
  char* write(const char* a_src, size_t n)
  {
    reserve(n);
    char* p = m_wr_ptr;
    commit(n);
    memcpy(p, a_src, n);
    return m_wr_ptr;
  }

  /// Advance the write pointer by \a n bytes (mark \a n bytes as written).
  void commit(int n)
  {
    assert(m_wr_ptr + n <= m_end);
    m_wr_ptr += n;
  }

  /// Move any unread data to the beginning of the buffer.
  void crunch()
  {
    if (m_rd_ptr == m_begin) return;
    size_t sz = size();
    if (sz) {
      if (m_begin + sz > m_rd_ptr)
        memmove(m_begin, m_rd_ptr, sz);
      else
        memcpy(m_begin, m_rd_ptr, sz);
    }
    m_rd_ptr = m_begin;
    m_wr_ptr = m_begin + sz;
  }

  /// Discard \a n bytes and crunch when capacity falls below wr_lwm().
  void discard_and_crunch(unsigned n)
  {
    discard(n);
    if (capacity() < m_wr_lwm) {
      crunch();
      assert(m_rd_ptr <= m_wr_ptr);
    }
  }
};

/**
 * \brief Record buffer: N records of sizeof(T) on stack, heap otherwise.
 */
template <typename T, int N, typename Alloc = std::allocator<char>>
class record_buffers : protected basic_io_buffer<sizeof(T) * N, Alloc> {
  using super = basic_io_buffer<sizeof(T) * N, Alloc>;

public:
  explicit record_buffers(const Alloc& a_alloc = Alloc()) : super(0, a_alloc) {}

  // clang-format off
  void     reserve(size_t n)  { super::reserve(sizeof(T) * n); }

  T*       begin()            { return reinterpret_cast<T*>(super::m_begin); }
  const T* begin()      const { return reinterpret_cast<const T*>(super::m_begin); }
  const T* end()        const { return reinterpret_cast<const T*>(super::m_end); }

  size_t   max_size()   const { return end() - begin(); }
  size_t   size()       const { return wr_ptr() - rd_ptr(); }
  size_t   capacity()   const { return end() - wr_ptr(); }
  bool     allocated()  const { return super::allocated(); }

  T*       wr_ptr()           { return reinterpret_cast<T*>(this->m_wr_ptr); }
  const T* wr_ptr()     const { return reinterpret_cast<const T*>(this->m_wr_ptr); }
  void     wr_ptr(T* p)       { super::wr_ptr(reinterpret_cast<char*>(p)); }

  T*       rd_ptr()           { return reinterpret_cast<T*>(super::m_rd_ptr); }
  const T* rd_ptr()     const { return reinterpret_cast<const T*>(super::m_rd_ptr); }

  T*       read()
  {
    return (size() < 1) nullptr : reinterpret_cast<T*>(super::read(sizeof(T)));
  }
  const T* read() const
  {
    return (size() < 1) ? nullptr : reinterpret_cast<const T*>(super::read(sizeof(T)));
  }
  // clang-format on

  T* write(const T& a_src)
  {
    assert(capacity() > 0);
    *wr_ptr() = a_src;
    return advance(1);
  }

  // clang-format off
  T*   advance(size_t n = 1) { super::commit(n * sizeof(T)); return wr_ptr(); }
  T*   write(size_t n = 1)   { return advance(n); }
  void reset()               { super::reset(); }
  // clang-format on

  using const_iterator = const T*;
  using iterator       = T*;
};

namespace detail {

  /**
   * \brief Generic buffer providing heap space for I/O data.
   */
  template <typename Alloc = std::allocator<char>>
  class basic_dynamic_io_buffer : public basic_io_buffer<0, Alloc> {
  public:
    explicit basic_dynamic_io_buffer(size_t a_initial_size, const Alloc& a_alloc = Alloc())
        : basic_io_buffer<0, Alloc>(LONG_MAX, a_alloc)
    { this->reserve(a_initial_size); }
  };

} // namespace detail

/// Generic buffer providing heap space for I/O data.
using dynamic_io_buffer = detail::basic_dynamic_io_buffer<std::allocator<char>>;

/// A buffer providing space for input and output I/O operations.
template <int InBufSize, int OutBufSize = InBufSize>
struct io_buffer {
  basic_io_buffer<InBufSize>  in;
  basic_io_buffer<OutBufSize> out;
};

template <int N, typename Alloc>
inline detail::basic_dynamic_io_buffer<Alloc>& basic_io_buffer<N, Alloc>::to_dynamic()
{ return *reinterpret_cast<detail::basic_dynamic_io_buffer<Alloc>*>(this); }

template <int N, typename Alloc>
inline const detail::basic_dynamic_io_buffer<Alloc>& basic_io_buffer<N, Alloc>::to_dynamic() const
{ return *reinterpret_cast<const detail::basic_dynamic_io_buffer<Alloc>*>(this); }

} // namespace utxx
