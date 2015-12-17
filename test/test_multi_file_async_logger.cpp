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
//#define USE_BOOST_POOL_ALLOC

#include <boost/test/unit_test.hpp>
#if defined(USE_BOOST_POOL_ALLOC) || defined(USE_BOOST_FAST_POOL_ALLOC)
#include <boost/pool/pool_alloc.hpp>
#elif defined(USE_NO_ALLOC) || defined(USE_CACHED_ALLOC)
#include <utxx/alloc_cached.hpp>
#endif
#define UTXX_DONT_UNDEF_ASYNC_TRACE
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
    struct test_traits : public multi_file_async_logger_traits {
#if   defined(USE_BOOST_POOL_ALLOC)
        typedef boost::pool_allocator<void>      allocator;
        typedef boost::fast_pool_allocator<void> fixed_size_allocator;
#elif defined(USE_BOOST_FAST_POOL_ALLOC)
        typedef boost::fast_pool_allocator<void> allocator;
        typedef boost::fast_pool_allocator<void> fixed_size_allocator;
#elif defined(USE_STD_ALLOC)
        typedef std::allocator<void>            allocator;
        typedef std::allocator<void>            fixed_size_allocator;
#elif defined(USE_NO_ALLOC)
        template <class T>
        struct no_alloc {
            static char             s_arena[4096];

            typedef T*              pointer;
            typedef const T*        const_pointer;
            typedef T               value_type;

            template <typename U>
            struct rebind {
                typedef no_alloc<U> other;
            };

            T* allocate(size_t n) {
                assert(n < sizeof(s_arena));
                return static_cast<T*>(s_arena);
            }
            void deallocate(char* p, size_t) {}
        };

        typedef no_alloc<char>                  allocator;
        typedef memory::cached_allocator<char>  fixed_size_allocator;
#else
        typedef memory::cached_allocator<char>  allocator;
        typedef memory::cached_allocator<char>  fixed_size_allocator;
#endif
    };

#if defined(USE_NO_ALLOC)
    template <class T>
    char test_traits::no_alloc<T>::s_arena[4096];
#endif
} // namespace

typedef basic_multi_file_async_logger<test_traits> logger_t;

void unlink() {
    for (size_t i = 0; i < s_file_num; i++)
        ::unlink(s_filename[i]);
}

auto worker = [](int id, int iterations, boost::barrier* barrier,
                 logger_t* logger, logger_t::file_id* files,
                 perf_histogram* histogram, double* elapsed)
{
    barrier->wait();
    histogram->reset();
    bool no_histogram = getenv("NOHISTOGRAM");

    timer tm;

    for (int i=0; i < iterations; i++) {
        char* p = logger->allocate(sizeof(s_str1)-1);
        char* q = logger->allocate(sizeof(s_str3)-1);
        strncpy(p, s_str1, sizeof(s_str1)-1);
        strncpy(q, s_str3, sizeof(s_str3)-1);
        if (!no_histogram) histogram->start();
        logger->write(files[0], std::string(), p, sizeof(s_str1)-1);
        if (!no_histogram) histogram->stop();
        //BOOST_REQUIRE_EQUAL(0, n);
        if (!no_histogram) histogram->start();
        logger->write(files[1], std::string(), q, sizeof(s_str3)-1);
        if (!no_histogram) histogram->stop();
        //BOOST_REQUIRE_EQUAL(0, m);
    }

    *elapsed = tm.elapsed();
    auto lat = tm.latency_usec(iterations);

    if (utxx::verbosity::level() != utxx::VERBOSE_NONE) {
        char buf[128];
        sprintf(buf,
                "Performance thread %d finished (speed=%7d ops/s, lat=%.3f us) "
                "total logged: %lu\n",
                id, int(double(iterations) / *elapsed), lat,
                logger->total_msgs_processed());
        BOOST_TEST_MESSAGE(buf);
    }
};

BOOST_AUTO_TEST_CASE( test_multi_file_logger_perf )
{
    static const int ITERATIONS =
        getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 250000u;
    static const int THREADS    =
        getenv("THREADS")    ? atoi(getenv("THREADS"))    : 3;

    unlink();

    logger_t::file_id l_fds[s_file_num];

    logger_t logger;

    for (size_t i = 0; i < s_file_num; i++) {
        l_fds[i] = logger.open_file(s_filename[i], false);
        BOOST_REQUIRE(l_fds[i].fd() >= 0);
        //logger.set_batch_size(l_fds[i], 25);
    }

    int ok = logger.start();

    BOOST_REQUIRE_EQUAL(0, ok);

    std::vector<std::shared_ptr<std::thread>>   threads(THREADS);
    boost::barrier                              barrier(THREADS+1);
    double                                      elapsed[THREADS];
    std::vector<perf_histogram>                 histograms(THREADS);

    for (int i=0; i < THREADS; i++) {
        new (&(histograms[i])) perf_histogram();
        threads[i].reset(
            new std::thread(worker,
                i+1, ITERATIONS, &barrier,
                &logger, l_fds, &(histograms[i]), &(elapsed[i])
            ));
    }

    barrier.wait();

    perf_histogram totals("Total performance");
    double sum_time = 0;

    for (int i=0; i < THREADS; i++) {
        if (threads[i]) threads[i]->join();
        totals   += histograms[i];
        sum_time += elapsed[i];
    }

    BOOST_TEST_MESSAGE("All threads finished!");

    if (verbosity::level() >= utxx::VERBOSE_DEBUG) {
        sum_time /= THREADS;
        char buf[128];
        sprintf(buf, "Avg speed = %8d it/s, latency = %.3f us\n",
               (int)((double)ITERATIONS / sum_time),
               sum_time * 1000000 / ITERATIONS);
        BOOST_TEST_MESSAGE(buf);
        if (!getenv("NOHISTOGRAM")) {
            std::stringstream s;
            totals.dump(s);
            BOOST_TEST_MESSAGE(s.str());
        }
    }

    std::stringstream s;
    s << "Max queue size = " << logger.max_queue_size();
    BOOST_TEST_MESSAGE(s.str());


#ifdef PERF_STATS
    std::cout << "Futex wake          count = " << logger.event().wake_count()          << std::endl;
    std::cout << "Futex wake_signaled count = " << logger.event().wake_signaled_count() << std::endl;
    std::cout << "Futex wait          count = " << logger.event().wait_count()          << std::endl;
    std::cout << "Futex wake_fast     count = " << logger.event().wake_fast_count()     << std::endl;
    std::cout << "Futex wait_fast     count = " << logger.event().wait_fast_count()     << std::endl;
    std::cout << "Futex wait_spin     count = " << logger.event().wait_spin_count()     << std::endl;
    std::cout << std::endl;

    std::cout << "Enqueue spins: " << logger.stats_enque_spins() << std::endl;
    std::cout << "Dequeue spins: " << logger.stats_deque_spins() << std::endl;
#endif


    logger.stop();

    BOOST_CHECK_EQUAL(0, logger.open_files_count());

#ifndef DONT_DELETE_FILE
    unlink();
#endif
}

BOOST_AUTO_TEST_CASE( test_multi_file_logger_close_file )
{
    static const int32_t ITERATIONS =
        getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 50;

    ::unlink(s_filename[0]);

    {
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
    }

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
            UTXX_ASYNC_TRACE(("  rewritten msg(%p, %lu) -> msg(%p, %lu)\n",
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
