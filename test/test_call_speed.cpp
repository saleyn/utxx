//----------------------------------------------------------------------------
/// \file   test_call_speed.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Tests invocation speed of various types of functions.
//----------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <functional>
#include <utxx/function.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

#include <boost/format.hpp>

const int iterations_count = 100000000;

static int s_values[] = {1, 2, 3, 4, 5, 6, 7, 8 };

template <class T>
void __attribute__ ((noinline)) inc(T& i)    { i += s_values[(int)i & 0x7]; }

template <class T>
void iinc(T& i) {
    i += s_values[(int)i & 0x7];
}

typedef void (*funp_adder)(int& i);

// a number interface
struct number {        
    virtual void increment(int&) = 0;
};

// number interface implementation for all types
struct number_inl : number {
    virtual void increment(int& t) { iinc(t); }
};

struct number_nin : number {
    virtual void increment(int& t) { inc(t); }
};

int use_virtual() {
    number_nin num_int;
    int n = 0;
    for (int i = 0; i < iterations_count; i++) num_int.increment(n);
    return n;
}

int use_inlined_virtual() {
    number_nin num_int;
    int n = 0;
    for (int i = 0; i < iterations_count; i++) num_int.increment(n);
    return n;
}

// measures not-inlined function call
int use_function() {
    int n = 0;
    for (int i = 0; i < iterations_count; i++) inc(n);
    return n;
}

int use_inlined_function() {
    int n = 0;
    for (int i = 0; i < iterations_count; i++) iinc(n);
    return n;
}

int use_utxx_function() {
    int i = 0;
    utxx::function<void()> f = [&]() { iinc(i); };
    for (int i = 0; i < iterations_count; i++) f();
    return i;
}

int use_std_function() {
    int i = 0;
    std::function<void()> f = [&]() { iinc(i); };
    for (int i = 0; i < iterations_count; i++) f();
    return i;
}

// a visitor that increments a variant by N
struct add_inlined : boost::static_visitor<> {
    template <typename T> void operator() (T& t) const { iinc(t); }
};

struct add : boost::static_visitor<> {
    template <typename T> void operator() (T& t) const { inc(t); }
};

// a visitor that increments a variant by N
template <typename T, typename V>
T get(const V& v) {
    struct getter : boost::static_visitor<T> {
        T operator() (T t) const { return t; }
    };
    return boost::apply_visitor(getter(), v);
}

template <class Adder>
int use_variant() {
    typedef boost::variant<int, float, double> number;

    number num = 0;

    for (int i = 0; i < iterations_count; i++) {
        boost::apply_visitor(Adder(), num);
    }

    return get<int>(num);
}

int use_funp() {
    int n = 0;
    funp_adder f = &iinc<int>;

    for (int i = 0; i < iterations_count; i++) { f(n); }

    return n;
}

int use_not_inlined_funp() {
    int n = 0;
    funp_adder f = &inc<int>;

    for (int i = 0; i < iterations_count; i++) { f(n); }

    return n;
}

typedef void (*int_add)(int& i);

// a visitor that increments a variant by N
struct adder : boost::static_visitor<void> {
    int& n;
    adder(int& n) : n(n) {}
    void operator() (int_add& f) const { f(n); }
};

int use_variant_fun() {
    typedef boost::variant<
        int_add
    > incr;
    incr num = [](int& i) { iinc(i); };
    int n = 0;

    for (int i = 0; i < iterations_count; i++) {
        boost::apply_visitor(adder(n), num);
    }

    return n;
}

BOOST_AUTO_TEST_CASE( test_call_speed )
{
    using namespace boost::posix_time;

    ptime start, end;
    time_duration d;

    std::vector<std::pair<std::string, std::function<int()>>> funs
    {
        {"function      (inlined)",     []() { return use_inlined_function(); }},
        {"function      (not-inlined)", []() { return use_function(); }},
        {"lambda        (inlined)",     []() { return ([]() {
                                                    int n=0; for (int i=0; i < iterations_count; i++) iinc(n);
                                                    return n;
                                             })(); }},
        {"lambda        (not-inlined)", []() { return ([]() {
                                                    int n=0; for (int i=0; i < iterations_count; i++) inc(n);
                                                    return n;
                                             })(); }},
        {"virtual fun   (inlined)",     []() { return use_inlined_virtual(); }},
        {"virtual fun   (not-inlined)", []() { return use_virtual(); }},
        {"fun ptr       (not-inlined)", []() { return use_funp(); }},
        {"fun ptr       (inlined)",     []() { return use_not_inlined_funp(); }},
        {"variant       (inlined)",     []() { return use_variant<add_inlined>(); }},
        {"variant       (not-inlined)", []() { return use_variant<add>(); }},
        {"variant fun   (inlined)",     []() { return use_variant_fun(); }},
        {"utxx::fun     (inlined)",     []() { return use_utxx_function(); }},
        {"std::fun      (inlined0",     []() { return use_std_function(); }}
    };

    for (auto& p : funs) {
        start = microsec_clock::universal_time();
        int i = p.second();
        end = microsec_clock::universal_time();
        d = end - start;
        char buf[128];
        sprintf(buf, "  %-30s:  %.3fns (%d)",
            p.first.c_str(), 1000.0 * double(d.total_microseconds()) / iterations_count, i);
        BOOST_TEST_MESSAGE(buf);
    }

    BOOST_CHECK(true);
}
