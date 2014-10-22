// If using VC, disable some warnings that trip in boost::serialization bowels
#if defined(__windows__) || defined(_WIN32) || defined(_WIN64)
#pragma warning(disable:4267)   // Narrowing conversion
#pragma warning(disable:4996)   // Deprecated functions
#endif

//#define BOOST_TEST_NO_AUTO_LINK

//#define BOOST_TEST_MODULE logger_test
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <utxx/logger.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/variant_tree.hpp>

//#define BOOST_TEST_MAIN

using namespace boost::property_tree;
using namespace utxx;

BOOST_AUTO_TEST_CASE( test_logger1 )
{
    variant_tree pt;

    pt.put("logger.timestamp",  variant("time-usec"));
    pt.put("logger.console.stdout-levels", variant("debug|info|warning|error|fatal|alert"));
    pt.put("logger.show-ident", true);

    if (utxx::verbosity::level() != utxx::VERBOSE_NONE)
        pt.dump(std::cout, 2, false, true, ' ', 2);

    auto count = pt.count("logger");
    BOOST_REQUIRE_EQUAL(1u, count);
    BOOST_REQUIRE(pt.get_child_optional("logger.console"));

    logger& log = logger::instance();
    log.init(pt);

    std::stringstream s;
    s << "test_logger." << getpid();
    log.set_ident(s.str().c_str());

    if (verbosity::level() > VERBOSE_NONE)
        log.dump(std::cout);

    for (int i = 0; i < 2; i++) {
        LOG_ERROR  ("This is an error %d #%d", i, 123);
        LOG_WARNING("This is a %d %s", i, "warning");
        LOG_FATAL  ("This is a %d %s", i, "fatal error");
    }

    for (int i = 0; i < 2; i++) {
        LOG_CAT_ERROR  ("Cat1", "This is an error %d #%d", i, 456);
        LOG_CAT_WARNING("Cat2", "This is a %d %s", i, "warning");
        LOG_CAT_FATAL  ("Cat3", "This is a %d %s", i, "fatal error");
    }

    LOG(ERROR) << "This is an error #" << 10 << " and bool " << true;
    //LOG(INFO)  << "This is an into  #" << 9 << std::endl << " and endl";
    
    BOOST_REQUIRE(true); // to remove run-time warning
}

//BOOST_AUTO_TEST_SUITE_END()
