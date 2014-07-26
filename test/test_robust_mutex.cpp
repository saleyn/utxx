/*
 * robust.c
 *
 * Demonstrates robust mutexes.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>
#ifdef HAVE_BOOST_TIMER_TIMER_HPP
#include <boost/timer/timer.hpp>
#endif
#include <utxx/robust_mutex.hpp>
#include <utxx/verbosity.hpp>

namespace {
    template <int N>
    struct basic_buffer {
        pthread_mutex_t mutex1;
        pthread_mutex_t mutex2;
        char data[N];
    };

    static const int N = 128;
    typedef basic_buffer<N> buffer_t;

    utxx::robust_mutex m1;
    utxx::robust_mutex m2;

    void failing_thread(buffer_t* b)
    {
        const char* name = "Owner   ";
        if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
            fprintf(stderr, "Started %s %d\n", name, getpid());

        /* Lock mutex1 to make thread1 wait */
        try { m1.lock(); } catch (std::exception& e) {
            if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
                fprintf(stderr, "%s %d: mutex1 error: %s\n", name, getpid(), e.what());
            exit(1);
        }
        try { m2.lock(); } catch (std::exception& e) {
            if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
                fprintf(stderr, "%s %d: mutex2 error: %s\n", name, getpid(), e.what());
            exit(1);
        }

        if (utxx::verbosity::level() > utxx::VERBOSE_NONE) {
            fprintf(stderr, "%s %d: mutex1 acquired\n", name, getpid());
            fprintf(stderr, "%s %d: mutex2 acquired\n", name, getpid());
        }

        sleep(1);

        if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
            fprintf(stderr, "%s %d: Allow threads to run\n", name, getpid());

        b->data[0]++;
        m1.unlock();

        if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
            fprintf(stderr, "%s %d: mutex1 released -> exiting\n", name, getpid());
    }

    int on_owner_dead(const char* name, utxx::robust_mutex* m) {
        int b = m->make_consistent();
        if (utxx::verbosity::level() > utxx::VERBOSE_NONE) {
            if (!b)
                fprintf(stderr, "%s %d: mutex2 owner died, made consistent\n", name, getpid());
            else
                fprintf(stderr, "%s %d: mutex2 owner died, consistent failed: %s\n",
                    name, getpid(), strerror(b));
        }

        return b;
    }

    void waiting_thread(buffer_t* b)
    {
        const char* name = "Consumer";

        if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
            fprintf(stderr, "%s %d: wait on mutex2\n", name, getpid());

        m2.on_make_consistent = boost::bind(&on_owner_dead, name, &m2);

        try { m2.lock(); } catch (std::exception& e) {
            if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
                fprintf(stderr, "%s %d: Error waiting on mutex2: %s", name, getpid(), e.what());
            exit(EXIT_FAILURE);
        }

        if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
            fprintf(stderr, "%s %d: mutex2 acquired\n", name, getpid());
        b->data[0]++;
        m2.unlock();
        if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
            fprintf(stderr, "%s %d: unlocked mutex2 and exiting\n", name, getpid());
    }

} // namespace

BOOST_AUTO_TEST_CASE( test_robust_mutex )
{
    buffer_t* buffer;

    int zfd = open("/dev/zero", O_RDWR);
    buffer = (buffer_t *)mmap(NULL, sizeof(buffer_t),
        PROT_READ|PROT_WRITE, MAP_SHARED, zfd, 0);
    (void)ftruncate(zfd, sizeof(buffer_t));
    close(zfd);

    bzero(buffer->data, sizeof(buffer->data));

    m1.init(buffer->mutex1);
    m2.init(buffer->mutex2);

    pid_t children[3];

    if ((children[0] = fork()) == 0) {
        failing_thread(buffer);
        exit(0);
    }

    sleep(1);

    for (size_t i=1; i < sizeof(children)/sizeof(children[0]); i++)
        if ((children[i] = fork()) == 0) {
            waiting_thread(buffer);
            exit(0);
        }

    int status;
    for (size_t i=0; i < sizeof(children)/sizeof(children[0]); i++)
        waitpid(children[i], &status, 0);

    int result = buffer->data[0];

    munmap(buffer, sizeof(buffer_t));

    if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
        fprintf(stderr, "Main process exited (b=%d)\n", result);

    BOOST_REQUIRE_EQUAL(3, result);
}

#ifdef HAVE_BOOST_TIMER_TIMER_HPP

BOOST_AUTO_TEST_CASE( test_robust_mutex_perf )
{
    using boost::timer::cpu_timer;
    using boost::timer::cpu_times;
    using boost::timer::nanosecond_type;

    pthread_mutex_t mutex1;
    utxx::robust_mutex m(mutex1, true);

    int k = getenv("ITERATIONS")   ? atoi(getenv("ITERATIONS")) : 100000;
    nanosecond_type t1, t2;

    {
        cpu_timer t;
        for (int i = 0; i < k; ++i) {
            m.lock();
            m.unlock();
        }
        cpu_times elapsed_times(t.elapsed());
        t1 = elapsed_times.system + elapsed_times.user;
        if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
            printf("Robust mutex time: %.3fs (%.3fus/call)\n",
                (double)t1 / 1000000000.0, (double)t1 / k / 1000.0);
    }

    {
        boost::mutex mtx;
        cpu_timer t;
        for (int i = 0; i < k; ++i) {
            mtx.lock();
            mtx.unlock();
        }
        cpu_times elapsed_times(t.elapsed());
        t2 = elapsed_times.system + elapsed_times.user;
        if (utxx::verbosity::level() > utxx::VERBOSE_NONE)
            printf("Simple mutex time: %.3fs (%.3fus/call)\n",
                (double)t2 / 1000000000.0, (double)t2 / k / 1000.0);
    }

    if (k > 25000) {
        BOOST_REQUIRE(t1);
        BOOST_REQUIRE(t2);
        BOOST_REQUIRE(t1 > t2);
    } else {
        BOOST_REQUIRE(true);
    }
}

#endif
