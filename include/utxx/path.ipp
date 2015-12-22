//----------------------------------------------------------------------------
/// \file  path.ipp
//----------------------------------------------------------------------------
/// \brief Collection of general purpose functions for path manipulation.
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
#pragma once

#include <utxx/path.hpp>
#include <sys/stat.h>
#include <assert.h>

namespace utxx {
namespace path {

inline const char* basename(const char* a_begin, const char* a_end) {
    assert(a_begin <= a_end);
    auto p = (const char*)memrchr(a_begin, slash(), a_end - a_begin);
    return p ? p+1 : a_begin;
}

inline std::string dirname(const std::string& a_filename) {
    auto n = a_filename.find_last_of(slash());
    return n == std::string::npos ? "" : a_filename.substr(0, n);
}

inline bool is_symlink(const char* a_path) {
    struct stat s;
    return lstat(a_path, &s) == 0 && S_ISLNK(s.st_mode);
}

inline bool is_regular(const std::string& a_path) {
    struct stat s;
    return lstat(a_path.c_str(), &s) == 0 && S_ISREG(s.st_mode);
}

inline bool is_dir(const std::string& a_path) {
    struct stat s;
    return lstat(a_path.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
}

inline std::string
replace_env_vars(const std::string& a_path, time_val a_now, bool a_utc,
                 const std::map<std::string, std::string>*  a_bindings)
{
    if (!a_now.empty()) {
        auto tm = a_now.to_tm(a_utc);
        return replace_env_vars(a_path, &tm, a_bindings);
    }
    return replace_env_vars(a_path, nullptr, a_bindings);
}

inline bool file_unlink(const char* a_path) {
    #if defined(_MSC_VER) || defined(_WIN32) || defined(__CYGWIN32__)
    return ::_unlink(a_path) == 0;
    #else
    return ::unlink(a_path) == 0;
    #endif
}

inline std::string file_readlink(const std::string& a_symlink) {
    char buf[256];
    int  n = ::readlink(a_symlink.c_str(), buf, sizeof(buf));
    return std::string(buf, n < 0 ? 0 : n);
}

inline bool file_symlink(const std::string& a_file, const std::string& a_link, bool a_verify) {

    if (a_verify) {
        if (a_link.empty() || !path::file_exists(a_file))
            return false;

        if (a_link == a_file)
            return true;

        // Does a file by name a_link exist and it's not a symlink?
        if (file_exists(a_link)) {
            if (!is_symlink(a_link)) {
                auto new_name = a_link + ".tmp";
                if (file_exists(new_name) && !file_unlink(new_name))
                    return false;
                if (!file_rename(a_link, new_name))
                    return false;
            } else {
                // Check if the symlink already points to existing file named a_file
                auto s = file_readlink(a_link);
                if (s == a_file)
                    return true;
                utxx::path::file_unlink(a_link);
            }
        }

    }

    return ::symlink(a_file.c_str(), a_link.c_str()) == 0;
}

inline long file_size(const char* a_filename) {
    struct stat stat_buf;
    int rc = stat(a_filename, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

inline long file_size(int fd) {
    struct stat stat_buf;
    int rc = fstat(fd, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

inline int file_exists(const char* a_path) {
    #if defined(_MSC_VER) || defined(_WIN32) || defined(__CYGWIN32__)
    std::ifstream l_stream;
    l_stream.open(a_path, std::ios_base::in);
    if(l_stream.is_open()) {
        l_stream.close();
        return true;
    }
    return false;
    #else
    struct stat buf;
    return ::lstat(a_path, &buf) < 0 ? 0 : buf.st_mode;
    #endif
}

inline bool create_directories(const std::string& a_path, int a_access) {
    if (a_path.empty())
        return false;

    std::string s(a_path);
    if (s[s.size()-1] == '/')
        s.erase(s.size()-1);

    size_t n = 0, i;
    while ((i = s.find('/', n)) != std::string::npos) {
        n = i+1;
        if (i == 0)
            continue;
        auto dir  = s.substr(0, i);
        auto mode = file_exists(dir.c_str());
        if (S_ISDIR(mode))
            continue;
        else if (S_ISREG(mode))
            return false;
        else if (!mode && mkdir(dir.c_str(), a_access) < 0)
            return false;
    }

    return file_exists(s) || mkdir(s.c_str(), a_access) >= 0;
}

inline std::string username() {
    char buf[L_cuserid];
    return unlikely(cuserid(buf) == nullptr) ? buf : "";
}

} // namespace path
} // namespace utxx