//----------------------------------------------------------------------------
/// \file  test_logger.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for the logging framework
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
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

    BOOST_CHECK_EQUAL(0, (as_int<LEVEL_NONE>()));
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

    BOOST_CHECK_EQUAL(0, (as_int(LEVEL_NONE)));
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

    BOOST_CHECK(LEVEL_NONE    == as_log_level(0 ));
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

    BOOST_CHECK(LEVEL_TRACE   == parse_log_level("trace"));
    BOOST_CHECK(LEVEL_TRACE5  == parse_log_level("trace5"));
    BOOST_CHECK(LEVEL_TRACE1  == parse_log_level("trace1"));
    BOOST_CHECK(LEVEL_INFO    == parse_log_level("info"));
    BOOST_CHECK(LEVEL_WARNING == parse_log_level("warning"));
    BOOST_CHECK(LEVEL_ERROR   == parse_log_level("error"));
    BOOST_CHECK(LEVEL_WARNING == parse_log_level("1"));
    BOOST_CHECK(LEVEL_NONE    == parse_log_level("none"));
    BOOST_CHECK_EQUAL(int(LEVEL_INFO),  int(parse_log_level("3")));
    BOOST_CHECK_EQUAL(int(LEVEL_DEBUG), int(parse_log_level("4")));
    BOOST_CHECK_EQUAL(int(LEVEL_TRACE), int(parse_log_level("5")));
    BOOST_CHECK_EQUAL(int(LEVEL_TRACE5),int(parse_log_level("11")));
    BOOST_CHECK_EQUAL(int(LEVEL_TRACE5),int(parse_log_level("110")));
    BOOST_CHECK_THROW(logger::parse_log_level("trace6"), std::runtime_error);

    BOOST_CHECK_EQUAL("TRACE5", log_level_to_string(utxx::LEVEL_TRACE5, false));
    BOOST_CHECK_EQUAL("TRACE",  log_level_to_string(utxx::LEVEL_TRACE5));
    BOOST_CHECK_EQUAL("TRACE1", log_level_to_string(utxx::LEVEL_TRACE1, false));
    BOOST_CHECK_EQUAL("TRACE",  log_level_to_string(utxx::LEVEL_TRACE1));
    BOOST_CHECK_EQUAL("TRACE",  log_level_to_string(utxx::LEVEL_TRACE, false));
    BOOST_CHECK_EQUAL("TRACE",  log_level_to_string(utxx::LEVEL_TRACE));
    BOOST_CHECK_EQUAL("DEBUG",  log_level_to_string(utxx::LEVEL_DEBUG, false));
    BOOST_CHECK_EQUAL("DEBUG",  log_level_to_string(utxx::LEVEL_DEBUG));
    BOOST_CHECK_EQUAL("FATAL",  log_level_to_string(utxx::LEVEL_FATAL));
    BOOST_CHECK_EQUAL("ALERT",  log_level_to_string(utxx::LEVEL_ALERT));
    BOOST_CHECK_EQUAL("LOG",    log_level_to_string(utxx::LEVEL_LOG));

    BOOST_CHECK_EQUAL("TRACE5|TRACE|DEBUG", log_levels_to_str(utxx::LEVEL_TRACE5 | utxx::LEVEL_DEBUG));
    BOOST_CHECK_EQUAL("TRACE|DEBUG",        log_levels_to_str(utxx::LEVEL_TRACE  | utxx::LEVEL_DEBUG));
    BOOST_CHECK_EQUAL("DEBUG|INFO",         log_levels_to_str(utxx::LEVEL_INFO   | utxx::LEVEL_DEBUG));

    BOOST_CHECK_EQUAL("T",                  log_level_to_abbrev(utxx::LEVEL_TRACE1));
    BOOST_CHECK_EQUAL("T",                  log_level_to_abbrev(utxx::LEVEL_TRACE5));
    BOOST_CHECK_EQUAL("T",                  log_level_to_abbrev(utxx::LEVEL_TRACE));
    BOOST_CHECK_EQUAL("D",                  log_level_to_abbrev(utxx::LEVEL_DEBUG));

    BOOST_CHECK_EQUAL(5u,                   log_level_size(utxx::LEVEL_TRACE1));
    BOOST_CHECK_EQUAL(5u,                   log_level_size(utxx::LEVEL_TRACE5));
    BOOST_CHECK_EQUAL(5u,                   log_level_size(utxx::LEVEL_TRACE));
    BOOST_CHECK_EQUAL(5u,                   log_level_size(utxx::LEVEL_DEBUG));
    BOOST_CHECK_EQUAL(5u,                   log_level_size(utxx::LEVEL_ERROR));
    BOOST_CHECK_EQUAL(5u,                   log_level_size(utxx::LEVEL_FATAL));
    BOOST_CHECK_EQUAL(5u,                   log_level_size(utxx::LEVEL_ALERT));
    BOOST_CHECK_EQUAL(7u,                   log_level_size(utxx::LEVEL_WARNING));
    BOOST_CHECK_EQUAL(3u,                   log_level_size(utxx::LEVEL_LOG));
    BOOST_CHECK_EQUAL(4u,                   log_level_size(utxx::LEVEL_NONE));

    pt.put("logger.timestamp",             "time-usec");
    pt.put("logger.min-level-filter",      "debug");
    pt.put("logger.console.stdout-levels", "debug|info|notice|warning|error|fatal|alert");
    pt.put("logger.show-thread",           true);
    pt.put("logger.show-ident",            true);
    pt.put("logger.ident",                 "my-logger");
    pt.put("logger.fatal-kill-signal",     0);
    pt.put("logger.silent-finish",         true);

    if (utxx::verbosity::level() != utxx::VERBOSE_NONE)
        pt.dump(std::cout, 2, false, true, ' ', 2);

    auto count = pt.count("logger");
    BOOST_REQUIRE_EQUAL(1u, count);
    BOOST_REQUIRE(pt.get_child_optional("logger.console"));

    logger& log = logger::instance();

    if (log.initialized())
        log.finalize();

    log.set_min_level_filter(LEVEL_DEBUG);
    auto ll = log.min_level_filter();
    BOOST_CHECK(ll == LEVEL_DEBUG);
    BOOST_CHECK((log.level_filter() & (int)LEVEL_TRACE ) != (int)LEVEL_TRACE );
    BOOST_CHECK((log.level_filter() & (int)LEVEL_DEBUG ) == (int)LEVEL_DEBUG );
    BOOST_CHECK((log.level_filter() & (int)LEVEL_INFO  ) == (int)LEVEL_INFO  );
    BOOST_CHECK((log.level_filter() & (int)LEVEL_NOTICE) == (int)LEVEL_NOTICE);

    log.set_min_level_filter(LEVEL_TRACE);
    ll = log.min_level_filter();
    BOOST_CHECK(ll == LEVEL_TRACE);
    BOOST_CHECK((log.level_filter() & (int)LEVEL_TRACE2) != (int)LEVEL_TRACE2);
    BOOST_CHECK((log.level_filter() & (int)LEVEL_TRACE1) != (int)LEVEL_TRACE1);
    BOOST_CHECK((log.level_filter() & (int)LEVEL_TRACE ) == (int)LEVEL_TRACE );
    BOOST_CHECK((log.level_filter() & (int)LEVEL_DEBUG ) == (int)LEVEL_DEBUG );
    BOOST_CHECK((log.level_filter() & (int)LEVEL_INFO  ) == (int)LEVEL_INFO  );
    BOOST_CHECK((log.level_filter() & (int)LEVEL_NOTICE) == (int)LEVEL_NOTICE);

    log.init(pt, nullptr, false);

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

BOOST_AUTO_TEST_CASE( test_logger_split_file_size )
{
    logger& log = logger::instance();

    auto write_test_data = [&log](auto& config) {
        log.init(config);
        for(int i=0; i < 100;i++) {
            auto temp_data = utxx::to_string("write count: ",i);
            LOG_INFO ("%s",temp_data.c_str());
        }
        log.finalize();
        return utxx::path::list_files("/tmp", "logger.file_*.log");
    };

    auto cleanup = [] (std::list<std::string> const& files = std::list<std::string>()) {
        std::list<std::string> list(files);
        if (list.empty())
            list = utxx::path::list_files("/tmp", "logger.file_*.log").second;
        for (auto& f : list)
            utxx::path::file_unlink(utxx::path::join("/tmp", f));
        utxx::path::file_unlink("/tmp/logger.log"); // delete symlink
    };

    auto check = [] (auto& title, auto& files, uint n) {
        // Check that "/tmp/logger.file_{1..n}.log" are created
        if (files.size() != n)
          std::cerr << title << " has wrong number of files ("
                    << files.size() << ", expected: " << n << ')' << std::endl;
        BOOST_CHECK_EQUAL(n, files.size());
        auto pad_wid = static_cast<int>(std::ceil(std::log10(n))) + ((n%10 == 0) ? 1 : 0);
        for (uint i=1; i <= n; ++i) {
            //auto file  = static_cast<logger_impl_file*>(log.get_impl("file"))->get_file_name(i, false);
            char file[128];
            sprintf(file, "logger.file_%0*d.log", pad_wid, i);
            auto found = std::find(files.begin(), files.end(), file) != files.end();
            if (!found)
                std::cerr << title << ": file " << file << " is missing!" << std::endl;
            BOOST_CHECK(found);
        }
    };

    // Initial directory cleanup
    cleanup();

    utxx::variant_tree pt;
    const std::string filename_prefix = "logger.file";
    const char*       filename        = "/tmp/logger.file.log";
    pt.put("logger.timestamp",          utxx::variant("none"));
    pt.put("logger.show-ident",         false);
    pt.put("logger.show-location",      false);
    pt.put("logger.silent-finish",      true);
    pt.put("logger.file.stdout-levels", utxx::variant("debug|info|warning|error|fatal|alert"));
    pt.put("logger.file.filename",      utxx::variant(filename));
    pt.put("logger.file.append",        false);
    pt.put("logger.file.no-header",     true);
    pt.put("logger.file.split-file",    true);
    pt.put("logger.file.split-order",   utxx::variant("first"));
    pt.put("logger.file.split-size",    250); //size in bytes
    pt.put("logger.file.symlink",       utxx::variant("/tmp/logger.log"));

    //--------------------------------------------------------------------------
    // Check FIRST part order (unrestricted)
    //--------------------------------------------------------------------------
    auto res = write_test_data(pt);

    // Check that "/tmp/logger.file_{1..8}.log" are created
    check("first unrestricted", res.second, 8);

    BOOST_CHECK_EQUAL("I|write count: 99\n", utxx::path::read_file("/tmp/logger.file_1.log"));
    BOOST_CHECK      (utxx::path::read_file("/tmp/logger.file_8.log")
                                      .find("I|write count: 0\n") == 0);

    // Reopen the log
    pt.put("logger.file.append",        true);
    log.init(pt);
    LOG_INFO("write count: 100");
    log.finalize();

    BOOST_CHECK_EQUAL("I|write count: 99\n"
                      "I|write count: 100\n",
                      utxx::path::read_file("/tmp/logger.file_1.log"));

    BOOST_CHECK_EQUAL("/tmp/logger.file_1.log", utxx::path::file_readlink("/tmp/logger.log"));

    cleanup(res.second);

    //--------------------------------------------------------------------------
    // Check FIRST part order (restricted to 3 parts)
    //--------------------------------------------------------------------------
    pt.put("logger.file.split-parts", 3);

    res = write_test_data(pt);

    // Check that "/tmp/logger.file_{1..3}.log" are created
    check("first restricted 3", res.second, 3);

    BOOST_CHECK_EQUAL("I|write count: 99\n", utxx::path::read_file("/tmp/logger.file_1.log"));

    BOOST_CHECK_EQUAL("/tmp/logger.file_1.log", utxx::path::file_readlink("/tmp/logger.log"));

    cleanup(res.second);

    // Restrict to 11 parts, the names < 10 should be padded with '0'
    pt.put("logger.file.split-size",    120); //size in bytes
    pt.put("logger.file.split-parts",    11);

    res = write_test_data(pt);

    // Check that "/tmp/logger.file_{01..11}.log" are created (note zero padding)
    check("first restricted 11", res.second, 11);

    BOOST_CHECK_EQUAL("/tmp/logger.file_01.log", utxx::path::file_readlink("/tmp/logger.log"));

    cleanup(res.second);

    //--------------------------------------------------------------------------
    // Check LAST part order (unrestricted)
    //--------------------------------------------------------------------------
    pt.put("logger.file.split-order",   utxx::variant("last"));
    pt.put("logger.file.append",        false);
    pt.put("logger.file.split-parts",   0);
    pt.put("logger.file.split-size",    250); //size in bytes

    res = write_test_data(pt);

    // Check that "/tmp/logger.file_{1..8}.log" are created
    check("last unrestricted", res.second, 8);

    BOOST_CHECK_EQUAL("I|write count: 99\n", utxx::path::read_file("/tmp/logger.file_8.log"));
    BOOST_CHECK      (utxx::path::read_file("/tmp/logger.file_1.log")
                                      .find("I|write count: 0\n") == 0);
    // Reopen the log
    pt.put  ("logger.file.append", true);
    log.init(pt);
    LOG_INFO("write count: 100");
    log.finalize();

    BOOST_CHECK_EQUAL("I|write count: 99\n"
                      "I|write count: 100\n",
                      utxx::path::read_file("/tmp/logger.file_8.log"));
    BOOST_CHECK_EQUAL("/tmp/logger.file_8.log", utxx::path::file_readlink("/tmp/logger.log"));

    cleanup(res.second);

    //--------------------------------------------------------------------------
    // Check LAST part order (restricted to 3 parts)
    //--------------------------------------------------------------------------
    pt.put("logger.file.split-parts", 3);

    res = write_test_data(pt);

    // Check that "/tmp/logger.file_{1..3}.log" are created
    check("last restricted", res.second, 3);

    BOOST_CHECK_EQUAL("I|write count: 99\n", utxx::path::read_file("/tmp/logger.file_3.log"));
    BOOST_CHECK_EQUAL("/tmp/logger.file_3.log", utxx::path::file_readlink("/tmp/logger.log"));

    cleanup(res.second);

    //--------------------------------------------------------------------------
    // Check ROTATE part order (restricted to 5 parts)
    //--------------------------------------------------------------------------
    pt.put("logger.file.split-parts", 5);
    pt.put("logger.file.split-order", utxx::variant("rotate"));

    res = write_test_data(pt);

    BOOST_CHECK_EQUAL("I|write count: 99\n", utxx::path::read_file("/tmp/logger.file_3.log"));
    BOOST_CHECK_EQUAL("/tmp/logger.file_3.log", utxx::path::file_readlink("/tmp/logger.log"));
    BOOST_CHECK      (utxx::path::read_file("/tmp/logger.file_4.log")
                                      .find("I|write count: 43\n") == 0);

    cleanup(res.second);

    // Create one log file and ensure that rotation starts off with it
    pt.put("logger.file.split-parts", 10);
    utxx::path::write_file("/tmp/logger.file_05.log", "I|write count: -1\n");

    res = write_test_data(pt);

    BOOST_CHECK_EQUAL("I|write count: 98\n"
                      "I|write count: 99\n", utxx::path::read_file("/tmp/logger.file_02.log"));
    BOOST_CHECK      ( utxx::path::file_exists("/tmp/logger.file_01.log"));
    BOOST_CHECK      ( utxx::path::file_exists("/tmp/logger.file_02.log"));
    BOOST_CHECK      (!utxx::path::file_exists("/tmp/logger.file_03.log"));
    BOOST_CHECK      (!utxx::path::file_exists("/tmp/logger.file_04.log"));
    BOOST_CHECK      ( utxx::path::file_exists("/tmp/logger.file_05.log"));
    BOOST_CHECK      ( utxx::path::file_exists("/tmp/logger.file_06.log"));
    BOOST_CHECK      ( utxx::path::file_exists("/tmp/logger.file_07.log"));
    BOOST_CHECK      ( utxx::path::file_exists("/tmp/logger.file_08.log"));
    BOOST_CHECK      ( utxx::path::file_exists("/tmp/logger.file_09.log"));
    BOOST_CHECK      ( utxx::path::file_exists("/tmp/logger.file_10.log"));
    BOOST_CHECK_EQUAL("/tmp/logger.file_02.log", utxx::path::file_readlink("/tmp/logger.log"));
    BOOST_CHECK      (utxx::path::read_file("/tmp/logger.file_05.log")
                                      .find("I|write count: -1\n"
                                            "I|write count: 0\n") == 0);
    cleanup(res.second);
}


BOOST_AUTO_TEST_CASE( test_logger2 )
{
    variant_tree pt;
    pt.put("logger.timestamp",             utxx::variant("time-usec"));
    pt.put("logger.show-thread",           false);
    pt.put("logger.show-ident",            false);
    pt.put("logger.ident",                 utxx::variant("my-logger"));
    pt.put("logger.silent-finish",         true);

    logger& log = logger::instance();

    {
        pt.put("logger.min-level-filter",      utxx::variant("debug"));
        pt.put("logger.console.stdout-levels", utxx::variant("info|notice|warning|error"));
        if (log.initialized())
            log.finalize();

        log.init(pt, nullptr, false);

        BOOST_REQUIRE(log.initialized());
    }

    {
        pt.put("logger.min-level-filter",      utxx::variant("debug"));
        pt.put("logger.console.stdout-levels", utxx::variant("debug|notice|warning|error"));

        if (log.initialized())
            log.finalize();

        log.init(pt, nullptr, false);
    }

    {
        pt.put("logger.min-level-filter",      utxx::variant("debug"));
        pt.put("logger.console.stdout-levels", utxx::variant("trace|debug|notice|warning|error"));

        if (log.initialized())
            log.finalize();

        try {
            log.init(pt, nullptr, false);
            BOOST_CHECK(false);
        } catch (utxx::runtime_error& e) {
            auto exp = "Console logger's stdout levels filter "
                       "'TRACE|DEBUG|NOTICE|WARNING|ERROR' is less granular "
                       "than logger's default 'DEBUG'";
            BOOST_CHECK_EQUAL(exp, e.str());
        }
    }

    log.finalize();
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

    pt.put("logger.timestamp",  utxx::variant("time-usec"));
    pt.put("logger.console.stdout-levels", utxx::variant("debug|notice|info|warning|error|fatal|alert"));
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
        BOOST_TEST_MESSAGE("Process has " << p.second.c_str() << " handler -> "
                           << sigismember(&mask, p.first));

    bool crash = getenv("UTXX_LOGGER_CRASH") && atoi(getenv("UTXX_LOGGER_CRASH"));

    if  (crash) {
        double* p = nullptr;
        kill(getpid(), SIGABRT);
        *p = 10.0;
    }

    #ifndef UTXX_STANDALONE
    BOOST_REQUIRE(true); // Keep the warning off
    #endif
}
