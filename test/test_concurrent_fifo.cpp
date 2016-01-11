//#define BOOST_TEST_MODULE shmem_alloc_test

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <utxx/container/concurrent_fifo.hpp>
#include <utxx/mt_queue.hpp>
#include <utxx/allocator.hpp>
#include <utxx/verbosity.hpp>
#include <iostream>
#include <stdio.h>

using namespace utxx;

template <typename Queue, int Size>
void test_queue_simple(int total)
{
    const unsigned int N = Size;
    Queue queue;

    for(int i=0; i < total; i++) {
        bool res = queue.enqueue(i);
        BOOST_REQUIRE(i < (int)N ? res : !res);
    }

    for(int i=0; i < total; i++) {
        unsigned long item;
        bool res = queue.dequeue(item);
        BOOST_REQUIRE(i < (int)N ? res : !res);
        if (res)
            BOOST_REQUIRE_EQUAL(i, (int)item);
    }

    BOOST_REQUIRE(queue.empty());
}

BOOST_AUTO_TEST_CASE( test_fifo_bound )
{
    const int N = 8;
    typedef container::bound_lock_free_queue<unsigned long, N> queue_t;
    test_queue_simple<queue_t, N>(10);
}

BOOST_AUTO_TEST_CASE( test_fifo_unbound )
{
    typedef container::unbound_lock_free_queue<unsigned long> queue_t;
    test_queue_simple<queue_t, 10>(10);
}

//-----------------------------------------------------------------------------
// Concurrent test
//-----------------------------------------------------------------------------

struct node_t {
    int data;
    int th;
    node_t(int _data, int _th) : data(_data), th(_th) {}
};

std::string node_to_str(node_t* const& nd) {
    std::stringstream s;
    int n = ((unsigned long)nd < 3 ? 0 : ((node_t*)((unsigned long)nd & ~0x3ul))->data);
    s << " [" << n << ']';
    return s.str();
}

enum { QUEUE_SIZE = 16 };

typedef container::blocking_bound_fifo<node_t*, QUEUE_SIZE> pointer_queue_t;
typedef container::blocking_unbound_fifo<node_t*>           pointer_unbound_queue_t;
typedef concurrent_queue<node_t*>                           posix_queue_t;

template <typename Queue>
struct producer {
    int              id;
    volatile long&   count;
    long             iterations;
    boost::barrier&  barrier;
    Queue&           queue;
    volatile bool&   terminate;

    producer(int a_id, long it, volatile long& a_cnt,
            boost::barrier& b, Queue& q, volatile bool& a_terminate)
        : id(a_id), count(a_cnt), iterations(it), barrier(b)
        , queue(q), terminate(a_terminate)
    {}

    void operator() () {
        barrier.wait();
        long i=0;
        node_t* p = NULL;
        char buf[128];

        while (!terminate && i < iterations) {
            if (p == NULL) {
                i++;
                atomic::inc(&count);
                p = new node_t(i, id);
                if (verbosity::level() >= utxx::VERBOSE_TRACE) {
                    sprintf(buf, "%d => put(%p) [%7d]\n", id, p, p->data);
                }
            }
            BOOST_ASSERT(p); // , "Out of memory id=" << id << " i=" << i);
            BOOST_ASSERT(((unsigned long)p & 0x3) == 0); // "Invalid alignment " << p);

            if (queue.enqueue(p, NULL) == 0) {
                if (verbosity::level() >= utxx::VERBOSE_TRACE)
                    puts(buf);
                p = NULL;
            }
        }
        if (verbosity::level() != utxx::VERBOSE_NONE)
            printf("Producer %d finished (count=%ld)\n", id, count);
    }
};

template <typename Queue>
struct consumer : public producer<Queue> {
    typedef producer<Queue> base;

    volatile long& prod_tot_cnt;
    volatile long* prod_counts;
    unsigned long& sum;
    consumer(int a_id, long it, volatile long& a_prod_tot_cnt,
            volatile long* a_prod_cnts,
            volatile long& a_cons_tot_cnt,
            unsigned long& a_sum,
            boost::barrier& b,
            Queue& q,
            volatile bool& a_terminate)
        : producer<Queue>(a_id, it, a_cons_tot_cnt, b, q, a_terminate)
        , prod_tot_cnt(a_prod_tot_cnt)
        , prod_counts(a_prod_cnts)
        , sum(a_sum)
    {}

    void operator() () {
        base::barrier.wait();
        //struct timespec ts = {1, 0};

        do {
            node_t* p;
            if (base::queue.dequeue(p, NULL) == 0) {
                BOOST_ASSERT(((unsigned long)p & 0x3) == 0);
                sum += p->data;
                atomic::add(&this->count, 1); // cons. total count
                atomic::add(&prod_counts[p->th-1], 1);
                if (verbosity::level() >= utxx::VERBOSE_TRACE) {
                    printf("%d <= get(%p) [%7d] count=%7ld prod_cnt=%d/%-7ld, "
                           "(sum=%ld, tot_prod_cnt=%7ld)\n",
                           base::id, p, p->data, base::count, p->th,
                           prod_counts[p->th-1], sum, prod_tot_cnt);
                }
                //BOOST_REQUIRE_EQUAL(p->data, count);
                if (p->data != prod_counts[p->th-1]) {
                    printf("!!! Producer %d => %d != %ld\n",
                        p->th, p->data, prod_counts[p->th-1]);
                    base::terminate = true;
                    #ifdef DEBUG
                    queue.dump(std::cout, node_to_str);
                    #endif
                    abort();
                }
                delete p;
            }
        } while (!base::terminate &&
                 (prod_tot_cnt < base::iterations || base::count < base::iterations));

        if (verbosity::level() != utxx::VERBOSE_NONE)
            printf("Consumer %d finished (count=%ld)\n", base::id, base::count);

        base::queue.terminate();
    }
};

template <typename Queue>
struct queue_test_runner
{
    void operator() () {
        volatile long prod_count = 0, cons_count = 0;

        const long iterations       = ::getenv("ITERATIONS")  ? atoi(::getenv("ITERATIONS"))  : 100000;
        const long producer_threads = ::getenv("PROD_THREAD") ? atoi(::getenv("PROD_THREAD")) : 1;
        const long consumer_threads = 1;  // Note that validation in this concurrency test
                                          // won't work with multiple consulers.
        volatile bool terminate = false;

        Queue queue;
        unsigned long sums[consumer_threads];
        volatile long prod_counts[producer_threads];

        typedef producer<Queue> prod_t;
        typedef consumer<Queue> cons_t;

        std::vector<std::shared_ptr<prod_t>>        producers(producer_threads);
        std::vector<std::shared_ptr<cons_t>>        consumers(consumer_threads);
        std::vector<std::shared_ptr<boost::thread>> thread(producer_threads + consumer_threads);
        boost::barrier barrier(producer_threads + consumer_threads + 1);

        bzero(sums, sizeof(sums));
        bzero(const_cast<long*>(prod_counts), sizeof(long)*producer_threads);

        for (int i=0; i < producer_threads; ++i) {
            producers[i].reset(
                new prod_t(i+1, iterations, prod_count, barrier, queue, terminate));
            thread[i].reset(new boost::thread(boost::ref(*producers[i])));
        }
        for (int i=0; i < consumer_threads; ++i) {
            consumers[i].reset(
                new cons_t(i+1, producer_threads*iterations,
                             prod_count, prod_counts,
                             cons_count, sums[i], barrier, queue, terminate));
            thread[i+producer_threads].reset(
                new boost::thread(boost::ref(*consumers[i])));
        }

        barrier.wait();

        for (int i=0; i < producer_threads + consumer_threads; ++i)
            thread[i]->join();

        unsigned long exp_sum = producer_threads * (iterations * (iterations+1) / 2);
        unsigned long real_sum = 0;
        for (int i=0; i < consumer_threads; ++i)
            real_sum += sums[i];

        BOOST_REQUIRE_EQUAL(exp_sum, real_sum);
        BOOST_REQUIRE_EQUAL(producer_threads*iterations, prod_count);
        BOOST_REQUIRE_EQUAL(producer_threads*iterations, cons_count);
    }
};

BOOST_AUTO_TEST_CASE( test_concurrent_bound_fifo )
{
    queue_test_runner<pointer_queue_t> test_case;
    test_case();
}

BOOST_AUTO_TEST_CASE( test_concurrent_unbound_fifo )
{
    queue_test_runner<pointer_unbound_queue_t> test_case;
    test_case();
}

BOOST_AUTO_TEST_CASE( test_concurrent_posix_fifo )
{
    queue_test_runner<posix_queue_t> test_case;
    test_case();
}

