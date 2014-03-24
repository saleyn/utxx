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

namespace utxx {
namespace path {

/**
 * Return a platform-specific slash character used as
 * path separator.
 */
static inline char slash() {
    #if defined(__windows__) || defined(_WIN32) || defined(_WIN64)
    return '\\';
    #else
    return '/';
    #endif
}

/**
 * Return basename of the filename defined between \a begin and \a end
 * arguments.
 * @param begin is the start of the filename
 * @param end is the end of the filename
 */
const char* basename(const char* begin, const char* end);

/**
 * Checks if a file exists
 */
bool file_exists(const char* a_path);

/**
 * Removes a file
 */
void file_unlink(const char* a_path);

/**
 * Return portable value of the home path
 */
std::string home();

/**
 * \brief Substitute all environment variables and day-time
 * formatting symbols (see strptime(3)) in a given filename.
 * The variables can be represented by "%VARNAME%" notation
 * on Windows and "${VARNAME}" notation on Unix. The function
 * also recognizes Unix-specific variables in the form
 * "$VARNAME", replaces leading '~' with value of "$HOME" directory
 * and replaces special variable "${EXEPATH}" with the
 * absolute path to the executable program.
 * If \a a_now is not null will also substitute time in the a_path
 * using strftime(3).
 */
std::string replace_env_vars(
    const std::string& a_path, const struct tm* a_now = NULL);

/**
 * Returns a pair containing a file name with substituted
 * environment variables and day-time formatting symbols
 * replaced (see strptime(3)), and a backup file name to
 * be used in case the filename exists in the filesystem.
 * The backup name is optionally prefixes with a \a
 * a_backup_dir directory and suffixed with \a a_backup_suffix.
 * If the later is not provided the "@YYYY-MM-DD.hhmmss"
 * string will be inserted between the base of the
 * filename and file extension.
 * @param a_filename is the name of the name of the file
 *          with substitution formatting macros.
 * @param a_backup_dir is the name of the backup directory
 *          to use as the leading part of the second value
 *          in the returned pair.
 * @param a_backup_suffix is the suffix to assign to the
 *          BackupName value instead of "@YYYY-MM-DD.hhmmss"
 * @param a_now is the timestamp to use for BackupName. If
 *          it is NULL, the local system time will be used.
 * @return a pair <Filename, BackupName>.
 */
std::pair<std::string, std::string>
filename_with_backup(const char* a_filename,
    const char* a_backup_dir = NULL, const char* a_backup_suffix = NULL,
    const struct tm* a_now = NULL);

extern "C" {
    extern const char* __progname;
    extern const char* __progname_full;
}

/**
 * Return the short name, full name, or full pathname of
 * current executable program.
 */
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

#endif // _UTXX_PATH_HPP_

