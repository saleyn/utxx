//----------------------------------------------------------------------------
/// \file   pcap.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Support for PCAP file format writing.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-21
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

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#pragma once

#include <utxx/error.hpp>
#include <utxx/endian.hpp>
#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>

namespace utxx {

/**
 * PCAP file reader/writer
 */
struct pcap {
    enum class proto {
        undefined = -1,
        other     = 0,
        tcp       = IPPROTO_TCP,
        udp       = IPPROTO_UDP
    };

    struct file_header {
        uint32_t magic_number;   /* magic number */
        uint16_t version_major;  /* major version number */
        uint16_t version_minor;  /* minor version number */
        int32_t  thiszone;       /* GMT to local correction */
        uint32_t sigfigs;        /* accuracy of timestamps */
        uint32_t snaplen;        /* max length of captured packets, in octets */
        uint32_t network;        /* data link type */
    };

    struct packet_header {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
    };

    /// IP packet frame
    struct ip_frame {
        iphdr  ip;

        uint8_t  protocol() const { return ip.protocol;       }
        uint32_t src_ip()   const { return ntohl(ip.saddr);   }
        uint32_t dst_ip()   const { return ntohl(ip.daddr);   }

        std::string src() const {
            char buf[32]; char* p = buf; fmt(p, src_ip());
            return std::string(buf);
        }
        std::string dst() const {
            char buf[32]; char* p = buf; fmt(p, dst_ip());
            return std::string(buf);
        }

    protected:
        static char* fmt(char* p, uint32_t ip) {
            int n = sprintf(p, "%u.%u.%u.%u",
                ip >> 24 & 0xFF, ip >> 16 & 0xFF, ip >> 8 & 0xFF, ip & 0xFF);
            return p + n;
        }
    };

    /// UDP packet frame
    struct udp_frame : public ip_frame {
        udphdr udp;

        uint16_t src_port() const { return ntohs(udp.source); }
        uint16_t dst_port() const { return ntohs(udp.dest);   }

        std::string src() const {
            char buf[32]; fmt(ip_frame::fmt(buf, src_ip()), src_port());
            return std::string(buf);
        }
        std::string dst() const {
            char buf[32]; fmt(ip_frame::fmt(buf, dst_ip()), dst_port());
            return std::string(buf);
        }

    private:
        static char* fmt(char* buf, uint16_t port) {
            return buf + sprintf(buf, ":%d", port);
        }
    };


    /// TCP packet frame
    struct tcp_frame : public ip_frame {
        tcphdr tcp;

        uint16_t src_port() const { return ntohs(tcp.source); }
        uint16_t dst_port() const { return ntohs(tcp.dest);   }

        std::string src() const {
            char buf[32]; fmt(ip_frame::fmt(buf, src_ip()), src_port());
            return std::string(buf);
        }
        std::string dst() const {
            char buf[32]; fmt(ip_frame::fmt(buf, dst_ip()), dst_port());
            return std::string(buf);
        }

    private:
        static char* fmt(char* buf, uint16_t port) {
            return buf + sprintf(buf, ":%d", port);
        }
    } __attribute__ ((packed));

    BOOST_STATIC_ASSERT(sizeof(ethhdr)         == 14);
    BOOST_STATIC_ASSERT(sizeof(iphdr)          == 20);
    BOOST_STATIC_ASSERT(sizeof(udphdr)         ==  8);
    BOOST_STATIC_ASSERT(sizeof(tcphdr)         == 20);
    BOOST_STATIC_ASSERT(sizeof(udp_frame::udp) ==  8);
    BOOST_STATIC_ASSERT(sizeof(tcp_frame::tcp) == 20);
    BOOST_STATIC_ASSERT(sizeof(udp_frame)      == 28);
    BOOST_STATIC_ASSERT(sizeof(tcp_frame)      == 40);

    pcap()
        : m_big_endian(true),  m_file(NULL)
        , m_own_handle(false), m_frame_offset(0)
        , m_eth_header{{0}}
        , m_tcp_seqnos{0,0}
    {
        m_eth_header.h_proto = htons(ETH_P_IP);
    }

    ~pcap() { close(); }

    long open_read(const std::string& a_filename, bool a_is_pipe = false) {
        return open(a_filename.c_str(), a_is_pipe ? "r" : "rb", a_is_pipe);
    }

    long open_write(const std::string& a_filename, bool a_is_pipe = false) {
        long sz = open(a_filename.c_str(), a_is_pipe ? "w" : "wb+", a_is_pipe);
        if (sz < 0)
            return sz;
        write_file_header();
        return sz;
    }

    long popen(const char* a_filename, const std::string& a_mode) {
        return open(a_filename, a_mode, true);
    }

    long open(const char* a_filename, const std::string& a_mode) {
        return open(a_filename, a_mode, false);
    }

    static bool is_pcap_header(const char* buf, size_t sz) {
        return sz >= 4 &&
             ( *reinterpret_cast<const uint32_t*>(buf) == 0xa1b2c3d4
            || *reinterpret_cast<const uint32_t*>(buf) == 0xd4c3b2a1);
    }

    // See: http://www.tcpdump.org/linktypes.html
    enum class link_type {
        ethernet = 1,       // Record ETHR,IP,TCP headers
        raw_tcp   = 12       // Record IP,TCP headers (no ETHR)
    };

    void init_file_header(link_type a_tp = link_type::ethernet) {
        encode_file_header(reinterpret_cast<char*>(&m_file_header), sizeof(file_header), a_tp);
        m_frame_offset = a_tp == link_type::raw_tcp ? 0 : sizeof(ethhdr);
        memset(&m_tcp_seqnos, 0, sizeof(m_tcp_seqnos));
    }

    static int encode_file_header(char* buf, size_t sz, link_type a_tp = link_type::ethernet) {
        BOOST_ASSERT(sz >= sizeof(file_header));
        file_header* p = reinterpret_cast<file_header*>(buf);
        p->magic_number     = 0xa1b2c3d4;
        p->version_major    = 2;
        p->version_minor    = 4;
        p->thiszone         = 0;
        p->sigfigs          = 0;
        p->snaplen          = 65535;
        p->network          = int(a_tp);
        return sizeof(file_header);
    }

    template <size_t N>
    static int encode_packet_header(char (&buf)[N], const struct timeval& tv,
                                    proto a_proto, size_t len) {
        return encode_packet_header(buf, N, tv, a_proto, len);
    }

    static int encode_packet_header(char* a_buf, size_t a_size,
                                    const struct timeval& tv,
                                    proto a_proto, size_t len)
    {
        assert(a_size >= sizeof(packet_header));
        packet_header* p = reinterpret_cast<packet_header*>(a_buf);
        int sz      = len + (a_proto == proto::tcp ? sizeof(tcp_frame) : sizeof(udp_frame));
        p->ts_sec   = tv.tv_sec;
        p->ts_usec  = tv.tv_usec;
        p->incl_len = sz;
        p->orig_len = sz;
        return sizeof(packet_header);
    }

    int read_file_header() {
        char buf[sizeof(file_header)];
        const char* p = buf;
        size_t n = read(buf, sizeof(buf));
        if (n < sizeof(buf))
            return -1;
        return read_file_header(p, sizeof(buf));
    }

    // For use with externally open files
    /// @return size of consumed file reader
    static int read_file_header(pcap& file, const char*& buf, size_t sz) {
        auto begin = buf;
        if (sz < sizeof(file_header) || !is_pcap_header(buf, sz))
            return -1;

        const uint8_t* p = (const uint8_t*)buf;
        file.m_big_endian = *p++ == 0xa1
                         && *p++ == 0xb2
                         && *p++ == 0xc3
                         && *p++ == 0xd4;
        if (!file.m_big_endian) {
            file.m_file_header = *reinterpret_cast<const file_header*>(buf);
            buf += sizeof(file_header);
        } else {
            file.m_file_header.magic_number  = get32be(buf);
            file.m_file_header.version_major = get16be(buf);
            file.m_file_header.version_minor = get16be(buf);
            file.m_file_header.thiszone      = (int32_t)get32be(buf);
            file.m_file_header.sigfigs       = get32be(buf);
            file.m_file_header.snaplen       = get32be(buf);
            file.m_file_header.network       = get32be(buf);
        }
        file.m_frame_offset = link_type(file.m_file_header.network) == link_type::raw_tcp
                            ? 0 : sizeof(ethhdr);
        return buf - begin;
    }

    int read_file_header(const char*& buf, size_t sz) {
        int rc = read_file_header(*this, buf, sz);
        return rc;
    }

    // For use with externally open files
    /// @param file is the PCAP file instance
    /// @param buf  holds the PCAP data to be read
    /// @param sz   is the size of the data in \a buf
    /// @return size of next packet
    static int read_packet_header(pcap& file, const char*& buf, size_t sz) {
        if (sz < sizeof(packet_header))
            return -1;
        if (!file.m_big_endian) {
            file.m_pkt_header = *reinterpret_cast<const packet_header*>(buf);
            buf += sizeof(packet_header);
        } else {
            file.m_pkt_header.ts_sec   = get32be(buf);
            file.m_pkt_header.ts_usec  = get32be(buf);
            file.m_pkt_header.incl_len = get32be(buf);
            file.m_pkt_header.orig_len = get32be(buf);
        }
        return file.m_pkt_header.incl_len;
    }

    int read_packet_header(const char*& buf, size_t sz) {
        return read_packet_header(*this, buf, sz);
    }

    static proto parse_protocol_type(const char* buf, size_t sz,
                                     link_type a_tp = link_type::ethernet) {
        size_t offset = a_tp == link_type::raw_tcp ? 0 : sizeof(ethhdr);
        if (sz < offset + sizeof(ip_frame))
            return proto::undefined;
        auto  p = (ip_frame*)(buf + offset);
        proto res;
        switch (p->ip.protocol) {
            case IPPROTO_TCP: res = proto::tcp; break;
            case IPPROTO_UDP: res = proto::udp; break;
            default         : res = proto::other;
        }
        return res;
    }

    template <typename Frame>
    typename std::enable_if<std::is_same<Frame, udp_frame>::value ||
                            std::is_same<Frame, tcp_frame>::value, int>::
    type parse_frame(const char*& buf, size_t sz) {
        return parse_frame
            (buf, sz,
             std::is_same<Frame, udp_frame>::value ? IPPROTO_UDP : IPPROTO_TCP,
             sizeof(Frame));
    }

    size_t read(char* buf, size_t sz) { return fread(buf, 1, sz, m_file); }

    int write_file_header(link_type a_tp = link_type::ethernet) {
        char buf[sizeof(file_header)];
        int n = encode_file_header(buf, sizeof(buf), a_tp);
        return fwrite(buf, 1, n, m_file);
    }

    int write_packet_header(
        const struct timeval& a_timestamp, proto a_proto, size_t a_packet_size)
    {
        char buf[sizeof(packet_header)];
        int n = encode_packet_header(buf, a_timestamp, a_proto, a_packet_size);
        return fwrite(buf, 1, n, m_file);
    }

    int write_packet_header(const packet_header& a_header) {
        return fwrite(&a_header, 1, sizeof(packet_header), m_file);
    }

    template <typename Frame>
    typename std::enable_if<std::is_same<Frame, udp_frame>::value ||
                            std::is_same<Frame, tcp_frame>::value, size_t>::
    type frame_size(size_t a_pkt_sz) {
      return sizeof(packet_header) + m_frame_offset + sizeof(Frame) + a_pkt_sz;
    }

    template <typename Frame>
    typename std::enable_if<std::is_same<Frame, udp_frame>::value ||
                            std::is_same<Frame, tcp_frame>::value, int>::
    type encode_frame(bool a_inbound, char* buf, size_t sz,
                      uint32_t a_src_ip, uint16_t a_src_port,
                      uint32_t a_dst_ip, uint16_t a_dst_port,
                      const char* a_data,  size_t a_data_sz)
    {
        return write_frame<Frame, true, true>
            (a_inbound, buf, sz, a_src_ip, a_src_port, a_dst_ip, a_dst_port,
             a_data, a_data_sz);
    }

    template <typename Frame>
    typename std::enable_if<std::is_same<Frame, udp_frame>::value ||
                            std::is_same<Frame, tcp_frame>::value, int>::
    type write_frame(bool a_inbound,
                     uint32_t a_src_ip, uint16_t a_src_port,
                     uint32_t a_dst_ip, uint16_t a_dst_port,
                     size_t a_data_sz)
    {
        static_assert(sizeof(Frame) <= sizeof(m_frame), "Invalid size");
        auto buf = reinterpret_cast<char*>(&m_frame);
        return write_frame<Frame, false, false>
            (a_inbound, buf, sizeof(Frame), a_src_ip, a_src_port, a_dst_ip, a_dst_port,
             nullptr, a_data_sz);
    }

    int write(const char* buf, size_t sz) {
        return fwrite(buf, 1, sz, m_file);
    }

    /// @param a_mask is an IP address mask in network byte order.
    bool match_dst_ip(uint32_t a_ip_mask, uint16_t a_port = 0) {
        uint8_t b = a_ip_mask >> 24 & 0xFF;
        // Note the IP frame's part is identical and port info is also
        // positioned the same in UDP/TCP, so it's irrelevant if we
        // reference udp or tcp in the union:
        if (b != 0 && (b != (m_frame.u.ip.daddr >> 24 & 0xFF)))
            return false;
        b = a_ip_mask >> 16 & 0xFF;
        if (b != 0 && (b != (m_frame.u.ip.daddr >> 16 & 0xFF)))
            return false;
        b = a_ip_mask >> 8 & 0xFF;
        if (b != 0 && (b != (m_frame.u.ip.daddr >> 8 & 0xFF)))
            return false;
        b = a_ip_mask & 0xFF;
        if (b != 0 && (b != (m_frame.u.ip.daddr & 0xFF)))
            return false;
        if (a_port != 0 && (a_port != m_frame.u.udp.dest))
            return false;
        return true;
    }

    bool     is_open()               const { return m_file != NULL; }
    uint64_t tell()                  const { return m_file ? ftell(m_file) : 0; }

    const    file_header&   header() const { return m_file_header;  }
    const    packet_header& packet() const { return m_pkt_header;   }
    const    udp_frame&     uframe() const { return m_frame.u;      }
    const    tcp_frame&     tframe() const { return m_frame.t;      }

    size_t   frame_offset()          const { return m_frame_offset; }

    void set_handle(FILE* a_handle) {
        close();
        m_own_handle = false;
        m_file = a_handle;
    }

private:
    bool          m_big_endian;
    FILE*         m_file;
    bool          m_own_handle;
    size_t        m_frame_offset;
    ethhdr        m_eth_header;
    uint          m_tcp_seqnos[2];
    union uframe {
        udp_frame u;
        tcp_frame t;
    }             m_frame;
    file_header   m_file_header;
    packet_header m_pkt_header;
    bool          m_is_pipe;

    void close() {
        if (m_file && m_own_handle) {
            m_is_pipe ? pclose(m_file) : fclose(m_file);
            m_is_pipe = false;
            m_file    = NULL;
        }
    }

    /// @return 0 if opening a pipe or stdin
    long open(const char* a_filename, const std::string& a_mode, bool a_is_pipe) {
        close();
        bool use_stdin = !a_is_pipe
                      && (!strcmp(a_filename, "-") || !strcmp(a_filename, "/dev/stdin"));
        if (use_stdin)
            a_filename = "/dev/stdin";
        if (a_is_pipe)
            m_file = ::popen(a_filename, a_mode.c_str());
        else
            m_file = ::fopen(a_filename, a_mode.c_str());
        if (!m_file)
            return -1;
        m_is_pipe    = a_is_pipe;
        m_own_handle = true;
        long sz;
        if (a_is_pipe || use_stdin)
            sz = 0;
        else {
            if (fseek(m_file, 0, SEEK_END) < 0) return -1;
            sz = ftell(m_file);
            if (fseek(m_file, 0, SEEK_SET) < 0) return -1;
        }
        return (m_file == NULL) ? -1 : sz;
    }

    int parse_frame(const char*& buf, size_t sz, int a_proto, size_t a_frame_sz) {
        auto frame_sz = sizeof(ethhdr) + a_frame_sz - m_frame_offset;
        if (sz < frame_sz)
            return -2;

        memcpy(&m_frame.u, buf+m_frame_offset, frame_sz); buf += frame_sz;

        return m_frame.u.ip.protocol != a_proto ? -1 : frame_sz;
    }

    template <typename Frame, bool WithEthr, bool WithData>
    typename std::enable_if<std::is_same<Frame, udp_frame>::value ||
                            std::is_same<Frame, tcp_frame>::value, int>::
    type write_frame(bool a_inbound, char* buf, size_t sz,
                     uint32_t a_src_ip, uint16_t a_src_port,
                     uint32_t a_dst_ip, uint16_t a_dst_port,
                     const char* a_data,  size_t a_data_sz)
    {
        auto ethrsz = WithEthr ? m_frame_offset : 0;
        auto expsz  = ethrsz + sizeof(Frame) + (WithData ? a_data_sz : 0);
        auto frame  = reinterpret_cast<Frame*>(buf + ethrsz);
        if (sz < expsz)
            return -1;
        if (WithEthr)
            memcpy(buf, &m_eth_header, m_frame_offset);
        auto& p    = frame->ip;
        p.version  = IPVERSION;
        p.ihl      = 5;    // 32-bit words
        p.tos      = 0;
        p.tot_len  = htons(a_data_sz + sizeof(Frame));
        p.id       = 0;
        p.frag_off = 0;
        p.ttl      = 64;   // Linux default time-to-live
        p.check    = 0;

        do_init_frame_proto(a_inbound, *frame, a_src_ip, a_src_port, a_dst_ip, a_dst_port, a_data_sz);

        if (WithData) {
            assert(a_data);
            memcpy(reinterpret_cast<char*>(frame)+sizeof(Frame), a_data, a_data_sz);
        }

        return expsz;
    }

    void do_init_frame_proto(bool a_inbound, tcp_frame& frame,
                             uint32_t a_src_ip, uint16_t a_src_port,
                             uint32_t a_dst_ip, uint16_t a_dst_port,
                             size_t   a_data_sz)
    {
        auto& seqno = m_tcp_seqnos[a_inbound];
        frame.ip.protocol = IPPROTO_TCP;
        frame.ip.saddr    = a_src_ip;
        frame.ip.daddr    = a_dst_ip;
        frame.tcp.source  = a_src_port;
        frame.tcp.dest    = a_dst_port;
        frame.tcp.seq     = htonl(seqno);
        frame.tcp.ack_seq = 0;
        frame.tcp.th_flags= 0;
        frame.tcp.syn     = !seqno;
        frame.tcp.ack     = 1;
        frame.tcp.doff    = 5;    // the size of TCP header in 32-bit words
        frame.tcp.window  = htons(32*1024);
        frame.tcp.check   = 0;
        frame.tcp.urg_ptr = 0;
        seqno += a_data_sz;
    }
    void do_init_frame_proto(bool, udp_frame& frame,
                             uint32_t a_src_ip, uint16_t a_src_port,
                             uint32_t a_dst_ip, uint16_t a_dst_port,
                             size_t   a_data_sz)
    {
        frame.ip.protocol = IPPROTO_UDP;
        frame.ip.saddr    = a_src_ip;
        frame.ip.daddr    = a_dst_ip;
        frame.udp.source  = a_src_port;
        frame.udp.dest    = a_dst_port;
        frame.udp.len     = htons(a_data_sz + sizeof(frame.udp));
        frame.udp.check   = 0;
    }

};

} // namespace utxx