#ifndef UTXX_STANDALONE
//#define BOOST_TEST_MODULE logger_test
#include <boost/test/unit_test.hpp>
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
#else
BOOST_AUTO_TEST_CASE( test_logger_scribe )
#endif
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

    for (int i = 0; i < 3; i++) {
        LOG_ERROR  (("This is an error %d #%d", i, 123));
        LOG_WARNING(("This is a %d %s", i, "warning"));
        LOG_FATAL  (("This is a %d %s", i, "fatal error"));
    }

#ifdef UTXX_STANDALONE
    return 0;
#endif
}

//BOOST_AUTO_TEST_SUITE_END()
