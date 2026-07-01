// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  test_reactor_types.cpp
//------------------------------------------------------------------------------
/// \brief Tests for reactor_types.hpp enums and HandlerT
//------------------------------------------------------------------------------
#include "test_main.hpp"
#include <reactor/reactor_fd_info.hpp>
#include <reactor/reactor_types.hxx>

using namespace utxx;

// ---- HType ------------------------------------------------------------------

TEST_CASE(htype_to_string)
{
  CHECK(std::string(to_string(HType::IO)) == "IO");
  CHECK(std::string(to_string(HType::RawIO)) == "RawIO");
  CHECK(std::string(to_string(HType::File)) == "File");
  CHECK(std::string(to_string(HType::Pipe)) == "Pipe");
  CHECK(std::string(to_string(HType::Event)) == "Event");
  CHECK(std::string(to_string(HType::Timer)) == "Timer");
  CHECK(std::string(to_string(HType::Signal)) == "Signal");
  CHECK(std::string(to_string(HType::Accept)) == "Accept");
  CHECK(std::string(to_string(HType::Error)) == "Error");
  CHECK(std::string(to_string(HType::UNDEFINED)) == "UNDEFINED");
}

// ---- IOType -----------------------------------------------------------------

TEST_CASE(iotype_to_string)
{
  CHECK(std::string(to_string(IOType::Read)) == "Read");
  CHECK(std::string(to_string(IOType::Write)) == "Write");
  CHECK(std::string(to_string(IOType::Connect)) == "Connect");
  CHECK(std::string(to_string(IOType::Accept)) == "Accept");
  CHECK(std::string(to_string(IOType::EndOfFile)) == "EndOfFile");
  CHECK(std::string(to_string(IOType::UserCode)) == "UserCode");
  CHECK(std::string(to_string(IOType::Fatal)) == "Fatal");
  CHECK(std::string(to_string(IOType::UNDEFINED)) == "UNDEFINED");
}

// ---- FdTypeT ----------------------------------------------------------------

TEST_CASE(fdtypet_to_string)
{
  CHECK(std::string(to_string(FdTypeT::Stream)) == "Stream");
  CHECK(std::string(to_string(FdTypeT::Datagram)) == "Datagram");
  CHECK(std::string(to_string(FdTypeT::File)) == "File");
  CHECK(std::string(to_string(FdTypeT::Signal)) == "Signal");
  CHECK(std::string(to_string(FdTypeT::UNDEFINED)) == "UNDEFINED");
}

// ---- HandlerT default-construct / clear ------------------------------------

TEST_CASE(handler_default_type_undefined)
{
  HandlerT h;
  CHECK(h.Type() == HType::UNDEFINED);
}

TEST_CASE(handler_nullptr_type_undefined)
{
  HandlerT h(nullptr);
  CHECK(h.Type() == HType::UNDEFINED);
}

TEST_CASE(handler_clear_resets_type)
{
  bool     called = false;
  HandlerT h(HType::Event, EventHandler([&called](FdInfo&, long) { called = true; }));
  CHECK(h.Type() == HType::Event);
  h.Clear();
  CHECK(h.Type() == HType::UNDEFINED);
}

// ---- HandlerT copy/move assignment -----------------------------------------

TEST_CASE(handler_copy_assignment)
{
  bool         called = false;
  EventHandler eh([&called](FdInfo&, long) { called = true; });
  HandlerT     src(HType::Event, eh);

  HandlerT     dst;
  dst = src;

  CHECK(dst.Type() == HType::Event);
  // Invoke to confirm the handler was actually copied
  // (we can't call AsEvent()() without a real FdInfo, but we can confirm type)
}

TEST_CASE(handler_move_assignment)
{
  bool         called = false;
  EventHandler eh([&called](FdInfo&, long) { called = true; });
  HandlerT     src(HType::Event, eh);
  HandlerT     dst;
  dst = std::move(src);

  CHECK(dst.Type() == HType::Event);
  CHECK(src.Type() == HType::UNDEFINED);
}

TEST_CASE(handler_io_type)
{
  // RWIOHandler is a raw function pointer, so use a plain function (non-capturing lambda won't do)
  static auto plain_rh = [](FdInfo&, dynamic_io_buffer&) -> int { return 0; };
  RWIOHandler rh       = plain_rh;
  HandlerT    h(HType::IO, HandlerT::IO{rh, nullptr});
  CHECK(h.Type() == HType::IO);
  CHECK(h.AsIO().rh != nullptr);
  CHECK(h.AsIO().wh == nullptr);
}

TEST_CASE(handler_rawio_type)
{
  RawIOHandler rh = [](FdInfo&, IOType, uint32_t) {};
  HandlerT     h(HType::RawIO, rh);
  CHECK(h.Type() == HType::RawIO);
  CHECK(h.AsRawIO() != nullptr);
}

TEST_CASE(handler_sig_type)
{
  SigHandler sh = [](FdInfo&, int, int) {};
  HandlerT   h(HType::Signal, sh);
  CHECK(h.Type() == HType::Signal);
  CHECK(h.AsSignal() != nullptr);
}

TEST_CASE(handler_accept_type)
{
  AcceptHandler ah = [](FdInfo&, const char*, int) -> bool { return true; };
  HandlerT      h(HType::Accept, ah);
  CHECK(h.Type() == HType::Accept);
  CHECK(h.AsAccept() != nullptr);
}

// ---- TriggerT ---------------------------------------------------------------

TEST_CASE(trigger_values_distinct)
{ CHECK(LEVEL_TRIGGERED != EDGE_TRIGGERED); }

// ---- main -------------------------------------------------------------------

int main()
{ return test_harness::run_all(); }
