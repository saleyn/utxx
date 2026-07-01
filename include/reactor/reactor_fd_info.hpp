// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_fd_info.hpp
//------------------------------------------------------------------------------
/// \brief I/O multiplexing event reactor file descriptor state
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <netinet/in.h>
#include <reactor/reactor_aio_reader.hpp>
#include <reactor/reactor_cmd_exec.hpp>
#include <reactor/reactor_types.hpp>
#include <type_traits>

namespace utxx {

//------------------------------------------------------------------------------
// FdTypeT enum
//------------------------------------------------------------------------------
enum class FdTypeT : int {
  UNDEFINED = -1,
  Stream    = 0,
  Datagram,
  SeqPacket,
  File,
  Pipe,
  Event,
  Timer,
  Signal
};

inline const char* to_string(FdTypeT v)
{
  switch (v) {
    case FdTypeT::UNDEFINED: return "UNDEFINED";
    case FdTypeT::Stream:    return "Stream";
    case FdTypeT::Datagram:  return "Datagram";
    case FdTypeT::SeqPacket: return "SeqPacket";
    case FdTypeT::File:      return "File";
    case FdTypeT::Pipe:      return "Pipe";
    case FdTypeT::Event:     return "Event";
    case FdTypeT::Timer:     return "Timer";
    case FdTypeT::Signal:    return "Signal";
    default:                 return "UNKNOWN";
  }
}

//------------------------------------------------------------------------------
class FdInfo {
public:
  explicit FdInfo(
      Reactor* a_reactor = nullptr, std::string const& a_name = std::string(), int a_fd = -1,
      FdTypeT a_fd_type = FdTypeT::UNDEFINED, ErrHandler const& a_on_error = nullptr,
      void* a_instance = nullptr, void* a_opaque = nullptr, int a_rd_bufsz = 0, int a_wr_bufsz = 0,
      dynamic_io_buffer* a_wr_buf = nullptr, ReadSizeEstim a_read_sz_fun = nullptr,
      TriggerT a_trigger = EDGE_TRIGGERED);

  template <typename Action>
  void SetHandler(HType a_type, const Action& a);

  FdInfo(FdInfo&& a_rhs) noexcept;
  FdInfo(FdInfo const& a_rhs)            = delete;
  FdInfo& operator=(FdInfo const& a_rhs) = delete;
  FdInfo& operator=(FdInfo&& a_rhs);

  ~FdInfo();

  // clang-format off
  HType                    Type()          const { return m_handler.Type();  }
  const std::string&       Name()          const { return m_name;            }
  void                     Name(const std::string& a) { m_name = a;          }
  int                      FD()            const { return m_fd;              }
  void                     FD(int a_fd)          { m_fd = a_fd;              }
  void*                    Instance()      const { return m_instance;        }
  void*                    Opaque()        const { return m_opaque;          }

  dynamic_io_buffer*       RdBuff()              { return m_rd_buff;         }
  dynamic_io_buffer const* RdBuff()        const { return m_rd_buff;         }
  dynamic_io_buffer*       WrBuff()              { return m_wr_buff;         }
  dynamic_io_buffer const* WrBuff()        const { return m_wr_buff;         }

  TriggerT                 Trigger()       const { return m_trigger;         }
  Reactor&                 Owner()               { assert(m_owner); return *m_owner; }

  AIOReader*               FileReader()          { return m_file_reader.get();}
  POpenCmd*                PipeReader()          { return m_exec_cmd.get();   }

  const std::string&       Ident()         const { return m_ident;           }

  time_val                 TsWire()        const { return m_ts_wire;         }
  void                     TsWire(time_val a)    { m_ts_wire = a;            }

  template <typename Action>
  void                     RdDebug(Action a_act) { m_rd_debug = a_act;       }
  ReadDebugAction   const& RdDebug()       const { return m_rd_debug;        }

  int                      Debug()         const;

  HandlerT&                Handler()             { return m_handler;         }

  in_addr_t                SockSrcAddr()   const { return m_sock_src_addr;   }
  in_port_t                SockSrcPort()   const { return m_sock_src_port;   }
  in_addr_t                SockDstAddr()   const { return m_sock_dst_addr;   }
  in_port_t                SockDstPort()   const { return m_sock_dst_port;   }
  void                     SockDstPort(in_port_t p) { m_sock_dst_port = p;   }
  in_addr_t                SockIfAddr()    const { return m_sock_if_addr;    }
  // clang-format on

  void EnableDgramPktInfo(bool a_enable = true);
  void EnablePktTimeStamps(bool a_enable);

  void Clear();
  void Reset();

  void SetFileReader(AIOReader* a_reader) { m_file_reader.reset(a_reader); }

  int  ReportError(
      IOType a_tp, int a_ec, const std::string& a_err, const src_info& a_si, bool a_throw = true);
  int ReportError(
      IOType a_tp, int a_ec, const std::string& a_err, src_info&& a_si, bool a_throw = true);

  template <class... Args>
  void Log(log_level a_level, src_info&& a_sinfo, Args&&... args);

private:
  Reactor*                           m_owner;
  std::string                        m_name;
  int                                m_fd;
  FdTypeT                            m_fd_type = FdTypeT::File;
  HandlerT                           m_handler;
  ErrHandler                         m_on_error;
  ReadSizeEstim                      m_read_at_least;
  void*                              m_instance;
  void*                              m_opaque;
  std::unique_ptr<dynamic_io_buffer> m_rd_buff_ptr;
  dynamic_io_buffer*                 m_rd_buff;
  std::unique_ptr<dynamic_io_buffer> m_wr_buff_ptr;
  dynamic_io_buffer*                 m_wr_buff;
  ReadDebugAction                    m_rd_debug;
  TriggerT                           m_trigger;
  std::unique_ptr<AIOReader>         m_file_reader;
  std::unique_ptr<POpenCmd>          m_exec_cmd;
  std::string                        m_ident;
  time_val                           m_ts_wire;
  bool                               m_with_pkt_info   = false;
  bool                               m_pkt_time_stamps = false;
  in_addr_t                          m_sock_src_addr   = 0;
  in_port_t                          m_sock_src_port   = 0;
  in_addr_t                          m_sock_dst_addr   = 0;
  in_port_t                          m_sock_dst_port   = 0;
  in_addr_t                          m_sock_if_addr    = 0;

  friend class Reactor;

  // clang-format off
  long Handle     (uint32_t a_events);
  long HandleIO   (uint32_t a_events);
  long HandleRawIO(uint32_t a_events);
  long HandlePipe (uint32_t a_events);
  long HandleFile (uint32_t a_events);
  long HandleEvent(uint32_t a_events, bool);
  long HandleTimer(uint32_t a_events);
  long HandleSignal(uint32_t a_events);
  long HandleAccept(uint32_t a_events);
  // clang-format on

  struct DebugFunEval {
    template <typename T>
    static void run(const T& a, const char* buf, size_t n)
    {
      if (UNLIKELY(a)) a(buf, n);
    }
    static void run(std::nullptr_t, const char*, size_t) {}
  };

  template <typename Action, typename DebugAction>
  std::pair<int, bool> ReadUntilEAgain(Action const& a_action, DebugAction const& a_debug_act);

  static std::string   Ident(Reactor*, int a_fd, std::string const& a_name);
};

} // namespace utxx
