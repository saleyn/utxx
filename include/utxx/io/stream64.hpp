//----------------------------------------------------------------------------
/// \file stream64.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// Defines input/output streams that can be used to read/write files
/// larger than 4Gb.
//----------------------------------------------------------------------------
// Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-06-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is a part of utxx open-source project.

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

#include <fcntl.h>
#include <fstream>
#include <assert.h>
#include <ext/stdio_filebuf.h>

namespace utxx {
namespace io   {

/// An input stream class that sets the O_LARGEFILE flag
template <typename Char, typename Traits = std::char_traits<Char> >
class basic_ifstream64 : public std::basic_ifstream<Char, Traits>
{
    using base   = std::basic_ifstream<Char, Traits>;
    using buffer = __gnu_cxx::stdio_filebuf<Char, Traits>;

    void do_open(int a_fd, std::ios_base::openmode a_mode, int a_buf_sz) {
        /* FIXME: do same with wfilebuf? */
        new (reinterpret_cast<buffer*>(this->rdbuf()))
            buffer(a_fd, a_mode | std::ios_base::in, a_buf_sz);
    }
public:
    explicit basic_ifstream64(int a_fd,
        std::ios_base::openmode a_mode      = std::ios_base::in
                                            | std::ios_base::binary,
        int                     a_buf_sz    = 1024*1024)
    {
        assert(a_fd > -1);
        do_open(a_fd, a_mode, a_buf_sz);
    }

    explicit basic_ifstream64(const char* a_filename,
        std::ios_base::openmode a_mode      = std::ios_base::in
                                            | std::ios_base::binary,
        int                     a_buf_sz    = 1024*1024,
        mode_t                  a_perm      = S_IRUSR | S_IWUSR
                                            | S_IRGRP | S_IROTH, /* 644 */
        int                     a_flags     = O_RDONLY| O_LARGEFILE)
    {
        int fd = ::open(a_filename, a_flags, a_perm);
        if (fd > -1)
            do_open(fd, a_mode, a_buf_sz);
    }

    virtual ~basic_ifstream64() override {}
};

/// An outinput stream class that sets the O_LARGEFILE flag
template <typename Char, typename Traits = std::char_traits<Char> >
class basic_ofstream64 : public std::basic_ofstream<Char, Traits>
{
    using base   = std::basic_ofstream<Char, Traits>;
    using buffer = __gnu_cxx::stdio_filebuf<Char, Traits>;

    void do_open(int a_fd, std::ios_base::openmode a_mode, int a_buf_sz) {
        /* FIXME: do same with wfilebuf? */
        new (reinterpret_cast<buffer*>(this->rdbuf()))
            buffer(a_fd, a_mode | std::ios_base::out, a_buf_sz);
    }
public:
    explicit basic_ofstream64(int a_fd,
        std::ios_base::openmode a_mode      = std::ios_base::out
                                            | std::ios_base::binary,
        int                     a_buf_sz    = 1024*1024)
    {
        assert(a_fd > -1);
        do_open(a_fd, a_mode, a_buf_sz);
    }

    explicit basic_ofstream64(const char* a_filename,
        std::ios_base::openmode a_mode      = std::ios_base::out
                                            | std::ios_base::binary,
        int                     a_buf_sz    = 1024*1024,
        mode_t                  a_perm      = S_IRUSR | S_IWUSR
                                            | S_IRGRP | S_IROTH, /* 644 */
        int                     a_flags     = O_CREAT | O_TRUNC
                                            | O_WRONLY| O_LARGEFILE)
    {
        if (a_mode & (std::ios_base::ate | std::ios_base::app))
            a_flags &= ~O_TRUNC | O_APPEND;
        int fd = ::open(a_filename, a_flags, a_perm);
        if (fd > -1)
            do_open(fd, a_mode, a_buf_sz);
    }

    virtual ~basic_ofstream64() override {}
};

} // namespace io
} // namespace utxx

namespace std
{
    using ofstream64 = utxx::io::basic_ofstream64<char>;
    using ifstream64 = utxx::io::basic_ifstream64<char>;
}