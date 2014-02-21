#ifndef UTXX_STANDALONE
//#define BOOST_TEST_MODULE logger_test
#include <boost/test/unit_test.hpp>
#else
#include <utxx/logger/logger_impl_scribe.hpp>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <boost/property_tree/ptree.hpp>
#pragma GCC diagnostic pop

#include <utxx/logger.hpp>
#include <utxx/time_val.hpp>

using namespace boost::property_tree;
using namespace utxx;

#ifdef UTXX_STANDALONE
int main(int argc, char* argv[])
{
    boost::shared_ptr<logger_impl_scribe> log( logger_impl_scribe::create("test") );

    #ifdef UTXX_STANDALONE
    if (argc > 1 && (!strcmp("-h", argv[1]) || !strcmp("--help", argv[1]))) {
        printf("Usage: %s ScribeURL [TimeoutMsec [Iterations]]\n"
               "    TimeoutMsec    - default 100\n"
               "    Iterations     - default 10\n"
               "Example:\n"
               "    %s uds:///tmp/scribed 1000 10\n",
               argv[0], argv[0]);
        exit(1);
    }
    #endif

    variant_tree pt;
    pt.put("logger.scribe.address", variant(
        #ifdef UTXX_STANDALONE
        argc > 1 ? argv[1] :
        #endif
        "uds:///var/run/scribed"
    ));
    pt.put("logger.scribe.levels", variant("debug|info|warning|error|fatal|alert"));

    log->init(pt);

    static const int TIMEOUT_MSEC =
        #ifdef UTXX_STANDALONE
        argc > 2 ? atoi(argv[2]) :
        #endif
        100;

    static const int ITERATIONS =
        #ifdef UTXX_STANDALONE
        argc > 3 ? atoi(argv[3]) :
        #endif
        10;

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

    return 0;
}
#else
BOOST_AUTO_TEST_CASE( test_logger_scribe )
{
    variant_tree pt;

    pt.put("logger.console.stderr_levels", variant("info|warning|error|fatal|alert"));
    pt.put("logger.scribe.address", variant(
        #ifdef UTXX_STANDALONE
        argc > 1 ? argv[1] :
        #endif
        "uds:///var/run/scribed"
    ));
    pt.put("logger.scribe.levels", variant("debug|info|warning|error|fatal|alert"));

    logger& log = logger::instance();

    log.set_ident("test_logger");

    // Initialize scribe logging implementation with the logging framework
    log.init(pt);

    for (int i = 0; i < 3; i++) {
        LOG_ERROR  (("This is an error %d #%d", i, 123));
        LOG_WARNING(("This is a %d %s", i, "warning"));
        LOG_FATAL  (("This is a %d %s", i, "fatal error"));
    }

    // Unregister scribe implementation from the logging framework
    log.delete_impl("scribe");

    BOOST_REQUIRE(true); // Just to suppress the warning
}
#endif

//BOOST_AUTO_TEST_SUITE_END()
