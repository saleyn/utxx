// If using VC, disable some warnings that trip in boost::serialization bowels
#if defined(__windows__) || defined(_WIN32) || defined(_WIN64)
#pragma warning(disable:4267)   // Narrowing conversion
#pragma warning(disable:4996)   // Deprecated functions
#endif

//#define BOOST_TEST_NO_AUTO_LINK

//#define BOOST_TEST_MODULE logger_test
#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_serialization.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <iostream>
#include <util/logger.hpp>
#include <util/verbosity.hpp>

//#define BOOST_TEST_MAIN

using namespace boost::property_tree;
using namespace util;

BOOST_AUTO_TEST_CASE( test_logger1 )
{
    variant_tree pt;

    pt.put("logger.timestamp", variant("time_with_usec"));
    pt.put("logger.console.stdout_levels", variant("debug|info|warning|error|fatal|alert"));
    pt.put("logger.show_ident", variant(true));

    if (util::verbosity::level() != util::VERBOSE_NONE)
        write_info(std::cout, pt);

    BOOST_REQUIRE_EQUAL(1u, pt.count("logger"));
    BOOST_REQUIRE(pt.get_child_optional("logger.console"));

    logger& log = logger::instance();
    log.init(pt);

    std::stringstream s;
    s << "test_logger." << getpid();
    log.set_ident(s.str().c_str());

    if (verbosity::level() > VERBOSE_NONE)
        log.dump(std::cout);

    for (int i = 0; i < 3; i++) {
        LOG_ERROR  (("This is an error #%d", 123));
        LOG_WARNING(("This is a %s", "warning"));
        LOG_FATAL  (("This is a %s", "fatal error"));
    }

    BOOST_REQUIRE(true); // to remove run-time warning
}

//BOOST_AUTO_TEST_SUITE_END()
