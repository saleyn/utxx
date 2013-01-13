//----------------------------------------------------------------------------
/// \file  pidfile.hpp
//----------------------------------------------------------------------------
/// \brief Creates and manages a file containing a process identifier.
/// This file is necessary for ease of administration of daemon processes.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-02-18
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
#ifndef _UTXX_PIDFILE_HPP_
#define _UTXX_PIDFILE_HPP_

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utxx/error.hpp>
#include <utxx/convert.hpp>


namespace utxx {

/**
 * PID file manager
 * Used to crete a file containing process identifier of currently
 * running process.  This file is used by administration scripts in
 * order to be able to kill the process by pid.
 */
struct pid_file {
    explicit pid_file(const char* a_filename, mode_t a_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
        throw (io_error)
        : m_filename(a_filename)
    {
        m_fd = open(a_filename, O_CREAT | O_RDWR | O_TRUNC, a_mode);
        if (m_fd < 0)
            throw io_error(errno, "Cannot open file:", a_filename);
        pid_t pid = ::getpid();
        std::string s = int_to_string(pid);
        if (::write(m_fd, s.c_str(), s.size()) < 0)
            throw io_error(errno, "Cannot write to file:", a_filename);
    }

    ~pid_file() {
        if (m_fd >= 0)
            ::close(m_fd);
        ::unlink(m_filename.c_str());
    }
private:
    std::string m_filename;
    int m_fd;
};

} // namespace utxx

#endif // _UTXX_PIDFILE_HPP_

