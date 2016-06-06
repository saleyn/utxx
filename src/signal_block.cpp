//----------------------------------------------------------------------------
/// \file   signal_block.cpp
/// \author Serge ALeynikov <saleyn@gmail.com>
//----------------------------------------------------------------------------
/// \brief Signal blocking class.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2014 Serge Aleynikov

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

#include <utxx/error.hpp>
#include <utxx/string.hpp>
#include <utxx/signal_block.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string.h>
#include <vector>

using namespace std;

namespace utxx {

namespace {
    const char** sig_names_init() {
        static const char* s_signames[66]; // Last entry will be NULL
        for (auto i = 0u; i < utxx::length(s_signames)-1; i++)
            s_signames[i] = "<UNDEFINED>";

        s_signames[1 ] = "SIGHUP"     ; s_signames[ 2] = "SIGINT"     ;
        s_signames[6 ] = "SIGABRT"    ; s_signames[ 7] = "SIGBUS"     ;
        s_signames[11] = "SIGSEGV"    ; s_signames[12] = "SIGUSR2"    ;
        s_signames[16] = "SIGSTKFLT"  ; s_signames[17] = "SIGCHLD"    ;
        s_signames[21] = "SIGTTIN"    ; s_signames[22] = "SIGTTOU"    ;
        s_signames[26] = "SIGVTALRM"  ; s_signames[27] = "SIGPROF"    ;
        s_signames[31] = "SIGSYS"     ; s_signames[34] = "SIGRTMIN"   ;
        s_signames[38] = "SIGRTMIN+4" ; s_signames[39] = "SIGRTMIN+5" ;
        s_signames[43] = "SIGRTMIN+9" ; s_signames[44] = "SIGRTMIN+10";
        s_signames[48] = "SIGRTMIN+14"; s_signames[49] = "SIGRTMIN+15";
        s_signames[53] = "SIGRTMAX-11"; s_signames[54] = "SIGRTMAX-10";
        s_signames[58] = "SIGRTMAX-6" ; s_signames[59] = "SIGRTMAX-5" ;
        s_signames[63] = "SIGRTMAX-1" ; s_signames[64] = "SIGRTMAX"   ;

        s_signames[ 3] = "SIGQUIT"    ; s_signames[ 4] = "SIGILL"     ;
        s_signames[ 8] = "SIGFPE"     ; s_signames[ 9] = "SIGKILL"    ;
        s_signames[13] = "SIGPIPE"    ; s_signames[14] = "SIGALRM"    ;
        s_signames[18] = "SIGCONT"    ; s_signames[19] = "SIGSTOP"    ;
        s_signames[23] = "SIGURG"     ; s_signames[24] = "SIGXCPU"    ;
        s_signames[28] = "SIGWINCH"   ; s_signames[29] = "SIGIO"      ;
        s_signames[35] = "SIGRTMIN+1" ; s_signames[36] = "SIGRTMIN+2" ;
        s_signames[40] = "SIGRTMIN+6" ; s_signames[41] = "SIGRTMIN+7" ;
        s_signames[45] = "SIGRTMIN+11"; s_signames[46] = "SIGRTMIN+12";
        s_signames[50] = "SIGRTMAX-14"; s_signames[51] = "SIGRTMAX-13";
        s_signames[55] = "SIGRTMAX-9" ; s_signames[56] = "SIGRTMAX-8" ;
        s_signames[60] = "SIGRTMAX-4" ; s_signames[61] = "SIGRTMAX-3" ;

        s_signames[ 5] = "SIGTRAP"    ;
        s_signames[10] = "SIGUSR1"    ;
        s_signames[15] = "SIGTERM"    ;
        s_signames[20] = "SIGTSTP"    ;
        s_signames[25] = "SIGXFSZ"    ;
        s_signames[30] = "SIGPWR"     ;
        s_signames[37] = "SIGRTMIN+3" ;
        s_signames[42] = "SIGRTMIN+8" ;
        s_signames[47] = "SIGRTMIN+13";
        s_signames[52] = "SIGRTMAX-12";
        s_signames[57] = "SIGRTMAX-7" ;
        s_signames[62] = "SIGRTMAX-2" ;
        return s_signames;
    }
}

//------------------------------------------------------------------------------
const char** sig_names() {
    static const char** s_signames = sig_names_init();
    return s_signames;
}

//------------------------------------------------------------------------------
const char* sig_name(int i) {
    return utxx::unlikely(i < 0 || i > 64) ? strsignal(i) : sig_names()[i];
}

//------------------------------------------------------------------------------
std::string sig_members(const sigset_t& a_set)
{
    std::stringstream s;
    int n = 0;
    for (int i=1; i < 64; ++i)
        if (sigismember(&a_set, i)) { if (n++) s << '|'; s << sig_name(i); }
    return s.str();
}

//------------------------------------------------------------------------------
sigset_t sig_members_parse(const string& a_signals, utxx::src_info&& a_si)
{
    vector<string> signals;
    auto s = boost::to_lower_copy(a_signals);
    boost::split(signals, s, boost::is_any_of(":;,| "), boost::token_compress_on);
    sigset_t res;
    sigemptyset(&res);

    for (auto& s : signals) {
        if (s.empty()) continue;
        bool found = false;
        for (int i=1; i < 64; ++i) {
            auto signame = sig_name(i);
            if ((strcasecmp(s.c_str(), signame) == 0)
                ||  (s.size() > 3 && s.substr(0,3) != "sig" &&
                    strcasecmp(("sig" + s).c_str(), signame) == 0))
                {
                    sigaddset(&res, i);
                    found = true;
                    break;
                }
            }
        if (!found)
            UTXX_SRC_THROW(utxx::runtime_error, std::move(a_si),
                           "Unrecognized signal name: ", s);
    }
    return res;
}

} // namespace utxx
