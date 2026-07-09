//#define BOOST_TEST_MODULE logger_test
#include <utxx/config.h>
#ifdef UTXX_HAVE_BOOST
#include <utxx/test_helper.hpp>
#include <boost/property_tree/ptree.hpp>
#include <utxx/logger.hpp>

using namespace boost::property_tree;
using namespace utxx;

TEST_CASE( test_logger_syslog )
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
        LOG_ALERT  ("This is a %s", "alert error");
    }
}

//TEST_SUITE_END()

#endif // UTXX_HAVE_BOOST
