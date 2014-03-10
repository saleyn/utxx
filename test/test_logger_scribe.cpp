#ifndef UTXX_STANDALONE
#   include <boost/test/unit_test.hpp>
#elif HAVE_THRIFT_H
#   include <utxx/logger/logger_impl_scribe.hpp>
#else
#   define BOOST_TEST_MODULE logger_test
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <boost/property_tree/ptree.hpp>
#pragma GCC diagnostic pop

#include <utxx/logger.hpp>
#include <utxx/logger/logger_impl_scribe.hpp>
#include <utxx/time_val.hpp>

using namespace boost::property_tree;
using namespace utxx;

#ifdef UTXX_STANDALONE
int main(int argc, char* argv[])
{
    #ifdef HAVE_THRIFT_H
    boost::shared_ptr<logger_impl_scribe> log( logger_impl_scribe::create("test") );

    if (argc > 1 && (!strcmp("-h", argv[1]) || !strcmp("--help", argv[1]))) {
        printf("Usage: %s ScribeURL [TimeoutMsec [Iterations]]\n"
               "    TimeoutMsec    - default 100\n"
               "    Iterations     - default 10\n"
               "Example:\n"
               "    %s uds:///tmp/scribed 1000 10\n",
               argv[0], argv[0]);
        exit(1);
    }

    variant_tree pt;
    pt.put("logger.scribe.address", argc > 1 ? argv[1] : variant("uds:///var/run/scribed"));
    pt.put("logger.scribe.levels", variant("debug|info|warning|error|fatal|alert"));

    log->init(pt);

    static const int TIMEOUT_MSEC = argc > 2 ? atoi(argv[2]) : 100;

    static const int ITERATIONS = argc > 3 ? atoi(argv[3]) : 10;

    for (int i=0; i < ITERATIONS; i++) {
        time_val tv(time_val::universal_time());
        std::stringstream s;
        s << timestamp::to_string(tv, TIME_WITH_USEC)
          << ": This is a message number " << i;
        log->log_bin("test2", s.str().c_str(), s.str().length());

        static const struct timespec tout = { TIMEOUT_MSEC / 1000, TIMEOUT_MSEC % 1000 };

        nanosleep(&tout, NULL);
    }

    log.reset();

    #endif

    return 0;
}
#else
BOOST_AUTO_TEST_CASE( test_logger_scribe )
{
    variant_tree pt;

    #ifdef HAVE_THRIFT_H
    pt.put("logger.console.stderr_levels",  std::string("info|warning|error|fatal|alert"));
    pt.put("logger.scribe.address",         std::string("uds:///var/run/scribed"));
    pt.put("logger.scribe.levels",          std::string("debug|info|warning|error|fatal|alert"));

    logger& log = logger::instance();

    log.set_ident("test_logger");

    // Initialize scribe logging implementation with the logging framework
    log.init(pt);

    for (int i = 0; i < 2; i++) {
        LOG_ERROR  (("This is an error %d #%d", i, 123));
        LOG_WARNING(("This is a %d %s", i, "warning"));
        LOG_FATAL  (("This is a %d %s", i, "fatal error"));
    }

    for (int i = 0; i < 2; i++) {
        LOG_CAT_ERROR  ("Cat1", ("This is an error %d #%d", i, 456));
        LOG_CAT_WARNING("Cat2", ("This is a %d %s", i, "warning"));
        LOG_CAT_FATAL  ("Cat3", ("This is a %d %s", i, "fatal error"));
    }

    // Unregister scribe implementation from the logging framework
    log.delete_impl("scribe");
    
    #endif

    BOOST_REQUIRE(true); // Just to suppress the warning
}
#endif

//BOOST_AUTO_TEST_SUITE_END()
