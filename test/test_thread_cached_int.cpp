/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <utxx/thread_cached_int.hpp>
#include <utxx/test_helper.hpp>
#include <boost/lexical_cast.hpp>
#include <atomic>
#include <thread>

using namespace utxx;

template <typename T>
inline T get_opt(const char* a, T a_default) {
    std::string value;
    if (!utxx::get_test_argv(a, value))
        return a_default;
    return boost::lexical_cast<T>(value);
}

BOOST_AUTO_TEST_CASE( test_thread_local_single_threaded_not_cached ) {
    thread_cached_int<int64_t> val(0, 0);
    BOOST_CHECK_EQUAL(0, val.read_fast());
    ++val;
    BOOST_CHECK_EQUAL(1, val.read_fast());
    for (int i = 0; i < 41; ++i) {
        val.increment(1);
    }
    BOOST_CHECK_EQUAL(42, val.read_fast());
    --val;
    BOOST_CHECK_EQUAL(41, val.read_fast());
}

// Note: This is somewhat fragile to the implementation.  If this causes
// problems, feel free to remove it.
BOOST_AUTO_TEST_CASE( test_thread_local_single_threaded_cached ) {
    thread_cached_int<int64_t> val(0, 10);
    BOOST_CHECK_EQUAL(0, val.read_fast());
    ++val;
    BOOST_CHECK_EQUAL(0, val.read_fast());
    for (int i = 0; i < 7; ++i)
        val.increment(1);
    BOOST_CHECK_EQUAL(0, val.read_fast());
    BOOST_CHECK_EQUAL(0, val.read_fast_and_reset());
    BOOST_CHECK_EQUAL(8, val.read_full());
    BOOST_CHECK_EQUAL(8, val.read_full_and_reset());
    BOOST_CHECK_EQUAL(0, val.read_full());
    BOOST_CHECK_EQUAL(0, val.read_fast());
}

thread_cached_int<int32_t> globalInt32(0, 11);
thread_cached_int<int64_t> globalInt64(0, 11);
int kNumInserts = 100000;
// Number simultaneous threads for benchmarks
int32_t numThreads = get_opt<int32_t>("num-threads", 8);
#define CREATE_INC_FUNC(size)                                           \
    void incFunc ## size () {                                           \
        const int num = kNumInserts / numThreads;                       \
        for (int i = 0; i < num; ++i) {                                 \
            ++globalInt ## size ;                                       \
        }                                                               \
    }
CREATE_INC_FUNC(64);
CREATE_INC_FUNC(32);

// Confirms counts are accurate with competing threads
BOOST_AUTO_TEST_CASE( test_thread_local_multi_threaded_cached ) {
    kNumInserts = 100000;
    BOOST_MESSAGE("FLAGS_numThreads must evenly divide kNumInserts ("
                  << kNumInserts << ").");
    BOOST_REQUIRE(0 == kNumInserts % numThreads);
    const int numPerThread = kNumInserts / numThreads;
    thread_cached_int<int64_t> TCInt64(0, numPerThread - 2);
    {
        std::atomic<bool> run(true);
        std::atomic<int> threadsDone(0);
        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i)
            threads.push_back(std::thread([&] {
                for (int k=0; k < numPerThread; ++k)
                    ++TCInt64;
                std::atomic_fetch_add(&threadsDone, 1);
                while (run.load()) { usleep(100); }
            }));

        // We create and increment another ThreadCachedInt here to make sure it
        // doesn't interact with the other instances
        thread_cached_int<int64_t> otherTCInt64(0, 10);
        otherTCInt64.set(33);
        ++otherTCInt64;

        while (threadsDone.load() < numThreads) { usleep(100); }

        ++otherTCInt64;

        // Threads are done incrementing, but caches have not been flushed yet, so
        // we have to read_full.
        BOOST_CHECK(kNumInserts != TCInt64.read_fast());
        BOOST_CHECK_EQUAL(kNumInserts, TCInt64.read_full());

        run.store(false);
        for (auto& t : threads)
            t.join();

    }  // Caches are flushed when threads finish

    BOOST_CHECK_EQUAL(kNumInserts, TCInt64.read_fast());
}
