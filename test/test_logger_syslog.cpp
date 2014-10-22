//#define BOOST_TEST_MODULE logger_test
#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>
#include <utxx/logger.hpp>

using namespace boost::property_tree;
using namespace utxx;

BOOST_AUTO_TEST_CASE( test_logger_syslog )
{
    variant_tree pt;

    pt.put("logger.syslog.levels", variant("debug|info|warning|error|fatal|alert"));
    pt.put("logger.syslog.facility", variant("log-local3"));

    logger& log = logger::instance();

    log.set_ident("test_logger");

    log.init(pt);

    for (int i = 0; i < 3; i++) {
        LOG_ERROR  ("This is an error #%d", 123);
        LOG_WARNING("This is a %s", "warning");
        LOG_FATAL  ("This is a %s", "fatal error");
    }
}

//BOOST_AUTO_TEST_SUITE_END()
