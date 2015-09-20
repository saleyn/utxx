//----------------------------------------------------------------------------
/// \file  test_helper.hpp
//----------------------------------------------------------------------------
/// \brief Unit test helping routines.
//----------------------------------------------------------------------------
// Author:  Serge Aleynikov
// Created: 2010-01-06
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

#include <boost/test/unit_test.hpp>

#define BOOST_CURRENT_TEST_NAME \
  boost::unit_test::framework::current_test_case().p_name->c_str()

#define UTXX_REQUIRE_NO_THROW(Expr)     \
    try { Expr; }                       \
    catch (std::exception const& e) {   \
      BOOST_TEST_MESSAGE(e.what());     \
      BOOST_REQUIRE_NO_THROW(Expr);     \
    }

#define UTXX_CHECK_NO_THROW(Expr)       \
    try { Expr; }                       \
    catch (std::exception const& e) {   \
      BOOST_TEST_MESSAGE(e.what());     \
      BOOST_CHECK_NO_THROW(Expr);       \
    }

namespace utxx {

inline long env(const char* a_var, long a_default) {
    const char* p = getenv(a_var);
    return p ? atoi(p) : a_default;
}

inline bool get_test_argv(const std::string& a_opt,
                          const std::string& a_long_opt = "") {
    int    argc = boost::unit_test::framework::master_test_suite().argc;
    char** argv = boost::unit_test::framework::master_test_suite().argv;

    if (a_opt.empty() && a_long_opt.empty()) return false;

    auto same = [=](const std::string& a, int i) {
        size_t n = strlen(argv[i]);
        return !a.empty() &&
               (a == argv[i] || (a.size()+1 <= n &&
                                 strncmp(a.c_str(), argv[i], a.size()) == 0 &&
                                 argv[i][a.size()] == '='));
    };

    for (int i=1; i < argc; i++) {
        if (same(a_opt,      i)) return true;
        if (same(a_long_opt, i)) return true;
    }

    return false;
}

inline bool get_test_argv(const std::string& a_opt,
                          const std::string& a_long_opt,
                          std::string& a_value) {
    int    argc = boost::unit_test::framework::master_test_suite().argc;
    char** argv = boost::unit_test::framework::master_test_suite().argv;

    if (a_opt.empty() && a_long_opt.empty()) return false;

    auto check = [=, &a_value](const std::string& a, int& i) {
        if (a.empty()) return false;
        if (a == argv[i]) { a_value = argv[++i]; return true; }
        size_t n = strlen(argv[i]);
        if (a.size()+1 <= n && strncmp(a.c_str(), argv[i], a.size()) == 0 &&
                               argv[i][a.size()] == '=') {
            a_value = argv[i] + a.size()+1;
            return true;
        }
        return false;
    };

    for (int i=1; i < argc; i++) {
        if (check(a_opt,      i)) return true;
        if (check(a_long_opt, i)) return true;
    }

    return false;
}

inline bool get_test_argv(const std::string& a_opt,
                          const std::string& a_long_opt,
                          int& a_value) {
    std::string s;
    if (!get_test_argv(a_opt, a_long_opt, s))
        return false;
    a_value = std::stoi(s);
    return true;
}

inline bool get_test_argv(const std::string& a_opt,
                          const std::string& a_long_opt,
                          bool& a_value) {
    std::string s;
    if (!get_test_argv(a_opt, a_long_opt, s))
        return false;
    a_value = s == "true" || s == "1" ||
             (s.length() > 0 && (s[0] == 'y' || s[0] == 'Y' || s[0] == 'T'));
    return true;
}

/// Prevent variable optimization by the compiler
#ifdef _MSC_VER
#pragma optimize("", off)
template <class T> void dont_optimize_var(T&& v) { v = v; }
#pragma optimize("", on)
#else
template <class T> void dont_optimize_var(T&& v) { asm volatile("":"+r" (v)); }
#endif

} // namespace utxx::test
