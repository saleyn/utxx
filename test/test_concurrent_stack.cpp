#include <boost/test/unit_test.hpp>
#include <boost/thread/barrier.hpp>
#include <utxx/container/concurrent_stack.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/atomic.hpp>
#include <vector>
#include <thread>
#include <stdio.h>

using namespace utxx;
using namespace utxx::container;

class int_t : public versioned_stack::node_t {
    int m_data;
    int m_id;
public:
    int  data() const { return m_data; }
    int  id()   const { return m_id; }
    void data(int n) { m_data = n; }
    int_t(int n, int a_id=0) : m_data(n), m_id(a_id) {}
    int_t() : m_data(0), m_id(0) {}
};

BOOST_AUTO_TEST_CASE( test_concurrent_stack_versioned )
{
    versioned_stack stack;

    int_t nodes[10];
    for(int i=0; i < 10; i++) {
        nodes[i].data(i+1);
        stack.push(&nodes[i]);
    }

    for(int i=10; i > 0; i--) {
        int_t* n = static_cast<int_t*>(stack.pop());
        BOOST_REQUIRE_EQUAL(i, n->data());
    }

    BOOST_REQUIRE(stack.empty());
}

BOOST_AUTO_TEST_CASE( test_concurrent_stack_versioned_reset )
{
    versioned_stack stack;

    int_t nodes[10];
    for(int i=0; i < 10; i++) {
        nodes[i].data(i+1);
        stack.push(&nodes[i]);
    }

    int_t* p = static_cast<int_t*>(stack.reset());
    BOOST_REQUIRE(stack.empty());

    int i;

    for(i=10; p; p = static_cast<int_t*>(p->next), i--) {
        BOOST_REQUIRE_EQUAL(i, p->data());
    }

    BOOST_REQUIRE_EQUAL(0, i);

    // Test list reversal
    for(i=0; i < 10; i++) {
        stack.push(&nodes[i]);
    }

    p = static_cast<int_t*>(stack.reset(true));
    BOOST_REQUIRE(stack.empty());

    for(i=1; p; p = static_cast<int_t*>(p->next), i++) {
        BOOST_REQUIRE_EQUAL(i, p->data());
    }

    BOOST_REQUIRE_EQUAL(11, i);
}

struct sproducer {
    int              id;
    volatile long&   count;
    int              iterations;
    boost::barrier&  barrier;
    blocking_versioned_stack<>& stack;

    sproducer(int a_id, int it, volatile long& a_cnt, boost::barrier& b,
            blocking_versioned_stack<>& s)
        : id(a_id), count(a_cnt), iterations(it), barrier(b), stack(s)
    {}

    void operator() () {
        barrier.wait();
        for (int i=0; i < iterations; i++) {
            atomic::inc(&count);
            int_t* p = new int_t(i+1, id);
            BOOST_ASSERT(p); // , "Out of memory id=" << id << " i=" << i);
            BOOST_ASSERT(((unsigned long)p & int_t::s_version_mask) == 0); // "Invalid alignment " << p);
            if (verbosity::level() >= utxx::VERBOSE_TRACE)
                fprintf(stderr, "  %d - Allocated %p (%d,%d) prod_cnt=%ld\n", id, p, id, i+1, count);
            stack.push(p);
        }
        if (verbosity::level() != utxx::VERBOSE_NONE)
            fprintf(stderr, "Producer %d finished (count=%ld)\n", id, count);
    }
};

struct sconsumer : public sproducer {
    volatile long& prod_cnt;
    unsigned long& sum;
    sconsumer(int a_id, int it, volatile long& a_prod_cnt, volatile long& a_cnt,
            unsigned long& a_sum,
            boost::barrier& b,
            blocking_versioned_stack<>& s)
        : sproducer(a_id, it, a_cnt, b, s), prod_cnt(a_prod_cnt)
        , sum(a_sum)
    {}

    void operator() () {
        barrier.wait();
        struct timespec ts = {1, 0};

        do {
            int_t* p = static_cast<int_t*>(stack.reset(&ts, true));
            while (p) {
                int_t* next = static_cast<int_t*>(p->next);
                BOOST_ASSERT(((unsigned long)p & int_t::s_version_mask) == 0); //, "Invalid alignment " << p);
                sum += p->data();
                atomic::add(&count, 1);
                if (verbosity::level() >= utxx::VERBOSE_TRACE)
                    fprintf(stderr, "  %d - Freeing %p (%d,%7d) prod_cnt=%7ld, cons_cnt=%7ld\n",
                            id, p, p->id(), p->data(), prod_cnt, count);
                delete p;
                p = next;
            }
        } while (prod_cnt < iterations || count < iterations);

        if (verbosity::level() != utxx::VERBOSE_NONE)
            fprintf(stderr, "Consumer %d finished (count=%ld)\n", id, count);
    }
};

BOOST_AUTO_TEST_CASE( test_concurrent_stack )
{
    blocking_versioned_stack<> stack;
    const long iterations       = ::getenv("ITERATIONS")  ? atoi(::getenv("ITERATIONS"))  : 100000;
    const long producer_threads = ::getenv("PROD_THREAD") ? atoi(::getenv("PROD_THREAD")) : 2;
    const long consumer_threads = ::getenv("CONS_THREAD") ? atoi(::getenv("CONS_THREAD")) : 2;

    unsigned long sums[consumer_threads];
    volatile long prod_count = 0, cons_count = 0;

    bzero(sums, sizeof(long)*consumer_threads);

    std::vector<std::shared_ptr<sproducer>>   producers(producer_threads);
    std::vector<std::shared_ptr<sconsumer>>   consumers(consumer_threads);
    std::vector<std::shared_ptr<std::thread>> thread(producer_threads + consumer_threads);
    boost::barrier                            barrier(producer_threads+consumer_threads+1);

    for (int i=0; i < producer_threads; ++i) {
        producers[i].reset(new sproducer(i+1, iterations, prod_count, barrier, stack));
        thread[i].reset(new std::thread(std::ref(*producers[i])));
    }
    for (int i=0; i < consumer_threads; ++i) {
        consumers[i].reset(new sconsumer(i+1, producer_threads*iterations,
                           prod_count, cons_count, sums[i], barrier, stack));
        thread[i+producer_threads].reset(new std::thread(std::ref(*consumers[i])));
    }

    barrier.wait();

    for (int i=0; i < (producer_threads + consumer_threads); ++i)
        thread[i]->join();

    unsigned long exp_sum = producer_threads * (iterations * (iterations+1) / 2);
    unsigned long real_sum = 0;
    for (int i=0; i < consumer_threads; ++i)
        real_sum += sums[i];

    BOOST_REQUIRE_EQUAL(exp_sum, real_sum);
    BOOST_REQUIRE_EQUAL(producer_threads*iterations, prod_count);
    BOOST_REQUIRE_EQUAL(producer_threads*iterations, cons_count);
}

