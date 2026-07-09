/**
 * \file
 * \brief shared buffer queue tests
 *
 * \author Dmitriy Kargapolov <dmitriy dot kargapolov at gmail dot com>
 * \since 06 Jun 2012
 */

/*
 * Copyright (C) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <utxx/test_helper.hpp>
#include <utxx/config.h>
#ifdef UTXX_HAVE_BOOST
#include <utxx/shared_buffer_queue.hpp>

struct deleter {
    int& cnt;
    deleter(int& n) : cnt(n) {}
    void operator()() { cnt++; }
};

TEST_CASE( shared_queue_test )
{
    int val = 0;
    boost::asio::const_buffer cbuf(&val, sizeof(val));

    int cnt = 0;
    {
        utxx::shared_const_buffer sbuf(cbuf, deleter(cnt));
        {
            utxx::shared_buffer_queue<> bq1;
            utxx::shared_buffer_queue<> bq2;
            utxx::shared_buffer_queue<> bq3;
            bq1.enqueue(sbuf);
            bq2.enqueue(sbuf);
            bq3.enqueue(sbuf);
        }
        REQUIRE_EQUAL(0, cnt);
    }
    REQUIRE_EQUAL(1, cnt);

    cnt = 0;
    {
        utxx::shared_buffer_queue<> bq1;
        utxx::shared_buffer_queue<> bq2;
        utxx::shared_buffer_queue<> bq3;
        bq1.enqueue(cbuf);
        bq2.enqueue(cbuf);
        bq3.enqueue(cbuf);
    }
    REQUIRE_EQUAL(0, cnt);
}

#endif // UTXX_HAVE_BOOST
