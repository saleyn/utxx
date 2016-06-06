#include <boost/test/unit_test.hpp>
#include <utxx/concurrent_spsc_queue.hpp>

#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <math.h>

namespace utxx {

static size_t iterations() { return getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 0; }

template<class T>
struct TestTraits {
    T limit() const { auto n = iterations(); return n ? n : 1 << 24; }
    T generate() const { return rand() % 26; }
};

template<>
struct TestTraits<std::string> {
    int limit() const { auto n = iterations(); return n ? n : 1 << 22; }
    std::string generate() const { return std::string(12, ' '); }
};

template<class QueueType, size_t Size, bool Pop = false>
struct PerfTest {
    typedef typename QueueType::value_type T;

    explicit PerfTest() : queue_(Size), done_(false) {}

    void operator()() {
        using namespace std::chrono;
        auto const startTime = system_clock::now();

        std::thread producer([this] { this->producer(); });
        std::thread consumer([this] { this->consumer(); });

        producer.join();
        done_ = true;
        consumer.join();

        auto duration = duration_cast<milliseconds>(
            system_clock::now() - startTime);
        BOOST_TEST_MESSAGE("     done: " << duration.count() << "ms");
    }

    void producer() {
        for (int i = 0, n = (int)traits_.limit(); i < n; ++i)
            while (!queue_.push(traits_.generate()));
    }

    void consumer() {
        if (Pop) {
            while (!done_)
                if (queue_.peek())
                    queue_.pop();
        } else {
            while (!done_) {
                T data;
                queue_.pop(out(data));
            }
        }
    }

    QueueType queue_;
    std::atomic<bool> done_;
    TestTraits<T> traits_;
};

template<class TestType> void doTest(const char* name) {
    BOOST_TEST_MESSAGE("  testing: " << name);
    std::unique_ptr<TestType> const t(new TestType());
    (*t)();
}

template<class T, bool Pop = false>
void perfTestType(const char* type) {
    const size_t size = 0xfffe;

    BOOST_TEST_MESSAGE("Type: " << type);
    doTest<PerfTest<concurrent_spsc_queue<T>,size,Pop>>("ProducerConsumerQueue");
}

template<class QueueType, size_t Size, bool Pop>
struct CorrectnessTest {
    typedef typename QueueType::value_type T;

    explicit CorrectnessTest()
        : queue_(Size)
        , done_(false)
    {
        const size_t testSize = traits_.limit();
        testData_.reserve(testSize);
        for (size_t i = 0; i < testSize; ++i)
            testData_.push_back(traits_.generate());
    }

    void operator()() {
        std::thread producer([this] { this->producer(); });
        std::thread consumer([this] { this->consumer(); });

        producer.join();
        done_ = true;
        consumer.join();
    }

    void producer() {
        for (auto& data : testData_)
            while (!queue_.push(data));
    }

    void consumer() {
        if (Pop)
            consumerPop();
        else
            consumerRead();
    }

    void consumerPop() {
        for (auto expect : testData_) {
        again:
            T* data;
            if (!!(data = queue_.peek()))
                queue_.pop();
            else if (!done_)
                goto again;
            // Try one more read; unless there's a bug in the queue class
            // there should still be more data sitting in the queue even
            // though the producer thread exited.
            else if (!(data = queue_.peek())) {
                BOOST_REQUIRE(0 && "Finished too early ...");
                return;
            }

            BOOST_REQUIRE_EQUAL(*data, expect);
        }
    }

    void consumerRead() {
        for (auto expect : testData_) {
        again:
            T data;
            if (!queue_.pop(out(data))) {
                if (!done_)
                    goto again;

                // Try one more read; unless there's a bug in the queue class
                // there should still be more data sitting in the queue even
                // though the producer thread exited.
                if (!queue_.pop(out(data))) {
                    BOOST_REQUIRE(0 && "Finished too early ...");
                    return;
                }
            }
            BOOST_REQUIRE_EQUAL(data, expect);
        }
    }

    std::vector<T>      testData_;
    QueueType           queue_;
    TestTraits<T>       traits_;
    std::atomic<bool>   done_;
};

template<class T, size_t Size, bool Pop = false>
void correctnessTestType(const std::string& type) {
    BOOST_TEST_MESSAGE("Type: " << type);
    doTest<CorrectnessTest<concurrent_spsc_queue<T>,Size,Pop> >(
        "ProducerConsumerQueue");
}

struct DtorChecker {
    static int numInstances;
    DtorChecker() { ++numInstances; }
    DtorChecker(const DtorChecker& o) { ++numInstances; }
    ~DtorChecker() { --numInstances; }
};

int DtorChecker::numInstances = 0;

//////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( test_concurrent_spsc_empty ) {
    concurrent_spsc_queue<int> queue(4);
    BOOST_REQUIRE(queue.empty());
    BOOST_REQUIRE(!queue.full());

    BOOST_REQUIRE(queue.push(1));
    BOOST_REQUIRE(!queue.empty());
    BOOST_REQUIRE(!queue.full());

    BOOST_REQUIRE(queue.push(2));
    BOOST_REQUIRE(!queue.empty());
    BOOST_REQUIRE(!queue.full());

    BOOST_REQUIRE(queue.push(3));
    BOOST_REQUIRE(!queue.empty());
    BOOST_REQUIRE(queue.full());  // Tricky: full after 3 writes, not 2.

    BOOST_REQUIRE(!queue.push(4));
    BOOST_REQUIRE_EQUAL(queue.count(), 3u);
}

BOOST_AUTO_TEST_CASE( test_concurrent_spsc_correctness ) {
    correctnessTestType<std::string,0xfffe,true>("string (front+pop)");
    correctnessTestType<std::string,0xffff>("string");
    correctnessTestType<int, 0xffff>("int");
    correctnessTestType<unsigned long long, 0xfffe>("unsigned long long");
}

BOOST_AUTO_TEST_CASE( test_concurrent_spsc_perf ) {
    perfTestType<std::string,true>("string (front+pop)");
    perfTestType<std::string>("string");
    perfTestType<int>("int");
    perfTestType<unsigned long long>("unsigned long long");
}

BOOST_AUTO_TEST_CASE( test_concurrent_spsc_destructor ) {
    // Test that orphaned elements in a ProducerConsumerQueue are
    // destroyed.
    {
        concurrent_spsc_queue<DtorChecker> queue(1024);
        for (int i = 0; i < 10; ++i)
            BOOST_REQUIRE(queue.push(DtorChecker()));

        BOOST_REQUIRE_EQUAL(DtorChecker::numInstances, 10);

        {
            DtorChecker ignore;
            BOOST_REQUIRE(queue.pop(out(ignore)));
            BOOST_REQUIRE(queue.pop(out(ignore)));
        }

        BOOST_REQUIRE_EQUAL(DtorChecker::numInstances, 8);
    }

    BOOST_REQUIRE_EQUAL(DtorChecker::numInstances, 0);

    // Test the same thing in the case that the queue write pointer has
    // wrapped, but the read one hasn't.
    {
        concurrent_spsc_queue<DtorChecker> queue(4);
        for (int i = 0; i < 3; ++i)
            BOOST_REQUIRE(queue.push(DtorChecker()));

        BOOST_REQUIRE_EQUAL(DtorChecker::numInstances, 3);
        {
            DtorChecker ignore;
            BOOST_REQUIRE(queue.pop(out(ignore)));
        }
        BOOST_REQUIRE_EQUAL(DtorChecker::numInstances, 2);
        BOOST_REQUIRE(queue.push(DtorChecker()));
        BOOST_REQUIRE_EQUAL(DtorChecker::numInstances, 3);
    }
    BOOST_REQUIRE_EQUAL(DtorChecker::numInstances, 0);
}

BOOST_AUTO_TEST_CASE( test_concurrent_spsc_iterator )
{
    size_t       C        = 10;
    const size_t CAPACITY = 8;
    size_t       sizes[]  = { 6, 7, 8, 9 };

    srand48(time(NULL));

    for (size_t N : sizes) {
        BOOST_TEST_MESSAGE("Capacity=" << C << ", NItems=" << N);

        concurrent_spsc_queue<size_t> q(C);
        BOOST_REQUIRE_EQUAL(size_t(CAPACITY), q.capacity());

        for (size_t i = 0; i < N; ++i) {
            size_t* res = q.push(i);
            BOOST_REQUIRE((i < (CAPACITY-1)) == (res != NULL));
        }

        size_t n = q.count();
        BOOST_REQUIRE_EQUAL(n, size_t(std::min(N, CAPACITY-1)));

        size_t i = 0;
        for (concurrent_spsc_queue<size_t>::iterator it = q.begin();
                it != q.end(); ++it)
            BOOST_REQUIRE_EQUAL(i++, *it);

        size_t r = size_t(floor(drand48() * double(n)));
        BOOST_REQUIRE(0 <= r && r < n);
        BOOST_TEST_MESSAGE("r =" << r);
        q.erase(q.begin() + r);

        BOOST_REQUIRE_EQUAL(n-1, q.count());

        i = 0;
        for (concurrent_spsc_queue<size_t>::const_iterator cit = q.cbegin();
                cit != q.cend(); ++cit, ++i)
            BOOST_REQUIRE_EQUAL(i < r ? i : i+1, *cit);

        i = n-1;
        for (concurrent_spsc_queue<size_t>::reverse_iterator rit = q.rbegin();
                rit != q.rend(); ++rit, --i)
            BOOST_REQUIRE_EQUAL(i > r ? i : i-1, *rit);

        i = n-1;
        for (concurrent_spsc_queue<size_t>::const_reverse_iterator crit = q.crbegin();
                crit != q.crend(); ++crit, --i)
            BOOST_REQUIRE_EQUAL(i > r ? i : i-1, *crit);
    }
}

} // namespace utxx
