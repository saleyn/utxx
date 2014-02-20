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
#include <persist_array.hpp>

using namespace boost::property_tree;
using namespace utxx;

#ifdef UTXX_STANDALONE
int main(int argc, char* argv[])
{
    boost::shared_ptr<logger_impl_scribe> log( logger_impl_scribe::create("test") );
    log->set_log_mgr(&logger::instance());

    variant_tree pt;
    pt.put("logger.scribe.address", variant(
        #ifdef UTXX_STANDALONE
        argc > 1 ? argv[1] :
        #endif
        "uds:///var/run/scribed"
    ));
    pt.put("logger.scribe.levels", variant("debug|info|warning|error|fatal|alert"));

    log->init(pt);

    for (int i=0; i < 10; i++) {
        std::stringstream s;
        s << "This is a message number " << i;
        log->log_bin("test2", s.str().c_str(), s.str().length());
    }

    sleep(
        #ifdef UTXX_STANDALONE
        argc > 2 ? atoi(argv[2]) :
        #endif
        2);

    log.reset();

    return 0;
}
#else
BOOST_AUTO_TEST_CASE( test_logger_scribe )
{
    variant_tree pt;

    pt.put("logger.scribe.address", variant(
        #ifdef UTXX_STANDALONE
        argc > 1 ? argv[1] :
        #endif
        "uds:///var/run/scribed"
    ));
    pt.put("logger.scribe.levels", variant("debug|info|warning|error|fatal|alert"));

    logger& log = logger::instance();

    log.set_ident("test_logger");

    log.init(pt);

    for (int i=0; i < 10; i++) {
        std::stringstream s;
        s << "This is a message number " << i;
        log.log("test2", s.str());
    }
/*
    for (int i = 0; i < 3; i++) {
        LOG_ERROR  (("This is an error %d #%d", i, 123));
        LOG_WARNING(("This is a %d %s", i, "warning"));
        LOG_FATAL  (("This is a %d %s", i, "fatal error"));
    }
*/

#ifdef UTXX_STANDALONE
    return 0;
#endif
}
#endif

//BOOST_AUTO_TEST_SUITE_END()
