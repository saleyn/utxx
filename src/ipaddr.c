// vim:ts=2 et sw=2
//----------------------------------------------------------------------------
/// \file ipaddr.c
//----------------------------------------------------------------------------
/// \brief Convert integer to string IP addresses
//----------------------------------------------------------------------------
// Copyright (c) 2016 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-08-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Convert integer to string IP addresses\n\n"
                        "Usage: %s IntegerIpAddr ...\n", argv[0]);
        exit(1);
    }

    for (int i=1; i < argc; ++i) {
        uint32_t value=atoi(argv[i]);
        struct in_addr addr = {value};
        printf("%-12u %s\n", value, inet_ntoa(addr));
    }
    return 0;
}