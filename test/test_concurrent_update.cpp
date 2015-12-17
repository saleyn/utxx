#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <utxx/container/concurrent_stack.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/atomic.hpp>
#include <vector>
#include <stdio.h>

using namespace utxx;

#define MAX_DATA_SZ 64
#define MAX_SLOTS   16

template <int N>
class array {
    volatile struct {
        unsigned char data[MAX_DATA_SZ];
        uint16_t      version;
        bool          writing;
    } m_data[N];
public:
    int get(int n, unsigned char* d, size_t sz) const {
        uint16_t ver = 0;
        int      looped = 0;
        BOOST_ASSERT(sz <= MAX_DATA_SZ);
        do {
            looped++;
            if (m_data[n].writing) continue;
            ver = m_data[n].version;
            if (m_data[n].writing) continue;
            memcpy(d, const_cast<unsigned char*>(m_data[n].data), sz);
        //} while (m_data[n].version != ver || m_data[n].writing);
        } while (m_data[n].version != ver);
        return looped-1;
    }
    void set(int n, unsigned char* d, size_t sz) { 
        BOOST_ASSERT(sz <= MAX_DATA_SZ);
        BOOST_ASSERT(n  <  N);
        m_data[n].writing = true;
        m_data[n].version++;
        atomic::memory_barrier();
        memcpy(const_cast<unsigned char*>(m_data[n].data), d, sz);
        m_data[n].writing = false;
        atomic::memory_barrier();
    }

    static uint32_t checksum(unsigned char* b, size_t sz) {
        uint32_t sum = 0;
        for (unsigned int i=0; i < sz; i++)
            sum += b[i];
        return sum;
    }

    array() { bzero((void*)m_data, sizeof(m_data)); }
};

typedef array<MAX_SLOTS> array_t;

struct producer_t {
    unsigned int     id, seed;
    volatile long&   count;
    int              iterations;
    boost::barrier&  barrier;
    array_t&         data;

    producer_t(uint32_t a_id, int it, volatile long& a_cnt, boost::barrier& b, array_t& a)
        : id(a_id), seed(id), count(a_cnt), iterations(it), barrier(b), data(a)
    {}

    void operator() () {
        barrier.wait();
        for (int i=0; i < iterations; i++) {
            atomic::inc(&count);
            unsigned char buf[MAX_DATA_SZ];
            for (int j=0; j < MAX_DATA_SZ; j++)
                buf[j] = (char)(rand_r(&seed) % 254 + 1);
            uint32_t sum = array_t::checksum(buf+4, MAX_DATA_SZ-4);
            memcpy(buf, &sum, sizeof(sum));
            data.set(i % MAX_SLOTS, buf, MAX_DATA_SZ);
        }
        if (verbosity::level() != utxx::VERBOSE_NONE)
            fprintf(stderr, "Producer %u finished (count=%ld)\n", id, count);
    }
};

struct consumer_t : public producer_t {
    consumer_t(uint32_t a_id, int it, volatile long& a_cnt, boost::barrier& b, array_t& a)
        : producer_t(a_id, it, a_cnt, b, a)
    {}

    void operator() () {
        barrier.wait();
        uint32_t i = 0, looped = 0;

        do {
            unsigned char buf[MAX_DATA_SZ];
            looped += data.get(i++ % MAX_SLOTS, buf, MAX_DATA_SZ);
            uint32_t old_sum = *(uint32_t*)&buf[0];
            uint32_t new_sum = array_t::checksum(buf+4, MAX_DATA_SZ-4);
            if (old_sum != new_sum) {
                fprintf(stderr, "Consumer%d: old_sum[%u] != new_sum[%u]\n", id, old_sum, new_sum);
                BOOST_REQUIRE_EQUAL(old_sum, new_sum);
            }
        } while (count < iterations);

        if (verbosity::level() != utxx::VERBOSE_NONE)
            fprintf(stderr, "Consumer %d finished (count=%ld, looped=%u)\n", id, count, looped);
    }
};

BOOST_AUTO_TEST_CASE( test_concurrent_update )
{
    array_t data;

    const long iterations       = ::getenv("ITERATIONS")  ? atoi(::getenv("ITERATIONS"))  : 1000000;
    const long producer_threads = 1;
    const long consumer_threads = ::getenv("CONS_THREAD") ? atoi(::getenv("CONS_THREAD")) : 1;

    volatile long count = 0;

    std::vector<std::shared_ptr<producer_t>>    producers(producer_threads);
    std::vector<std::shared_ptr<consumer_t>>    consumers(consumer_threads);
    std::vector<std::shared_ptr<boost::thread>> thread(producer_threads + consumer_threads);
    boost::barrier                              barrier(producer_threads+consumer_threads+1);

    for (int i=0; i < producer_threads; ++i) {
        producers[i] = std::shared_ptr<producer_t>(
            new producer_t(i+1, iterations, count, barrier, data));
        thread[i] = std::shared_ptr<boost::thread>(
            new boost::thread(boost::ref(*producers[i])));
    }
    for (int i=0; i < consumer_threads; ++i) {
        consumers[i] = std::shared_ptr<consumer_t>(
            new consumer_t(i+1, producer_threads*iterations, count, barrier, data));
        thread[i+producer_threads] = std::shared_ptr<boost::thread>(
            new boost::thread(boost::ref(*consumers[i])));
    }

    barrier.wait();

    for (int i=0; i < (producer_threads + consumer_threads); ++i)
        thread[i]->join();
}

