// vim:ts=4:sw=4:et
//------------------------------------------------------------------------------
/// @file   TestFileAIO.cpp
/// @author Serge Aleynikov
//------------------------------------------------------------------------------
/// @brief Test cases for ReactorAIOReader.cpp
//------------------------------------------------------------------------------
// Copyright (c) 2015 Omnibius, LLC
// Created:  2015-03-10
// License:  property of Omnibius licensed under terms found in LICENSE.txt
//------------------------------------------------------------------------------
#include <boost/test/unit_test.hpp>
#include <utxx/test_helper.hpp>
#include <utxx/path.hpp>
#include <utxx/buffer.hpp>
#include <utxx/io/ReactorAIOReader.hpp>
#include <poll.h>
#include <fcntl.h>

using namespace std;
using utxx::io::AIOReader;

struct temp_file {
    temp_file()  : m_filename("/tmp/test-aio.pcap") { erase(); create(); }
    ~temp_file() { erase(); }

    std::string const& filename() const { return m_filename; }
private:
    std::string m_filename;

    void create() {
        std::ofstream in(m_filename.c_str());
        for (int i=0; i < 1024*1024; i++)
          in << char('a'+i%32);
    }
    void erase() { utxx::path::file_unlink(m_filename); }
};

static long WaitAsync(int a_efd, int a_timeout)
{
    struct pollfd pfd;

    pfd.fd = a_efd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    if (poll(&pfd, 1, a_timeout) < 0) {
        perror("poll");
        return -1;
    }
    if ((pfd.revents & POLLIN) == 0) {
        fprintf(stderr, "no results completed\n");
        return 0;
    }

    return 1;
}

BOOST_AUTO_TEST_CASE( test_reactor_file1 )
{
    int efd = eventfd(0, EFD_NONBLOCK);
    BOOST_REQUIRE(efd >= 0);

    temp_file f;
    AIOReader file;

    UTXX_REQUIRE_NO_THROW(file.Init(efd, f.filename().c_str()));

    utxx::dynamic_io_buffer buf(256);

    int rc = file.AsyncRead(buf.wr_ptr(), buf.capacity());

    BOOST_CHECK(rc >= 0);

    int it = 0;

    do {
        rc = WaitAsync(file.EventFD(), 5000);

        long a_events = 0;
/*
        BOOST_REQUIRE(rc >= 0);

        if (read(efd, &eval, sizeof(eval)) != sizeof(eval))
            perror("read");
        */
        a_events = file.CheckEvents();
        std::string err;
        //rc = io_getevents(file.m_ctx, 1, std::min<long>(1,a_events), file.m_io_event, &tmo);
        std::tie(rc, err) = file.ReadEvents(a_events);
        if (rc == 0) continue;

        if (rc < 0)
            BOOST_TEST_MESSAGE(err << ": " << strerror(errno));
        BOOST_REQUIRE(rc >= 0);

        buf.read_and_crunch(rc);

        rc = file.AsyncRead(buf.wr_ptr(), buf.capacity());

        it++;
    } while (rc > 0);

    BOOST_CHECK_EQUAL(4096, it);
}

BOOST_AUTO_TEST_CASE( test_reactor_file )
{
    int efd = eventfd(0, EFD_NONBLOCK);
    BOOST_REQUIRE(efd >= 0);

    temp_file f;
    AIOReader file;

    UTXX_REQUIRE_NO_THROW(file.Init(efd, f.filename().c_str()));

    utxx::dynamic_io_buffer buf(256);

    //boost::progress_display progress(file.Size(), cerr);

    int rc = file.AsyncRead(buf.wr_ptr(), buf.capacity());

    BOOST_REQUIRE(rc >= 0);

    while (file.Remaining() > 0) {
        int rc = WaitAsync(efd, 5000);

        BOOST_REQUIRE(rc >= 0);

        if (rc == 0) continue;

        rc = file.CheckEvents();

        BOOST_REQUIRE(rc == 1);

        auto res = file.ReadEvents(rc);

        if (res.first < 0)
            BOOST_TEST_MESSAGE(res.second << ": " << strerror(errno));

        BOOST_REQUIRE(res.first >= 0);

        //progress += res.first;

        buf.read_and_crunch(res.first);

        file.AsyncRead(buf.wr_ptr(), buf.capacity());
    }
}
