// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  test_main.hpp
//------------------------------------------------------------------------------
/// \brief Minimal test harness for standalone reactor tests (no boost/utxx)
//------------------------------------------------------------------------------
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// ---- assertion macros -------------------------------------------------------

#define CHECK(expr)                                                           \
  do {                                                                        \
    if (!(expr)) {                                                            \
      fprintf(stderr, "FAIL  %s:%d: CHECK(%s)\n", __FILE__, __LINE__, #expr); \
      s_failures++;                                                           \
    } else {                                                                  \
      s_passed++;                                                             \
    }                                                                         \
  } while (0)

#define CHECK_EQUAL(a, b)                                                                       \
  do {                                                                                          \
    auto _a = (a);                                                                              \
    auto _b = (b);                                                                              \
    if (!(_a == _b)) {                                                                          \
      fprintf(                                                                                  \
          stderr, "FAIL  %s:%d: CHECK_EQUAL(%s, %s)  [%s != %s]\n", __FILE__, __LINE__, #a, #b, \
          std::to_string(_a).c_str(), std::to_string(_b).c_str());                              \
      s_failures++;                                                                             \
    } else {                                                                                    \
      s_passed++;                                                                               \
    }                                                                                           \
  } while (0)

#define CHECK_THROWS(expr)                                                                      \
  do {                                                                                          \
    bool _threw = false;                                                                        \
    try {                                                                                       \
      (void)(expr);                                                                             \
    } catch (...) {                                                                             \
      _threw = true;                                                                            \
    }                                                                                           \
    if (!_threw) {                                                                              \
      fprintf(                                                                                  \
          stderr, "FAIL  %s:%d: CHECK_THROWS(%s) — no exception\n", __FILE__, __LINE__, #expr); \
      s_failures++;                                                                             \
    } else {                                                                                    \
      s_passed++;                                                                               \
    }                                                                                           \
  } while (0)

#define CHECK_NO_THROW(expr)                                                                \
  do {                                                                                      \
    try {                                                                                   \
      (void)(expr);                                                                         \
      s_passed++;                                                                           \
    } catch (std::exception & e) {                                                          \
      fprintf(                                                                              \
          stderr, "FAIL  %s:%d: CHECK_NO_THROW(%s) threw: %s\n", __FILE__, __LINE__, #expr, \
          e.what());                                                                        \
      s_failures++;                                                                         \
    }                                                                                       \
  } while (0)

// ---- test registration ------------------------------------------------------

namespace test_harness {

struct test_case {
  const char*           name;
  std::function<void()> fn;
};

inline std::vector<test_case>& registry()
{
  static std::vector<test_case> r;
  return r;
}

struct registrar {
  registrar(const char* name, std::function<void()> fn)
  { registry().push_back({name, std::move(fn)}); }
};

inline int s_failures = 0;
inline int s_passed   = 0;

inline int run_all()
{
  for (auto& tc : registry()) {
    fprintf(stdout, "[ RUN  ] %s\n", tc.name);
    try {
      tc.fn();
      fprintf(stdout, "[  OK  ] %s\n", tc.name);
    } catch (std::exception& e) {
      fprintf(stderr, "[ FAIL ] %s — uncaught exception: %s\n", tc.name, e.what());
      s_failures++;
    } catch (...) {
      fprintf(stderr, "[ FAIL ] %s — unknown exception\n", tc.name);
      s_failures++;
    }
  }
  fprintf(stdout, "\n%d passed, %d failed\n", s_passed, s_failures);
  return s_failures ? 1 : 0;
}

} // namespace test_harness

// Pull counters into scope for CHECK macros
using test_harness::s_failures;
using test_harness::s_passed;

#define TEST_CASE(name)                                            \
  static void                    _test_##name();                   \
  static test_harness::registrar _reg_##name(#name, _test_##name); \
  static void                    _test_##name()
