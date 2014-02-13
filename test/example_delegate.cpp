#include <stdio.h>
#include <utxx/event.hpp>
#include <utxx/delegate.hpp>
#include <boost/signals2.hpp>
#include <boost/timer/timer.hpp>
#include <utxx/perf_histogram.hpp>
#include <iostream>

typedef utxx::delegate<
    void (const char*, /* format */
          int          /* args */)
> delegate_t;

void m(const char* fmt, int n) {
    static int s_sum;
    s_sum += n;
}

struct Test {
    int m_sum;

    Test() : m_sum(0) {}

    void operator() (const char* fmt, int n) { m_sum += n; }
    static void ms(const char* fmt, int n) {
        static int s_sum;
        s_sum += n;
    }

    void test_binder(int ITERS, int a_mask, bool a_hist) {
        utxx::event_binder<delegate_t>  bin_binder[3];
        utxx::event_source<delegate_t>  source;

        if (a_mask & (1 << 0))
            bin_binder[0].bind(source, delegate_t::from_method<Test, &Test::operator()>(this));
        if (a_mask & (1 << 1))
            bin_binder[1].bind(source, delegate_t::from_function<&Test::ms>());
        if (a_mask & (1 << 2))
            bin_binder[2].bind(source, delegate_t::from_function<&m>());

        if (a_hist) {
            utxx::perf_histogram h("binder");

            for (int i = 0; i < ITERS; ++i) {
                utxx::perf_histogram::sample s(h);
                source(delegate_t::invoker_type("binder %d\n", 1));
            }
            h.dump(std::cout, 100);
        } else {
            boost::timer::auto_cpu_timer t;
            for (int i = 0; i < ITERS; ++i)
                source(delegate_t::invoker_type("binder %d\n", 1));
            boost::timer::cpu_times elapsed = t.elapsed();
            std::cout << std::left << std::setw(25)
                << "binder speed: "
                << std::setprecision(4)
                << ((double)(elapsed.user+elapsed.system) / ITERS / 1000)
                << " us/call" << std::endl;
        }
    }

    void test_signal(int ITERS, int a_mask, bool a_hist) {
        utxx::signal<delegate_t> signal;

        if (a_mask & (1 << 0))
            signal.connect(delegate_t::from_method<Test, &Test::operator()>(this));
        if (a_mask & (1 << 1))
            signal.connect(delegate_t::from_function<&Test::ms>());
        if (a_mask & (1 << 2))
            signal.connect(delegate_t::from_function<&m>());

        if (a_hist) {
            utxx::perf_histogram h("signal");
            for (int i = 0; i < ITERS; ++i) {
                utxx::perf_histogram::sample s(h);
                signal(delegate_t::invoker_type("signal %d\n", 1));
            }
            h.dump(std::cout, 100);
        } else {
            boost::timer::auto_cpu_timer t;
            for (int i = 0; i < ITERS; ++i)
                signal(delegate_t::invoker_type("signal %d\n", 1));
            boost::timer::cpu_times elapsed = t.elapsed();
            std::cout << std::left << std::setw(25)
                << "signal speed: "
                << std::setprecision(4)
                << ((double)(elapsed.user+elapsed.system) / ITERS / 1000)
                << " us/call" << std::endl;
        }
    }

    void test_signals2(int ITERS, int a_mask, bool a_hist) {
        boost::signals2::signal<void (const char*, int)> sig;

        if (a_mask & (1 << 0)) sig.connect(*this);
        if (a_mask & (1 << 1)) sig.connect(&Test::ms);
        if (a_mask & (1 << 2)) sig.connect(&m);

        if (a_hist) {
            utxx::perf_histogram h("boost::signals2");
            for (int i = 0; i < ITERS; ++i) {
                utxx::perf_histogram::sample s(h);
                sig("boost::signals %d\n", 1);
            }
            h.dump(std::cout, 100);
        } else {
            boost::timer::auto_cpu_timer t;
            for (int i = 0; i < ITERS; ++i)
                sig("boost::signal2 %d\n", 1);
            boost::timer::cpu_times elapsed = t.elapsed();
            std::cout << std::left << std::setw(25)
                << "boost::signals2 speed: "
                << std::setprecision(4)
                << ((double)(elapsed.user+elapsed.system) / ITERS / 1000)
                << " us/call" << std::endl;
        }
    }

    void test_bind(int ITERS, int a_mask, bool a_hist) {
        typedef boost::function<void (const char*, int)> bind_type;
        typedef typename std::vector<bind_type*>::iterator bind_iter;
        std::vector<bind_type*> sig;
        if (a_mask & (1 << 0))
            sig.push_back(new bind_type(boost::bind(&Test::operator(), this, _1, _2)));
        if (a_mask & (1 << 1))
            sig.push_back(new bind_type(&Test::ms));
        if (a_mask & (1 << 2))
            sig.push_back(new bind_type(&m));

        if (a_hist) {
            utxx::perf_histogram h("boost::bind");
            for (int i = 0; i < ITERS; ++i) {
                utxx::perf_histogram::sample s(h);
                for (bind_iter it=sig.begin(), e=sig.end(); it != e; ++it)
                   if (**it) (**it)("boost::bind %d\n", 1);
            }
            h.dump(std::cout, 100);
        } else {
            boost::timer::auto_cpu_timer t;
            for (int i = 0; i < ITERS; ++i) {
                for (bind_iter it=sig.begin(), e=sig.end(); it != e; ++it)
                   if (**it) (**it)("boost::bind %d\n", 1);
            }
            boost::timer::cpu_times elapsed = t.elapsed();
            std::cout << std::left << std::setw(25)
                << "boost::bind speed: "
                << std::setprecision(4)
                << ((double)(elapsed.user+elapsed.system) / ITERS / 1000)
                << " us/call" << std::endl;
        }

        for (bind_iter it=sig.begin(), e=sig.end(); it != e; ++it)
            delete (*it);
    }
};

int main(int argc, char* argv[])
{
    std::cout
        << "This program measures performance of utxx::signal vs boost::signals2\n\n"
        << "Usage: [ITERATIONS=Integer]" << argv[0] << " TimingMethod Tests\n"
        << "    TimingMethod    - UseHistogram(1) | UseGenericTimer(0)\n"
        << "    Tests           - Integer mask of which methods to profile:\n"
        << "                        1 - member function\n"
        << "                        2 - static class member function\n"
        << "                        4 - static function\n"
        << "                        7 - all of the above\n"
        << std::endl;
    const int ITERS = ::getenv("ITERATIONS") ? atoi(::getenv("ITERATIONS")) : 1000000;

    int hist = argc > 1 ? atoi(argv[1]) : true;
    int mask = argc > 2 ? atoi(argv[2]) : 7;

    std::cout << "Iterations: " << ITERS << std::endl;

    { Test t; t.test_binder     (ITERS, mask, hist); }
    { Test t; t.test_signal     (ITERS, mask, hist); }
    { Test t; t.test_bind       (ITERS, mask, hist); }
    { Test t; t.test_signals2   (ITERS, mask, hist); }

    return 0;
}
