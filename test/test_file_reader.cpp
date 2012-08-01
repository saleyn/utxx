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

#include <boost/test/unit_test.hpp>
#include <util/file_reader.hpp>

#include <boost/assign/std/list.hpp>
using namespace boost::assign;

namespace file_reader_test {

struct string_codec {

    typedef std::string data_t;

    size_t encode(const data_t& a_msg, char *a_buf, size_t a_len) {
        size_t l = a_msg.size();
        size_t h = sizeof(size_t);
        size_t n = h + l;
        if (n > a_len)
            return 0;
        *((size_t*)((void*)a_buf)) = l;
        a_msg.copy(a_buf + h, l);
        return n;
    }

    ssize_t decode(data_t& a_msg, const char *a_buf, size_t a_len, size_t) const {
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


size_t write_file(const char *a_fname, std::list<std::string>& a_lst) {
    string_codec codec;
    std::ofstream l_file(a_fname, std::ios::out | std::ios::binary);
    char l_buf[64];
    size_t total = 0;
    BOOST_FOREACH(std::string a_str, a_lst) {
        size_t n = codec.encode(a_str, l_buf, sizeof(l_buf));
        if (!n)
            return 0;
        l_file.write(l_buf, n);
        total += n;
    }
    l_file.close();
    return total;
}

typedef util::data_file_reader<string_codec> reader_t;
typedef typename reader_t::iterator it_t;

const char* fname = "file.dat";

BOOST_AUTO_TEST_SUITE( test_file_reader )

BOOST_AUTO_TEST_CASE( simple_write )
{
    std::list<std::string> lst;
    lst += "couple", "strings";
    size_t n = write_file(fname, lst);
    size_t exp = 2 * sizeof(size_t) + 6 + 7;
    BOOST_REQUIRE_EQUAL(exp, n);
}

BOOST_AUTO_TEST_CASE( exceptions )
{
    // unexisting file case
    BOOST_REQUIRE_THROW(reader_t r("hf/sdf/hfhd/fvdfk"), std::ifstream::failure);
    // non-regular file case
    reader_t r("/");
    BOOST_REQUIRE_THROW(it_t it = r.begin(), std::ifstream::failure);
}

BOOST_AUTO_TEST_CASE( initial_value ) {
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
    it_t it2 = r2.begin(), e2 = r2.end();
    BOOST_REQUIRE_EQUAL(true, it2 == e2);
    BOOST_REQUIRE_EQUAL(false, it2 != e2);
}

BOOST_AUTO_TEST_CASE( simple_read )
{
    // writing sequence
    std::list<std::string> in;
    in += "couple", "strings";
    size_t n = write_file(fname, in);
    size_t exp = 2 * sizeof(size_t) + 6 + 7;
    BOOST_REQUIRE_EQUAL(exp, n);

    // reading sequence
    reader_t r(fname);
    std::list<std::string> out;
    for (it_t it = r.begin(), e = r.end(); it != e; ++it) {
        out.push_back(*it);
    }

    BOOST_REQUIRE_EQUAL_COLLECTIONS(in.begin(), in.end(), out.begin(), out.end());
}

BOOST_AUTO_TEST_CASE( foreach )
{
    const char* fname = "file.dat";

    // writing sequence
    std::list<std::string> in;
    in += "couple", "more", "strings", "about", "nothing";
    write_file(fname, in);

    // reading sequence
    reader_t r(fname);
    std::list<std::string> out;
    BOOST_FOREACH(std::string& s, r) {
        out.push_back(s);
    }

    BOOST_REQUIRE_EQUAL_COLLECTIONS(in.begin(), in.end(), out.begin(), out.end());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace file_reader_test
