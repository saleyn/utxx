// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  ReactorFdInfo.hpp
//------------------------------------------------------------------------------
/// \brief I/O multiplexing event reactor's file descriptor state
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#pragma once

#include <utxx/io/ReactorTypes.hpp>
#include <utxx/io/ReactorAIOReader.hpp>
#include <utxx/io/ReactorCmdExec.hpp>
#include <utxx/enum.hpp>
#include <type_traits>
#include <netinet/in.h>

namespace utxx {
namespace io   {

UTXX_ENUM
(FdTypeT, int,
  Stream,
  Datagram,
  SeqPacket,
  File,
  Pipe,
  Event,
  Timer,
  Signal
);

class FdInfo {
public:
  explicit FdInfo (
    Reactor*            a_reactor     = nullptr,
    std::string const&  a_name        = std::string(),
    int                 a_fd          = -1,
    FdTypeT             a_fd_type     = FdTypeT::UNDEFINED,
    ErrHandler  const&  a_on_error    = nullptr,
    void*               a_instance    = nullptr,
    void*               a_opaque      = nullptr,
    int                 a_rd_bufsz    = 0,
    int                 a_wr_bufsz    = 0,
    dynamic_io_buffer*  a_wr_buf      = nullptr,
    ReadSizeEstim       a_read_sz_fun = nullptr,
    TriggerT            a_trigger     = EDGE_TRIGGERED
  );

  template<typename Action>
  void SetHandler(HType a_type, const Action& a);

  // Note: we mark this constructor as "noexcept" so that the std::vector
  // knows to call it when it grows (instead of calling the deleted
  // copy constructor:
  FdInfo(FdInfo&& a_rhs) noexcept;
  FdInfo(FdInfo const& a_rhs) = delete;

  ~FdInfo();

  HType                    Type()          const { return m_handler.Type(); }
  const std::string&       Name()          const { return m_name;      }
  void                     Name(const std::string& a) { m_name = a;    }
  int                      FD()            const { return m_fd;        }
  void                     FD(int a_fd)          { m_fd = a_fd;        }
  void*                    Instance()      const { return m_instance;  }
  void*                    Opaque()        const { return m_opaque;    }

  dynamic_io_buffer*       RdBuff()              { return m_rd_buff;   }
  dynamic_io_buffer const* RdBuff()        const { return m_rd_buff;   }

  dynamic_io_buffer*       WrBuff()              { return m_wr_buff;   }
  dynamic_io_buffer const* WrBuff()        const { return m_wr_buff;   }

  TriggerT                 Trigger()       const { return m_trigger;   }

  Reactor&                 Owner()         { assert(m_owner); return *m_owner;}

  AIOReader*               FileReader()          { return m_file_reader.get();}
  POpenCmd*                PipeReader()          { return m_exec_cmd.get();   }

  /// Identifier used as the logging prefix for this component
  const std::string&       Ident()         const { return m_ident;     }

  template <typename Action>
  void                     RdDebug(Action a_act) { m_rd_debug = a_act; }
  ReadDebugAction   const& RdDebug()       const { return m_rd_debug;  }

  int                      Debug()         const;

  HandlerT&                Handler()             { return m_handler;   }

  /// Datagram socket source address      (enabled with EnableDgramPktInfo())
  in_addr_t                SockSrcAddr()   const { return m_sock_src_addr; }
  /// Datagram socket source port         (enabled with EnableDgramPktInfo())
  in_port_t                SockSrcPort()   const { return m_sock_src_port; }
  /// Datagram socket destination address (enabled with EnableDgramPktInfo())
  in_addr_t                SockDstAddr()   const { return m_sock_dst_addr; }
  /// Datagram socket destination port    (enabled with EnableDgramPktInfo())
  in_port_t                SockDstPort()   const { return m_sock_dst_port; }
  void                     SockDstPort(in_port_t port)  { m_sock_dst_port=port;}
  /// Datagram socket interface   address (enabled with EnableDgramPktInfo())
  in_addr_t                SockIfAddr()    const { return m_sock_if_addr;  }

  // Note: avoid copying m_handler, as it leads to double free error
  // upon deallocation!
  FdInfo& operator= (FdInfo const& a_rhs) = delete;
  FdInfo& operator= (FdInfo&&      a_rhs);

  /// Enable receiving of UDP source/destination address
  void EnableDgramPktInfo(bool a_enable = true);

  void Clear();  ///< Unregister m_fd from reactor (set to -1) and call Reset()
  void Reset();  ///< Reset internal state (m_fd must be -1)

  void SetFileReader(AIOReader* a_reader) { m_file_reader.reset(a_reader); }
  int  ReportError(IOType a_tp, int a_ec, const std::string& a_err,
                   const utxx::src_info& a_si, bool a_throw = true);
  int  ReportError(IOType a_tp, int a_ec, const std::string& a_err,
                   utxx::src_info&& a_si, bool  a_throw = true);

  /// Pass-through method to the reactor's logger
  template <class... Args>
  void Log(utxx::log_level a_level, utxx::src_info&& a_sinfo, Args&&... args);

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
  dynamic_io_buffer*                 m_rd_buff; // May be not owned
  std::unique_ptr<dynamic_io_buffer> m_wr_buff_ptr;
  dynamic_io_buffer*                 m_wr_buff; // May be not owned
  ReadDebugAction                    m_rd_debug;
  TriggerT                           m_trigger;
  std::unique_ptr<AIOReader>         m_file_reader;
  std::unique_ptr<POpenCmd>          m_exec_cmd;
  std::string                        m_ident;
  bool                               m_with_pkt_info  = false;
  in_addr_t                          m_sock_src_addr  = 0;
  in_port_t                          m_sock_src_port  = 0;
  in_addr_t                          m_sock_dst_addr  = 0;
  in_port_t                          m_sock_dst_port  = 0;
  in_addr_t                          m_sock_if_addr   = 0;

  friend class Reactor;

  // Handle event from the reactor
  long Handle(uint32_t a_events);

  long HandleIO    (uint32_t a_events);
  long HandleRawIO (uint32_t a_events);
  long HandlePipe  (uint32_t a_events);
  long HandleFile  (uint32_t a_events);
  long HandleEvent (uint32_t a_events, bool);
  long HandleTimer (uint32_t a_events);
  long HandleSignal(uint32_t a_events);
  long HandleAccept(uint32_t a_events);

  struct DebugFunEval {
    template <typename T>
    static void run(const T& a, const char* buf, size_t n)
                   { if (UNLIKELY(a)) a(buf, n);   }
    static void run(std::nullptr_t, const char* buf, size_t n) {}
  };

  template<typename Action, typename ReadDebugAction>
  std::pair<int, bool>
  ReadUntilEAgain(Action const& a_action, ReadDebugAction const& a_debug_act);

  static std::string Ident(Reactor*, int a_fd, std::string const& a_name);
};

} // namespace io
} // namespace utxx