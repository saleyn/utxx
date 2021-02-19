//----------------------------------------------------------------------------
/// \file  path.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of general purpose functions for path manipulation.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-06
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

#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <utxx/path.ipp>
#include <utxx/error.hpp>
#include <string.h>

#if defined(_WIN32) || defined (_WIN64) || defined(_MSC_VER)
#include <fstream>
#endif

namespace utxx {

varbinds_t merge_vars(varbinds_t const& a_src, varbinds_t const& a_add, bool a_overwrite)
{
    auto res = a_src;

    for (auto& [k,v] : a_add)
        if (res.find(k) == res.end() || a_overwrite)
            res[k] = v;

   return res;
}

namespace path {

std::string home() {
    const char* env = getenv("HOME");
    if (env)
        return env;
    #if defined(_MSC_VER) || defined(_WIN32) || defined(__CYGWIN32__)
    env = getenv("USERPROFILE");
    if (env)
        return env;
    char const* c = getenv("HOMEDRIVE"), *home = getenv("HOMEPATH");
    return (!c || !home) ? "" : std::string(c) + home;
    #else
    return "";
    #endif
}

std::string temp_path(std::string const& a_filename) {
    #if defined(__windows__) || defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    auto    p = getenv("TEMP");
    if (!p) p = "";
    #else
    auto    p = P_tmpdir;
    #endif
    auto r = std::string(p);
    return (a_filename.empty()) ? r : r + slash_str() + a_filename;
}

namespace {
    const char* strip_ext(const char* b, const char* e, const std::string& a_ext, bool case_sense) {
        auto n = a_ext.size();
        if (!a_ext.empty() && (e-b) >= int(n)) {
            auto q = e - n;
            if ((case_sense ? strncmp    (q, a_ext.c_str(), n)
                            : strncasecmp(q, a_ext.c_str(), n)) == 0)
                e = q;
        }
        return e;
    }
}

std::string basename(const std::string& a_file, const std::string& a_strip_ext, bool case_sense) {
    auto e = a_file.c_str() + a_file.size();
    auto p = basename(a_file.c_str(), e);
    e = strip_ext(p, e, a_strip_ext, case_sense);
    return std::string(p, e - p);
}

std::string extension(const std::string& a_file, const std::string& a_strip_ext, bool case_sense)
{
    auto p = a_file.c_str();
    auto e = p + a_file.size();
    e = strip_ext(p, e, a_strip_ext, case_sense);
    return boost::filesystem::extension(std::string(p, e - p));
}

std::string
replace_env_vars(const std::string& a_path, const struct tm* a_now,
                 const varbinds_t*  a_bindings)
{
    using namespace boost::posix_time;
    using namespace boost::xpressive;

    auto regex_format_fun = [a_bindings](const smatch& what) {
        if (what[1].str() == "EXEPATH") // special case
            return program::abs_path();

        if (a_bindings) {
            auto it = a_bindings->find(what[1].str());
            if (it != a_bindings->end())
                return it->second;
        }

        const char* env = getenv(what[1].str().c_str());
        return std::string(env ? env : "");
    };

    auto regex_home_var = [](const smatch& what) { return home(); };

    std::string x(a_path);
    {
        #if defined(_MSC_VER) || defined(_WIN32) || defined(__CYGWIN32__)
        sregex re = "%"  >> (s1 = +_w) >> '%';
        #else
        sregex re = "${" >> (s1 = +_w) >> '}';
        #endif

        x = regex_replace(x, re, regex_format_fun);
    }
    #if !defined(_MSC_VER) && !defined(_WIN32) && !defined(__CYGWIN32__)
    {
        sregex re = '$' >> (s1 = +_w);
        x = regex_replace(x, re, regex_format_fun);
    }
    {
        sregex re = bos >> '~';
        x = regex_replace(x, re, regex_home_var);
    }
    #endif

    if (a_now != NULL && x.find('%') != std::string::npos) {
        char buf[384];
        if (strftime(buf, sizeof(buf), x.c_str(), a_now) == 0)
            throw badarg_error("Invalid time specification!");
        return buf;
    }
    return x;
}

std::string
replace_macros(const std::string& a_path,
               const varbinds_t&  a_bindings)
{
    using namespace boost::posix_time;
    using namespace boost::xpressive;

    auto regex_format_fun = [&](const smatch& what) {
        auto it = a_bindings.find(what[1].str());
        if (it != a_bindings.end())
            return it->second;
        return std::string();
    };

    std::string x(a_path);
    //sregex re = "{{" >> (s1 = +_w) >> "}}";
    sregex re = sregex::compile( "\\{\\{([\\w-_]+)\\}\\}");
    return regex_replace(x, re, regex_format_fun);
}

std::pair<std::string, std::string>
filename_with_backup(const char* a_filename,
    const char* a_backup_dir, const char* a_backup_suffix, const struct tm* a_now)
{
    //boost::algorithm::replace_all(l_filename, std::string(l_slash), "-");
    std::string l_filename = replace_env_vars(a_filename, a_now);
    std::string l_backup(l_filename);
    size_t slash_pos = l_backup.rfind(slash());
    size_t dot_pos   = l_backup.rfind('.');
    std::string l_ext;

    if (dot_pos != std::string::npos &&
        (slash_pos == std::string::npos || dot_pos > slash_pos)) {
        l_ext = l_backup.substr(dot_pos);
        l_backup.erase(dot_pos);
    }

    using namespace boost::posix_time;
    struct tm  tm;
    const struct tm* ptm = &tm;
    if (a_now != NULL) {
        ptm = a_now;
    } else {
        ptime now = second_clock::local_time();
        tm = boost::posix_time::to_tm(now);
    }

    int n;
    char buf[384];
    if (a_backup_dir == NULL)
        n = snprintf(buf, sizeof(buf), "%s", l_backup.c_str());
    else
        n = snprintf(buf, sizeof(buf), "%s%c%s",
            a_backup_dir, slash(), l_backup.c_str());
    if (a_backup_suffix != NULL)
        snprintf(buf+n, sizeof(buf)-n, "%s.%s", l_ext.c_str(), a_backup_suffix);
    else
        snprintf(buf+n, sizeof(buf)-n, "@%04d-%02d-%02d.%02d%02d%02d%s",
            ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec, l_ext.c_str());
    return std::make_pair(l_filename, buf);
}

program::program()
{
#ifdef __linux__
    m_exe      = __progname;
    m_rel_path = __progname_full;
    size_t n   = m_rel_path.rfind('/');
    if (n != std::string::npos)
        m_rel_path.erase(n);
    char exe[1024];
    n = readlink("/proc/self/exe", exe, sizeof(exe)-1);
    m_abs_path = std::string(exe, n);

#elif defined(_MSC_VER) || defined(_WIN32) || defined(__CYGWIN32__)
    EXTERN_C IMAGE_DOS_HEADER __ImageBase;
    TCHAR str_dll_path[MAX_PATH];
    //::GetModuleFileName((HINSTANCE)&__ImageBase, str_dll_path, sizeof(str_dll_path));

    // HWND h = ???
    DWORD dwPid=0;
    ::GetWindowThreadProcessId((HWND)&__ImageBase, &dwPid);
    HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS ,0,dwPid);
    if ( hProcess == 0 )
        throw std::runtime_error("Cannot open process in order to get executable path!");

    typedef DWORD (WINAPI *tGetModuleFileNameExA)(  HANDLE, HMODULE,LPTSTR,DWORD);
    tGetModuleFileNameExA pGetModuleFileNameExA =0;

    boost::scoped_ptr<HINSTANCE>  hIns(::LoadLibrary("Psapi.dll"), &::FreeLibrary);

    if (hIns)
        pGetModuleFileNameExA =
            (tGetModuleFileNameExA)::GetProcAddress(hIns.get(),"GetModuleFileNameExA");

    if(pGetModuleFileNameExA)
        pGetModuleFileNameExA(hProcess, 0, str_dll_path, sizeof(str_dll_path));

    m_abs_path = str_dll_path;
    m_exe = m_abs_path;
    size_t n = m_abs_path.rfind('\\');
    if (n != std::string::npos) {
        m_abs_path.erase(n);
        m_exe.erase(0, n+1);
    } else {
        m_abs_path = "";
    }
    m_rel_path = m_abs_path;
#endif
    m_fullname = m_abs_path;
    n = m_abs_path.rfind('/');
    if (n != std::string::npos)
        m_abs_path.erase(n);
}

} // namespace path
} // namespace utxx

