// #define BOOST_TEST_MODULE high_res_timer_test 

#include <boost/test/unit_test.hpp>
#include <utxx/high_res_timer.hpp>
#include <utxx/cpu.hpp>
#include <utxx/verbosity.hpp>
#include <stdio.h>

using namespace utxx;

BOOST_AUTO_TEST_CASE( calibration_test )
{
    int iter  = atoi(getenv("HR_ITERATIONS") ? getenv("HR_ITERATIONS") : "4");
    int us    = atoi(getenv("HR_USEC")       ? getenv("HR_USEC") : "250000");
    int afreq = high_res_timer::get_cpu_frequency();
    int bfreq = high_res_timer::calibrate(us, iter);

    if (verbosity::level() != VERBOSE_NONE) {
        printf("high_res_timer::get_cpu_frequency()  = %d\n", afreq);
        printf("high_res_timer::calibrate(%d, %d) = %d\n", us, iter, bfreq);
    }

    BOOST_CHECK((iter*us) > 4000000 ? bfreq >= afreq : true);
}

BOOST_AUTO_TEST_CASE( get_cpu_time_test )
{
    high_res_timer timer;
    int iterations = 1000000;
    int N = detail::cpu_count();
    BOOST_REQUIRE(N > 0);
    int cpus[64];

    memset(cpus, 0, sizeof(cpus));

    for (int i=0; i < iterations; i++) {
        timer.start_incr();
        int n = detail::apic_id();
        timer.stop_incr();
        cpus[n]++;
    }

    if (verbosity::level() != VERBOSE_NONE)
        printf("apic_id      latency = %llu\n", timer.elapsed_nsec_incr() / iterations);
    /*
    for(int i=0; i < sizeof(cpus)/sizeof(int); ++i)
        if (cpus[i] > 0)
            printf("  used cpu%d\n", i);
    */
    memset(cpus, 0, sizeof(cpus));
    timer.reset();

    for (int i=0; i < iterations; i++) {
        timer.start_incr();
        int n = sched_getcpu();
        timer.stop_incr();
        cpus[n]++;
    }

    if (verbosity::level() != VERBOSE_NONE) {
        printf("sched_getcpu latency = %llu\n", timer.elapsed_nsec_incr() / iterations);
        for(unsigned int i=0; i < sizeof(cpus)/sizeof(int); ++i)
            if (cpus[i] > 0 )
                printf("  used cpu%u\n", i);
    }

    timer.reset();

    timeval tv;
    for (int i=0; i < iterations; i++) {
        timer.start_incr();
        gettimeofday(&tv, NULL);
        timer.stop_incr();
    }

    if (verbosity::level() != VERBOSE_NONE)
        printf("gettimeofday latency = %llu\n", timer.elapsed_nsec_incr() / iterations);
    BOOST_CHECK(true);
}

#include <sched.h>
#include <unistd.h>

BOOST_AUTO_TEST_CASE( cpu_calibration )
{
    cpu_set_t cs;
    BOOST_REQUIRE(sched_getaffinity (getpid (), sizeof (cs), &cs) >= 0);

    int cpu = 0;

    while (CPU_COUNT (&cs) != 0) {
        if (CPU_ISSET (cpu, &cs)) {
            cpu_set_t cs2;
            CPU_ZERO (&cs2);
            CPU_SET (cpu, &cs2);
            BOOST_REQUIRE(sched_setaffinity (getpid (), sizeof (cs2), &cs2) >= 0);
            int cpu2 = sched_getcpu();
            BOOST_REQUIRE(cpu2 > -1);   // if -1 && errno == ENOSYS
                                        // => getcpu syscall not implemented
            BOOST_REQUIRE_EQUAL(cpu, cpu2);

            // Perform cpu-specific calibration
            {
                const time_val sleep_time(0, 1000);
                unsigned long long delta_hrtime  = 0;
                unsigned long long actual_sleeps = 0;
                unsigned int iterations = 10;

                for (int i = 0; i < (int)iterations; ++i) {
                    timeval t1;
                    ::gettimeofday(&t1, NULL);
                    const hrtime_t start = detail::get_tick_count();
                    ::usleep(sleep_time.microseconds());
                    const hrtime_t stop  = detail::get_tick_count();
                    time_val actual_delta(abs_time(-t1.tv_sec, -t1.tv_usec));

                    unsigned long long delta   = actual_delta.microseconds();
                    unsigned long long hrdelta =
                        (stop > start) ? (stop - start) : (~start + 1 + stop);
                    // Store the sample.
                    delta_hrtime  += hrdelta;
                    actual_sleeps += delta;
                    /*
                    if (hrdelta / delta != 2660)
                        printf("  cpu%d hrdelta = %12llu delta = %9llu (%d)\n", 
                                cpu, hrdelta, delta, hrdelta / delta);
                    */
                }

                delta_hrtime  /= iterations;
                actual_sleeps /= iterations;

                // The addition of 5 below rounds instead of truncates.
                int global_scale_factor =
                    static_cast<int>((double)delta_hrtime / actual_sleeps + .5);
                if (verbosity::level() != VERBOSE_NONE)
                    printf("CPU%d calibration = %d\n", cpu2, global_scale_factor);
            }

            CPU_CLR (cpu, &cs);
        }
        ++cpu;
    }
}

