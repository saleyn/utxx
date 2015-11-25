//----------------------------------------------------------------------------
/// \file  logger_crash_handler.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of crash handler in the logging framework.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-04
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

/* ===========================================================================
* 2011 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
* with no warranties. This code is yours to share, use and modify with no
* strings attached and no restrictions or obligations.
* ============================================================================*/

#include <utxx/logger.hpp>
#include <utxx/logger/logger_crash_handler.hpp>
#include <utxx/signal_block.hpp>
#include <sys/syscall.h>
#include <csignal>
#include <cstring>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)

#include <process.h> // getpid
#define getpid _getpid

#else

#include <unistd.h>
#include <execinfo.h>

#include <cxxabi.h>
#include <cstdlib>

#if defined(__clang__) || defined(__APPLE__)
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif

#endif

// Redirecting and using signals. In case of fatal signals utxx logs the fatal
// signal and flushes the log queue and then "rethrow" the signal to exit
namespace utxx {

namespace {

    // Dump of stack,. then exit through g2log background worker
    // ALL thanks to this thread at StackOverflow. Pretty much borrowed from:
    // Ref: http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes
    void crash_handler(int a_signo, siginfo_t* a_info, void* a_context)
    {
        std::ostringstream oss;

    #if !(defined(WIN33) || defined(_WIN32) || defined(__WIN32__))
        const size_t max_dump_size = 50;
        void* dump[max_dump_size];
        size_t size = backtrace(dump, max_dump_size);
        char** msg  = backtrace_symbols(dump, size); // overwrite sigaction with caller's address

        oss << "Received fatal signal: " << sig_name(a_signo)
            << " (" << a_signo << ")\n"  << "\tPID: " << getpid()
            << "\tTID: "       << syscall(SYS_gettid) << std::endl;

        // dump stack: skip first frame, since that is here
        for(size_t idx = 1; idx < size && msg != nullptr; ++idx)
        {
            char *mangled_name = 0, *offset_begin = 0, *offset_end = 0;
            // find parantheses and +address offset surrounding mangled name
            for (char *p = msg[idx]; *p; ++p) {
                switch (*p) {
                    case '(': mangled_name = p; break;
                    case '+': offset_begin = p; break;
                    case ')': offset_end   = p; goto DONE;
                    default : continue;
                }
            }

        DONE:
            // if the line could be processed, attempt to demangle the symbol
            if (mangled_name && offset_begin && offset_end &&
                mangled_name < offset_begin)
            {
                *mangled_name++ = '\0';
                *offset_begin++ = '\0';
                *offset_end++   = '\0';

                int status;
                char * real_name = abi::__cxa_demangle(mangled_name, 0, 0, &status);
                // if demangling is successful, output the demangled function name
                if (status == 0)
                    oss << "\tstack dump [" << idx << "]  " << msg[idx]   << " : "
                        << real_name << "+" << offset_begin << offset_end << '\n';
                else
                    // otherwise, output the mangled function name
                    oss << "\tstack dump ["    << idx << "]  " << msg[idx]
                        << mangled_name << "+" << offset_begin << offset_end << '\n';
                free(real_name); // mallocated by abi::__cxa_demangle(...)
            }
            else
                // no demangling done -- just dump the whole line
                oss << "\tstack dump [" << idx << "]  " << msg[idx] << std::endl;
        } // END: for(size_t idx = 1; idx < size && messages != nullptr; ++idx)

        free(msg);
    #endif // not Windows

        UTXX_LOG_FATAL(
            "\n\n***** FATAL TRIGGER RECEIVED ******* \n"
            "%s\n\n***** RETHROWING SIGNAL %s (%d)\n",
            oss.str().c_str(), sig_name(a_signo), a_signo);
    }

} // end anonymous namespace

// References:
// sigaction : change the default action if a specific signal is received
//  http://linux.die.net/man/2/sigaction
//  http://publib.boulder.ibm.com/infocenter/aix/v6r1/index.jsp?topic=%2Fcom.ibm.aix.basetechref%2Fdoc%2Fbasetrf2%2Fsigaction.html
//
// signal:
//  http://linux.die.net/man/7/signal and
//  http://msdn.microsoft.com/en-us/library/xdkz3x12%28vs.71%29.asp
//  http://stackoverflow.com/questions/6878546/why-doesnt-parent-process-return-to-the-exact-location-after-handling-signal_number
namespace detail {

    // Triggered by g2log->g2LogWorker after receiving a FATAL trigger
    // which is LOG(FATAL), CHECK(false) or a fatal signal our signalhandler caught.
    // --- If LOG(FATAL) or CHECK(false) the signal_number will be SIGABRT
    void exit_with_default_sighandler(int a_signo)
    {
        // Restore our signalhandling to default
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
        if(SIG_ERR == signal(SIGABRT, SIG_DFL)) perror("signal - SIGABRT");
        if(SIG_ERR == signal(SIGFPE,  SIG_DFL)) perror("signal - SIGABRT");
        if(SIG_ERR == signal(SIGSEGV, SIG_DFL)) perror("signal - SIGABRT");
        if(SIG_ERR == signal(SIGILL,  SIG_DFL)) perror("signal - SIGABRT");
        if(SIG_ERR == signal(SIGTERM, SIG_DFL)) perror("signal - SIGABRT");

        raise(signal_number);
    #else
        std::cerr << "Exiting - FATAL SIGNAL: " << a_signo << "   " << std::flush;
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        sigemptyset(&action.sa_mask);
        action.sa_handler = SIG_DFL; // take default action for the signal
        sigaction(a_signo, &action, NULL);
        kill(getpid(), a_signo);
        abort(); // should never reach this
    #endif
    }

} // namespace detail


bool install_sighandler(bool a_install, const sigset_t* a_signals)
{
    static std::atomic<bool> s_installed;

    // Don't install anything if the handlers are already installed
    if (!a_install || s_installed.exchange(true))
        return false;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    if (SIG_ERR == signal(SIGABRT, crash_handler)) perror("signal - SIGABRT");
    if (SIG_ERR == signal(SIGFPE,  crash_handler)) perror("signal - SIGFPE");
    if (SIG_ERR == signal(SIGSEGV, crash_handler)) perror("signal - SIGSEGV");
    if (SIG_ERR == signal(SIGILL,  crash_handler)) perror("signal - SIGILL");
    if (SIG_ERR == signal(SIGTERM, crash_handler)) perror("signal - SIGTERM");
#else
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = &crash_handler;  // callback for fatal signals
    // sigaction to use sa_sigaction file. ref:
    // http://www.linuxprogrammingblog.com/code-examples/sigaction
    action.sa_flags = SA_SIGINFO;

    // do it verbose style - install all signal actions
    static const sigset_t s_default =
        sig_init_set(SIGABRT, SIGFPE, SIGILL, SIGSEGV, SIGTERM);

    if (!a_signals)
        a_signals = &s_default;

    if (sigisemptyset(a_signals))
        return false;

    for (uint i=1; i < sig_names_count(); ++i)
        if (sigismember(a_signals, i) && sigaction(i, &action, nullptr) < 0)
            UTXX_THROW_IO_ERROR(errno, "Error in sigaction - ", sig_name(i));

    return true;
#endif
}

} // namespace utxx
