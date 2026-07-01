// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  test_aio_reader.cpp
//------------------------------------------------------------------------------
/// \brief Tests for AIOReader (Linux AIO + eventfd)
//------------------------------------------------------------------------------
#include "test_main.hpp"
#include <fcntl.h>
#include <fstream>
#include <poll.h>
#include <reactor/reactor_aio_reader.hxx>
#include <reactor/reactor_platform.hpp>
#include <string>

using namespace utxx;

// ---- helpers ----------------------------------------------------------------

struct TempFile {
  std::string path;

  TempFile(const char* p, size_t sz) : path(p)
  {
    ::unlink(p);
    std::ofstream f(p, std::ios::binary);
    for (size_t i = 0; i < sz; i++) f.put(static_cast<char>('A' + i % 26));
  }
  ~TempFile() { ::unlink(path.c_str()); }
};

static long wait_efd(int efd, int timeout_ms = 3000)
{
  struct pollfd pfd{efd, POLLIN, 0};
  if (poll(&pfd, 1, timeout_ms) <= 0) return -1;
  if (!(pfd.revents & POLLIN)) return -1;
  return 1;
}

// ---- AIOReader::Init --------------------------------------------------------

TEST_CASE(aio_reader_init_bad_efd)
{
  utxx::AIOReader reader;
  CHECK_THROWS(reader.Init(-1, "/etc/hostname"));
}

TEST_CASE(aio_reader_init_missing_file)
{
  int efd = reactor_eventfd_create();
  CHECK(efd >= 0);
  utxx::AIOReader reader;
  CHECK_THROWS(reader.Init(efd, "/tmp/__reactor_no_such_file__"));
  reactor_eventfd_close(efd);
}

TEST_CASE(aio_reader_init_valid)
{
  TempFile f("/tmp/__reactor_aio_test__.bin", 128);
  int      efd = reactor_eventfd_create();
  CHECK(efd >= 0);

  CHECK_NO_THROW({
    utxx::AIOReader reader;
    reader.Init(efd, f.path.c_str());
    CHECK(reader.Size() == 128);
    CHECK(reader.Remaining() == 128);
    CHECK(reader.Offset() == 0);
    CHECK(reader.Position() == 0);
    CHECK(reader.EventFD() == efd);
  });

  reactor_eventfd_close(efd);
}

// ---- AIOReader::AsyncRead + ReadEvents --------------------------------------

TEST_CASE(aio_reader_read_full_file)
{
  const size_t FILE_SZ = 4096;
  TempFile     f("/tmp/__reactor_aio_full__.bin", FILE_SZ);

  int          efd = reactor_eventfd_create();
  CHECK(efd >= 0);

  utxx::AIOReader reader(efd, f.path.c_str());
  CHECK_EQUAL((long)FILE_SZ, reader.Size());

  std::vector<char> buf(FILE_SZ);
  int               rc = reader.AsyncRead(buf.data(), FILE_SZ);
  CHECK(rc >= 0);

  long ev = wait_efd(efd);
  CHECK(ev > 0);

  int n_events = reader.CheckEvents();
  CHECK(n_events > 0);

  auto [got, err] = reader.ReadEvents(n_events);
  CHECK(got == (long)FILE_SZ);
  CHECK(std::string(err) == "");
  CHECK_EQUAL((long)FILE_SZ, reader.Offset());
  CHECK_EQUAL(0L, reader.Remaining());

  // verify first few bytes
  for (int i = 0; i < 26; i++) CHECK(buf[i] == 'A' + i % 26);

  reactor_eventfd_close(efd);
}

TEST_CASE(aio_reader_remaining_zero_prevents_double_read)
{
  const size_t       FILE_SZ = 64;
  TempFile           f("/tmp/__reactor_aio_zero__.bin", FILE_SZ);

  int                efd = reactor_eventfd_create();
  utxx::AIOReader reader(efd, f.path.c_str());

  std::vector<char>  buf(FILE_SZ);
  reader.AsyncRead(buf.data(), FILE_SZ);
  wait_efd(efd);
  auto events = reader.CheckEvents();
  reader.ReadEvents(events);

  // Now Remaining() == 0 — a second AsyncRead should fail
  int rc = reader.AsyncRead(buf.data(), FILE_SZ);
  CHECK(rc < 0);
  CHECK(errno == ENODATA);

  reactor_eventfd_close(efd);
}

TEST_CASE(aio_reader_no_events_returns_zero)
{
  TempFile           f("/tmp/__reactor_aio_noev__.bin", 32);
  int                efd = reactor_eventfd_create();
  utxx::AIOReader reader(efd, f.path.c_str());

  // Don't issue any reads — ReadEvents(0) should return {0, ""}
  auto [got, err] = reader.ReadEvents(0);
  CHECK_EQUAL(0L, got);
  CHECK(std::string(err) == "");

  reactor_eventfd_close(efd);
}

// ---- AIOReader::Clear -------------------------------------------------------

TEST_CASE(aio_reader_clear_is_idempotent)
{
  TempFile           f("/tmp/__reactor_aio_clear__.bin", 16);
  int                efd = reactor_eventfd_create();
  utxx::AIOReader reader(efd, f.path.c_str());
  CHECK_NO_THROW(reader.Clear());
  CHECK_NO_THROW(reader.Clear());
  reactor_eventfd_close(efd);
}

// ---- main -------------------------------------------------------------------

int main()
{ return test_harness::run_all(); }
