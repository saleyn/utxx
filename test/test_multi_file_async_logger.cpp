//----------------------------------------------------------------------------
/// \file  test_multi_file_async_logger.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating multi_file_async_logger.
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

//#define DEBUG_ASYNC_LOGGER
#define DEBUG_USE_BOOST_POOL_ALLOC

#include <boost/test/unit_test.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <utxx/multi_file_async_logger.hpp>
#include <utxx/perf_histogram.hpp>
#include <utxx/verbosity.hpp>

static const size_t s_file_num  = 2;
static const char* s_filename[] = { "/tmp/test_multi_file_async_logger1.log",
                                    "/tmp/test_multi_file_async_logger2.log" };
static const char s_str1[]      = "This is a const char* string line:%d\n";
static const char s_str2[]      = "This is an stl std::string line:";
static const char s_str3[]      = "This is another const char* string without line\n";

using namespace boost::unit_test;
using namespace utxx;

namespace {
#ifdef DEBUG_USE_BOOST_POOL_ALLOC
    struct test_traits : public multi_file_async_logger_traits {
        typedef boost::pool_allocator<void> allocator;
    };
#else
    typedef multi_file_async_logger_traits test_traits;
#endif
} // namespace

typedef basic_multi_file_async_logger<test_traits> logger_t;

void unlink() {
    for (size_t i = 0; i < s_file_num; i++)
        ::unlink(s_filename[i]);
}

BOOST_AUTO_TEST_CASE( test_multi_file_logger_perf )
{
    static const int32_t ITERATIONS =
        getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 250000u;

    unlink();

    logger_t::file_id l_fds[s_file_num];

    logger_t l_logger;

    for (size_t i = 0; i < s_file_num; i++) {
        l_fds[i] = l_logger.open_file(s_filename[i], false);
        BOOST_REQUIRE(l_fds[i].fd() >= 0);
    }

    int ok = l_logger.start();

    BOOST_REQUIRE_EQUAL(0, ok);

    perf_histogram perf("Async logger latency");

    for (int i = 0; i < ITERATIONS; i++) {
        char* p = l_logger.allocate(sizeof(s_str1));
        char* q = l_logger.allocate(sizeof(s_str3));
        strncpy(p, s_str1, sizeof(s_str1));
        strncpy(q, s_str3, sizeof(s_str3));
        perf.start();
        int n = l_logger.write(l_fds[0], std::string(), p, sizeof(s_str1));
        perf.stop();
        BOOST_REQUIRE_EQUAL(0, n);
        perf.start();
        int m = l_logger.write(l_fds[1], std::string(), q, sizeof(s_str3));
        perf.stop();
        BOOST_REQUIRE_EQUAL(0, m);
    }

    for (size_t i = 0; i < s_file_num; i++) {
        l_logger.close_file(l_fds[i], false);
        BOOST_REQUIRE(l_fds[i].fd() < 0);
    }

    BOOST_REQUIRE_EQUAL(0,  l_logger.open_files_count());

    if (verbosity::level() != VERBOSE_NONE) {
        perf.dump(std::cout);
        printf("Max queue size = %d\n", l_logger.max_queue_size());
    }

    l_logger.stop();
    unlink();
}

BOOST_AUTO_TEST_CASE( test_multi_file_logger_close_file )
{
    static const int32_t ITERATIONS =
        getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 50;

    ::unlink(s_filename[0]);

    logger_t l_logger;

    logger_t::file_id l_fd;
    BOOST_REQUIRE(!l_fd);
    int ec = l_logger.close_file(l_fd);
    BOOST_REQUIRE_EQUAL(0, ec);

    l_fd = l_logger.open_file(s_filename[0], false);
    BOOST_REQUIRE(l_fd.fd() >= 0);
    BOOST_REQUIRE(l_fd);

    int ok = l_logger.start();

    BOOST_REQUIRE_EQUAL(0, ok);

    for (int i = 0; i < ITERATIONS; i++) {
        char buf[128];
        int n = snprintf(buf, sizeof(buf), s_str1, i);
        char* p = l_logger.allocate(n);
        strncpy(p, buf, n);
        n = l_logger.write(l_fd, std::string(), p, n);
        BOOST_REQUIRE_EQUAL(0, n);
    }

    ec = l_logger.last_error(l_fd);
    BOOST_REQUIRE_EQUAL(0, ec);

    l_logger.close_file(l_fd, false);

    ec = l_logger.open_files_count();
    BOOST_REQUIRE_EQUAL(0,  ec);
    ec = l_logger.last_error(l_fd);
    BOOST_REQUIRE_EQUAL(-1, ec);

    l_logger.stop();
    ::unlink(s_filename[0]);
}

namespace {
    class formatter {
        logger_t& m_logger;
        std::string m_prefix;
    public:
        formatter(logger_t& a_logger)
            : m_logger(a_logger), m_prefix("ABCDEFG")
        {}

        const std::string& prefix() const { return m_prefix; }

        iovec operator() (const std::string& a_category, iovec& a_msg) {
            size_t n = a_msg.iov_len + m_prefix.size();
            char* p = m_logger.allocate(n);
            memcpy(p, m_prefix.c_str(), m_prefix.size());
            memcpy(p + m_prefix.size(), a_msg.iov_base, a_msg.iov_len);
            m_logger.deallocate(static_cast<char*>(a_msg.iov_base), a_msg.iov_len);
            ASYNC_TRACE(("  rewritten msg(%p, %lu) -> msg(%p, %lu)\n",
                    a_msg.iov_base, a_msg.iov_len, p, n));
            a_msg.iov_base = p;
            a_msg.iov_len  = n;
            // In this case we instructuct the caller to write the same
            // content as it will free.  However, we could return some other
            // pointer if we wanted to write less data than what's available
            // in the a_msg
            return a_msg;
        }
    };
}

BOOST_AUTO_TEST_CASE( test_multi_file_logger_formatter )
{
    static const int32_t ITERATIONS = 3;

    ::unlink(s_filename[0]);

    logger_t l_logger;

    logger_t::file_id l_fd =
        l_logger.open_file(s_filename[0], false);
    BOOST_REQUIRE(l_fd.fd() >= 0);

    formatter l_formatter(l_logger);
    l_logger.set_formatter(l_fd, l_formatter);

    int ok = l_logger.start();

    BOOST_REQUIRE_EQUAL(0, ok);

    std::string s = std::string(s_str2) + '\n';

    for (int i = 0; i < ITERATIONS; i++) {
        int n = l_logger.write(l_fd, std::string(), s);
        BOOST_REQUIRE_EQUAL(0, n);
    }

    BOOST_REQUIRE_EQUAL(0, l_logger.last_error(l_fd));

    l_logger.close_file(l_fd, false);

    BOOST_REQUIRE_EQUAL(0,  l_logger.open_files_count());
    BOOST_REQUIRE_EQUAL(-1, l_logger.last_error(l_fd));

    l_logger.stop();

    {
        std::string st = l_formatter.prefix() + s_str2;

        std::ifstream file(s_filename[0], std::ios::in);
        for (int i = 0; i < ITERATIONS; i++) {
            std::string sg;
            std::getline(file, sg);
            BOOST_REQUIRE( !file.fail() );
            BOOST_REQUIRE_EQUAL( st, sg );
        }

        std::getline(file, st);
        BOOST_REQUIRE(file.fail());
        BOOST_REQUIRE(file.eof());
        file.close();

        ::unlink(s_filename[0]);
    }
}

//-----------------------------------------------------------------------------
/*
BOOST_AUTO_TEST_CASE( 
    test_multi_file_logger_append )
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
BOOST_AUTO_TEST_CASE( 
    test_multi_file_logger_concurrent )
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

*/
