// ----------------------------------------------------------------------------
// Copyright (C) 2014 Serge Aleynikov (adapted for checking SCON format)
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#include <boost/test/unit_test.hpp>
#include <boost/property_tree/detail/ptree_utils.hpp>
#include <boost/format.hpp>
#include <utxx/variant_tree_parser.hpp>
#include <utxx/time_val.hpp>
#include <typeinfo>  //for 'typeid' to work

///////////////////////////////////////////////////////////////////////////////
// Test data

namespace utxx {
namespace test {

namespace bpd = boost::property_tree::detail;

const char *ok_data_00 =
    "k1 # No data\n"
    "$include testok7_inc.config\n"
    "$include {testok7_inc.config, root=k4.k5}\n"
    "k2\n";

const char *ok_data_0 =
    "k1 # No data\n"
    "{\n"
    "   k2 # No data\n"
    "}\n"
    "k3 { k4 }\n"
    "k5 { k6 v6 }\n"
    "k7\n"
    "{ k8=v8 }\n"
    "k9 v9 { k10=v10, k11=v11 }\n";

const char *ok_data_1 = 
    "#Test file for scon_parser\n"
    "\n"
    "key { k=10, k=\"abc\"\\\n"
    "                   \"efg\"}"
    "key1 data1\n"
    "{\n"
    "\tkey data\n"
    "}\n"
    "$include{\"testok1_inc.config\"}\n"
    "$include{\"testok1_inc.config\"}\n"
    "key2 \"data2  \" {\n"
    "\tkey data\n"
    "\tkey = data\n"
    "}\n"
    "$\tinclude { \"testok1_inc.config\" } # Comment\n"
    "key3   =   \"data\"\n"
    "\t \"3\" {\n"
    "\tkey data\n"
    "\tkey data, key = data, key = \"data\"\n"
    "}\n"
    "\t$include {\"testok1_inc.config\"}\n"
    "\n"
    "\"key4\" data4\n"
    "{\n"
    "\tkey data\n"
    "}\n"
    "\"key.5\" \"data.5\" { \n"
    "\tkey data \n"
    "}\n"
    "$\tinclude{\"testok1_inc.config\"}\n"
    "\"key6\" = \"data\"\n"
    "\t   \"6\" {\n"
    "\tkey data\n"
    "}\n"
    "\t$include { \"testok1_inc.config\" }\n"
    "   \n"
    "key1 data1# comment\n"
    "{# comment\n"
    "\tkey data# comment\n"
    "}# comment\n"
    "$include {\"testok1_inc.config\"}\n"
    "key2 \"data2  \" {# comment\n"
    "\tkey data# comment\n"
    "}# comment\n"
    "$\tinclude {  \"testok1_inc.config\"  }\n"
    "key3 \"data\"# comment\n"
    "\t \"3\" {# comment\n"
    "\tkey data# comment\n"
    "}# comment\n"
    "\t$include{ \"testok1_inc.config\" }\n"
    "\n"
    "\"key4\" data4# comment\n"
    "{# comment\n"
    "\tkey data# comment\n"
    "}# comment\n"
    "$include { \"testok1_inc.config\" }\n"
    "\"key.5\" \"data.5\" {# comment\n"
    "\tkey data# comment\n"
    "}# comment\n"
    "$\tinclude \"testok1_inc.config\"\n"
    "\"key6\" \"data\"# comment\n"
    "\t   \"6\" {# comment\n"
    "\tkey data# comment\n"
    "}# comment\n"
    "\t$include \"testok1_inc.config\"\n"
    "\\\\key\\t7 data7\\n\\\"data7\\\"\n"
    "{\n"
    "\tkey data\n"
    "}\n"
    "\"\\\\key\\t8\" \"data8\\n\\\"data8\\\"\"\n"
    "{\n"
    "\tkey data\n"
    "}\n"
    "key { k9=100, k10=true }\n"
    "\n";

const char *ok_data_1_inc = 
    "#Test file for scon_parser\n"
    "\n"
    "inc_key inc_data ### comment\\";

const char *ok_data_2 = 
    "";

const char *ok_data_3 = 
    "key1 \"\"\n"
    "key2 =\"\"\n"
    "key3= \"\"\n"
    "key4 = \"\"\n";

const char *ok_data_4 = 
    "key1 data, key2 = data\n"
    "key3 data  key4 = data\n"
    "key5{key6=value}\n";

const char *ok_data_5 = 
    "key { key \"\", key \"\" }\n";

const char *ok_data_6 = 
    "\"key with spaces\" = \"data with spaces\"\n"
    "\"key with spaces\"=\"multiline data\"\\\n"
    "\"cont\"\\\n"
    "\"cont\"";

const char *ok_data_7 =
    "k1 d1 {k12=d12,}\n"
    "k2 d2 {k12=d12}\n"
    "k3 d3 {\n"
    "   k31=d31\n"
    "  ,k32=d32\n"
    "}\n"
    "k4 {\n"
    "  k5 {\n"
    "    k6 {\n"
    "      k61=a\n"
    "      k62=b\n"
    "}}}\n";

const char *ok_data_8 =
    "k1 \"d1\" {k12=d12}\n"
    "k2 d2 {\n"
    "  $include{testok7_inc.config, root=\"k4.k5.k6\"} },\n"
    "k3 d3 {\n"
    "   k31=d31\n"
    "  ,k32=d32\n"
    "}\n";

const char *error_data_1 =
    "#Test file for scon_parser\n"
    "$include \"bogus_file\"\n";                // Nonexistent include file

const char *error_data_2 = 
    "#Test file for scon_parser\n"
    "key \"data with bad escape: \\q\"\n";      // Bad escape

const char *error_data_3 = 
    "#Test file for scon_parser\n"
    "{\n";                                      // Opening brace without key

const char *error_data_4 = 
    "#Test file for scon_parser\n"
    "}\n";                                      // Closing brace without opening brace

const char *error_data_5 = 
    "#Test file for scon_parser\n"
    "key data\n"
    "{\n"
    "";                                         // No closing brace
const char *error_data_6 =
    "key1 data1, ,key2 data2\n";                // Extra ',' delimiter

template<class Ptree>
typename Ptree::size_type calc_total_size(const Ptree &pt)
{
    typename Ptree::size_type size = 1;
    for (typename Ptree::const_iterator it = pt.begin(); it != pt.end(); ++it)
        size += calc_total_size(it->second);
    return size;
}

template<class Ptree>
typename Ptree::size_type calc_total_keys_size(const Ptree &pt)
{
    typename Ptree::size_type size = 0;
    for (typename Ptree::const_iterator it = pt.begin(); it != pt.end(); ++it)
    {
        size += it->first.size();
        size += calc_total_keys_size(it->second);
    }
    return size;
}

template<class Ptree>
typename Ptree::size_type calc_total_data_size(const Ptree &pt)
{
    typename Ptree::size_type size = pt.data().is_null() ? 0 : pt.data().to_string().size();
    for (typename Ptree::const_iterator it = pt.begin(); it != pt.end(); ++it)
        size += calc_total_data_size(it->second);
    return size;
}

template<class Ptree>
Ptree get_test_ptree()
{
    using namespace boost::property_tree;
    typedef typename Ptree::key_type::value_type Ch;
    Ptree pt;
    pt.put_value(bpd::widen<Ch>("data0"));
    pt.put(bpd::widen<Ch>("key1"),       bpd::widen<Ch>("data1"));
    pt.put(bpd::widen<Ch>("key1.key"),   bpd::widen<Ch>("data2"));
    pt.put(bpd::widen<Ch>("key2"),       bpd::widen<Ch>("data3"));
    pt.put(bpd::widen<Ch>("key2.key"),   bpd::widen<Ch>("data4"));
    return pt;
}

class test_file {
public:
    test_file(const char *test_data, const char *filename)
    {
        if (test_data && filename)
        {
            m_name = (boost::filesystem::temp_directory_path() / filename).string();
            std::ofstream stream(m_name.c_str());
            using namespace std;
            stream.write(test_data, strlen(test_data));
            BOOST_REQUIRE(stream.good());
        }
    }
    ~test_file()
    {
        if (!m_name.empty())
            remove(m_name.c_str());
    }
    const std::string& name() const { return m_name; }
private:
    std::string m_name;
};

// Generic test for file parser
template<class Ptree, class ReadFunc, class WriteFunc>
void generic_parser_test
(
    Ptree      &pt,
    ReadFunc    rf,
    WriteFunc   wf,
    const char *test_data_1,
    const char *test_data_2,
    const char *filename_1,
    const char *filename_2,
    const char *filename_out
)
{
    using namespace boost::property_tree;
    using namespace boost::filesystem;
    typedef typename Ptree::key_type::value_type Ch;

    // Create test files
    test_file file_1(test_data_1, filename_1);
    test_file file_2(test_data_2, filename_2);
    test_file file_out("", filename_out);

    rf(file_1.name().c_str(),   pt);      // Read file
    wf(file_out.name().c_str(), pt);      // Write file
    Ptree pt2;
    rf(file_out.name().c_str(), pt2);     // Read file again

    // Compare original with read
    if (pt != pt2) {
        BOOST_MESSAGE("Expected tree:\n" << pt.to_string());
        BOOST_MESSAGE("\nActual tree:\n" << pt2.to_string());
    }
    BOOST_REQUIRE(pt == pt2);
}

// Generic test for file parser with expected success
template<class Ptree, class ReadFunc, class WriteFunc>
void generic_parser_test_ok
(
    ReadFunc    rf,
    WriteFunc   wf,
    const char *test_data_1,
    const char *test_data_2,
    const char *filename_1,
    const char *filename_2,
    const char *filename_out,
    unsigned int total_size,
    unsigned int total_data_size,
    unsigned int total_keys_size
)
{
    using namespace boost::property_tree;

    BOOST_MESSAGE("(progress) Starting parser test with file \"" << filename_1 << "\"");

    try
    {
        // Read file
        Ptree pt;
        generic_parser_test<Ptree, ReadFunc, WriteFunc>
        (
            pt, rf, wf,
            test_data_1, test_data_2,
            filename_1, filename_2, filename_out
        );

        // Determine total sizes
        typename Ptree::size_type actual_total_size = calc_total_size(pt);
        typename Ptree::size_type actual_data_size = calc_total_data_size(pt);
        typename Ptree::size_type actual_keys_size = calc_total_keys_size(pt);
        if (actual_total_size != total_size ||
            actual_data_size != total_data_size ||
            actual_keys_size != total_keys_size)
            BOOST_MESSAGE(
                "File: " << filename_1 << "Sizes: " <<
                "total={" << actual_total_size << ',' << total_size << "}, " <<
                "keys={"  << actual_data_size  << ',' << total_data_size << "}, " <<
                "data={"  << actual_keys_size  << ',' << total_keys_size << "}");

        // Check total sizes
        BOOST_CHECK_EQUAL(actual_total_size, total_size);
        BOOST_CHECK_EQUAL(actual_data_size, total_data_size);
        BOOST_CHECK_EQUAL(actual_keys_size, total_keys_size);

    }
    catch (std::runtime_error &e)
    {
        BOOST_ERROR(e.what());
    }
}

// Generic test for file parser with expected error
template<class Ptree, class ReadFunc, class WriteFunc, class Error>
void generic_parser_test_error
(
    ReadFunc    rf,
    WriteFunc   wf,
    const char *test_data_1,
    const char *test_data_2,
    const char *filename_1,
    const char *filename_2,
    const char *filename_out,
    unsigned long expected_error_line
)
{
    BOOST_MESSAGE("(progress) Starting parser test with file \"" << filename_1 << "\"");

    {
        // Create ptree as a copy of test ptree (to test if read failure does not damage ptree)
        Ptree pt(get_test_ptree<Ptree>());
        try
        {
            generic_parser_test<Ptree, ReadFunc, WriteFunc>
            (
                pt, rf, wf,
                test_data_1, test_data_2,
                filename_1, filename_2, filename_out
            );
            BOOST_ERROR("No required exception thrown");
        }
        catch (Error &e)
        {
            if (expected_error_line != e.line())
                BOOST_MESSAGE("Error test file: " << filename_1);
            BOOST_CHECK_EQUAL(expected_error_line, e.line()); // Test line number
            BOOST_REQUIRE(pt == get_test_ptree<Ptree>());   // Test if error damaged contents
        }
        catch (std::exception& e)
        {
            BOOST_ERROR("Invalid exception type '" << typeid(e).name() << "' thrown: "
                        << e.what());
            throw;
        }

    }
}

struct ReadFunc
{
    static bool inc_filename_resolver(std::string& a_name) {
        a_name = (boost::filesystem::temp_directory_path() / a_name).string();
        return boost::filesystem::exists(a_name);
    }

    template<class Ptree>
    void operator()(const std::string &filename, Ptree &pt) const
    {
        read_scon(filename, pt, &ReadFunc::inc_filename_resolver, std::locale());
    }
};

struct WriteFunc
{
    template<class Ptree>
    void operator()(const std::string &filename, const Ptree &pt) const
    {
        write_scon(filename, pt);
    }
};

template<class Ptree>
bool test_scon_parser()
{
    using namespace boost::property_tree;
    using boost::property_tree::file_parser_error;

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_00, ok_data_7,
        "testok00.config", "testok7_inc.config", "testok00out.config", 18, 22, 42
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_0, NULL,
        "testok0.config", NULL, "testok0out.config", 12, 12, 24
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_1, ok_data_1_inc,
        "testok1.config", "testok1_inc.config", "testok1out.config", 55, 271, 217
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_2, NULL,
        "testok2.config", NULL, "testok2out.config", 1, 0, 0
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_3, NULL,
        "testok3.config", NULL, "testok3out.config", 5, 0, 16
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_4, NULL,
        "testok4.config", NULL, "testok4out.config", 7, 21, 24
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_5, NULL,
        "testok5.config", NULL, "testok5out.config", 4, 0, 9
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_6, NULL,
        "testok6.config", NULL, "testok6out.config", 3, 38, 30
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_7, NULL,
        "testok7.config", NULL, "testok7out.config", 13, 20, 30
    );

    generic_parser_test_ok<Ptree, ReadFunc, WriteFunc>
    (
        ReadFunc(), WriteFunc(), ok_data_8, ok_data_7,
        "testok8.config", "testok7_inc.config", "testok8out.config", 9, 17, 21
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, file_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_1, NULL,
        "testerr1.config", NULL, "testerr1out.config", 2
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, file_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_2, NULL,
        "testerr2.config", NULL, "testerr2out.config", 2
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, file_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_3, NULL,
        "testerr3.config", NULL, "testerr3out.config", 2
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, file_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_4, NULL,
        "testerr4.config", NULL, "testerr4out.config", 2
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, file_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_5, NULL,
        "testerr5.config", NULL, "testerr5out.config", 4
    );

    generic_parser_test_error<Ptree, ReadFunc, WriteFunc, file_parser_error>
    (
        ReadFunc(), WriteFunc(), error_data_6, NULL,
        "testerr6.config", NULL, "testerr6out.config", 1
    );

    return true;
}

BOOST_AUTO_TEST_CASE( test_variant_tree_scon_parser )
{
    BOOST_REQUIRE(test_scon_parser<utxx::variant_tree>());
    //test_scon_parser<iptree>();
//#ifndef BOOST_NO_CWCHAR
    //test_scon_parser<wptree>();
    //test_scon_parser<wiptree>();
//#endif
}

BOOST_AUTO_TEST_CASE( test_variant_tree_scon_parse_macros )
{
    std::string home = utxx::path::home();
    std::string home_tmp = home + "/tmp";

    setenv("TTT", "abc", true);
    struct tm tm, now, now_utc;
    time_t nowtime = ::time(NULL);
    localtime_r(&nowtime, &tm);

    now.tm_year = 100;
    now.tm_mon  = 10;
    now.tm_mday = 20;
    now.tm_hour = 11;
    now.tm_min  = 25;
    now.tm_sec  = 33;
    time_t faketime = mktime(&now);

    localtime_r(&faketime, &now);
    gmtime_r(&faketime, &now_utc);

    std::string stime = (boost::format("\"%d-%02d-%02d %02d:%02d:%02d\"")
        % (1900+tm.tm_year) % (tm.tm_mon+1) % tm.tm_mday
        % tm.tm_hour        % tm.tm_min     % tm.tm_sec).str();

    std::string snow = (boost::format("\"%d-%02d-%02d %02d:%02d:%02d\"")
        % (1900+now.tm_year) % (now.tm_mon+1) % now.tm_mday
        % now.tm_hour        % now.tm_min     % now.tm_sec).str();

    std::string snow_utc = (boost::format("\"%d-%02d-%02d %02d:%02d:%02d\", utc=true")
        % (1900+now_utc.tm_year) % (now_utc.tm_mon+1) % now_utc.tm_mday
        % now_utc.tm_hour        % now_utc.tm_min     % now_utc.tm_sec).str();
//            % (1900+now_utc.year) % now_utc.tm_mon % now_utc.tm_mday
//            % now_utc.tm_hour     % now_utc.tm_min % now_utc.tm_sec;

    struct t {
        static std::string get(const std::string& tree, const char* key) {
            variant_tree t;
            std::stringstream s; s << tree;
            read_scon(s, t);
            {
                std::stringstream s;
                t.dump(s, 2, false);
                //BOOST_MESSAGE("Tree=====\n" << s.str());
            }
            return t.get<std::string>(key);
        }

        static std::string get(const char* t1, const std::string& t2, const char* t3, const char* key) {
            return get(std::string(t1) + t2 + t3, key);
        }
    };

    std::string home_tmp_file_time =
        (boost::format("%s/tmp/file%d%02d%02d-%02d%02d.log")
            % home % (1900+tm.tm_year) % (tm.tm_mon+1) % tm.tm_mday
            % tm.tm_hour % tm.tm_min).str();
    std::string home_tmp_file_now =
        (boost::format("%s/tmp/file%d%02d%02d-%02d%02d.log")
            % home % (1900+now.tm_year) % (now.tm_mon+1) % now.tm_mday
            % now.tm_hour % now.tm_min).str();
    std::string home_tmp_file_now_utc =
        (boost::format("%s/tmp/file%d%02d%02d-%02d%02d.log")
            % home % (1900+now_utc.tm_year) % (now_utc.tm_mon+1) % now_utc.tm_mday
            % now_utc.tm_hour % now_utc.tm_min).str();
    std::string date = (boost::format("%d%02d%02d-%02d")
            % (1900+tm.tm_year) % (tm.tm_mon+1) % tm.tm_mday % tm.tm_hour).str();
    std::string date_now = (boost::format("%d%02d%02d-%02d")
            % (1900+now.tm_year) % (now.tm_mon+1) % now.tm_mday % now.tm_hour).str();
    std::string date_now_utc = (boost::format("%d%02d%02d-%02d")
            % (1900+now_utc.tm_year) % (now_utc.tm_mon+1) % now_utc.tm_mday
            % now_utc.tm_hour).str();
    std::string time_home_date = std::string("Time: ") + home + date;

    BOOST_REQUIRE_EQUAL(home+" abc",            t::get("k10 \"${HOME} $env{TTT}\"\n", "k10"));
    BOOST_REQUIRE_EQUAL(home,                   t::get("k1 ${HOME}\n",      "k1"));
    BOOST_REQUIRE_EQUAL("abc",                  t::get("k2 ${\"TTT\"}\n",   "k2"));
    BOOST_REQUIRE_EQUAL("abc",                  t::get("k3 $env{TTT}\n",    "k3"));
    BOOST_REQUIRE_EQUAL(home_tmp,               t::get("k4 $path{~/tmp}\n", "k4"));
    BOOST_REQUIRE_EQUAL(home_tmp_file_time,     t::get("k5 $path{~/tmp/file%Y%m%d-%H%M.log}\n", "k5"));
    BOOST_REQUIRE_EQUAL(home_tmp_file_now,      t::get("k6 $path{\"~/tmp/file%Y%m%d-%H%M.log\", now=",
                                                       snow, "}\n", "k6"));
    BOOST_REQUIRE_EQUAL(home_tmp_file_now_utc,  t::get("k7 $path{\"~/tmp/file%Y%m%d-%H%M.log\", now=",
                                                       snow_utc, "}\n", "k7"));
    BOOST_REQUIRE_EQUAL(home+"abc",             t::get("k8 \"${HOME}${TTT}\"\n", "k8"));
    BOOST_CHECK_THROW  (t::get("k9 ${HOME}${TTT}\n", "k9"), boost::property_tree::file_parser_error);
    BOOST_REQUIRE_EQUAL(date,                   t::get("k11 $date{\"%Y%m%d-%H\"}\n", "k11"));
    BOOST_REQUIRE_EQUAL(date_now,               t::get("k12 $date{\"%Y%m%d-%H\", now=", snow,     "}\n", "k12"));
    BOOST_REQUIRE_EQUAL(date_now_utc,           t::get("k13 $date{'%Y%m%d-%H', now=", snow_utc, "}\n", "k13"));
    BOOST_REQUIRE_EQUAL(time_home_date,         t::get("k14 \"Time: ${HOME}$date{'%Y%m%d-%H'}\"\n", "k14"));
    BOOST_REQUIRE_EQUAL(home+"-abc/file.log",   t::get("k15 $path{'${HOME}-${TTT}/file.log'}", "k15"));
    BOOST_REQUIRE_EQUAL(path::program::abs_path()+"/f", t::get("k16 $path{'${EXEPATH}/f'}", "k16"));

}


}} // namespace utxx::test