#include <boost/test/unit_test.hpp>
#define BOOST_TEST_MODULE logger_test

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <boost/property_tree/ptree.hpp>
#pragma GCC diagnostic pop

#include <utxx/logger.hpp>
#include <utxx/time_val.hpp>
#include <utxx/config.h>

#ifdef UTXX_HAVE_THRIFT_H
#   include <utxx/logger/logger_impl_scribe.hpp>
#endif

using namespace boost::property_tree;
using namespace utxx;

#ifdef UTXX_HAVE_THRIFT_H

BOOST_AUTO_TEST_CASE( test_logger_scribe1 )
{
    std::shared_ptr<logger_impl_scribe> log( logger_impl_scribe::create("test") );

    const int ITERATIONS = getenv("UTXX_SCRIBE_ITERATIONS")
                         ? atoi(getenv("UTXX_SCRIBE_ITERATIONS")) : 10;
    const int TIMEOUT    = getenv("UTXX_SCRIBE_TIMEOUT_MSEC")
                         ? atoi(getenv("UTXX_SCRIBE_TIMEOUT_MSEC")) : 100;
    const char* ADDRESS  = getenv("UTXX_SCRIBE_ADDRESS")
                         ? getenv("UTXX_SCRIBE_ADDRESS")
                         : "uds:///var/run/scribed";

    variant_tree pt;
    pt.put("logger.scribe.address", variant(ADDRESS));
    pt.put("logger.scribe.levels",  variant("debug|info|warning|error|fatal|alert"));

    try {
        log->init(pt);
    } catch (utxx::runtime_error& e) {
        static const char s_err[] = "Failed to open connection";
        if (strncmp(s_err, e.what(), sizeof(s_err)-1) == 0) {
            BOOST_TEST_MESSAGE("SCRIBED server not running - skipping scribed logging test!");
            return;
        }
        throw;
    }

    for (int i=0; i < ITERATIONS; i++) {
        time_val tv(time_val::universal_time());
        std::stringstream s;
        s << timestamp::to_string(tv, TIME_WITH_USEC)
          << ": This is a message number " << i;
        auto str = s.str();
        logger::msg msg(LEVEL_INFO, "test2", str, UTXX_LOG_SRCINFO);
        log->log_msg(msg, str.c_str(), str.size());

        static const struct timespec tout = { TIMEOUT / 1000, TIMEOUT % 1000 };

        nanosleep(&tout, NULL);
    }

    // Unregister scribe implementation from the logging framework
    log.reset();
}

BOOST_AUTO_TEST_CASE( test_logger_scribe2 )
{
    variant_tree pt;

    pt.put("logger.console.stderr-levels",  std::string("info|warning|error|fatal|alert"));
    pt.put("logger.scribe.address",         std::string("uds:///var/run/scribed"));
    pt.put("logger.scribe.levels",          std::string("debug|info|warning|error|fatal|alert"));

    logger& log = logger::instance();

    log.set_ident("test_logger");

    // Initialize scribe logging implementation with the logging framework
    try {
        log.init(pt);
    } catch (utxx::runtime_error& e) {
        static const char s_err[] = "Failed to open connection";
        if (strncmp(s_err, e.what(), sizeof(s_err)-1) == 0) {
            BOOST_TEST_MESSAGE("SCRIBED server not running - skipping scribed logging test!");
            return;
        }
        throw;
    }

    for (int i = 0; i < 2; i++) {
        LOG_ERROR  ("This is an error %d #%d", i, 123);
        LOG_WARNING("This is a %d %s", i, "warning");
        LOG_FATAL  ("This is a %d %s", i, "fatal error");
    }

    for (int i = 0; i < 2; i++) {
        CLOG_ERROR  ("Cat1", "This is an error %d #%d", i, 456);
        CLOG_WARNING("Cat2", "This is a %d %s", i, "warning");
        CLOG_FATAL  ("Cat3", "This is a %d %s", i, "fatal error");
    }

    // Unregister scribe implementation from the logging framework
    log.delete_impl("scribe");

    BOOST_REQUIRE(true); // Just to suppress the warning
}

#endif // UTXX_HAVE_THRIFT_H

//BOOST_AUTO_TEST_SUITE_END()
