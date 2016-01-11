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

BOOST_AUTO_TEST_CASE( test_async_file_logger_err_handler )
{
    text_file_logger<> logger;
    logger.on_error = &on_error;

    T t;

    logger.on_error = [&t](int ec, const char* s) { t.on_error(ec, s); };
    BOOST_CHECK_EQUAL(-2, logger.start("/proc/xxxx/yyyy"));
    logger.stop();
}

BOOST_AUTO_TEST_CASE( test_async_file_logger_perf )
{
    enum { ITERATIONS = 500000 };

    {
        unlink(s_filename);

        text_file_logger<> logger;

        int ok = logger.start(s_filename);

        BOOST_CHECK_EQUAL(0, ok);

        perf_histogram perf("Async logger latency");

        for (int i = 0; i < ITERATIONS; i++) {
            perf.start();
            int n = logger.fwrite(s_str1, i);
            perf.stop();
            BOOST_REQUIRE(n > 0);
        }

        perf.dump(std::cout);
    }
}

//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( test_async_file_logger_append )
{
    enum { ITERATIONS = 10 };

    {
        unlink(s_filename);

        text_file_logger<> logger;

        for (int k=0; k < 2; k++) {

            int ok = logger.start(s_filename);
            BOOST_REQUIRE_EQUAL(0, ok);

            for (int i = 0; i < ITERATIONS; i++) {
                int n = logger.fwrite(s_str1, i);
                BOOST_CHECK(n > 0);
            }

            logger.stop();
        }

        std::ifstream file(s_filename, std::ios::in);
        for (int k = 0; k < 2; k++)
            for (int i = 0; i < ITERATIONS; i++) {
                std::string s;
                std::getline(file, s);
                BOOST_REQUIRE(!file.fail());

                char buf[256];
                sprintf(buf, s_str1, i);
                s += '\n';
                BOOST_CHECK_EQUAL(s, buf);
            }

        {
            std::string s;
            std::getline(file, s);
            BOOST_CHECK(file.fail());
            BOOST_CHECK(file.eof());
            file.close();
        }
        ::unlink(s_filename);
    }
}

class producer {
    int m_instance;
    int m_iterations;
    text_file_logger<>& m_logger;
    perf_histogram* m_histogram;
public:
    producer(text_file_logger<>& a_logger, int n, int iter, perf_histogram* h)
        : m_instance(n), m_iterations(iter), m_logger(a_logger), m_histogram(h)
    {}

    void operator() () {
        std::stringstream str;
        str << m_instance << "| " << s_str1;

        m_histogram->reset();

        for (int i = 0; i < m_iterations; i++) {
            m_histogram->start();
            if (m_logger.fwrite(str.str().c_str(), i) < 0) {
                std::stringstream s;
                s << "Thread " << m_instance << " iter "
                  << m_iterations << " error writing to file ("
                  << (i+1) << ", max_q_size: "
                  << m_logger.max_queue_size()
                  << "): " << strerror(errno) << std::endl;
                std::cerr << s.str();
                return;
            }
            m_histogram->stop();
            if (i % 4 == 0)
                ::sched_yield();
        }
    }
};

//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( test_async_file_logger_concurrent )
{
    const int nthreads  =  ::getenv("THREAD") ? atoi(::getenv("THREAD")) : 2;
    const int immediate = !::getenv("NO_WAKEUP");

    ::unlink(s_filename);

    text_file_logger<> logger;

    int ok = logger.start(s_filename, immediate);
    BOOST_CHECK ( ok == 0 );

    perf_histogram totals("Total async_file_logger performance");

    {
        std::vector<std::unique_ptr<std::thread>> threads(nthreads);
        std::vector<perf_histogram>               histograms(nthreads);
        for (int i=0; i < nthreads; i++)
            threads[i].reset
                (new std::thread(producer(logger, i+1, ITERATIONS, &histograms[i])));

        for (int i=0; i < nthreads; i++) {
            threads[i]->join();
            threads[i].reset();
            totals += histograms[i];
        }
    }

    logger.stop();

    std::stringstream s;
    s << "Max queue size: " << logger.max_queue_size() << std::endl;
    if (::getenv("VERBOSE"))
        totals.dump(s);
    BOOST_TEST_MESSAGE(s.str());

    int cur_count[nthreads];

    bzero(cur_count, sizeof(int)*nthreads);

    std::ifstream file(s_filename, std::ios::in);
    for (int i = 0; i < ITERATIONS*nthreads; i++) {
        std::string s;
        std::getline(file, s);
        BOOST_REQUIRE( ! file.fail() );

        size_t n1 = s.find_first_of('|');
        BOOST_CHECK( n1 != std::string::npos );

        std::string num = s.substr(0, n1);
        int thread_num = atoi(num.c_str());
        BOOST_CHECK( thread_num >= 1 && thread_num <= nthreads );

        size_t n2 = s.find_first_of(':');
        BOOST_CHECK( n2 != std::string::npos );
        num = s.substr(n2+1);
        int count = atoi(num.c_str());
        BOOST_CHECK_EQUAL( count, cur_count[thread_num-1] );

        std::stringstream str;
        str << thread_num << "| " << s_str1;

        char buf[128];
        sprintf(buf, str.str().c_str(), cur_count[thread_num-1]);
        s += '\n';

        BOOST_CHECK_EQUAL( s, buf );

        cur_count[thread_num-1]++;
    }

    {
        std::string s;
        std::getline(file, s);
    }
    BOOST_CHECK(file.fail());
    BOOST_CHECK(file.eof() );
    file.close();

    for (int i=0; i < nthreads; i++)
        BOOST_CHECK(cur_count[i] == ITERATIONS);

    ::unlink(s_filename);
}
