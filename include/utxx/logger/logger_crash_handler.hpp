/** ==========================================================================
* 2011 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
* with no warranties. This code is yours to share, use and modify with no
* strings attached and no restrictions or obligations.
* ============================================================================*/
#pragma once

#include <string>
#include <csignal>

namespace utxx {

namespace detail {
    /// Exit the app by rethrowing a fatal signal, previously caught.
    /// Do not use it elsewhere. It is triggered by the logger after flushing
    /// all queued messages.
    void exit_with_default_sighandler(int a_signo);
} // namespace detail


/// Install signal handler that catches FATAL C-runtime or OS signals
/// SIGABRT  ABORT (ANSI), abnormal termination
/// SIGFPE   Floating point exception (ANSI): http://en.wikipedia.org/wiki/SIGFPE
/// SIGILL   ILlegal instruction (ANSI)
/// SIGSEGV  Segmentation violation i.e. illegal memory reference
/// SIGTERM  TERMINATION (ANSI)
void install_sighandler();

} // namespace utxx::logger