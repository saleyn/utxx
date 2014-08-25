//#define BOOST_TEST_MODULE logger_test
#include <boost/test/unit_test.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/property_tree/info_parser.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <utxx/perf_histogram.hpp>
#include <utxx/logger.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/test_helper.hpp>
#include <utxx/high_res_timer.hpp>
#include <atomic>

using namespace boost::property_tree;
using namespace utxx;

BOOST_AUTO_TEST_CASE( test_async_logger )
{
    variant_tree pt;
    const char* filename = "/tmp/logger.async.file.log";
    const int iterations = 1000;

    pt.put("logger.timestamp",    variant("no_timestamp"));
    pt.put("logger.show_ident",   variant(false));
    pt.put("logger.show_location",variant(false));
    pt.put("logger.async_file.stdout_levels", variant("debug|info|warning|error|fatal|alert"));
    pt.put("logger.async_file.filename",  variant(filename));
    pt.put("logger.async_file.append", variant(false));

    if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
        write_info(std::cout, pt);

    BOOST_REQUIRE(pt.get_child_optional("logger.async_file"));

    logger& log = logger::instance();
    log.init(pt);

    for (int i = 0, n = 0; i < iterations; i++) {
        LOG_ERROR  (("(%d) This is an error #%d", ++n, 123));
        LOG_WARNING(("(%d) This is a %s", ++n, "warning"));
        LOG_FATAL  (("(%d) This is a %s", ++n, "fatal error"));
        LOG_CAT_ERROR  ("Cat1", ("(%d) This is an error #%d", ++n, 456));
        LOG_CAT_WARNING("Cat2", ("(%d) This is a %s", ++n, "warning"));
        LOG_CAT_FATAL  ("Cat3", ("(%d) This is a %s", ++n, "fatal error"));
    }


    log.finalize();

    {
        std::ifstream in(filename);
        BOOST_REQUIRE(in);
        std::string s, exp;
        for (int i = 0, n = 0; i < iterations; i++) {
            char buf[128];
            getline(in, s);
            sprintf(buf, "|ERROR  |||(%d) This is an error #%d", ++n, 123); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|WARNING|||(%d) This is a %s", ++n, "warning"); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|FATAL  |||(%d) This is a %s", ++n, "fatal error"); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|ERROR  ||Cat1|(%d) This is an error #%d", ++n, 456); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|WARNING||Cat2|(%d) This is a warning", ++n); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
            getline(in, s);
            sprintf(buf, "|FATAL  ||Cat3|(%d) This is a fatal error", ++n); exp = buf;
            BOOST_REQUIRE_EQUAL(exp, s);
        }
        BOOST_REQUIRE(!getline(in, s));
    }

    ::unlink(filename);
}

std::string get_data(std::ifstream& in, int& thread, int& num, struct tm& tm) {
    std::string s;
    if (!getline(in, s))
        return "";
    char* end;
    int n = strtol(s.c_str(), &end, 10);
    int y = n / 10000; n -= y*10000;
    int m = n / 100;   n -= m*100;
    tm.tm_year = y - 1900;
    tm.tm_mon  = m - 1;
    tm.tm_mday = n;
    tm.tm_hour = strtol(end+1, &end, 10);
    tm.tm_min  = strtol(end+1, &end, 10);
    tm.tm_sec  = strtol(end+1, &end, 10);
    end += 2; // Skip "||"
    const char* p = end + 16;
    thread = strtol(p, &end, 10);
    while(*end == ' ') end++;
    p = end;
    num = strtol(p, &end, 10);

    return s.c_str() + s.find_first_of('|');
}

void verify_result(const char* filename, int threads, int iterations, int thr_msgs)
{
    std::ifstream in(filename);
    BOOST_REQUIRE(!in.fail());
    std::string s, exp;
    long num[threads], last_time[threads];

    time_t t = time(NULL);
    struct tm exptm, tm;
    localtime_r(&t, &exptm);

    for (int i=0; i < threads; ++i)
        num[i] = -1, last_time[i] = 0;

    unsigned long cur_time, l_time = 0, time_miss = 0;

    for (int i = 0, n = 0; i < threads * iterations; i++) {
        char buf[128];
        int th, j;
        const struct {
            const char* type;
            const char* msg;
        } my_data[] = {{"ERROR  ", "This is an error #123"}
                      ,{"WARNING", "This is a warning"}
                      ,{"FATAL  ", "This is a fatal error"}};
        for (int k=0; k < thr_msgs; ++k) {
            n++;
            s = get_data(in, th, j, tm);
            int idx = j % thr_msgs;
            sprintf(buf, "|%s|||%d %9ld %s", my_data[idx].type, th, ++num[th-1], my_data[idx].msg);
            exp = buf;
            if (exp != s) std::cerr << "File " << filename << ":" << n << std::endl;
            BOOST_REQUIRE_EQUAL(exp, s);
            BOOST_REQUIRE_EQUAL(exptm.tm_year, tm.tm_year);
            BOOST_REQUIRE_EQUAL(exptm.tm_mon,  tm.tm_mon);
            BOOST_REQUIRE_EQUAL(exptm.tm_mday, tm.tm_mday);
            cur_time = tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
            if (last_time[th-1] > (long)cur_time)
                std::cerr << "File " << filename << ":" << n
                          << " last_time=" << (last_time[th-1] / 3600) << ':'
                          << (last_time[th-1] % 3600 / 60) << ':' << (last_time[th-1] % 60)
                          << ", cur_time=" << tm.tm_hour << ':'
                          << tm.tm_min << ':' << tm.tm_sec << std::endl;
            if (l_time > cur_time)
                time_miss++;
            BOOST_REQUIRE(last_time[th-1] <= (long)cur_time);
            last_time[th-1] = cur_time;
            l_time = cur_time;
        }
    }
    BOOST_REQUIRE(!getline(in, s));

    if (utxx::verbosity::level() > utxx::VERBOSE_NONE) {
        for(int i=1; i <= threads; i++)
            fprintf(stderr, "Verified %ld messages for thread %d\n", num[i-1]+1, i);
        fprintf(stderr, "Out of sequence time stamps: %ld\n", time_miss);
    }
}

struct worker {
    int                 id;
    std::atomic<long>&  count;
    int                 iterations;
    boost::barrier&     barrier;

    worker(int a_id, int it, std::atomic<long>& a_cnt, boost::barrier& b)
        : id(a_id), count(a_cnt), iterations(it), barrier(b)
    {}

    void operator() () {
        barrier.wait();
        for (int i=0, n=-1; i < iterations; i++) {
            count.fetch_add(1, std::memory_order_relaxed);
            LOG_ERROR  (("%d %9d This is an error #%d", id, ++n, 123));
            LOG_WARNING(("%d %9d This is a %s", id, ++n, "warning"));
            LOG_FATAL  (("%d %9d This is a %s", id, ++n, "fatal error"));
        }
        if (utxx::verbosity::level() != utxx::VERBOSE_NONE)
            fprintf(stderr, "Worker %d finished (count=%ld)\n",
                    id, count.load(std::memory_order_relaxed));
    }
};

BOOST_AUTO_TEST_CASE( test_async_logger_concurrent )
{
    variant_tree pt;
    const char* filename = "/tmp/logger.file.log";
    const int iterations = 100000;

    pt.put("logger.timestamp",    variant("date-time-usec"));
    pt.put("logger.show-ident",   variant(false));
    pt.put("logger.show-location",variant(false));
    pt.put("logger.async-file.stdout_levels", variant("debug|info|warning|error|fatal|alert"));
    pt.put("logger.async-file.filename",  variant(filename));
    pt.put("logger.async-file.append", variant(false));

    BOOST_REQUIRE(pt.get_child_optional("logger.async-file"));

    logger& log = logger::instance();
    log.init(pt);

    const int threads = ::getenv("THREAD") ? atoi(::getenv("THREAD")) : 3;
    boost::barrier barrier(threads+1);
    std::atomic<long> count(0);
    int id = 0;

    boost::shared_ptr<worker> workers[threads];
    boost::shared_ptr<boost::thread>  thread [threads];

    for (int i=0; i < threads; i++) {
        workers[i] = boost::shared_ptr<worker>(
                        new worker(++id, iterations, count, barrier));
        thread[i]  = boost::shared_ptr<boost::thread>(new boost::thread(boost::ref(*workers[i])));
    }

    barrier.wait();

    for (int i=0; i < threads; i++)
        thread[i]->join();

    log.finalize();

    verify_result(filename, threads, iterations, 3);

    ::unlink(filename);
}

struct latency_worker {
    int              id;
    int              iterations;
    boost::barrier&  barrier;
    perf_histogram*  histogram;
    double*          elapsed;

    latency_worker(int a_id, int it, boost::barrier& b, perf_histogram* h, double* time)
        : id(a_id), iterations(it), barrier(b), histogram(h), elapsed(time)
    {}

    void operator() () {
        barrier.wait();
        histogram->reset(to_string("Hist", id).c_str());
        bool no_histogram = getenv("NOHISTOGRAM");

        time_val start = time_val::universal_time();

        for (int i=0; i < iterations; i++) {
            if (!no_histogram) histogram->start();
            LOG_ERROR  (("%d %9d This is an error #123", id, i));
            if (!no_histogram) histogram->stop();
        }

        *elapsed = time_val::now_diff(start);

        if (utxx::verbosity::level() != utxx::VERBOSE_NONE)
            fprintf(stdout,
                    "Performance thread %d finished (speed=%7d ops/s, lat=%.3f us)\n",
                    id, (int)((double)iterations / *elapsed),
                    *elapsed * 1000000 / iterations);
    }
};

enum open_mode {
      MODE_APPEND
    , MODE_OVERWRITE
    , MODE_NO_MUTEX
};

void run_test(const char* config_type, open_mode mode, int def_threads)
{
    BOOST_MESSAGE("Testing back-end: " << config_type);
    variant_tree pt;
    const char* filename = "/tmp/logger.file.log";
    const int iterations = ::getenv("ITERATIONS") ? atoi(::getenv("ITERATIONS")) : 1000000;

    ::unlink(filename);

    pt.put("logger.timestamp",    variant("date-time-usec"));
    pt.put("logger.show-ident",   variant(false));
    pt.put("logger.show-location",variant(false));

    std::string s("logger."); s += config_type;
    pt.put(s + ".stdout-levels", variant("debug|info|warning|error|fatal|alert"));
    pt.put(s + ".filename",  filename);
    pt.put(s + ".append",    mode == MODE_APPEND);
    pt.put(s + ".use-mutex", mode == MODE_OVERWRITE);

    logger& log = logger::instance();
    log.init(pt);

    const int threads = ::getenv("THREAD") ? atoi(::getenv("THREAD")) : def_threads;
    boost::barrier barrier(threads+1);
    int id = 0;

    boost::shared_ptr<latency_worker>   workers[threads];
    boost::shared_ptr<boost::thread>    thread [threads];
    double                              elapsed[threads];
    perf_histogram                      histograms[threads];

    for (int i=0; i < threads; i++) {
        workers[i] = boost::shared_ptr<latency_worker>(
                        new latency_worker(++id, iterations, barrier,
                                           &histograms[i], &elapsed[i]));
        thread[i]  = boost::shared_ptr<boost::thread>(
                        new boost::thread(boost::ref(*workers[i])));
    }

    barrier.wait();

    perf_histogram totals("Total logger_async_file performance");
    double sum_time = 0;

    for (int i=0; i < threads; i++) {
        thread[i]->join();
        totals += histograms[i];
        sum_time += elapsed[i];
    }

    log.finalize();


    if (verbosity::level() >= utxx::VERBOSE_DEBUG) {
        sum_time /= threads;
        printf("Avg speed = %8d it/s, latency = %.3f us\n",
               (int)((double)iterations / sum_time),
               sum_time * 1000000 / iterations);
        if (!getenv("NOHISTOGRAM"))
            totals.dump(std::cout);
    }

    if (!getenv("NOVERIFY"))
        verify_result(filename, threads, iterations, 1);

    ::unlink(filename);
}

BOOST_AUTO_TEST_CASE( test_logger_async_file_perf )
{
    run_test("async-file", MODE_OVERWRITE, 3);
}

BOOST_AUTO_TEST_CASE( test_logger_file_perf_overwrite )
{
    run_test("file", MODE_OVERWRITE, 3);
}

BOOST_AUTO_TEST_CASE( test_logger_file_perf_append )
{
    run_test("file", MODE_APPEND, 3);
}

// Note that this test should fail when THREAD environment is set to
// a value > 1 for thread-safety reasons described in the logger_impl_file.hpp.
// We use default thread count = 1 to avoid the failure.
BOOST_AUTO_TEST_CASE( test_logger_file_perf_no_mutex )
{
    run_test("file", MODE_NO_MUTEX, 1);
}

