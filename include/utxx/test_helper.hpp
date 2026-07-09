//----------------------------------------------------------------------------
/// \file  test_helper.hpp
//----------------------------------------------------------------------------
/// \brief Unit test helping routines — standalone harness, no Boost required.
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

#include <utxx/config.h>
#include <utxx/get_option.hpp>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_harness {

struct test_abort {};  // thrown by REQUIRE macros to stop the current test case

// Helper: compare two values, treating char* as C-strings.
// Handles CHECK_EQUAL("abc", obj) where obj.operator==(const char*) is preferred
// over pointer comparison via implicit conversion.
namespace detail {
    // is_char_ptr: true when T is char* or const char* (but NOT a char array)
    template <typename T> struct is_char_ptr : std::false_type {};
    template <> struct is_char_ptr<char*>       : std::true_type {};
    template <> struct is_char_ptr<const char*> : std::true_type {};

    // When A is a char pointer/array and B is not, call b == a to use B's operator==
    template <typename A, typename B>
    inline bool test_eq_impl(const A& a, const B& b, std::true_type /*A is char-like*/,
                                                      std::false_type /*B is not*/) {
        return b == a; // call B's operator==(const char*) or char[]
    }
    // Otherwise compare a == b
    template <typename A, typename B>
    inline bool test_eq_impl(const A& a, const B& b, ...) {
        using CA = typename std::common_type<A, B>::type;
        return (CA)a == (CA)b;
    }
}
template <typename A, typename B>
inline bool test_eq(const A& a, const B& b) {
    using DA = typename std::decay<A>::type;
    using DB = typename std::decay<B>::type;
    using A_is_char = std::integral_constant<bool,
        std::is_same<DA, const char*>::value || std::is_same<DA, char*>::value ||
        (std::is_array<A>::value && std::is_same<typename std::remove_extent<A>::type, char>::value) ||
        (std::is_array<A>::value && std::is_same<typename std::remove_extent<A>::type, const char>::value)>;
    using B_is_char = std::integral_constant<bool,
        std::is_same<DB, const char*>::value || std::is_same<DB, char*>::value ||
        (std::is_array<B>::value && std::is_same<typename std::remove_extent<B>::type, char>::value) ||
        (std::is_array<B>::value && std::is_same<typename std::remove_extent<B>::type, const char>::value)>;
    return detail::test_eq_impl(a, b, A_is_char{}, B_is_char{});
}
inline bool test_eq(const char* a, const char* b) {
    return a && b ? std::strcmp(a, b) == 0 : a == b;
}
inline bool test_eq(char* a, const char* b) { return test_eq((const char*)a, b); }
inline bool test_eq(const char* a, char* b) { return test_eq(a, (const char*)b); }
inline bool test_eq(char* a, char* b) { return test_eq((const char*)a, (const char*)b); }

inline int  s_failures    = 0;
inline int  s_passed      = 0;
inline int  s_argc        = 0;
inline char** s_argv      = nullptr;
inline const char* s_current_test = "";

inline const char* current_test_name() { return s_current_test; }

struct test_case {
    const char*           name;
    std::function<void()> fn;
};

inline std::vector<test_case>& registry() {
    static std::vector<test_case> s;
    return s;
}

struct registrar {
    registrar(const char* name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

inline int run_all(int argc = 0, char** argv = nullptr) {
    s_argc     = argc;
    s_argv     = argv;
    s_failures = 0;
    s_passed   = 0;

    for (auto& tc : registry()) {
        s_current_test = tc.name;
        printf("Running %s\n", tc.name);
        try {
            tc.fn();
        } catch (const test_abort&) {
            // already counted via s_failures
        } catch (const std::exception& e) {
            fprintf(stderr, "  EXCEPTION in %s: %s\n", tc.name, e.what());
            ++s_failures;
        } catch (...) {
            fprintf(stderr, "  UNKNOWN EXCEPTION in %s\n", tc.name);
            ++s_failures;
        }
    }

    printf("\n%d passed, %d failed\n", s_passed, s_failures);
    return s_failures ? 1 : 0;
}

} // namespace test_harness

// ─── CHECK (non-fatal) macros ────────────────────────────────────────────────

#define CHECK(expr) \
    do { if (expr) { ++test_harness::s_passed; } \
         else { \
             fprintf(stderr, "  FAIL %s:%d: CHECK(%s)\n", __FILE__, __LINE__, #expr); \
             ++test_harness::s_failures; \
         } \
    } while(0)

#define CHECK_EQUAL(a, b) \
    do { auto _a = (a); auto _b = (b); \
         if (test_harness::test_eq(_a, _b)) { ++test_harness::s_passed; } \
         else { \
             std::ostringstream _s; \
             _s << "  FAIL " << __FILE__ << ":" << __LINE__ \
                << ": CHECK_EQUAL(" #a ", " #b "): " << _a << " != " << _b; \
             fprintf(stderr, "%s\n", _s.str().c_str()); \
             ++test_harness::s_failures; \
        } \
    } while(0)

#define CHECK_NE(a, b) \
    do { auto _a = (a); auto _b = (b); \
         if (!test_harness::test_eq(_a, _b)) { ++test_harness::s_passed; } \
         else { \
             std::ostringstream _s; \
             _s << "  FAIL " << __FILE__ << ":" << __LINE__ \
                << ": CHECK_NE(" #a ", " #b "): " << _a << " == " << _b; \
             fprintf(stderr, "%s\n", _s.str().c_str()); \
             ++test_harness::s_failures; \
        } \
    } while(0)

#define CHECK_THROW(expr, ExcType) \
    do { bool _threw = false; \
         try { expr; } \
         catch (const ExcType&) { _threw = true; } \
         catch (...) {} \
         if (_threw) { ++test_harness::s_passed; } \
         else { \
             fprintf(stderr, "  FAIL %s:%d: CHECK_THROW(%s, %s) — did not throw\n", \
                     __FILE__, __LINE__, #expr, #ExcType); \
             ++test_harness::s_failures; \
        } \
    } while(0)

#define CHECK_NO_THROW(expr) \
    do { bool _ok = true; std::string _msg; \
         try { (void)(expr); } \
         catch (const std::exception& e) { _ok = false; _msg = e.what(); } \
         catch (...) { _ok = false; _msg = "<unknown>"; } \
         if (_ok) { ++test_harness::s_passed; } \
         else { \
             fprintf(stderr, "  FAIL %s:%d: CHECK_NO_THROW(%s) threw: %s\n", \
                     __FILE__, __LINE__, #expr, _msg.c_str()); \
             ++test_harness::s_failures; \
         } \
    } while(0)

#define CHECK_EQUAL_COLLECTIONS(b1, e1, b2, e2) \
    do { auto _i1 = (b1); auto _e1 = (e1); auto _i2 = (b2); auto _e2 = (e2); \
         bool _ok = true; int _idx = 0; \
         for (; _i1 != _e1 && _i2 != _e2; ++_i1, ++_i2, ++_idx) { \
             if (!(*_i1 == *_i2)) { \
                 std::ostringstream _s; \
                 _s << "  FAIL " << __FILE__ << ":" << __LINE__ \
                    << ": collections differ at index " << _idx \
                    << ": " << *_i1 << " != " << *_i2; \
                 fprintf(stderr, "%s\n", _s.str().c_str()); \
                 ++test_harness::s_failures; _ok = false; break; } } \
         if (_ok && _i1 != _e1) { \
             fprintf(stderr, "  FAIL %s:%d: first collection is longer\n", \
                     __FILE__, __LINE__); \
             ++test_harness::s_failures; _ok = false; } \
         if (_ok && _i2 != _e2) { \
             fprintf(stderr, "  FAIL %s:%d: second collection is longer\n", \
                     __FILE__, __LINE__); \
             ++test_harness::s_failures; _ok = false; } \
         if (_ok) ++test_harness::s_passed; \
    } while(0)

#define TEST_MESSAGE(msg) \
    do { std::ostringstream _s; _s << msg; \
         fprintf(stdout, "  # %s\n", _s.str().c_str()); \
    } while(0)

#define TEST_FAIL(msg) \
    do { std::ostringstream _s; _s << msg; \
         fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, _s.str().c_str()); \
         ++test_harness::s_failures; \
    } while(0)

// ─── REQUIRE (fatal — throws test_abort) macros ──────────────────────────────

#define REQUIRE(expr) \
    do { if (expr) { ++test_harness::s_passed; } \
         else { \
             fprintf(stderr, "  FAIL %s:%d: REQUIRE(%s)\n", __FILE__, __LINE__, #expr); \
             ++test_harness::s_failures; throw test_harness::test_abort{}; \
         } \
    } while(0)

#define REQUIRE_EQUAL(a, b) \
    do { auto _a = (a); auto _b = (b); \
         if (test_harness::test_eq(_a, _b)) { ++test_harness::s_passed; } \
         else { \
             std::ostringstream _s; \
             _s << "  FAIL " << __FILE__ << ":" << __LINE__ \
                << ": REQUIRE_EQUAL(" #a ", " #b "): " << _a << " != " << _b; \
             fprintf(stderr, "%s\n", _s.str().c_str()); \
             ++test_harness::s_failures; throw test_harness::test_abort{}; \
        } \
    } while(0)

#define REQUIRE_THROW(expr, ExcType) \
    do { bool _threw = false; \
         try { expr; } \
         catch (const ExcType&) { _threw = true; } \
         catch (...) {} \
         if (_threw) { ++test_harness::s_passed; } \
         else { \
             fprintf(stderr, "  FAIL %s:%d: REQUIRE_THROW(%s, %s) — did not throw\n", \
                     __FILE__, __LINE__, #expr, #ExcType); \
             ++test_harness::s_failures; throw test_harness::test_abort{}; \
        } \
    } while(0)

#define REQUIRE_NO_THROW(expr) \
    do { bool _ok = true; std::string _msg; \
         try { (void)(expr); } \
         catch (const std::exception& e) { _ok = false; _msg = e.what(); } \
         catch (...) { _ok = false; _msg = "<unknown>"; } \
         if (_ok) { ++test_harness::s_passed; } \
         else { \
             fprintf(stderr, "  FAIL %s:%d: REQUIRE_NO_THROW(%s) threw: %s\n", \
                     __FILE__, __LINE__, #expr, _msg.c_str()); \
             ++test_harness::s_failures; throw test_harness::test_abort{}; \
        } \
    } while(0)

// ─── Compat aliases for existing test_helper macros ──────────────────────────

#define UTXX_REQUIRE_NO_THROW(expr) REQUIRE_NO_THROW(expr)
#define UTXX_CHECK_NO_THROW(expr)   CHECK_NO_THROW(expr)

// ─── Test registration macros ────────────────────────────────────────────────

#define TEST_CASE(name) \
    static void _test_##name(); \
    static test_harness::registrar _reg_##name(#name, _test_##name); \
    static void _test_##name()

#define FIXTURE_TEST_CASE(name, Fixture) \
    struct _fixture_##name : public Fixture { void run(); }; \
    static test_harness::registrar _reg_##name(#name, []{ _fixture_##name _f; _f.run(); }); \
    void _fixture_##name::run()

#define TEST_SUITE(name)    namespace name {
#define TEST_SUITE_END()    }

// ─── Compat alias ────────────────────────────────────────────────────────────

#define BOOST_CURRENT_TEST_NAME test_harness::current_test_name()

// ─── Utilities ───────────────────────────────────────────────────────────────

namespace utxx {

inline bool get_test_argv(const std::string& a_opt,
                          const std::string& a_long_opt = "") {
    return get_opt(test_harness::s_argc, test_harness::s_argv,
                   (int*)nullptr, a_opt, a_long_opt);
}

template <typename T>
inline bool get_test_argv(const std::string& a_opt,
                          const std::string& a_long_opt,
                          T& a_value) {
    return get_opt(test_harness::s_argc, test_harness::s_argv,
                   &a_value, a_opt, a_long_opt);
}

#ifdef _MSC_VER
#pragma optimize("", off)
template <class T> void dont_optimize_var(T&& v) { v = v; }
#pragma optimize("", on)
#else
template <class T> void dont_optimize_var(T&& v) { asm volatile("":"+r" (v)); }
#endif

} // namespace utxx
