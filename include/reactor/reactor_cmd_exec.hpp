// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_cmd_exec.hpp
//------------------------------------------------------------------------------
/// \brief Execute a command and connect its STDOUT to a pollable fd
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#pragma once

#include <fcntl.h>
#include <reactor/compat.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace utxx {

//------------------------------------------------------------------------------
/// Run a shell command and allocate a pollable file descriptor for its output
//------------------------------------------------------------------------------
class POpenCmd {
  enum { READ_END, WRITE_END };

public:
  POpenCmd()                           = default;

  POpenCmd(const POpenCmd&)            = delete;
  POpenCmd& operator=(const POpenCmd&) = delete;

  POpenCmd(POpenCmd&& a_rhs)
  {
    m_pid       = a_rhs.m_pid;
    m_stdout    = a_rhs.m_stdout;
    m_cmd       = std::move(a_rhs.m_cmd);
    a_rhs.m_pid = -1;
  }

  POpenCmd(const std::string& a_cmd) : m_cmd(a_cmd)
  {
    REACTOR_PRETTY_FUNCTION();

    int fds[2];
    if (::pipe(fds) < 0) REACTOR_THROWX_IO_ERROR(errno, "Failed to call pipe");

    m_pid = fork();
    if (m_pid < 0)
      REACTOR_THROWX_IO_ERROR(errno, "Failed to fork a pipe process");
    else if (m_pid == 0) { // child
      close(STDIN_FILENO);
      close(STDERR_FILENO);
      close(fds[READ_END]);
      dup2(fds[WRITE_END], STDOUT_FILENO);
      for (int i = STDOUT_FILENO + 1; i < 1024; ++i) close(i);
      const char* shell = getenv("SHELL");
      if (!shell) shell = "/usr/bin/sh";
      const char* argv[] = {shell, "-c", a_cmd.c_str(), nullptr};
      int         rc     = execv(shell, const_cast<char**>(argv));
      if (rc < 0) REACTOR_THROWX_IO_ERROR(errno, "Failed to call pipe command '", a_cmd, "'");
      exit(0);
    } else { // parent
      close(fds[WRITE_END]);
      m_stdout = fds[READ_END];
    }
  }

  ~POpenCmd()
  {
    if (m_pid < 0) return;
    close(m_stdout);
    int status;
    waitpid(m_pid, &status, WNOHANG);
  }

  // clang-format off
  int                FD()      const { return m_stdout; }
  int                PID()     const { return m_pid;    }
  const std::string& Command() const { return m_cmd;    }
  // clang-format on

private:
  pid_t       m_pid    = -1;
  int         m_stdout = -1;
  std::string m_cmd;
};

} // namespace utxx
