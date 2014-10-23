//----------------------------------------------------------------------------
/// \file  test_function.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for benchmarking utxx::function functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-10-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of utxx open-source project.

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

//#define BOOST_TEST_NO_AUTO_LINK

//#define BOOST_TEST_MODULE logger_test
#include <boost/test/unit_test.hpp>
#include <utxx/function.hpp>
#include <utxx/time_val.hpp>
#include <iostream>
#include <random>
#include <string>
#include <chrono>
#include <memory>

//#define BOOST_TEST_MAIN

static const size_t num_allocations = 100;
static const size_t num_calls = getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 100000;

struct Updateable
{
    virtual ~Updateable() {}
    virtual void update(float dt) = 0;
};

struct UpdateableA : Updateable
{
    UpdateableA() : calls(0) {}
    virtual void update(float) override { ++calls; }

    size_t calls;
};
struct UpdateableB : Updateable
{
    virtual void update(float) override { ++calls; }
    static size_t calls;
};
size_t UpdateableB::calls = 0;


struct LambdaA
{
    template<typename F>
    explicit LambdaA(std::vector<F> & update_loop)
        : calls(0)
    {
        update_loop.emplace_back([this](float dt) { update(dt); });
    }
    void update(float) { ++calls; }
    size_t calls;
};

struct LambdaB
{
    template<typename F>
    explicit LambdaB(std::vector<F> & update_loop)
    {
        update_loop.emplace_back([this](float dt) { update(dt); });
    }
    void update(float) { ++calls; }
    static size_t calls;
};
size_t LambdaB::calls = 0;

struct ScopedMeasurer
{
    ScopedMeasurer(std::string name, size_t iterations)
        : name(std::move(name))
        , iterations(iterations)
    {}
    ~ScopedMeasurer()
    {
        printf("  %30s speed=%9.0f calls/s latency=%.3fus\n",
               name.c_str(), timer.speed(iterations), timer.latency_usec(iterations));
    }
    std::string name;
    size_t      iterations;
    utxx::timer timer;
};

void measure_only_call(const std::vector<Updateable *> & container)
{
    ScopedMeasurer measure("virtual function", num_calls * container.size());
    for (size_t i = 0; i < num_calls; ++i)
        for (auto & updateable : container)
            updateable->update(0.016f);
}

void time_virtual(int seed)
{
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> random_int(0, 1);
    std::vector<std::unique_ptr<Updateable>> updateables;
    std::vector<Updateable *> update_loop;
    for (size_t i = 0; i < num_allocations; ++i)
    {
        if (random_int(generator))
            updateables.emplace_back(new UpdateableA);
        else
            updateables.emplace_back(new UpdateableB);
        update_loop.push_back(updateables.back().get());
    }
    measure_only_call(update_loop);
}

template<typename Container>
void measure_only_call(const Container& container, const std::string & name)
{
    ScopedMeasurer measure(name, num_calls * container.size());
    for (size_t i = 0; i < num_calls; ++i)
        for (auto & updateable : container)
            updateable(0.016f);
}

template<typename std_function>
void time_function(int seed, const std::string & name)
{
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> random_int(0, 1);
    std::vector<std_function> update_loop;
    std::vector<std::unique_ptr<LambdaA>> slotsA;
    std::vector<std::unique_ptr<LambdaB>> slotsB;
    for (size_t i = 0; i < num_allocations; ++i)
    {
        if (random_int(generator))
            slotsA.emplace_back(new LambdaA(update_loop));
        else
            slotsB.emplace_back(new LambdaB(update_loop));
    }
    measure_only_call(update_loop, name);
}


BOOST_AUTO_TEST_CASE( test_function_latency )
{
    time_t seed = time(NULL);
    time_function<std::function <void (float)>>(seed, "std::function");
    time_function<utxx::function<void (float)>>(seed, "utxx::function");
    time_virtual(seed);

    BOOST_REQUIRE(true); // to remove run-time warning
}

//BOOST_AUTO_TEST_SUITE_END()
