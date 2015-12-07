#ifndef UTXX_STANDALONE
#include <boost/test/unit_test.hpp>
#endif

#include <iostream>
#include <utxx/logger.hpp>
#include <utxx/logger/logger_impl_console.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/variant_tree.hpp>
#include <signal.h>
#include <string.h>

//#define BOOST_TEST_MAIN

using namespace boost::property_tree;
using namespace utxx;
using namespace std;

namespace {
    struct test {
        struct inner {
            static void log (int i) {
                LOG_DEBUG("This is a %d debug",  i);
            }
            static void clog(int i) { CLOG_DEBUG("Cat5","This is a %d debug",i); }
        };
    };
}

#ifndef UTXX_STANDALONE
BOOST_AUTO_TEST_CASE( test_logger1 )
{
    variant_tree pt;

    BOOST_CHECK_EQUAL(1, (as_int<LEVEL_ALERT>()));
    BOOST_CHECK_EQUAL(1, (as_int<LEVEL_FATAL>()));
    BOOST_CHECK_EQUAL(1, (as_int<LEVEL_ERROR>()));
    BOOST_CHECK_EQUAL(1, (as_int<LEVEL_WARNING>()));
    BOOST_CHECK_EQUAL(2, (as_int<LEVEL_NOTICE>()));
    BOOST_CHECK_EQUAL(3, (as_int<LEVEL_INFO>()));
    BOOST_CHECK_EQUAL(4, (as_int<LEVEL_DEBUG>()));
    BOOST_CHECK_EQUAL(5, (as_int<LEVEL_TRACE>()));
    BOOST_CHECK_EQUAL(6, (as_int<LEVEL_TRACE1>()));
    BOOST_CHECK_EQUAL(7, (as_int<LEVEL_TRACE2>()));
    BOOST_CHECK_EQUAL(8, (as_int<LEVEL_TRACE3>()));
    BOOST_CHECK_EQUAL(9, (as_int<LEVEL_TRACE4>()));
    BOOST_CHECK_EQUAL(10,(as_int<LEVEL_TRACE5>()));

    BOOST_CHECK_EQUAL(1, (as_int(LEVEL_ALERT)));
    BOOST_CHECK_EQUAL(1, (as_int(LEVEL_FATAL)));
    BOOST_CHECK_EQUAL(1, (as_int(LEVEL_ERROR)));
    BOOST_CHECK_EQUAL(1, (as_int(LEVEL_WARNING)));
    BOOST_CHECK_EQUAL(2, (as_int(LEVEL_NOTICE)));
    BOOST_CHECK_EQUAL(3, (as_int(LEVEL_INFO)));
    BOOST_CHECK_EQUAL(4, (as_int(LEVEL_DEBUG)));
    BOOST_CHECK_EQUAL(5, (as_int(LEVEL_TRACE)));
    BOOST_CHECK_EQUAL(6, (as_int(LEVEL_TRACE1)));
    BOOST_CHECK_EQUAL(7, (as_int(LEVEL_TRACE2)));
    BOOST_CHECK_EQUAL(8, (as_int(LEVEL_TRACE3)));
    BOOST_CHECK_EQUAL(9, (as_int(LEVEL_TRACE4)));
    BOOST_CHECK_EQUAL(10,(as_int(LEVEL_TRACE5)));

    BOOST_CHECK(LEVEL_ERROR   == as_log_level(0 ));
    BOOST_CHECK(LEVEL_WARNING == as_log_level(1 ));
    BOOST_CHECK(LEVEL_NOTICE  == as_log_level(2 ));
    BOOST_CHECK(LEVEL_INFO    == as_log_level(3 ));
    BOOST_CHECK(LEVEL_DEBUG   == as_log_level(4 ));
    BOOST_CHECK(LEVEL_TRACE   == as_log_level(5 ));
    BOOST_CHECK(LEVEL_TRACE1  == as_log_level(6 ));
    BOOST_CHECK(LEVEL_TRACE2  == as_log_level(7 ));
    BOOST_CHECK(LEVEL_TRACE3  == as_log_level(8 ));
    BOOST_CHECK(LEVEL_TRACE4  == as_log_level(9 ));
    BOOST_CHECK(LEVEL_TRACE5  == as_log_level(10));

    BOOST_CHECK(LEVEL_TRACE   == logger::parse_log_level("trace"));
    BOOST_CHECK(LEVEL_TRACE5  == logger::parse_log_level("trace5"));
    BOOST_CHECK(LEVEL_TRACE1  == logger::parse_log_level("trace1"));
    BOOST_CHECK(LEVEL_INFO    == logger::parse_log_level("info"));
    BOOST_CHECK(LEVEL_WARNING == logger::parse_log_level("warning"));
    BOOST_CHECK(LEVEL_ERROR   == logger::parse_log_level("error"));
    BOOST_CHECK(LEVEL_WARNING == logger::parse_log_level("1"));
    BOOST_CHECK_EQUAL(int(LEVEL_INFO),  int(logger::parse_log_level("3")));
    BOOST_CHECK_EQUAL(int(LEVEL_DEBUG), int(logger::parse_log_level("4")));
    BOOST_CHECK_EQUAL(int(LEVEL_TRACE), int(logger::parse_log_level("5")));
    BOOST_CHECK_EQUAL(int(LEVEL_TRACE5),int(logger::parse_log_level("11")));
    BOOST_CHECK_EQUAL(int(LEVEL_TRACE5),int(logger::parse_log_level("110")));
    BOOST_CHECK_THROW(logger::parse_log_level("trace6"), std::runtime_error);

    BOOST_CHECK_EQUAL("TRACE5", logger::log_level_to_string(utxx::LEVEL_TRACE5, false));
    BOOST_CHECK_EQUAL("TRACE",  logger::log_level_to_string(utxx::LEVEL_TRACE5));
    BOOST_CHECK_EQUAL("TRACE1", logger::log_level_to_string(utxx::LEVEL_TRACE1, false));
    BOOST_CHECK_EQUAL("TRACE",  logger::log_level_to_string(utxx::LEVEL_TRACE1));
    BOOST_CHECK_EQUAL("TRACE",  logger::log_level_to_string(utxx::LEVEL_TRACE, false));
    BOOST_CHECK_EQUAL("TRACE",  logger::log_level_to_string(utxx::LEVEL_TRACE));
    BOOST_CHECK_EQUAL("DEBUG",  logger::log_level_to_string(utxx::LEVEL_DEBUG, false));
    BOOST_CHECK_EQUAL("DEBUG",  logger::log_level_to_string(utxx::LEVEL_DEBUG));
    BOOST_CHECK_EQUAL("FATAL",  logger::log_level_to_string(utxx::LEVEL_FATAL));
    BOOST_CHECK_EQUAL("ALERT",  logger::log_level_to_string(utxx::LEVEL_ALERT));
    BOOST_CHECK_EQUAL("LOG",    logger::log_level_to_string(utxx::LEVEL_LOG));

    BOOST_CHECK_EQUAL("TRACE5|TRACE|DEBUG", logger::log_levels_to_str(utxx::LEVEL_TRACE5 | utxx::LEVEL_DEBUG));
    BOOST_CHECK_EQUAL("TRACE|DEBUG",        logger::log_levels_to_str(utxx::LEVEL_TRACE  | utxx::LEVEL_DEBUG));
    BOOST_CHECK_EQUAL("DEBUG|INFO",         logger::log_levels_to_str(utxx::LEVEL_INFO   | utxx::LEVEL_DEBUG));

    BOOST_CHECK_EQUAL("T",                  logger::log_level_to_abbrev(utxx::LEVEL_TRACE1));
    BOOST_CHECK_EQUAL("T",                  logger::log_level_to_abbrev(utxx::LEVEL_TRACE5));
    BOOST_CHECK_EQUAL("T",                  logger::log_level_to_abbrev(utxx::LEVEL_TRACE));
    BOOST_CHECK_EQUAL("D",                  logger::log_level_to_abbrev(utxx::LEVEL_DEBUG));

    BOOST_CHECK_EQUAL(5u,                   logger::log_level_size(utxx::LEVEL_TRACE1));
    BOOST_CHECK_EQUAL(5u,                   logger::log_level_size(utxx::LEVEL_TRACE5));
    BOOST_CHECK_EQUAL(5u,                   logger::log_level_size(utxx::LEVEL_TRACE));
    BOOST_CHECK_EQUAL(5u,                   logger::log_level_size(utxx::LEVEL_DEBUG));
    BOOST_CHECK_EQUAL(5u,                   logger::log_level_size(utxx::LEVEL_ERROR));
    BOOST_CHECK_EQUAL(5u,                   logger::log_level_size(utxx::LEVEL_FATAL));
    BOOST_CHECK_EQUAL(5u,                   logger::log_level_size(utxx::LEVEL_ALERT));
    BOOST_CHECK_EQUAL(7u,                   logger::log_level_size(utxx::LEVEL_WARNING));
    BOOST_CHECK_EQUAL(3u,                   logger::log_level_size(utxx::LEVEL_LOG));

    pt.put("logger.timestamp",             variant("time-usec"));
    pt.put("logger.min-level-filter",      variant("debug"));
    pt.put("logger.console.stdout-levels", variant("debug|info|notice|warning|error|fatal|alert"));
    pt.put("logger.show-thread",           true);
    pt.put("logger.show-ident",            true);
    pt.put("logger.ident",                 variant("my-logger"));

    if (utxx::verbosity::level() != utxx::VERBOSE_NONE)
        pt.dump(std::cout, 2, false, true, ' ', 2);

    auto count = pt.count("logger");
    BOOST_REQUIRE_EQUAL(1u, count);
    BOOST_REQUIRE(pt.get_child_optional("logger.console"));

    logger& log = logger::instance();
    log.init(pt);

    auto console = static_cast<const logger_impl_console*>(log.get_impl("console"));

    BOOST_REQUIRE(console);

    BOOST_REQUIRE_EQUAL(
        LEVEL_DEBUG | LEVEL_INFO  | LEVEL_NOTICE | LEVEL_WARNING |
        LEVEL_ERROR | LEVEL_FATAL | LEVEL_ALERT,
        console->stdout_levels());

    std::stringstream s;
    s << "test_logger." << getpid();
    log.set_ident(s.str().c_str());

    if (verbosity::level() > VERBOSE_NONE)
        log.dump(std::cout);

    pthread_setname_np(pthread_self(), "log_tester");

    for (int i = 0; i < 2; i++) {
        LOG_ERROR  ("This is a %d %s #%d", i, "error", 123);
        LOG_WARNING("This is a %d %s", i, "warning");
        LOG_FATAL  ("This is a %d %s", i, "fatal error");
        LOG_INFO   ("This is a %d %s", i, "info");
        test::inner::log(i);
    }

    for (int i = 0; i < 2; i++) {
        CLOG_ERROR  ("Cat1", "This is an error %d #%d", i, 456);
        CLOG_WARNING("Cat2", "This is a %d %s", i, "warning");
        CLOG_FATAL  ("Cat3", "This is a %d %s", i, "fatal error");
        CLOG_INFO   ("Cat4", "This is a %d %s", i, "info");
        test::inner::clog(i);
    }

    UTXX_LOG(INFO, "A") << "This is an error #" << 10 << " and bool "
                        << true << ' ' << std::endl;

    UTXX_LOG(ERROR)     << "This is an error #" << 10 << " and bool "
                        << true << ' ' << std::endl;

    UTXX_LOG(INFO)      << std::endl;

    log.finalize();

    BOOST_REQUIRE(true); // to remove run-time warning
}
#endif

#ifdef UTXX_STANDALONE

    void hdl (int sig, siginfo_t *siginfo, void *context)
    {
        printf ("Sending PID: %ld, UID: %ld signal %d\n",
                (long)siginfo->si_pid, (long)siginfo->si_uid, sig);
        if (sig == SIGSEGV) {
            struct sigaction action;
            memset(&action, 0, sizeof(action));
            sigemptyset(&action.sa_mask);
            action.sa_handler = SIG_DFL; // take default action for the signal
            sigaction(sig, &action, NULL);

            std::cout << "Restoring old signal handler" << std::endl;

            kill(getpid(), sig);
            abort(); // should never reach this
        }
    }

    void enable_sig_handler(const std::vector<std::pair<int,std::string>>& sigs) {
        struct sigaction act;

        memset (&act, '\0', sizeof(act));
        sigemptyset(&act.sa_mask);

        /* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
        act.sa_flags |= SA_SIGINFO;
        /* Use the sa_sigaction field because the handles has two additional parameters */
        act.sa_sigaction = &hdl;

        for (auto& p : sigs)
            printf("Set %s handler -> %d\n", p.second, sigaction(p.first, &act, NULL));
    }

int main(int argc, char* argv[])
#else
BOOST_AUTO_TEST_CASE( test_logger_crash )
#endif
{
    variant_tree pt;

    pt.put("logger.timestamp",  variant("time-usec"));
    pt.put("logger.console.stdout-levels", variant("debug|notice|info|warning|error|fatal|alert"));
    pt.put("logger.show-ident", false);
    pt.put("logger.handle-crash-signals", true); // This is default behavior

    std::vector<std::pair<int,std::string>> sigs
        {{SIGTERM, "SIGTERM"},
         {SIGABRT, "SIGABRT"},
         {SIGSEGV, "SIGSEGV"},
         {SIGINT,  "SIGINT"}};

#ifdef UTXX_STANDALONE
    enable_sig_handler(sigs);
#endif

    sigset_t mask;
    if (sigprocmask(0, NULL, &mask) < 0)
        perror("sigprocmask");

    for (auto& p : sigs)
        printf("Process has %s handler -> %d\n", p.second.c_str(), sigismember(&mask, p.first));

    fflush(stdout);

    bool crash = getenv("UTXX_LOGGER_CRASH") && atoi(getenv("UTXX_LOGGER_CRASH"));

    if (crash) {
        double* p = nullptr;
        kill(getpid(), SIGABRT);
        *p = 10.0;
    }

    #ifndef UTXX_STANDALONE
    BOOST_REQUIRE(true); // Keep the warning off
    #endif
}
