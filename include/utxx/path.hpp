//----------------------------------------------------------------------------
/// \file  path.hpp
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
#ifndef _UTXX_PATH_HPP_
#define _UTXX_PATH_HPP_

#include <string>
#include <list>
#include <fstream>
#include <utxx/error.hpp>
#include <utxx/time_val.hpp>
#include <unistd.h>

namespace utxx {

enum class FileMatchT {
    REGEX,
    PREFIX,
    WILDCARD
};

namespace path {

/// Return a platform-specific slash path separator character
static inline char slash() {
    #if defined(__windows__) || defined(_WIN32) || defined(_WIN64)
    return '\\';
    #else
    return '/';
    #endif
}

/// Return a platform-specific slash path separator character (as string)
static inline const char* slash_str() {
    #if defined(__windows__) || defined(_WIN32) || defined(_WIN64)
    return "\\";
    #else
    return "/";
    #endif
}


/// Return basename of the filename defined between \a begin and \a end
/// arguments.
/// @param begin is the start of the filename
/// @param end is the end of the filename
const char* basename(const char* begin, const char* end);

/// Return basename of the filename (no directory name)
/// @param a_file is the filename to get the basename of
/// @param a_strip_ext when not empty, the basename's extension matching
///                    \a a_strip_ext (taken verbatim) will be erased
std::string basename(const std::string& a_file, const std::string& a_strip_ext = "");

/// Return dirname portion of the filename
std::string dirname(const std::string& a_filename);

/// Checks if a file exists
bool        file_exists(const char* a_path);
inline bool file_exists(const std::string& a) { return file_exists(a.c_str()); }

/// Check if given path is a symlink
bool        is_symlink (const char* a_path);
inline bool is_symlink (const std::string& a) { return is_symlink(a.c_str());  }

/// Check if given \a a_path is a regular file
bool        is_regular (const std::string& a_path);

/// Check if given \a a_path is a directory
bool        is_dir     (const std::string& a_path);

/// Get the name of the file pointed by \a a_symlink
/// @param a_symlink name of a symlink
/// @return filename name pointed by \a a_symlink; empty string if
///         \a a_symlink is not a symlink or there is some other error.
std::string file_readlink(const std::string& a_symlink);

/// Get file size
long        file_size(const char* a_filename);
inline long file_size(const std::string& a_filename) { return file_size(a_filename.c_str()); }
/// Get file size of the file associated with the file descriptor
long        file_size(int fd);

/// Create a symlink to file
inline bool file_symlink(const std::string& a_file, const std::string& a_symlink) {
    return ::symlink(a_file.c_str(), a_symlink.c_str()) == 0;
}

/// Removes a file
bool        file_unlink(const char* a_path);
inline bool file_unlink(const std::string& a_path) { return file_unlink(a_path.c_str()); }

/// Renames a file
inline bool file_rename(const std::string& a_from, const std::string& a_to) {
    return ::rename(a_from.c_str(), a_to.c_str()) == 0;
}

/// Get current working directory
inline std::string curdir() { char buf[512]; getcwd(buf,sizeof(buf)); return buf; }

/// Read the entire content of given file to string
inline const std::string read_file(std::ifstream& a_in) {
    return static_cast<std::stringstream const&>(std::stringstream() << a_in.rdbuf()).str();
}

/// Read the entire content of given file to string
inline const std::string read_file(const std::string& a_filename) {
    std::ifstream in(a_filename);
    if (!in) UTXX_THROW_IO_ERROR(errno, "Unable to open file: ", a_filename);
    return read_file(in);
}

/// Write string to file
inline bool write_file(const std::string& a_file, const std::string& a_data, bool a_append=false) {
    std::ofstream out(a_file, a_append ? std::ios_base::app : std::ios_base::trunc);
    if (!out) return false;
    out << a_data;
    return true;
}
inline bool write_file(const std::string& a_file, std::string&& a_data, bool a_append=false) {
    std::ofstream out(a_file, a_append ? std::ios_base::app : std::ios_base::trunc);
    out << a_data;
    return true;
}

/// Split \a a_path to directory and filename
inline std::pair<std::string, std::string> split(std::string const& a_path) {
    auto found = a_path.find_last_of(slash());
    return found == std::string::npos
         ? std::make_pair("", a_path)
         : std::make_pair(a_path.substr(0, found), a_path.substr(found+1));
}

/// Join \a a_dir and \a a_file
inline std::string join(std::string const& a_dir, std::string const& a_file) {
    return a_dir.empty()                    ? a_file
         : a_dir[a_dir.size()-1] == slash() ? a_dir + a_file
         : a_dir + slash_str() + a_file;
}

/// List files in a directory
/// @param a_dir directory to check
/// @param a_filter is a regular expression/prefix/wildcard used to match filename
/// @param a_match_type type of matching to perform on filenames
std::pair<bool, std::list<std::string>>
list_files(std::string const& a_dir, std::string const& a_filter = "",
           FileMatchT a_match_type = FileMatchT::WILDCARD);

std::pair<bool, std::list<std::string>>
inline list_files(std::string const& a_dir_with_file_mask,
                  FileMatchT a_match_type = FileMatchT::WILDCARD) {
    auto   res = split(a_dir_with_file_mask);
    return list_files(res.first, res.second, a_match_type);
}

/// Return portable value of the home path
std::string home();

/// @brief Substitute all environment variables and day-time
/// formatting symbols (see strptime(3)) in a given filename.
/// The variables can be represented by "%VARNAME%" notation
/// on Windows and "${VARNAME}" notation on Unix. The function
/// also recognizes Unix-specific variables in the form
/// "$VARNAME", replaces leading '~' with value of "$HOME" directory
/// and replaces special variable "${EXEPATH}" with the
/// absolute path to the executable program.
/// @param a_now      when it is not null, the function will also substitute
///                   time in the a_path using strftime(3).
/// @param a_bindings contains optional value map that will be used
///                   for variable substitution prior to using environment
///                   variables.
std::string replace_env_vars(
    const std::string& a_path, const struct tm* a_now = NULL,
    const std::map<std::string, std::string>* a_bindings = NULL);

/// Replace environment variables and time in strptime(3) format in \a a_str.
/// @param a_str string to replace
/// @param a_now now time (if contains time_val(), no strftime(3) call is made)
/// @param a_utc treat \a a_now as UTC time instead of local time
/// @param a_bindings contains optional value map that will be used
///                   for variable substitution prior to using environment
///                   variables.
std::string replace_env_vars(
    const std::string& a_str, time_val a_now, bool a_utc = false,
    const std::map<std::string, std::string>* a_bindings = NULL);

/// Replace macro variables found in the \a a_bindings.
/// The values should be defined in the string in the following form:
/// "My name is {{name}}"
/// @param a_str string to replace containing macros surrounded by "{{" and "}}"
/// @param a_bindings contains optional value map that will be used
///                   for variable substitution.
std::string replace_macros(
    const std::string& a_path,
    const std::map<std::string, std::string>& a_bindings);

/// Returns a pair containing a file name with substituted
/// environment variables and day-time formatting symbols
/// replaced (see strptime(3)), and a backup file name to
/// be used in case the filename exists in the filesystem.
/// The backup name is optionally prefixes with a \a
/// a_backup_dir directory and suffixed with \a a_backup_suffix.
/// If the later is not provided the "@YYYY-MM-DD.hhmmss"
/// string will be inserted between the base of the
/// filename and file extension.
/// @param a_filename is the name of the name of the file
///          with substitution formatting macros.
/// @param a_backup_dir is the name of the backup directory
///          to use as the leading part of the second value
///          in the returned pair.
/// @param a_backup_suffix is the suffix to assign to the
///          BackupName value instead of "@YYYY-MM-DD.hhmmss"
/// @param a_now is the timestamp to use for BackupName. If
///          it is NULL, the local system time will be used.
/// @return a pair <Filename, BackupName>.
std::pair<std::string, std::string>
filename_with_backup(const char* a_filename,
    const char* a_backup_dir = NULL, const char* a_backup_suffix = NULL,
    const struct tm* a_now = NULL);

extern "C" {
    extern const char* __progname;
    extern const char* __progname_full;
}

/// Return the short name, full name, or full pathname of current program.
class program {
    std::string m_exe;
    std::string m_rel_path;
    std::string m_abs_path;

    program();

    static program& instance() {
        static program s_instance;
        return s_instance;
    }

public:
    /// Return name of current program
    static const std::string& name()        { return instance().m_exe; }
    /// Return relative path of current program
    static const std::string& rel_path()    { return instance().m_rel_path; }
    /// Return name of current program
    static const std::string& abs_path()    { return instance().m_abs_path; }
};

} // namespace path
} // namespace utxx

#include <utxx/path.ipp>
#endif // _UTXX_PATH_HPP_