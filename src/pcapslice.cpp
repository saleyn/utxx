//------------------------------------------------------------------------------
/// \file  pcapcut.cpp
//------------------------------------------------------------------------------
/// \brief Utility for cutting a part of a large pcap file
//------------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
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
    if (!err.empty())
        std::cerr << "Invalid option: " << err << "\n\n";

    std::cerr <<
        utxx::path::program::name() <<
        " - Tool for extracting packets from a pcap file\n"
        "Copyright (c) 2016 Serge Aleynikov\n\n"
        "Usage: " << utxx::path::program::name() <<
        "[-V] [-h] -f InputFile -s StartPktNum -e EndPktNum [-n PktCount]"
                 " -o OutputFile [-h]\n\n"
        "   -V|--version            - Version\n"
        "   -h|--help               - Help screen\n"
        "   -f InputFile            - Input file name\n"
        "   -o OutputFile           - Ouput file name (don't overwrite)\n"
        "   -O OutputFile           - Ouput file name (overwrite if exists)\n"
        "   -s|--start StartPktNum  - Starting packet number (counting from 1)\n"
        "   -e|--end   EndPktNum    - Ending packet number (must be >= StartPktNum\n"
        "   -n|--count PktCount     - Number of packets to save\n\n";
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

    utxx::opts_parser opts(argc, argv);

    while (opts.next()) {
        if (opts.match("-f", "",        &in_file))   continue;
        if (opts.match("-o", "",        &out_file))  continue;
        if (opts.match("-O", "",        &out_file)){ overwrite=true; continue; }
        if (opts.match("-s", "--start", &pk_start))  continue;
        if (opts.match("-e", "--end",   &pk_end))    continue;
        if (opts.match("-n", "--count", &pk_cnt))    continue;
        if (opts.match("-V", "--version")) {
            std::cerr << VERSION() << '\n';
            exit(1);
        }
        if (opts.is_help())             usage();

        usage(utxx::to_string("Invalid option: ", opts()));
    }

    if (pk_end > 0 && pk_cnt > 0) {
        std::cerr << "Cannot specify both -n and -e options!\n";
        exit(1);
    } else if (!pk_end && !pk_cnt) {
        std::cerr << "Must specify either -n or -e option!\n";
        exit(1);
    } else if (!pk_start) {
        std::cerr << "PktStartNumber (-s) must be greater than 0!\n";
        exit(1);
    } else if (in_file.empty() || out_file.empty()) {
        std::cerr << "Must specify -f and -o options!\n";
        exit(1);
    } else if (utxx::path::file_exists(out_file)) {
        if (!overwrite) {
            std::cerr << "Found existing output file: " << out_file << endl;
            exit(1);
        }
        if (!utxx::path::file_unlink(out_file)) {
            std::cerr << "Error deleting file "  << out_file
                      << ": " << strerror(errno) << endl;
            exit(1);
        }
    }

    if (pk_cnt) {
        pk_end = pk_start + pk_cnt - 1;
        pk_cnt = 0;
    }

    utxx::pcap fin;
    if (fin.open_read(in_file) < 0) {
        std::cerr << "Error opening " << in_file << ": " << strerror(errno) << '\n';
        exit(1);
    } else if (fin.read_file_header() < 0) {
        std::cerr << "File " << in_file << " is not in PCAP format!\n";
        exit(1);
    }

    utxx::pcap fout(fin.big_endian(), fin.nsec_time());
    if (fout.open_write(out_file, false, fin.get_link_type()) < 0) {
        std::cerr << "Error creating file "  << out_file
                  << ": " << strerror(errno) << endl;
        exit(2);
    }

    utxx::basic_io_buffer<(64*1024)> buf;
    int n;

    while ((n = fin.read(buf.wr_ptr(), buf.capacity())) > 0) {
        buf.commit(n);
        while (buf.size() > sizeof(utxx::pcap::packet_header)) {
            const char*       header;
            const char*       begin  = header = buf.rd_ptr();
            int               frame_sz, sz;
            utxx::pcap::proto proto;

            // sz - total size of payload including frame_sz
            std::tie(frame_sz, sz, proto) = fin.read_packet_hdr_and_frame(begin, n);

            if (frame_sz < 0 || int(buf.size()) < sz) { // Not enough data in the buffer
                buf.reserve(sz);
                break;
            }

            if (++pk_cnt >= pk_start) {
                if (pk_cnt > pk_end)
                    goto DONE;

                // Write to the output file
                fout.write_packet_header(fin.packet());
                buf.read(sizeof(utxx::pcap::packet_header));
                sz -= sizeof(utxx::pcap::packet_header);
                fout.write(buf.rd_ptr(), sz);
            }

            buf.read(sz);
        }

        buf.crunch();
    }

  DONE:
    fout.close();
    fin.close();

    return 0;
}