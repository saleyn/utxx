// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// @file  ReactorCmdExec.hpp
//------------------------------------------------------------------------------
/// \brief Execute a command and connect its STDOUT to a pollable fd.
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string>
#include <utxx/error.hpp>

namespace mqt {
namespace io  {

//------------------------------------------------------------------------------
/// Run a shell command and allocate a pollable file descriptor for its output
//------------------------------------------------------------------------------
class POpenCmd {
  enum { READ_END, WRITE_END };
public:
  POpenCmd() = default;

  POpenCmd(const POpenCmd& a_rhs)      = delete;
  POpenCmd& operator=(const POpenCmd&) = delete;

  POpenCmd(POpenCmd&& a_rhs) {
    m_pid       = a_rhs.m_pid;
    m_stdout    = a_rhs.m_stdout;
    a_rhs.m_pid = -1;
  }

  POpenCmd(const std::string& a_cmd) {
    UTXX_PRETTY_FUNCTION(); // Cache pretty function name

    int fds[2];
    if (::pipe(fds) < 0)
      UTXX_THROWX_IO_ERROR(errno, "Failed to call pipe");

    m_pid = fork();
    if (m_pid < 0)
      UTXX_THROWX_IO_ERROR(errno, "Failed to fork a pipe process");
    else if (m_pid == 0) { // child
      close(STDIN_FILENO);
      close(STDERR_FILENO);
      close(fds[READ_END]);
      dup2 (fds[WRITE_END], STDOUT_FILENO);
      for  (int i=STDOUT_FILENO+1; i < 1024; ++i)
        close(i);
      const char* shell  = getenv("SHELL");
      if (!shell) shell  = "/usr/bin/sh";
      const char* argv[] = {shell, "-c", a_cmd.c_str(), nullptr};
      int rc = execv(shell, const_cast<char**>(argv)); // Never returns
      if (rc < 0)
        UTXX_THROWX_IO_ERROR(errno, "Failed to call pipe command '",
                         a_cmd, "'");
      exit (0);
    } else { // parent
      close(fds[WRITE_END]);
      m_stdout = fds[READ_END];
    }
  }

  ~POpenCmd() {
    if (m_pid < 0)
      return;

    close(m_stdout);
    int status;
    waitpid(m_pid, &status, WNOHANG);
  }

  int FD()  const { return m_stdout; }
  int PID() const { return m_pid;    }

private:
  pid_t   m_pid    = -1;
  int     m_stdout = -1;
};

} // namespace io
} // namespace utxx