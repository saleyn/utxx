// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  compat.hpp
//------------------------------------------------------------------------------
/// \brief Standalone replacements for utxx/boost types used by the reactor
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
//------------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <time.h>
#include <vector>

//------------------------------------------------------------------------------
// Branch prediction hints (was: utxx/compiler_hints.hpp)
//------------------------------------------------------------------------------
#ifndef LIKELY
#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#endif
#ifndef UNLIKELY
#define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#endif

#define REACTOR_STRINGIFY(x)      #x
#define REACTOR_TOSTRING(x)       REACTOR_STRINGIFY(x)
#define REACTOR_FILE_SRC_LOCATION __FILE__ ":" REACTOR_TOSTRING(__LINE__)

#define REACTOR_SRC  utxx::src_info(REACTOR_FILE_SRC_LOCATION, __PRETTY_FUNCTION__)
#define REACTOR_SRCX REACTOR_SRC

// Caches the pretty-function name in a static local; used in hot paths.
#define REACTOR_PRETTY_FUNCTION()                                            \
  static const std::string __reactor_pretty_function__ = __PRETTY_FUNCTION__

//------------------------------------------------------------------------------
// src_info (was: utxx::src_info)
//------------------------------------------------------------------------------
namespace utxx {

class src_info {
  const char* m_srcloc;
  const char* m_fun;
  uint16_t    m_srcloc_len;
  uint16_t    m_fun_len;

public:
  constexpr src_info() noexcept : m_srcloc(""), m_fun(""), m_srcloc_len(0), m_fun_len(0) {}

  constexpr src_info(const char* a_srcloc, const char* a_fun, bool /*verbatim*/ = false) noexcept
      : m_srcloc(a_srcloc), m_fun(a_fun),
        m_srcloc_len(static_cast<uint16_t>(a_srcloc ? strlen(a_srcloc) : 0)),
        m_fun_len(static_cast<uint16_t>(a_fun ? strlen(a_fun) : 0))
  {}

  // clang-format off
  const char* srcloc()     const { return m_srcloc;     }
  int         srcloc_len() const { return m_srcloc_len; }
  const char* fun()        const { return m_fun;        }
  int         fun_len()    const { return m_fun_len;    }
  bool        empty()      const { return m_srcloc_len == 0; }
  // clang-format on

  std::string to_str() const
  {
    std::string s;
    if (m_srcloc_len) s += m_srcloc;
    if (m_fun_len) {
      s += " ";
      s += m_fun;
    }
    return s;
  }

  char* to_string(char* buf, int cap, const char* pfx = "", const char* sfx = "\n") const
  {
    int n = snprintf(buf, cap, "%s%s%s", pfx, m_srcloc ? m_srcloc : "", sfx);
    return buf + std::min(n, cap - 1);
  }
};

//------------------------------------------------------------------------------
// Exception helpers (was: utxx error macros + exception types)
//------------------------------------------------------------------------------

/// Base exception carrying src_info
struct reactor_error : public std::runtime_error {
  src_info m_src;
  reactor_error(const src_info& si, const std::string& msg) : std::runtime_error(msg), m_src(si) {}
  const src_info&   src() const { return m_src; }
  const std::string str() const { return what(); }
};

struct runtime_error : reactor_error {
  runtime_error(const src_info& si, const std::string& msg) : reactor_error(si, msg) {}
};

struct badarg_error : reactor_error {
  badarg_error(const src_info& si, const std::string& msg) : reactor_error(si, msg) {}
};

struct io_error : reactor_error {
  int m_errno;
  io_error(const src_info& si, int ec, const std::string& msg)
      : reactor_error(si, msg + (ec ? std::string(": ") + strerror(ec) : "")), m_errno(ec)
  {}
};

namespace detail {
  template <class... Args>
  inline std::string concat(Args&&... args)
  {
    std::ostringstream os;
    (void)std::initializer_list<int>{(os << std::forward<Args>(args), 0)...};
    return os.str();
  }
} // namespace detail

} // namespace utxx

// UTXX_THROW(ExcType, args...) → throw ExcType(REACTOR_SRC, concat(args))
#define REACTOR_THROW(ExcType, ...) throw ExcType(REACTOR_SRC, utxx::detail::concat(__VA_ARGS__))

#define REACTOR_THROWX_IO_ERROR(ec, ...)                                           \
  throw utxx::io_error(REACTOR_SRC, (ec), utxx::detail::concat(__VA_ARGS__))

#define REACTOR_SRC_THROW(ExcType, SrcInfo, ...)                 \
  throw ExcType((SrcInfo), utxx::detail::concat(__VA_ARGS__))

// Map old utxx macro names used in the code
#define UTXX_THROW           REACTOR_THROW
#define UTXX_THROWX_IO_ERROR REACTOR_THROWX_IO_ERROR
#define UTXX_SRC_THROW       REACTOR_SRC_THROW
#define UTXX_PRETTY_FUNCTION REACTOR_PRETTY_FUNCTION
#define UTXX_SRC             REACTOR_SRC
#define UTXX_SRCX            REACTOR_SRCX

//------------------------------------------------------------------------------
// log_level (was: utxx::log_level / logger_enums.hpp)
//------------------------------------------------------------------------------
namespace utxx {

enum class log_level : int {
  NONE = 0,
  FATAL,
  ERROR,
  WARNING,
  INFO,
  DEBUG,
  TRACE,
  TRACE1,
  TRACE2,
  TRACE3,
  TRACE4,
  TRACE5,
};

inline const char* log_level_to_string(log_level lev)
{
  switch (lev) {
    case log_level::NONE:    return "NONE";
    case log_level::FATAL:   return "FATAL";
    case log_level::ERROR:   return "ERROR";
    case log_level::WARNING: return "WARNING";
    case log_level::INFO:    return "INFO";
    case log_level::DEBUG:   return "DEBUG";
    case log_level::TRACE:   return "TRACE";
    case log_level::TRACE1:  return "TRACE1";
    case log_level::TRACE2:  return "TRACE2";
    case log_level::TRACE3:  return "TRACE3";
    case log_level::TRACE4:  return "TRACE4";
    case log_level::TRACE5:  return "TRACE5";
    default:                 return "UNKNOWN";
  }
}

inline int as_int(log_level lev)
{ return static_cast<int>(lev); }

} // namespace utxx

// Map utxx log level names used verbatim in reactor code
#define LEVEL_DEBUG  utxx::log_level::DEBUG
#define LEVEL_TRACE  utxx::log_level::TRACE
#define LEVEL_TRACE5 utxx::log_level::TRACE5

//------------------------------------------------------------------------------
// time_val (was: utxx::time_val)
//------------------------------------------------------------------------------
namespace utxx {

struct time_val {
  long tv_sec  = 0;
  long tv_nsec = 0;

  time_val()   = default;
  explicit time_val(const struct timespec& ts) : tv_sec(ts.tv_sec), tv_nsec(ts.tv_nsec) {}
  explicit time_val(long sec, long nsec = 0) : tv_sec(sec), tv_nsec(nsec) {}

  bool empty() const { return tv_sec == 0 && tv_nsec == 0; }
};

} // namespace utxx

//------------------------------------------------------------------------------
// to_string helpers (was: utxx::to_string)
//------------------------------------------------------------------------------
namespace utxx {

template <class... Args>
inline std::string to_string(Args&&... args)
{ return detail::concat(std::forward<Args>(args)...); }

} // namespace utxx

//------------------------------------------------------------------------------
// verbosity (was: utxx::verbosity / utxx::VERBOSE_WIRE)
// Stub: wire verbosity is disabled by default.
//------------------------------------------------------------------------------
namespace utxx {

enum class verbose_level { NONE = 0, WIRE = 1 };

struct verbosity {
  static bool enabled(verbose_level) { return false; }
  template <typename F>
  static void if_enabled(verbose_level lev, F&& f)
  {
    if (enabled(lev)) f();
  }
};

constexpr verbose_level VERBOSE_WIRE = verbose_level::WIRE;

} // namespace utxx

//------------------------------------------------------------------------------
// to_bin_string (was: utxx::to_bin_string)
//------------------------------------------------------------------------------
namespace utxx {

inline std::string to_bin_string(const char* buf, size_t sz)
{
  std::string out;
  out.reserve(sz * 4);
  for (size_t i = 0; i < sz; i++) {
    char          tmp[8];
    unsigned char c = static_cast<unsigned char>(buf[i]);
    if (c >= 32 && c < 127) {
      tmp[0] = c;
      tmp[1] = '\0';
    } else {
      snprintf(tmp, sizeof(tmp), "\\x%02x", c);
    }
    out += tmp;
  }
  return out;
}

} // namespace utxx

//------------------------------------------------------------------------------
// os::getenv (was: utxx::os::getenv)
//------------------------------------------------------------------------------
namespace utxx {
namespace os {

  template <typename T>
  inline T getenv(const char* name, T def)
  {
    const char* v = ::getenv(name);
    if (!v) return def;
    if constexpr (std::is_same_v<T, long> || std::is_same_v<T, int>)
      return static_cast<T>(std::stol(v));
    if constexpr (std::is_same_v<T, bool>) return std::stoi(v) != 0;
    return def;
  }

} // namespace os
} // namespace utxx

//------------------------------------------------------------------------------
// scope_exit (was: utxx::scope_exit)
//------------------------------------------------------------------------------
namespace utxx {

template <typename F>
struct scope_exit_t {
  F    m_f;
  bool m_enabled = true;
  explicit scope_exit_t(F f) : m_f(std::move(f)) {}
  ~scope_exit_t()
  {
    if (m_enabled) m_f();
  }
  void disable() { m_enabled = false; }
};

template <typename F>
inline scope_exit_t<F> scope_exit(F f)
{ return scope_exit_t<F>(std::move(f)); }

} // namespace utxx

//------------------------------------------------------------------------------
// path::file_size (was: utxx::path::file_size)
//------------------------------------------------------------------------------
#include <sys/stat.h>

namespace utxx {
namespace path {

  inline long file_size(int fd)
  {
    struct stat st;
    if (::fstat(fd, &st) < 0) return -1;
    return st.st_size;
  }

} // namespace path
} // namespace utxx

//------------------------------------------------------------------------------
// dynamic_io_buffer (was: utxx::dynamic_io_buffer / utxx::detail::basic_dynamic_io_buffer)
//
// A simple growable buffer tracking read/write positions.
//------------------------------------------------------------------------------
namespace utxx {

class dynamic_io_buffer {
public:
  explicit dynamic_io_buffer(size_t a_capacity = 4096) : m_data(a_capacity), m_rd(0), m_wr(0) {}

  dynamic_io_buffer(const dynamic_io_buffer&)            = delete;
  dynamic_io_buffer& operator=(const dynamic_io_buffer&) = delete;

  dynamic_io_buffer(dynamic_io_buffer&&)                 = default;
  dynamic_io_buffer& operator=(dynamic_io_buffer&&)      = default;

  /// Pointer to write position (where new data should be written)
  char*              wr_ptr() { return m_data.data() + m_wr; }
  /// Pointer to read position (start of unconsumed data)
  char*              rd_ptr() { return m_data.data() + m_rd; }
  const char*        rd_ptr() const { return m_data.data() + m_rd; }

  /// Number of unconsumed bytes
  size_t             size() const { return m_wr - m_rd; }
  /// Remaining write capacity
  size_t             capacity() const { return m_data.size() - m_wr; }
  /// Total allocated storage
  size_t             max_size() const { return m_data.size(); }

  /// Commit n bytes written at wr_ptr()
  void               commit(size_t n)
  {
    m_wr += n;
    assert(m_wr <= m_data.size());
  }

  /// Mark n bytes at rd_ptr() as consumed; crunch buffer if it's now empty
  void read_and_crunch(size_t n)
  {
    m_rd += n;
    if (m_rd >= m_wr) {
      m_rd = m_wr = 0;
    } else if (m_rd > 0) {
      size_t remaining = m_wr - m_rd;
      ::memmove(m_data.data(), m_data.data() + m_rd, remaining);
      m_rd = 0;
      m_wr = remaining;
    }
  }

  /// Ensure at least `n` bytes total capacity is available (may reallocate)
  void reserve(size_t n)
  {
    if (n <= m_data.size()) return;
    std::vector<char> tmp(n);
    size_t            used = size();
    if (used) ::memcpy(tmp.data(), rd_ptr(), used);
    m_data = std::move(tmp);
    m_rd   = 0;
    m_wr   = used;
  }

  void reset() { m_rd = m_wr = 0; }

private:
  std::vector<char> m_data;
  size_t            m_rd;
  size_t            m_wr;
};

} // namespace utxx

//------------------------------------------------------------------------------
// function (was: utxx::function — a fast delegate; here we use std::function)
//------------------------------------------------------------------------------
namespace utxx {
template <typename Sig>
using function = std::function<Sig>;
}

//------------------------------------------------------------------------------
// Timestamp helpers (was: utxx::timestamp)
//------------------------------------------------------------------------------
namespace utxx {

struct timestamp {
  static std::string to_string(int /*fmt*/ = 0)
  {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char      buf[64];
    struct tm tm_info;
    gmtime_r(&ts.tv_sec, &tm_info);
    int n = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    snprintf(buf + n, sizeof(buf) - n, ".%06ld", ts.tv_nsec / 1000);
    return buf;
  }
};

constexpr int DATE_TIME_WITH_USEC = 1;

} // namespace utxx

//------------------------------------------------------------------------------
// buffered_print — full implementation (reactor/print.hpp)
//------------------------------------------------------------------------------
#include <reactor/print.hpp>

//------------------------------------------------------------------------------
// running_stat stub (was: utxx::running_stat — only referenced via include,
// not directly used in the reactor headers)
//------------------------------------------------------------------------------
namespace utxx {
struct running_stat {
  void   add(double) {}
  double mean() const { return 0; }
  double stddev() const { return 0; }
};
} // namespace utxx


//------------------------------------------------------------------------------
// UTXX_RLOG macro (was in ReactorLog.hpp; placed here so all files can use it)
//------------------------------------------------------------------------------
#define UTXX_RLOG(Obj, Level, ...)                                  \
  do {                                                              \
    if (UNLIKELY((Obj)->Debug() >= utxx::as_int(LEVEL_##Level))) \
      (Obj)->Log(LEVEL_##Level, REACTOR_SRC, ##__VA_ARGS__);        \
  } while (0)

//------------------------------------------------------------------------------
// UTXX_SCOPE_EXIT (was: UTXX_SCOPE_EXIT macro from utxx/scope_exit.hpp)
//------------------------------------------------------------------------------
#define UTXX_SCOPE_EXIT(f)   auto UTXX_ANON_VAR_(_scope_exit_) = utxx::scope_exit(f)
#define UTXX_ANON_VAR_(name) UTXX_CONCAT_(name, __LINE__)
#define UTXX_CONCAT_(a, b)   a##b

//------------------------------------------------------------------------------
// UTXX_THROW_IO_ERROR, UTXX_THROWX_BADARG_ERROR, UTXX_THROWX_RUNTIME_ERROR
// Additional macro aliases used in Reactor.cpp
//------------------------------------------------------------------------------
#define UTXX_THROW_IO_ERROR(ec, ...)                                               \
  throw utxx::io_error(REACTOR_SRC, (ec), utxx::detail::concat(__VA_ARGS__))
#define UTXX_THROWX_BADARG_ERROR(...)                                            \
  throw utxx::badarg_error(REACTOR_SRC, utxx::detail::concat(__VA_ARGS__))
#define UTXX_THROWX_RUNTIME_ERROR(...)                                            \
  throw utxx::runtime_error(REACTOR_SRC, utxx::detail::concat(__VA_ARGS__))

//------------------------------------------------------------------------------
// now_utc — minimal time_val for AddTimer (returns current UTC time)
//------------------------------------------------------------------------------
namespace utxx {

struct now_utc_t : time_val {
  now_utc_t()
  {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    tv_sec  = ts.tv_sec;
    tv_nsec = ts.tv_nsec;
  }
  now_utc_t add_msec(long ms) const
  {
    now_utc_t r  = *this;
    r.tv_nsec   += ms * 1000000L;
    if (r.tv_nsec >= 1000000000L) {
      r.tv_sec  += r.tv_nsec / 1000000000L;
      r.tv_nsec %= 1000000000L;
    }
    return r;
  }
  long sec() const { return tv_sec; }
  long nsec() const { return tv_nsec; }
};

inline now_utc_t now_utc()
{ return now_utc_t{}; }

// fixed(val, decimals) — printf-style fixed-point formatting
inline std::string fixed(double v, int decimals)
{
  char buf[64];
  snprintf(buf, sizeof(buf), "%.*f", decimals, v);
  return buf;
}

// sig_members — stringify sigset (stub; only used in debug log/throw)
inline std::string sig_members(const sigset_t&)
{ return "<signals>"; }

} // namespace utxx

namespace utxx {
using utxx::fixed;
using utxx::now_utc;
using utxx::sig_members;
} // namespace utxx

//------------------------------------------------------------------------------
// path::file_exists / file_unlink (was: utxx::path::file_exists/file_unlink)
//------------------------------------------------------------------------------
#include <unistd.h>

namespace utxx {
namespace path {

  inline bool file_exists(const std::string& path)
  { return ::access(path.c_str(), F_OK) == 0; }

  inline bool file_unlink(const std::string& path)
  { return ::unlink(path.c_str()) == 0; }

} // namespace path
} // namespace utxx

namespace utxx {
namespace path {
  using utxx::path::file_exists;
  using utxx::path::file_unlink;
} // namespace path
} // namespace utxx
