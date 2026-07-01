// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  test_compat.cpp
//------------------------------------------------------------------------------
/// \brief Tests for compat.hpp standalone types
//------------------------------------------------------------------------------
#include "test_main.hpp"
#include <chrono>
#include <fcntl.h>
#include <reactor/compat.hpp>
#include <thread>
#include <unistd.h>

// ---- src_info ---------------------------------------------------------------

TEST_CASE(src_info_default_empty)
{
  utxx::src_info si;
  CHECK(si.empty());
  CHECK(si.srcloc_len() == 0);
}

TEST_CASE(src_info_location)
{
  utxx::src_info si("file.cpp:42", __PRETTY_FUNCTION__);
  CHECK(!si.empty());
  CHECK(si.srcloc() != nullptr);
  CHECK(std::string(si.srcloc()) == "file.cpp:42");
}

TEST_CASE(src_info_macro)
{
  utxx::src_info si = REACTOR_SRC;
  CHECK(!si.empty());
  CHECK(si.srcloc_len() > 0);
}

// ---- error types ------------------------------------------------------------

TEST_CASE(runtime_error_message)
{
  try {
    throw utxx::runtime_error(REACTOR_SRC, "test error");
  } catch (utxx::runtime_error& e) {
    CHECK(std::string(e.what()).find("test error") != std::string::npos);
    CHECK(!e.src().empty());
  }
}

TEST_CASE(io_error_includes_errno_string)
{
  try {
    throw utxx::io_error(REACTOR_SRC, ENOENT, "open failed");
  } catch (utxx::io_error& e) {
    std::string msg = e.what();
    CHECK(msg.find("open failed") != std::string::npos);
    CHECK(msg.find("No such file") != std::string::npos);
  }
}

TEST_CASE(io_error_zero_errno)
{
  try {
    throw utxx::io_error(REACTOR_SRC, 0, "custom msg");
  } catch (utxx::io_error& e) {
    CHECK(std::string(e.what()) == "custom msg");
  }
}

TEST_CASE(utxx_throw_macro)
{ CHECK_THROWS(UTXX_THROW(utxx::badarg_error, "bad arg")); }

// ---- dynamic_io_buffer ------------------------------------------------------

TEST_CASE(buffer_initial_state)
{
  utxx::dynamic_io_buffer buf(64);
  CHECK_EQUAL((size_t)64, buf.capacity());
  CHECK_EQUAL((size_t)0, buf.size());
}

TEST_CASE(buffer_commit_and_read)
{
  utxx::dynamic_io_buffer buf(16);
  const char*                src = "hello";
  ::memcpy(buf.wr_ptr(), src, 5);
  buf.commit(5);

  CHECK_EQUAL((size_t)5, buf.size());
  CHECK_EQUAL((size_t)11, buf.capacity());
  CHECK(std::string(buf.rd_ptr(), 5) == "hello");
}

TEST_CASE(buffer_read_and_crunch)
{
  utxx::dynamic_io_buffer buf(16);
  ::memcpy(buf.wr_ptr(), "abcdef", 6);
  buf.commit(6);

  buf.read_and_crunch(3); // consume "abc"

  CHECK_EQUAL((size_t)3, buf.size());
  CHECK_EQUAL((size_t)13, buf.capacity());
  CHECK(std::string(buf.rd_ptr(), 3) == "def");
}

TEST_CASE(buffer_read_and_crunch_all)
{
  utxx::dynamic_io_buffer buf(8);
  ::memcpy(buf.wr_ptr(), "xyz", 3);
  buf.commit(3);
  buf.read_and_crunch(3);

  CHECK_EQUAL((size_t)0, buf.size());
  CHECK_EQUAL((size_t)8, buf.capacity());
}

TEST_CASE(buffer_reserve_grows)
{
  utxx::dynamic_io_buffer buf(8);
  buf.reserve(32);
  CHECK(buf.capacity() >= 32u);
}

TEST_CASE(buffer_reserve_preserves_data)
{
  utxx::dynamic_io_buffer buf(8);
  ::memcpy(buf.wr_ptr(), "hi", 2);
  buf.commit(2);
  buf.reserve(64);
  CHECK(std::string(buf.rd_ptr(), 2) == "hi");
}

// ---- scope_exit -------------------------------------------------------------

TEST_CASE(scope_exit_runs_on_exit)
{
  bool ran = false;
  {
    auto g = utxx::scope_exit([&ran]() { ran = true; });
  }
  CHECK(ran);
}

TEST_CASE(scope_exit_disabled_does_not_run)
{
  bool ran = false;
  {
    auto g = utxx::scope_exit([&ran]() { ran = true; });
    g.disable();
  }
  CHECK(!ran);
}

// ---- log_level --------------------------------------------------------------

TEST_CASE(log_level_to_string)
{
  CHECK(std::string(utxx::log_level_to_string(utxx::log_level::DEBUG)) == "DEBUG");
  CHECK(std::string(utxx::log_level_to_string(utxx::log_level::INFO)) == "INFO");
  CHECK(std::string(utxx::log_level_to_string(utxx::log_level::WARNING)) == "WARNING");
  CHECK(std::string(utxx::log_level_to_string(utxx::log_level::ERROR)) == "ERROR");
}

// ---- time_val ---------------------------------------------------------------

TEST_CASE(time_val_default_empty)
{
  utxx::time_val tv;
  CHECK(tv.empty());
}

TEST_CASE(time_val_from_timespec)
{
  struct timespec   ts{123, 456};
  utxx::time_val tv(ts);
  CHECK(tv.tv_sec == 123);
  CHECK(tv.tv_nsec == 456);
  CHECK(!tv.empty());
}

// ---- to_string / print ------------------------------------------------------

TEST_CASE(to_string_concatenates)
{
  auto s = utxx::to_string("hello", ' ', 42);
  CHECK(s == "hello 42");
}

TEST_CASE(print_same_as_to_string)
{
  auto s = utxx::print("x=", 7);
  CHECK(s == "x=7");
}

// ---- path helpers -----------------------------------------------------------

TEST_CASE(path_file_size)
{
  // /etc/hostname exists on any Linux system
  int fd = ::open("/etc/hostname", O_RDONLY);
  if (fd >= 0) {
    long sz = utxx::path::file_size(fd);
    CHECK(sz > 0);
    ::close(fd);
  }
}

TEST_CASE(path_file_exists_not_exists)
{ CHECK(!utxx::path::file_exists("/tmp/__reactor_nonexistent_file_xyz__")); }

TEST_CASE(path_file_exists_and_unlink)
{
  const char* path = "/tmp/__reactor_test_unlink__";
  ::unlink(path);
  // create it
  int fd = ::open(path, O_CREAT | O_WRONLY, 0600);
  if (fd >= 0) ::close(fd);

  CHECK(utxx::path::file_exists(path));
  CHECK(utxx::path::file_unlink(path));
  CHECK(!utxx::path::file_exists(path));
}

// ---- now_utc / fixed --------------------------------------------------------

TEST_CASE(now_utc_reasonable_timestamp)
{
  auto t = utxx::now_utc();
  // seconds since epoch > 1700000000 (Nov 2023)
  CHECK(t.sec() > 1700000000L);
}

TEST_CASE(now_utc_add_msec)
{
  auto t  = utxx::now_utc();
  auto t2 = t.add_msec(500);
  CHECK(t2.nsec() != t.nsec() || t2.sec() != t.sec());
}

TEST_CASE(fixed_formatting)
{
  CHECK(utxx::fixed(3.14159, 2) == "3.14");
  CHECK(utxx::fixed(1.0, 3) == "1.000");
}

// ---- os::getenv -------------------------------------------------------------

TEST_CASE(getenv_missing_returns_default)
{
  long v = utxx::os::getenv("__REACTOR_NOSUCHVAR__", 99L);
  CHECK_EQUAL(99L, v);
}

TEST_CASE(getenv_present_returns_value)
{
  ::setenv("__REACTOR_TESTVAR__", "42", 1);
  long v = utxx::os::getenv("__REACTOR_TESTVAR__", 0L);
  ::unsetenv("__REACTOR_TESTVAR__");
  CHECK_EQUAL(42L, v);
}

// ---- to_bin_string ----------------------------------------------------------

TEST_CASE(to_bin_string_printable)
{
  auto s = utxx::to_bin_string("abc", 3);
  CHECK(s == "abc");
}

TEST_CASE(to_bin_string_non_printable)
{
  char buf[2] = {'\x01', '\x02'};
  auto s      = utxx::to_bin_string(buf, 2);
  CHECK(s.find("\\x01") != std::string::npos);
  CHECK(s.find("\\x02") != std::string::npos);
}

// ---- main -------------------------------------------------------------------

int main()
{ return test_harness::run_all(); }
