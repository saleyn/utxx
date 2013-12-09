// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief Test cases for persistent trie in aho-corasick mode
 *
 * \author Dmitriy Kargapolov <dmitriy dot kargapolov at gmail dot com>
 * \since 27 September 2013
 *
 * Copyright (C) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 *
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <config.h>
#include <utxx/pnode_ss.hpp>
#include <utxx/pnode_ss_ro.hpp>
#include <utxx/ptrie.hpp>
#include <utxx/mmap_ptrie.hpp>
#include <utxx/simple_node_store.hpp>
#include <utxx/flat_data_store.hpp>
#include <utxx/svector.hpp>
#include <utxx/sarray.hpp>

#include <set>

#include <boost/test/unit_test.hpp>

#if defined HAVE_BOOST_CHRONO
#include <boost/chrono/system_clocks.hpp>
#endif

namespace actrie_test {

#if defined HAVE_BOOST_CHRONO
typedef boost::chrono::high_resolution_clock clock;
typedef clock::time_point time_point;
typedef clock::duration duration;
using boost::chrono::nanoseconds;
using boost::chrono::microseconds;
using boost::chrono::milliseconds;
using boost::chrono::duration_cast;
#endif

#define NTAGS 1000
#define NSAMPLES 100000

// generate random string of N..2*N-1 digits
template<int N>
static const char *make_number() {
    static char buf[N+N];
    static const char digits[] = "0123456789";
    int n = N + rand() % N;
    for (int i=0; i<n; ++i)
        buf[i] = digits[rand() % (sizeof(digits) - 1)];
    buf[n] = 0;
    return &buf[0];
}

typedef std::set<std::string> set_t;
typedef std::list<std::string> ret_t;
typedef uint32_t offset_t;

struct f0 {
    // expandable aho-corasick trie node (with suffix link and shift field)
    typedef utxx::pnode_ss<
        utxx::simple_node_store<>, std::string, utxx::svector<>
    > node_t;

    // expandable aho-corasick trie type
    typedef utxx::ptrie<node_t> trie_t;

    // node store type
    typedef typename trie_t::store_t store_t;

    // fold functor to gather matched tags
    static bool lookup(ret_t& ret, const std::string& data,
            const store_t&, uint32_t, uint32_t, bool) {
        if (!data.empty())
            ret.push_back(data);
        return true;
    }
};

struct f1 {
    // export variant
    struct data {
        std::string str;
        data() : str("") {}
        data(const char *s) : str(s) {}
        bool empty() const { return str.empty(); }

        // data header in exported format
        template<typename OffsetType> struct ext_header {
            // offset to variable-length payload
            OffsetType offset;
            // write data header to file
            void write_to_file(std::ofstream& a_ofs) const {
                a_ofs.write((const char *)&offset, sizeof(offset));
            }
        };

        // write nested data payload to file, fill header
        template<typename OffsetType, typename Store>
        void write_to_file(ext_header<OffsetType>& hdr, const Store&,
                std::ofstream& f) const {
            uint8_t n = str.size();
            if (n > 0) {
                hdr.offset =
                    boost::numeric_cast<OffsetType, std::streamoff>(f.tellp());
                f.write((const char *)&n, sizeof(n));
                f.write((const char *)str.c_str(), n + 1);
            } else {
                hdr.offset = 0;
            }
        }
    };

    // expandable trie with export functions
    typedef utxx::pnode_ss<
        utxx::simple_node_store<>, data, utxx::svector<>, offset_t
    > node_t;

    // expandable trie type with export functions
    typedef utxx::ptrie<node_t> trie_t;
};

struct f2 {
    struct data {
        uint8_t len;
        char str[0];
    };
    typedef utxx::pnode_ss_ro<
        utxx::flat_data_store<void, offset_t>, offset_t, utxx::sarray<>
    > node_t;
    typedef utxx::mmap_ptrie<node_t> trie_t;
    typedef typename trie_t::store_t store_t;

    static offset_t root(const void *m_addr, size_t m_size) {
        size_t s = sizeof(offset_t);
        if (m_size < s) throw std::runtime_error("no space for root");
        offset_t ret = *(const offset_t *)((const char *)m_addr + m_size - s);
        return ret;
    }

    // fold functor to gather matched tags
    static
    bool lookup(ret_t& ret, offset_t off, const store_t& store,
            uint32_t, uint32_t, bool) {
        if (off == store_t::null)
            return true;
        data *ptr = store.native_pointer<data>(off);
        if (ptr == NULL)
            throw std::runtime_error("bad store pointer");
        ret.push_back(ptr->str);
        return true;
    }

    // fold functor to find first match
    static bool find_first(const data*& ret, offset_t off, const store_t& store,
            uint32_t, uint32_t, bool) {
        if (off == store_t::null)
            return true;
        data *ptr = store.native_pointer<data>(off);
        if (ptr == NULL)
            throw std::runtime_error("bad store pointer");
        ret = ptr;
        return false;
    }
};

BOOST_AUTO_TEST_SUITE( test_actrie )

BOOST_FIXTURE_TEST_CASE( isolate_test, f0 )
{
    trie_t trie;
    trie.store("123", "123");
    trie.store("567", "567");
    trie.make_links();

    ret_t ret;
    std::string exp[] = {"123", "567"};
    trie.fold_full("012345678", ret, lookup);
    BOOST_REQUIRE_EQUAL_COLLECTIONS ( ret.begin(), ret.end(), exp, exp + 2 );
}

BOOST_FIXTURE_TEST_CASE( overlap_test, f0 )
{
    trie_t trie;
    trie.store("123", "123");
    trie.store("345", "345");
    trie.make_links();

    ret_t ret;
    std::string exp[] = {"123", "345"};
    trie.fold_full("0123456", ret, lookup);
    BOOST_REQUIRE_EQUAL_COLLECTIONS ( ret.begin(), ret.end(), exp, exp + 2 );
}

BOOST_FIXTURE_TEST_CASE( include_test, f0 )
{
    trie_t trie;
    trie.store("1234", "1234");
    trie.store("23", "23");
    trie.make_links();

    ret_t ret;
    std::string exp[] = {"23", "1234"};
    trie.fold_full("012345", ret, lookup);
    BOOST_REQUIRE_EQUAL_COLLECTIONS ( ret.begin(), ret.end(), exp, exp + 2 );
}

BOOST_FIXTURE_TEST_CASE( recurring_pattern_test, f0 )
{
    trie_t trie;
    trie.store("232323", "232323");
    trie.store("323232", "323232");
    trie.make_links();

    ret_t ret;
    std::string exp[] = {
        "232323", "323232", "232323", "323232", "232323",
        "323232", "232323", "323232", "232323"
    };
    trie.fold_full("23232323232323", ret, lookup);
    BOOST_REQUIRE_EQUAL_COLLECTIONS ( ret.begin(), ret.end(), exp, exp + 9 );
}

BOOST_FIXTURE_TEST_CASE( random_test, f0 )
{
    trie_t trie;
    set_t tags;

    // making unique tags to search for
    srand(1);
    for (int i=0; i<NTAGS; ++i) {
        const char *num = make_number<4>();
        trie.store(num, num);
        tags.insert(num);
    }
    trie.make_links();

    BOOST_TEST_MESSAGE( "querying actrie agains random strings" );

    // looking for tags in random strings
    for (int i=0; i<NSAMPLES; ++i) {
        ret_t ret;
        const char *num = make_number<15>();
        trie.fold_full(num, ret, lookup);
        ret_t exp;
        BOOST_FOREACH(const std::string& s, tags) {
            const char *p = num;
            while ((p = strstr(p, s.c_str())) != 0) {
                exp.push_back(s);
                ++p;
            }
        }
        exp.sort();
        ret.sort();
        BOOST_CHECK_EQUAL_COLLECTIONS ( ret.begin(), ret.end(),
                exp.begin(), exp.end() );
        if (exp.size() != ret.size()) {
            std::cout << "returned: ";
            std::copy(ret.begin(), ret.end(),
                std::ostream_iterator<std::string>(std::cout, " "));
            std::cout << std::endl;
            std::cout << "expected: ";
            std::copy(exp.begin(), exp.end(),
                std::ostream_iterator<std::string>(std::cout, " "));
            std::cout << std::endl;
        }
    }
}

BOOST_FIXTURE_TEST_CASE( prepare_and_write_test, f1 )
{
    trie_t trie;

    BOOST_TEST_MESSAGE( "generating actrie" );
    srand(1);
    for (int i=0; i<NTAGS; ++i) {
        const char *num = make_number<4>();
        trie.store(num, data(num));
    }
    trie.make_links();

    BOOST_TEST_MESSAGE( "writing actrie to file" );
    BOOST_REQUIRE_NO_THROW(( trie.write_to_file<offset_t>("pepepe") ));
}

BOOST_FIXTURE_TEST_CASE( mmap_test, f2 )
{
    trie_t trie("pepepe", root);
    set_t tags;

    // making unique tags to search for
    srand(1);
    for (int i=0; i<NTAGS; ++i)
        tags.insert(make_number<4>());

    BOOST_TEST_MESSAGE( "querying mmap_actrie agains random strings" );

    // looking for tags in random strings
    for (int i=0; i<NSAMPLES; ++i) {
        ret_t ret;
        const char *num = make_number<15>();
        trie.fold_full(num, ret, lookup);
        ret_t exp;
        BOOST_FOREACH(const std::string& s, tags) {
            const char *p = num;
            while ((p = strstr(p, s.c_str())) != 0) {
                exp.push_back(s);
                ++p;
            }
        }
        exp.sort();
        ret.sort();
        BOOST_CHECK_EQUAL_COLLECTIONS ( ret.begin(), ret.end(),
                exp.begin(), exp.end() );
        if (exp.size() != ret.size()) {
            std::cout << "returned: ";
            std::copy(ret.begin(), ret.end(),
                std::ostream_iterator<std::string>(std::cout, " "));
            std::cout << std::endl;
            std::cout << "expected: ";
            std::copy(exp.begin(), exp.end(),
                std::ostream_iterator<std::string>(std::cout, " "));
            std::cout << std::endl;
        }
    }
}

#if defined HAVE_BOOST_CHRONO

BOOST_FIXTURE_TEST_CASE( chrono_mmap_test, f2 )
{
    trie_t trie("pepepe", root);
    set_t tags;

    // making unique tags to search for
    srand(1);
    for (int i=0; i<NTAGS; ++i)
        tags.insert(make_number<4>());

    BOOST_TEST_MESSAGE( "measuring mmap_actrie full lookup time" );

    time_point tp = clock::now();

    // looking for tags in random strings
    for (int i=0; i<NSAMPLES; ++i) {
        ret_t ret;
        const char *num = make_number<15>();
        trie.fold_full(num, ret, lookup);
    }

    duration d = (clock::now() - tp) / NSAMPLES;

    BOOST_TEST_MESSAGE( "mmap_actrie full lookup time "
        << duration_cast<nanoseconds>(d).count() << " ns" );
}

BOOST_FIXTURE_TEST_CASE( chrono_mmap_test_2, f2 )
{
    trie_t trie("pepepe", root);
    set_t tags;

    // making unique tags to search for
    srand(1);
    for (int i=0; i<NTAGS; ++i)
        tags.insert(make_number<4>());

    BOOST_TEST_MESSAGE( "measuring mmap_actrie first match lookup time" );

    time_point tp = clock::now();

    // looking for tags in random strings
    for (int i=0; i<NSAMPLES; ++i) {
        const data *ret = 0;
        const char *num = make_number<15>();
        trie.fold_full(num, ret, find_first);
    }

    duration d = (clock::now() - tp) / NSAMPLES;

    BOOST_TEST_MESSAGE( "mmap_actrie first match lookup time "
        << duration_cast<nanoseconds>(d).count() << " ns" );
}

#endif // HAVE_BOOST_CHRONO

BOOST_AUTO_TEST_SUITE_END()

} // namespace actrie_test
