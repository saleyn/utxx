//----------------------------------------------------------------------------
/// \file  test_async_file_logger.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating async_file_logger functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2012-03-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of utxx open-source project.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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

#include <fstream>
#include <iomanip>
#include <unistd.h>

//#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <utxx/logger/async_file_logger.hpp>
#include <utxx/perf_histogram.hpp>
#include <utxx/verbosity.hpp>

const char* s_filename = "/tmp/test_async_file_logger.log";
const char* s_str1     = "This is a const char* string line:%d\n";
const char* s_str2     = "This is an stl std::string line:";
const char* s_str3     = "This is another const char* string without line\n";

static const int32_t ITERATIONS =
    getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 100000u;

using namespace boost::unit_test;
using namespace utxx;

struct T {
    void on_error(int, const char* s) {
        if (verbosity::level() > VERBOSE_NONE)
            std::cerr << "This error is supposed to happen: " << s << std::endl;
    }
};

void on_error(int, const char*) {} 

BOOST_AUTO_TEST_CASE( test_async_logger_err_handler )
{
    text_file_logger<> l_logger;
    l_logger.on_error = &on_error;

    T t;

    l_logger.on_error = boost::bind(&T::on_error, &t, _1, _2);
    BOOST_REQUIRE_EQUAL(-2, l_logger.start("/proc/xxxx/yyyy"));
    l_logger.stop();
}

BOOST_AUTO_TEST_CASE( test_async_logger_perf )
{
    enum { ITERATIONS = 500000 };
    
    {
        unlink(s_filename);

        text_file_logger<> l_logger;

        int ok = l_logger.start(s_filename);

        BOOST_REQUIRE ( ok == 0 );

        perf_histogram perf("Async logger latency");

        for (int i = 0; i < ITERATIONS; i++) {
            perf.start();
            int n = l_logger.fwrite(s_str1, i);
            perf.stop();
            BOOST_REQUIRE( 0 == n );
        }

        perf.dump(std::cout);
    }
}

//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( test_async_logger_append )
{
    enum { ITERATIONS = 10 };
    
    {
        unlink(s_filename);

        text_file_logger<> l_logger;

        for (int k=0; k < 2; k++) {
            
            int ok = l_logger.start(s_filename);
            BOOST_REQUIRE ( ok == 0 );

            for (int i = 0; i < ITERATIONS; i++)
                BOOST_REQUIRE( 0 == l_logger.fwrite(s_str1, i) );

            l_logger.stop();
        }
        
        std::ifstream file(s_filename, std::ios::in);
        for (int k = 0; k < 2; k++)
            for (int i = 0; i < ITERATIONS; i++) {
                std::string s;
                std::getline(file, s);
                BOOST_REQUIRE( ! file.fail() );
                
                char buf[256];
                sprintf(buf, s_str1, i);
                s += '\n';
                BOOST_REQUIRE_EQUAL( s, buf );
            }
        
        {
            std::string s;
            std::getline(file, s);
            BOOST_REQUIRE(file.fail());
            BOOST_REQUIRE(file.eof());
            file.close();
        }
        ::unlink(s_filename);
    }
}

class producer {
    int m_instance;
    int m_iterations;
    text_file_logger<>& m_logger;
public:
    producer(text_file_logger<>& a_logger, int n, int iter) 
        : m_instance(n), m_iterations(iter), m_logger(a_logger)
    {}

    void operator() () {
        std::stringstream str;
        str << m_instance << "| " << s_str1;

        for (int i = 0; i < m_iterations; i++) {
            if (m_logger.fwrite(str.str().c_str(), i) < 0) {
                std::cerr << "Thread " << m_instance << " iter "
                          << m_iterations << " error writing to file ("
                          << i+1 << "): " << strerror(errno) << std::endl;
                return;
            }
            if (i % 4 == 0)
                ::sched_yield();
        }
    }
};

//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( test_async_file_logger_concurrent )
{
    const int threads = ::getenv("THREAD") ? atoi(::getenv("THREAD")) : 2;

    ::unlink(s_filename);

    text_file_logger<> l_logger;
    
    int ok = l_logger.start(s_filename);
    BOOST_REQUIRE ( ok == 0 );

    {
        boost::shared_ptr<boost::thread> l_threads[threads];
        for (int i=0; i < threads; i++)
            l_threads[i] = boost::shared_ptr<boost::thread>(
                new boost::thread(producer(l_logger, i+1, ITERATIONS)));

        for (int i=0; i < threads; i++)
            (*l_threads)->join();
    }

    l_logger.stop();

    int MAX = threads;
    int cur_count[MAX];
    
    bzero(cur_count, sizeof(cur_count));

    std::ifstream file(s_filename, std::ios::in);
    for (int i = 0; i < ITERATIONS*MAX; i++) {
        std::string s;
        std::getline(file, s);
        BOOST_REQUIRE( ! file.fail() );
        
        size_t n1 = s.find_first_of('|');
        BOOST_REQUIRE( n1 != std::string::npos );

        std::string num = s.substr(0, n1);
        int thread_num = atoi(num.c_str());
        BOOST_REQUIRE( thread_num >= 1 && thread_num <= MAX );

        size_t n2 = s.find_first_of(':');
        BOOST_REQUIRE( n2 != std::string::npos );
        num = s.substr(n2+1);
        int count = atoi(num.c_str());
        BOOST_REQUIRE_EQUAL( count, cur_count[thread_num-1] );

        std::stringstream str;
        str << thread_num << "| " << s_str1;

        char buf[128];
        sprintf(buf, str.str().c_str(), cur_count[thread_num-1]);
        s += '\n';

        BOOST_REQUIRE_EQUAL( s, buf );

        cur_count[thread_num-1]++;
    }

    {
        std::string s;
        std::getline(file, s);
    }
    BOOST_REQUIRE( file.fail() );
    BOOST_REQUIRE( file.eof() );
    file.close();

    for (int i=0; i < MAX; i++)
        BOOST_REQUIRE(cur_count[i] == ITERATIONS);

    ::unlink(s_filename);
}
