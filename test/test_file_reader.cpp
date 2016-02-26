/**
 * \file
 * \brief Test cases for classes and functions in the file_reader.hpp
 *
 * \author Dmitriy Kargapolov <dmitriy dot kargapolov at gmail dot com>
 * \since 28 Jul 2012
 *
 * Copyright (C) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Copyright (C) 2012 Serge Aleynikov <saleyn@gmail.com>
 *
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <utxx/file_writer.hpp>
#include <utxx/file_reader.hpp>
#include <boost/test/unit_test.hpp>

#include <boost/assign/std/list.hpp>
using namespace boost::assign;

namespace file_reader_test {

struct string_codec {

    typedef std::string data_t;

    // Encoder
    size_t operator()(const data_t& a_msg, char *a_buf, size_t a_len) {
        size_t l = a_msg.size();
        size_t h = sizeof(size_t);
        size_t n = h + l;
        if (n > a_len)
            return 0;
        *((size_t*)((void*)a_buf)) = l;
        a_msg.copy(a_buf + h, l);
        return n;
    }

    // Decoder
    ssize_t operator()(data_t& a_msg, const char *a_buf, size_t a_len, size_t a_offset) const {
        // BOOST_TEST_MESSAGE( getpid() << ": decode at " << a_offset );
        size_t h = sizeof(size_t);
        if (a_len < h)
            return 0;
        size_t l = *((size_t*)((void*)a_buf));
        size_t n = h + l;
        if (n > a_len)
            return 0;
        a_msg.assign(a_buf + h, l);
        return n;
    }
};

size_t write_file1(const char *a_fname, std::list<std::string>& a_lst) {
    string_codec codec;
    std::ofstream l_file(a_fname, std::ios::out | std::ios::binary | std::ios::app);
    char l_buf[64];
    size_t total = 0;
    for(auto& a_str : a_lst) {
        size_t n = codec(a_str, l_buf, sizeof(l_buf));
        if (!n)
            return 0;
        l_file.write(l_buf, n);
        total += n;
    }
    l_file.close();
    return total;
}

typedef utxx::data_file_writer<string_codec> writer_t;
typedef utxx::data_file_reader<string_codec> reader_t;
typedef reader_t::iterator it_t;

size_t write_file(const char *a_fname, std::list<std::string>& a_lst) {
    writer_t l_writer(a_fname, true);
    for(auto& a_str : a_lst)
        l_writer.push_back(a_str);
    return l_writer.data_offset();
}

struct f0 {
    const char* fname;
    f0() : fname("file.dat") {}
    ~f0() { unlink(fname); }
};

struct f1 {
    const char* fname;
    std::list<std::string> in;
    f1() : fname("file.dat") {
        in += "couple", "more", "strings", "about", "nothing";
        write_file(fname, in);
    }
    ~f1() {
        unlink(fname);
    }
};

BOOST_AUTO_TEST_SUITE( test_file_reader )

BOOST_AUTO_TEST_CASE( exceptions )
{
    // unexisting file case
    BOOST_REQUIRE_THROW(reader_t r("hf/sdf/hfhd/fvdfk"), std::ifstream::failure);
    // non-regular file case
    reader_t r("/");
    BOOST_REQUIRE_THROW(r.begin(), std::ifstream::failure);
}

BOOST_FIXTURE_TEST_CASE( simple_write, f0 )
{
    std::list<std::string> lst;
    lst += "couple", "strings";
    size_t n = write_file(fname, lst);
    size_t exp = 2 * sizeof(size_t) + 6 + 7;
    BOOST_REQUIRE_EQUAL(exp, n);
}

BOOST_FIXTURE_TEST_CASE( initial_value, f1 ) {
    reader_t r;
    r.open(fname);

    it_t it = r.begin();
    it_t e = r.end();
    BOOST_REQUIRE_EQUAL(false, it == e);
    BOOST_REQUIRE_EQUAL(true, it != e);

    it_t it1 = r.begin(), e1 = r.end();
    BOOST_REQUIRE_EQUAL(false, it1 == e1);
    BOOST_REQUIRE_EQUAL(true, it1 != e1);

    reader_t r2;
    BOOST_REQUIRE_EQUAL((r2.begin() == r2.end()), true);
}

BOOST_FIXTURE_TEST_CASE( simple_read, f1 )
{
    reader_t r(fname);
    std::list<std::string> out;
    for (it_t it = r.begin(), e = r.end(); it != e; ++it) {
        out.push_back(*it);
    }
    BOOST_REQUIRE_EQUAL_COLLECTIONS(in.begin(), in.end(), out.begin(), out.end());
}

BOOST_FIXTURE_TEST_CASE( foreach, f1 )
{
    reader_t r(fname);
    std::list<std::string> out;
    for(auto& s : r)
        out.push_back(s);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(in.begin(), in.end(), out.begin(), out.end());
}

BOOST_FIXTURE_TEST_CASE( foreach_2, f1 )
{
    reader_t r(fname);
    std::list<std::string> out;
    // read first two elements
    int k = 0;
    for(auto& s : r) {
        if (++k > 2) break;
        out.push_back(s);
    }
    // read rest of elements
    for(auto& s : r)
        out.push_back(s);
    // compare...
    BOOST_REQUIRE_EQUAL_COLLECTIONS(in.begin(), in.end(), out.begin(), out.end());
}

BOOST_FIXTURE_TEST_CASE( fork_writer, f0 )
{
    std::list<std::string> in;
    in += "couple", "strings", "more";

    int c2p[2], p2c[2];
    (void)pipe(c2p);
    (void)pipe(p2c);
    int marker = 0;

    pid_t pid = fork();

    if (pid == 0) {
        close(c2p[0]);
        close(p2c[1]);

        BOOST_TEST_MESSAGE( "CHILD: 1st writing..." );
        write_file(fname, in);

        BOOST_TEST_MESSAGE( "CHILD: 1st write done, signalling..." );
        (void)write(c2p[1], &marker, sizeof(marker));

        BOOST_TEST_MESSAGE( "CHILD: 1st write done, waiting..." );
        (void)read(p2c[0], &marker, sizeof(marker));

        write_file(fname, in);

        BOOST_TEST_MESSAGE( "CHILD: 2nd write done, signalling..." );
        (void)write(c2p[1], &marker, sizeof(marker));

        exit(0);

    } else {
        close(c2p[1]);
        close(p2c[0]);

        BOOST_TEST_MESSAGE( "PARNT: waiting for 1st write to complete..." );
        (void)read(c2p[0], &marker, sizeof(marker));

        BOOST_TEST_MESSAGE( "PARNT: got marker, reading..." );

        reader_t r(fname, 0);
        std::list<std::string> out;
        for(auto& s : r) out.push_back(s);
        BOOST_TEST_MESSAGE( "PARNT: compare collections" );
        BOOST_REQUIRE_EQUAL_COLLECTIONS(in.begin(), in.end(), out.begin(), out.end());

        BOOST_TEST_MESSAGE( "PARNT: 1st read done, signalling..." );
        (void)write(p2c[1], &marker, sizeof(marker));

        BOOST_TEST_MESSAGE( "PARNT: waiting for 2nd write to complete..." );
        (void)read(c2p[0], &marker, sizeof(marker));

        BOOST_TEST_MESSAGE( "PARNT: got marker, reading..." );

        out.clear();
        for(auto& s : r) out.push_back(s);
        BOOST_TEST_MESSAGE( "PARNT: compare collections" );
        BOOST_REQUIRE_EQUAL_COLLECTIONS(in.begin(), in.end(), out.begin(), out.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace file_reader_test
