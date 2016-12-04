//------------------------------------------------------------------------------
/// \file  pcapcut.cpp
//------------------------------------------------------------------------------
/// \brief Utility for cutting a part of a large pcap file
/// \see <a href="https://github.com/M0Rf30/xplico/blob/master/system/trigcap">
///      Alternative implementation using pcap.h</a>
//------------------------------------------------------------------------------
// Copyright (c) 2016 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-11-28
//------------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2016 Serge Aleynikov <saleyn@gmail.com>

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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <signal.h>
#include <utxx/pcap.hpp>
#include <utxx/string.hpp>
#include <utxx/path.hpp>
#include <utxx/get_option.hpp>
#include <utxx/buffer.hpp>
#include <utxx/version.hpp>

using namespace std;

//------------------------------------------------------------------------------
void usage(std::string const& err="")
{
    auto prog = utxx::path::basename(
        utxx::path::program::name().c_str(),
        utxx::path::program::name().c_str() + utxx::path::program::name().size()
    );

    if (!err.empty())
        cerr << "Invalid option: " << err << "\n\n";
    else {
        cerr << prog <<
        " - Tool for extracting packets from a pcap file\n"
        "Copyright (c) 2016 Serge Aleynikov\n"  <<
        VERSION() << "\n\n"                     <<
        "Usage: " << prog                       <<
        "[-V] [-h] -f InputFile -s StartPktNum -e EndPktNum [-n NumPkts] [-c|--count]"
                    " -o|-O OutputFile [-h]\n\n"
        "   -V|--version            - Version\n"
        "   -h|--help               - Help screen\n"
        "   -f InputFile            - Input file name\n"
        "   -o OutputFile           - Ouput file name (don't overwrite if exists)\n"
        "   -O OutputFile           - Ouput file name (overwrite if exists)\n"
        "   -s|--start StartPktNum  - Starting packet number (counting from 1)\n"
        "   -e|--end   EndPktNum    - Ending packet number (must be >= StartPktNum)\n"
        "   -n|--num   TotNumPkts   - Number of packets to save\n"
        "   -r|--raw                - Output raw packet payload only without pcap format\n"
        "   -c|--count              - Count number of packets in the file\n"
        "   -v                      - Verbose\n\n";
    }

    exit(1);
}

//------------------------------------------------------------------------------
void unhandled_exception() {
  auto p = current_exception();
  try    { rethrow_exception(p); }
  catch  ( exception& e ) { cerr << e.what() << endl; }
  catch  ( ... )          { cerr << "Unknown exception" << endl; }
  exit(1);
}

//------------------------------------------------------------------------------
//  MAIN
//------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    string in_file;
    string out_file;
    size_t pk_start  = 1, pk_end = 0, pk_cnt = 0;
    bool   overwrite = false;
    bool   raw_mode  = false;
    bool   count     = false;
    bool   verbose   = false;

    set_terminate (&unhandled_exception);

    utxx::opts_parser opts(argc, argv);

    while (opts.next()) {
        if (opts.match("-f", "",        &in_file))  continue;
        if (opts.match("-o", "",        &out_file)) continue;
        if (opts.match("-O", "",        &out_file)){overwrite=true; continue;}
        if (opts.match("-r", "--raw",   &raw_mode)) continue;
        if (opts.match("-s", "--start", &pk_start)) continue;
        if (opts.match("-e", "--end",   &pk_end))   continue;
        if (opts.match("-n", "--num",   &pk_cnt))   continue;
        if (opts.match("-c", "--count", &count))    continue;
        if (opts.match("-v", "",        &verbose))  continue;
        if (opts.match("-V", "--version")) throw std::runtime_error(VERSION());
        if (opts.is_help())                         usage();

        usage(opts());
    }

    if (pk_end > 0 && pk_cnt > 0)
        throw std::runtime_error("Cannot specify both -n and -e options!");
    else if (!pk_end && !pk_cnt && !count)
        throw std::runtime_error("Must specify either -n or -e option!");
    else if (!pk_start && !count)
        throw std::runtime_error("PktStartNumber (-s) must be greater than 0!");
    else if (pk_end && pk_end < pk_start)
        throw std::runtime_error
             ("Ending packet number (-e) must not be less than starting packet number (-s)!");
    else if (in_file.empty() || (!count && out_file.empty()))
        throw std::runtime_error("Must specify -f and -o options!");
    else if (!count && utxx::path::file_exists(out_file)) {
        if (!overwrite)
            throw std::runtime_error("Found existing output file: " + out_file);
        if (!utxx::path::file_unlink(out_file))
            throw std::runtime_error("Error deleting file " + out_file +
                                     ": " + strerror(errno));
    }

    if (pk_cnt) {
        pk_end = pk_start + pk_cnt - 1;
        pk_cnt = 0;
    }

    utxx::pcap fin;
    if (fin.open_read(in_file) < 0)
        throw std::runtime_error("Error opening " + in_file + ": " + strerror(errno));
    else if (fin.read_file_header() < 0)
        throw std::runtime_error("File " + in_file + " is not in PCAP format!");

    int n = 0;
    utxx::pcap fout(fin.big_endian(), fin.nsec_time());

    if (!count) {
        n = raw_mode ? fout.open(out_file.c_str(), "wb")
                    : fout.open_write(out_file, false, fin.get_link_type());
        if (n < 0)
            throw std::runtime_error("Error creating file " + out_file + ": " + strerror(errno));
    }

    utxx::basic_io_buffer<(1024*1024)> buf;

    while ((n = fin.read(buf.wr_ptr(), buf.capacity())) > 0) {
        buf.commit(n);
        if (verbose)
            cerr << "Read "    << n   << " bytes from source file (offset="
                 << fin.tell() << ')' << endl;

        while (buf.size() > sizeof(utxx::pcap::packet_header)) {
            const char*       header;
            const char*       begin  = header = buf.rd_ptr();
            int               frame_sz, sz;
            utxx::pcap::proto proto;

            // sz - total size of payload including frame_sz
            std::tie(frame_sz, sz, proto) = fin.read_packet_hdr_and_frame(begin, n);

            if (frame_sz < 0 || int(buf.size()) < sz) { // Not enough data in the buffer
                if (verbose && frame_sz < 0)
                    cerr << "Pkt#" << (pk_cnt+1) << ": Cannot read frame size of packet\n";
                buf.reserve(sz);
                break;
            }

            if (verbose)
                cerr << "Pkt#"      << (pk_cnt+1)   << " FrameSz=" << setw(2)    << frame_sz
                     << " Bytes="   << sz           << " BufSz="   << buf.size()
                     << " (BufPos=" << buf.rd_ptr() << ')'         << endl;

            if (++pk_cnt >= pk_start && !count) {
                if (pk_cnt > pk_end)
                    goto DONE;

                // Write to the output file
                if (!raw_mode) {
                    fout.write_packet_header(fin.packet());
                    buf.read(sizeof(utxx::pcap::packet_header));
                    sz    -= sizeof(utxx::pcap::packet_header);
                } else {
                    buf.read(frame_sz);
                    sz    -= frame_sz;
                }
                if (fout.write(buf.rd_ptr(), sz) < 0)
                    throw std::runtime_error(string("Error writing to file: ") + strerror(errno));
            }

            buf.read(sz);
        }

        buf.crunch();
    }

  DONE:
    fout.close();
    fin.close();

    if (count)
        cout << pk_cnt << " packets\n";

    return 0;
}