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

struct test {
    struct inner {
        static void log (int i) { LOG_DEBUG("This is a %d debug",  i); }
        static void clog(int i) { CLOG_DEBUG("Cat5", "This is a %d debug", i); }
    };
};

#ifndef UTXX_STANDALONE
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

    auto console = static_cast<const logger_impl_console*>(log.get_impl("console"));

    BOOST_REQUIRE(console);

    BOOST_REQUIRE_EQUAL(
        LEVEL_DEBUG | LEVEL_INFO  | LEVEL_WARNING |
        LEVEL_ERROR | LEVEL_FATAL | LEVEL_ALERT,
        console->stdout_levels());

    std::stringstream s;
    s << "test_logger." << getpid();
    log.set_ident(s.str().c_str());

    if (verbosity::level() > VERBOSE_NONE)
        log.dump(std::cout);

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
    pt.put("logger.console.stdout-levels", variant("debug|info|warning|error|fatal|alert"));
    pt.put("logger.show-ident", false);
    pt.put("logger.handle-crash-signals", true); // This is default behavior

    logger& log = logger::instance();
    //log.init(pt);

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

    bool crash = getenv("UTXX_LOGGER_CRASH");

    if (crash) {
        double* p = nullptr;
        kill(getpid(), SIGABRT);
        *p = 10.0;
    }

    #ifndef UTXX_STANDALONE
    BOOST_REQUIRE(true); // Keep the warning off
    #endif
}
