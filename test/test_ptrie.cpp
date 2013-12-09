// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief Test cases for persistent trie
 *
 * \author Dmitriy Kargapolov <dmitriy dot kargapolov at gmail dot com>
 * \since 06 October 2013
 *
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 *
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <config.h>
#include <utxx/pnode.hpp>
#include <utxx/pnode_ro.hpp>
#include <utxx/ptrie.hpp>
#include <utxx/mmap_ptrie.hpp>
#include <utxx/simple_node_store.hpp>
#include <utxx/flat_data_store.hpp>
#include <utxx/svector.hpp>
#include <utxx/sarray.hpp>
#include <utxx/memstat_alloc.hpp>

#include <boost/test/unit_test.hpp>

#if defined HAVE_BOOST_CHRONO
#include <boost/chrono/system_clocks.hpp>
#endif

#include <boost/unordered_map.hpp>
#include <map>

namespace ptrie_test {

#if defined HAVE_BOOST_CHRONO
typedef boost::chrono::high_resolution_clock clock;
typedef clock::time_point time_point;
typedef clock::duration duration;
using boost::chrono::nanoseconds;
using boost::chrono::microseconds;
using boost::chrono::milliseconds;
using boost::chrono::duration_cast;
#endif

#define NSAMPLES 1000000

template<int N>
struct memstat {
    static void inc(size_t n) { cnt += n; }
    static void dec(size_t n) { cnt -= n; }
    static size_t cnt;
};

template<int N>
size_t memstat<N>::cnt = 0;

// generate random string of N..2*N-1 digits
template<int N>
static const char *make_number(int *cnt = 0) {
    static char buf[N+N];
    static const char digits[] = "0123456789";
    int n = N + rand() % N;
    for (int i=0; i<n; ++i)
        buf[i] = digits[rand() % (sizeof(digits) - 1)];
    if (cnt) *cnt += n;
    buf[n] = 0;
    return &buf[0];
}

typedef uint32_t offset_t;

struct f0 {

    // memory counters
    enum { cKey, cData, cTabData, cMap, cStore, cTrie };

    typedef utxx::memstat_alloc<char, memstat<cKey> > key_alloc;
    typedef std::basic_string<char, std::char_traits<char>, key_alloc> key_t;

    typedef utxx::memstat_alloc<char, memstat<cData> > data_alloc;
    typedef std::basic_string<char, std::char_traits<char>, data_alloc> data_t;

    typedef utxx::memstat_alloc<char, memstat<cTabData> > tab_data_alloc;
    typedef std::basic_string<char, std::char_traits<char>, tab_data_alloc>
        tab_data_t;

    typedef std::pair<const key_t, tab_data_t> pair_t;
    typedef utxx::memstat_alloc<pair_t, memstat<cMap> > map_alloc;

    typedef std::map<key_t, tab_data_t, std::less<key_t>, map_alloc> map_t;

    typedef boost::unordered_map<key_t, tab_data_t, boost::hash<key_t>,
                                std::equal_to<key_t>, map_alloc> tab_t;
    typedef typename tab_t::const_iterator tab_it_t;

    typedef utxx::memstat_alloc<char, memstat<cStore> > node_alloc;
    typedef utxx::memstat_alloc<char, memstat<cTrie> > trie_alloc;

    // expandable trie node type
    typedef utxx::pnode<
        utxx::simple_node_store<void, node_alloc>,
        data_t,
        utxx::svector<char, utxx::idxmap<1>, trie_alloc>
    > node_t;

    // expandable trie type
    typedef utxx::ptrie<node_t> trie_t;

    // node store type
    typedef typename trie_t::store_t store_t;

    // fold functor to perform lookup
    static bool lookup(const data_t*& ret, const data_t& data, store_t&,
            uint32_t, bool) {
        if (!data.empty())
            ret = &data;
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
    typedef utxx::pnode<
        utxx::simple_node_store<>, data, utxx::svector<>
    > node_t;

    // expandable trie type with export functions
    typedef utxx::ptrie<node_t> trie_t;
};

struct f2 {
    struct data {
        uint8_t m_len;
        char m_str[0];
        bool empty() const { return false; }
        bool empty(bool exact) const { return !exact; }
    };
    typedef utxx::pnode_ro<
        utxx::flat_data_store<void, offset_t>, offset_t, utxx::sarray<>
    > node_t;
    typedef utxx::mmap_ptrie<node_t> trie_t;
    typedef typename trie_t::store_t store_t;

    static offset_t root(const void *m_addr, size_t m_size) {
        size_t s = sizeof(offset_t);
        if (m_size < s) throw std::runtime_error("no space for root");
        return *(const offset_t *)((const char *)m_addr + m_size - s);
    }

    // fold functor to save exact lookup result in a string
    static bool copy_exact_f(std::string &acc, offset_t off,
            store_t& store, uint32_t, bool has_next) {
        if (has_next || off == store_t::null)
            return true;
        data *ptr = store.native_pointer<data>(off);
        if (ptr == NULL)
            throw std::runtime_error("bad store pointer");
        acc.assign(ptr->m_str, ptr->m_len);
        return false;
    }

    // fold functor to perform simple lookup
    static bool lookup_simple(const data*& ret, offset_t off,
            store_t& store, uint32_t, bool) {
        if (off == store_t::null)
            return true;
        data *ptr = store.native_pointer<data>(off);
        if (ptr == NULL)
            throw std::runtime_error("bad store pointer");
        ret = ptr;
        return true;
    }

    // fold functor to perform exact lookup
    static bool lookup_exact(const data*& ret, offset_t off,
            store_t& store, uint32_t, bool has_next) {
        if (has_next || off == store_t::null)
            return true;
        data *ptr = store.native_pointer<data>(off);
        if (ptr == NULL)
            throw std::runtime_error("bad store pointer");
        if (!ptr->empty())
            ret = ptr;
        return false;
    }

    // fold functor to perform a lookup
    static bool lookup(const data*& ret, offset_t off,
            store_t& store, uint32_t, bool) {
        if (off == store_t::null)
            return true;
        data *ptr = store.native_pointer<data>(off);
        if (ptr == NULL)
            throw std::runtime_error("bad store pointer");
        if (!ptr->empty())
            ret = ptr;
        return true;
    }
};

BOOST_AUTO_TEST_SUITE( test_ptrie )

BOOST_FIXTURE_TEST_CASE( write_read_test, f0 )
{
    { // start objects' life

    int l_total = NSAMPLES;
    int l_cnt = 0;

    memstat<cData>::cnt = 0;
    memstat<cStore>::cnt = 0;
    memstat<cTrie>::cnt = 0;
    memstat<cKey>::cnt = 0;
    memstat<cTabData>::cnt = 0;
    memstat<cMap>::cnt = 0;

    trie_t l_data;
    tab_t l_tab;

    srand(1);

    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>(&l_cnt);
        // insert data into s-trie
        l_data.store(l_num, data_t(l_num));
        // insert data into unordered map (hash-table)
        l_tab.insert(pair_t(l_num, l_num));
    }

    size_t l_trie_cnt =
        memstat<cData>::cnt + memstat<cStore>::cnt + memstat<cTrie>::cnt;
    size_t l_htab_cnt =
        memstat<cKey>::cnt + memstat<cTabData>::cnt + memstat<cMap>::cnt;
    BOOST_TEST_MESSAGE(
        "\n      unique objects count: " << l_tab.size() <<
        "\ntrie: num of chars in keys: " << l_cnt <<
        "\n" <<
        "\ntrie: data bytes allocated: " << memstat<cData>::cnt <<
        "\ntrie:      nodes allocated: " << l_data.store().count() <<
        "\ntrie: node bytes allocated: " << memstat<cStore>::cnt <<
        "\ntrie:       bytes per node: " <<
                memstat<cStore>::cnt / l_data.store().count() <<
        "\ntrie: nptr bytes allocated: " << memstat<cTrie>::cnt <<
        "\ntrie: total byte allocated: " << l_trie_cnt <<
        "\ntrie:     bytes per object: " << l_trie_cnt / l_tab.size() <<
        "\n" <<
        "\nhtab:  key bytes allocated: " << memstat<cKey>::cnt <<
        "\nhtab: data bytes allocated: " << memstat<cTabData>::cnt <<
        "\nhtab:  tab bytes allocated: " << memstat<cMap>::cnt <<
        "\nhtab: total byte allocated: " << l_htab_cnt <<
        "\nhtab:     bytes per object: " << l_htab_cnt / l_tab.size() <<
        "\n"
    );

    // looking for random matches
    srand(123);
    int l_found = 0, l_exact = 0;
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>();
        // direct data lookup
        const data_t *l_data_ptr = 0;
        l_data.fold(l_num, l_data_ptr, lookup);
        // if data found
        if (l_data_ptr) {
            // full or substring match only
            BOOST_REQUIRE_EQUAL(0,
                strncmp(l_num, l_data_ptr->c_str(), l_data_ptr->length()));
            ++l_found;
            if (!strcmp(l_num, l_data_ptr->c_str()))
                ++l_exact;
        }
    }
    BOOST_TEST_MESSAGE( "from " << l_total << " found: " << l_found
        << ", exact: " << l_exact );

    // compare full strings matches to hash table
    BOOST_FOREACH(pair_t& p, l_tab) {
        const data_t *l_data_ptr = 0;
        l_data.fold(p.first.c_str(), l_data_ptr, lookup);
        BOOST_REQUIRE(l_data_ptr != 0);
        BOOST_REQUIRE_EQUAL(0, strcmp(p.second.c_str(), l_data_ptr->c_str()));
    }

    } // end of all objects life

    // make sure all memory released
    BOOST_REQUIRE_EQUAL(0, memstat<cData>::cnt );
    BOOST_REQUIRE_EQUAL(0, memstat<cStore>::cnt );
    BOOST_REQUIRE_EQUAL(0, memstat<cTrie>::cnt );
    BOOST_REQUIRE_EQUAL(0, memstat<cKey>::cnt );
    BOOST_REQUIRE_EQUAL(0, memstat<cTabData>::cnt );
    BOOST_REQUIRE_EQUAL(0, memstat<cMap>::cnt );
}

BOOST_FIXTURE_TEST_CASE( compact_test, f1 )
{
    trie_t l_data;

    int l_total = NSAMPLES;
    srand(1);

    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>();
        // insert data into s-trie
        l_data.store(l_num, data(l_num));
    }

    BOOST_REQUIRE_NO_THROW((
        l_data.write_to_file<offset_t>("lalala")
    ));
}

BOOST_FIXTURE_TEST_CASE( mmap_test, f2 )
{
    trie_t l_trie("lalala", root);
    BOOST_TEST_MESSAGE( "reading trie from file" );

    // looking for random matches
    int l_total = NSAMPLES;
    srand(123);
    int l_found = 0, l_exact = 0;
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>();
        const data *l_data_ptr = 0;
        l_trie.fold(l_num, l_data_ptr, lookup_simple);
        if (l_data_ptr) {
            // full or substring match only
            BOOST_REQUIRE_EQUAL(0,
                strncmp(l_num, l_data_ptr->m_str, l_data_ptr->m_len));
            ++l_found;
            if (!strcmp(l_num, l_data_ptr->m_str))
                ++l_exact;
        }
    }
    BOOST_TEST_MESSAGE( "from " << l_total << " found: " << l_found
        << ", exact: " << l_exact );

    // looking for random exact matches
    srand(123); l_found = 0;
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>();
        // direct data lookup
        const data *l_data_ptr = 0;
        l_trie.fold(l_num, l_data_ptr, lookup_exact);
        // alternative way to find data
        std::string l_ret;
        l_trie.fold(l_num, l_ret, copy_exact_f);
        if (l_data_ptr) {
            // check results are equal
            BOOST_REQUIRE_EQUAL(0, strcmp(l_ret.c_str(), l_data_ptr->m_str));
            // full match only
            BOOST_REQUIRE_EQUAL(0, strcmp(l_num, l_data_ptr->m_str));
            ++l_found;
        } else {
            BOOST_REQUIRE_EQUAL(0, l_ret.size());
        }
    }
    BOOST_TEST_MESSAGE( "from " << l_total << " found: " << l_found );

    BOOST_REQUIRE_EQUAL(l_exact, l_found);

    // compare full strings matches
    l_total = NSAMPLES;
    srand(1);
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>();
        const data *l_data_ptr = 0;
        l_trie.fold(l_num, l_data_ptr, lookup_simple);
        BOOST_REQUIRE(l_data_ptr != 0);
        BOOST_REQUIRE_EQUAL(0, strcmp(l_num, l_data_ptr->m_str));
    }
    BOOST_TEST_MESSAGE( l_total << " full strings matched" );
}

#if defined HAVE_BOOST_CHRONO

BOOST_FIXTURE_TEST_CASE( chrono_test, f0 )
{
    trie_t l_data;
    int l_total = NSAMPLES;
    int l_cnt = 0;

    time_point tp1 = clock::now();
    for (int i=0; i<l_total; ++i) make_number<5>(&l_cnt);
    duration d1 = clock::now() - tp1;

    srand(1);
    l_cnt = 0;

    map_t l_map;
    time_point tp2 = clock::now();
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>(&l_cnt);
        l_map.insert(pair_t(l_num, l_num));
    }
    duration d2 = (clock::now() - tp2 - d1) / l_total;

    BOOST_TEST_MESSAGE( "map insert time "
        << duration_cast<nanoseconds>(d2).count() << " ns" );

    srand(1);
    l_cnt = 0;
    memstat<cKey>::cnt = 0;
    memstat<cData>::cnt = 0;
    memstat<cMap>::cnt = 0;

    tab_t l_tab;
    time_point tp2a = clock::now();
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>(&l_cnt);
        l_tab.insert(pair_t(l_num, l_num));
    }
    duration d2a = (clock::now() - tp2a - d1) / l_total;

    BOOST_TEST_MESSAGE( "tab insert time "
        << duration_cast<nanoseconds>(d2a).count() << " ns" );

    srand(1);
    l_cnt = 0;
    memstat<cKey>::cnt = 0;
    memstat<cData>::cnt = 0;

    time_point tp3 = clock::now();
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>(&l_cnt);
        l_data.store(l_num, data_t(l_num));
    }
    duration d3 = (clock::now() - tp3 - d1) / l_total;

    BOOST_TEST_MESSAGE( "trie insert time "
        << duration_cast<nanoseconds>(d3).count() << " ns" );

    srand(123);
    const data_t *l_data_ptr = 0;
    time_point tp4 = clock::now();
    for (int i=0; i<l_total; ++i)
        l_data.fold(make_number<5>(), l_data_ptr, lookup);
    duration d4 = (clock::now() - tp4 - d1) / l_total;
    BOOST_TEST_MESSAGE( "trie lookup time "
        << duration_cast<nanoseconds>(d4).count() << " ns" );

    srand(123);
    time_point tp5 = clock::now();
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>();
        l_map.find(l_num);
    }
    duration d5 = (clock::now() - tp5 - d1) / l_total;
    BOOST_TEST_MESSAGE( "map lookup time "
        << duration_cast<nanoseconds>(d5).count() << " ns" );

    srand(123);
    time_point tp6 = clock::now();
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>();
        l_tab.find(l_num);
    }
    duration d6 = (clock::now() - tp6 - d1) / l_total;
    BOOST_TEST_MESSAGE( "tab lookup time "
        << duration_cast<nanoseconds>(d6).count() << " ns" );

    srand(123);
    time_point tp7 = clock::now();
    tab_it_t e = l_tab.end();
    for (int i=0; i<l_total; ++i) {
        const char *l_num = make_number<5>();
        size_t n = strlen(l_num);
        while (n > 0) {
            key_t l_key(l_num, n);
            tab_it_t it = l_tab.find(l_num);
            if (it != e)
                break;
            --n;
        }
    }
    duration d7 = (clock::now() - tp7 - d1) / l_total;
    BOOST_TEST_MESSAGE( "tab extended lookup time "
        << duration_cast<nanoseconds>(d7).count() << " ns" );
}

BOOST_FIXTURE_TEST_CASE( chrono_mmap_test, f2 )
{
    trie_t l_trie("lalala", root);
    int l_total = NSAMPLES;
    srand(123);
    time_point tp0 = clock::now();
    for (int i=0; i<l_total; ++i)
        make_number<5>();
    duration d0 = clock::now() - tp0;
    srand(123);
    const data *l_data_ptr = 0;
    time_point tp1 = clock::now();
    for (int i=0; i<l_total; ++i)
        l_trie.fold(make_number<5>(), l_data_ptr, lookup);
    duration d = (clock::now() - tp1 - d0) / l_total;
    BOOST_TEST_MESSAGE( "mmap_trie lookup time "
        << duration_cast<nanoseconds>(d).count() << " ns" );
}

BOOST_FIXTURE_TEST_CASE( chrono_mmap_test_simple, f2 )
{
    trie_t l_trie("lalala", root);
    int l_total = NSAMPLES;
    srand(123);
    time_point tp0 = clock::now();
    for (int i=0; i<l_total; ++i)
        make_number<5>();
    duration d0 = clock::now() - tp0;
    srand(123);
    const data *l_data_ptr = 0;
    time_point tp1 = clock::now();
    for (int i=0; i<l_total; ++i)
        l_trie.fold(make_number<5>(), l_data_ptr, lookup_simple);
    duration d = (clock::now() - tp1 - d0) / l_total;
    BOOST_TEST_MESSAGE( "mmap_trie lookup time "
        << duration_cast<nanoseconds>(d).count() << " ns" );
}

#endif // HAVE_BOOST_CHRONO

BOOST_AUTO_TEST_SUITE_END()

} // namespace ptrie_test
