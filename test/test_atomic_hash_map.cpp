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

#include <utxx/test_helper.hpp>
#include <utxx/atomic_hash_map.hpp>
#include <utxx/string.hpp>
#include <utxx/cpu.hpp>
#include <boost/lexical_cast.hpp>

#include <sys/time.h>
#include <thread>
#include <atomic>
#include <memory>

using std::vector;
using std::string;
using utxx::atomic_hash_map;
using utxx::atomic_hash_array;

// Tunables:
template <typename T>
inline T get_opt(const char* a, T a_default) {
    std::string value;
    return utxx::get_test_argv(a, value) ? boost::lexical_cast<T>(value) : a_default;
}

// Max before growth
const double  maxLoadFactor = get_opt<double>("max-load-factor", 0.80);
// Threads to use for concurrency tests
const int32_t numThreads    = get_opt<int32_t>("num-threads", utxx::detail::cpu_count());

static int64_t nowInUsec() {
    timeval tv;
    gettimeofday(&tv, 0);
    return int64_t(tv.tv_sec) * 1000 * 1000 + tv.tv_usec;
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_map_BasicStrings) {
    using AHM = atomic_hash_map<int64_t,string,
                                std::hash<int64_t>, std::equal_to<int64_t>,
                                std::allocator<char>>;
    AHM myMap(1024);
    BOOST_CHECK(myMap.begin() == myMap.end());

    for (int i = 0; i < 100; ++i)
        myMap.insert(std::make_pair(i, std::to_string(i)));
    for (int i = 0; i < 100; ++i)
        BOOST_CHECK_EQUAL(myMap.find(i)->second, std::to_string(i));

    myMap.insert(std::make_pair(999, "A"));
    myMap.insert(std::make_pair(999, "B"));
    BOOST_CHECK_EQUAL(myMap.find(999)->second, "A"); // shouldn't have overwritten
    myMap.find(999)->second = "B";
    myMap.find(999)->second = "C";
    BOOST_CHECK_EQUAL(myMap.find(999)->second, "C");
    BOOST_CHECK_EQUAL(myMap.find(999)->first, 999);
}


BOOST_AUTO_TEST_CASE( test_atomic_hash_map_basic_noncopyable) {
    using hash_map = atomic_hash_map<int64_t,std::unique_ptr<int>,
                                     std::hash<int64_t>, std::equal_to<int64_t>,
                                     std::allocator<char>>;
    hash_map myMap(1024);
    BOOST_CHECK(myMap.begin() == myMap.end());

    for (int i =  0; i <  50; ++i)
        myMap.insert(make_pair(i, std::unique_ptr<int>(new int(i))));
    for (int i = 50; i < 100; ++i)
        myMap.insert(i, std::unique_ptr<int>(new int (i)));
    for (int i =  0; i < 100; ++i)
        BOOST_CHECK_EQUAL(*(myMap.find(i)->second), i);
    for (int i =  0; i < 100; i+=4)
        myMap.erase(i);
    for (int i =  0; i < 100; i+=4)
        BOOST_CHECK(myMap.find(i) == myMap.end());
}

using KeyT     = int32_t;
using ValueT   = int32_t;

using AHMapT   = atomic_hash_map<KeyT,ValueT, std::hash<KeyT>,
                                 std::equal_to<KeyT>, std::allocator<char>>;
using RecordT  = typename AHMapT::value_type;
using AHArrayT = atomic_hash_array<KeyT,ValueT>;

AHArrayT::config                                config;
std::allocator<char>                            allocator;
static AHArrayT::SmartPtr<std::allocator<char>> globalAHA(nullptr, allocator);
static std::unique_ptr<AHMapT>                  globalAHM;

// Generate a deterministic value based on an input key
static int genVal(int key) { return key / 3; }

BOOST_AUTO_TEST_CASE( test_atomic_hash_map_grow ) {
    BOOST_MESSAGE("Overhead: " << sizeof(AHArrayT) << " (array) " <<
        sizeof(AHMapT) + sizeof(AHArrayT) << " (map/set) Bytes.");
    uint64_t numEntries = 1000;
    float sizeFactor = 0.46;

    std::unique_ptr<AHMapT> m(new AHMapT(int(numEntries * sizeFactor), config));

    // load map - make sure we succeed and the index is accurate
    bool success = true;
    for (uint64_t i = 0; i < numEntries; i++) {
        auto ret = m->insert(RecordT(i, genVal(i)));
        success &= ret.second;
        success &= (m->find_at(ret.first.index())->second == genVal(i));
    }
    // Overwrite vals to make sure there are no dups
    // Every insert should fail because the keys are already in the map.
    success = true;
    for (uint64_t i = 0; i < numEntries; i++) {
        auto ret = m->insert(RecordT(i, genVal(i * 2)));
        success &= (ret.second == false);  // fail on collision
        success &= (ret.first->second == genVal(i)); // return the previous value
        success &= (m->find_at(ret.first.index())->second == genVal(i));
    }
    BOOST_CHECK(success);

    // check correctness
    BOOST_CHECK(m->num_submaps() > 1);  // make sure we grew
    success = true;
    BOOST_CHECK_EQUAL(m->size(), numEntries);
    for (size_t i = 0; i < numEntries; i++)
        success &= (m->find(i)->second == genVal(i));
    BOOST_CHECK(success);

    // Check findAt
    success = true;
    AHMapT::const_iterator retIt;
    for (int32_t i = 0; i < int32_t(numEntries); i++) {
        retIt = m->find(i);
        retIt = m->find_at(retIt.index());
        success &= (retIt->second == genVal(i));
        // We use a uint32_t index so that this comparison is between two
        // variables of the same type.
        success &= (retIt->first == i);
    }
    BOOST_CHECK(success);

    // Try modifying value
    m->find(8)->second = 5309;
    BOOST_CHECK_EQUAL(m->find(8)->second, 5309);

    // check clear()
    m->clear();
    success = true;
    for (uint64_t i = 0; i < numEntries / 2; i++)
        success &= m->insert(RecordT(i, genVal(i))).second;
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(m->size(), numEntries / 2);
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_map_iterator) {
    int   numEntries = 10000;
    float sizeFactor = .46;
    std::unique_ptr<AHMapT> m(new AHMapT(int(numEntries * sizeFactor), config));

    // load map - make sure we succeed and the index is accurate
    for (int i = 0; i < numEntries; i++) {
        m->insert(RecordT(i, genVal(i)));
    }

    bool success = true;
    int  count   = 0;
    for (auto it : *m) {
        success &= (it.second == genVal(it.first));
        ++count;
    }
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(count, numEntries);
}

class Counters {
private:
    // Note: Unfortunately can't currently put a std::atomic<int64_t> in
    // the value in ahm since it doesn't support types that are both non-copy
    // and non-move constructible yet.
    atomic_hash_map<int64_t,int64_t> ahm;

public:
    explicit Counters(size_t numCounters) : ahm(numCounters) {}

    void increment(int64_t obj_id) {
        auto ret = ahm.insert(std::make_pair(obj_id, 1));
        if (!ret.second) {
        // obj_id already exists, increment count
        __sync_fetch_and_add(&ret.first->second, 1);
        }
    }

    int64_t getValue(int64_t obj_id) {
        auto ret = ahm.find(obj_id);
        return ret != ahm.end() ? ret->second : 0;
    }

    // export the counters without blocking increments
    string toString() {
        string ret = "{\n";
        ret.reserve(ahm.size() * 32);
        for (const auto& e : ahm)
            ret += utxx::to_string("  [", e.first, ":", e.second, "]\n");
        ret += "}\n";
        return ret;
    }
};

// If you get an error "terminate called without an active exception", there
// might be too many threads getting created - decrease numKeys and/or mult.
BOOST_AUTO_TEST_CASE( test_atomic_hash_map_counter) {
    const int numKeys = 10;
    const int mult = 10;
    Counters c(numKeys);
    vector<int64_t> keys;
    for (int i=1; i < numKeys; ++i)
        keys.push_back(i);

    vector<std::thread> threads;
    for (auto key : keys)
        for (int i=0; i < key * mult; ++i)
            threads.push_back(std::thread([&, key] { c.increment(key); }));

    for (auto& t : threads)
        t.join();

    string str = c.toString();
    for (auto key : keys) {
        int val = key * mult;
        BOOST_CHECK_EQUAL(val, c.getValue(key));
        BOOST_CHECK(string::npos != str.find(utxx::to_string("[",key,":",val,"]")));
    }
}

class Integer {
public:
    explicit Integer(KeyT v = 0) : v_(v) {}

    Integer& operator=(const Integer& a) {
        static bool throwException_ = false;
        throwException_ = !throwException_;
        if (throwException_)
            throw 1;
        v_ = a.v_;
        return *this;
    }

    bool operator==(const Integer& a) const { return v_ == a.v_; }

    KeyT value() const { return v_; }
private:
    KeyT v_;
};

namespace std {
    template <>
    struct equal_to<Integer> {
        bool operator()(const Integer& a, const Integer& b) const { return a == b; }
    };
    template <>
    struct hash<Integer> : public __hash_base<size_t, Integer> {
        size_t operator()(const Integer& a) {
            return std::_Hash_impl::hash(a.value());
        }
    };
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_map_map_exception_safety) {
    using MyMapT = atomic_hash_map<KeyT,Integer>;

    int   numEntries = 10000;
    float sizeFactor = 0.46;
    std::unique_ptr<MyMapT> m(new MyMapT(int(numEntries * sizeFactor)));

    bool success = true;
    int count = 0;
    for (int i = 0; i < numEntries; i++) {
        try {
            m->insert(i, Integer(genVal(i)));
            success &= (m->find(i)->second == Integer(genVal(i)));
            ++count;
        } catch (...) {
            success &= !m->exists(i);
        }
    }
    BOOST_CHECK_EQUAL(count, int(m->size()));
    BOOST_CHECK(success);
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_map_basic_erase) {
    size_t numEntries = 1000;

    std::unique_ptr<AHMapT> s(new AHMapT(numEntries, config));
    // Iterate filling up the map and deleting all keys a few times
    // to test more than one subMap.
    for (int iterations = 0; iterations < 4; ++iterations) {
        // Testing insertion of keys
        bool success = true;
        for (size_t i = 0; i < numEntries; ++i) {
            success &= !s->exists(i);
            auto ret =  s->insert(RecordT(i, i));
            success &=  s->exists(i);
            success &=  ret.second;
        }
        BOOST_CHECK(success);
        BOOST_CHECK_EQUAL(s->size(), numEntries);

        // Delete every key in the map and verify that the key is gone and the the
        // size is correct.
        success = true;
        for (size_t i = 0; i < numEntries; ++i) {
            success &= s->erase(i);
            success &= (s->size() == numEntries - 1 - i);
            success &= !(s->exists(i));
            success &= !(s->erase(i));
        }
        BOOST_CHECK(success);
    }
    BOOST_MESSAGE("Final number of subMaps = " << s->num_submaps());
}

namespace {

inline KeyT randomizeKey(int key) {
    // We deterministically randomize the key to more accurately simulate
    // real-world usage, and to avoid pathalogical performance patterns (e.g.
    // those related to __gnu_cxx::hash<int64_t>()(1) == 1).
    //
    // Use a hash function we don't normally use for ints to avoid interactions.
    return std::hash<KeyT>()(key);
}

int numOpsPerThread = 0;

std::atomic<bool> runThreadsCreatedAllThreads;
void runThreads(void *(*thread)(void*), int numThreads, void **statuses) {
    runThreadsCreatedAllThreads.store(false);
    vector<pthread_t> threadIds;
    for (int64_t j = 0; j < numThreads; j++) {
        pthread_t tid;
        if (pthread_create(&tid, nullptr, thread, (void*) j) != 0)
            BOOST_MESSAGE("Could not start thread");
        else
            threadIds.push_back(tid);
    }

    runThreadsCreatedAllThreads.store(true);
    for (size_t i = 0; i < threadIds.size(); ++i)
        pthread_join(threadIds[i], statuses == nullptr ? nullptr : &statuses[i]);
}

void runThreads(void *(*thread)(void*)) {
    runThreads(thread, numThreads, nullptr);
}

}

BOOST_AUTO_TEST_CASE( test_atomic_hash_map_collision_test) {
    const int numInserts = 100000 / 4;

    // Doing the same number on each thread so we collide.
    numOpsPerThread = numInserts;

    float sizeFactor = 0.46;
    int entrySize = sizeof(KeyT) + sizeof(ValueT);
    BOOST_MESSAGE("Testing " << numInserts << " unique " << entrySize <<
        " Byte entries replicated in " << numThreads <<
        " threads with " << maxLoadFactor * 100.0 << "% max load factor.");

    globalAHM.reset(new AHMapT(int(numInserts * sizeFactor), config));

    size_t sizeInit = globalAHM->capacity();
    BOOST_MESSAGE("  Initial capacity: " << sizeInit);

    double start = nowInUsec();
    runThreads([](void*) -> void* { // collisionInsertThread
        for (int i = 0; i < numOpsPerThread; ++i) {
            KeyT key = randomizeKey(i);
            globalAHM->insert(key, genVal(key));
        }
        return nullptr;
    });
    double elapsed = nowInUsec() - start;

    size_t finalCap = globalAHM->capacity();
    size_t sizeAHM = globalAHM->size();
    BOOST_MESSAGE(elapsed/sizeAHM << " usec per " << numThreads <<
        " duplicate inserts (atomic).");
    BOOST_MESSAGE("  Final capacity: " << finalCap << " in " <<
        globalAHM->num_submaps() << " sub maps (" <<
        sizeAHM * 100 / finalCap << "% load factor, " <<
        (finalCap - sizeInit) * 100 / sizeInit << "% growth).");

    // check correctness
    BOOST_CHECK_EQUAL(int(sizeAHM), numInserts);
    bool success = true;
    for (int i = 0; i < numInserts; ++i) {
        KeyT key = randomizeKey(i);
        success &= (globalAHM->find(key)->second == genVal(key));
    }
    BOOST_CHECK(success);

    // check colliding finds
    start = nowInUsec();
    runThreads([](void*) -> void* { // collisionFindThread
        KeyT key(0);
        for (int i = 0; i < numOpsPerThread; ++i) {
        globalAHM->find(key);
        }
        return nullptr;
    });

    elapsed = nowInUsec() - start;

    BOOST_MESSAGE((elapsed/sizeAHM) << " usec per " << numThreads <<
        " duplicate finds (atomic).");
}

namespace {

const int kInsertPerThread = 100000;
int raceFinalSizeEstimate;

void* raceIterateThread(void* jj) {
    int count = 0;

    AHMapT::iterator it = globalAHM->begin();
    AHMapT::iterator end = globalAHM->end();
    for (; it != end; ++it) {
        ++count;
        if (count > raceFinalSizeEstimate) {
            BOOST_CHECK(false);
            BOOST_MESSAGE("Infinite loop in iterator.");
            return nullptr;
        }
    }
    return nullptr;
}

void* raceInsertRandomThread(void* jj) {
    for (int i = 0; i < kInsertPerThread; ++i) {
        KeyT key = rand();
        globalAHM->insert(key, genVal(key));
    }
    return nullptr;
}

}

// Test for race conditions when inserting and iterating at the same time and
// creating multiple submaps.
BOOST_AUTO_TEST_CASE( test_atomic_hash_map_race_insert_iterate_thread_test) {
    const int kInsertThreads  = 20;
    const int kIterateThreads = 20;
    raceFinalSizeEstimate = kInsertThreads * kInsertPerThread;

    BOOST_MESSAGE("Testing iteration and insertion with " << kInsertThreads
        << " threads inserting and " << kIterateThreads << " threads iterating.");

    globalAHM.reset(new AHMapT(raceFinalSizeEstimate / 9, config));

    vector<pthread_t> threadIds;
    for (int64_t j = 0; j < kInsertThreads + kIterateThreads; j++) {
        pthread_t tid;
        void *(*thread)(void*) =
            (j < kInsertThreads ? raceInsertRandomThread : raceIterateThread);
        if (pthread_create(&tid, nullptr, thread, (void*) j) != 0) {
            BOOST_CHECK(false);
            BOOST_MESSAGE("Could not start thread");
        } else
            threadIds.push_back(tid);
    }
    for (size_t i = 0; i < threadIds.size(); ++i) {
        pthread_join(threadIds[i], nullptr);
    }
    BOOST_MESSAGE("Ended up with "     << globalAHM->num_submaps() << " submaps");
    BOOST_MESSAGE("Final size of map " << globalAHM->size());
}

namespace {

const int kTestEraseInsertions = 20000;
std::atomic<int32_t> insertedLevel;

void* testEraseInsertThread(void*) {
    for (int i = 0; i < kTestEraseInsertions; ++i) {
        KeyT key = randomizeKey(i);
        globalAHM->insert(key, genVal(key));
        insertedLevel.store(i, std::memory_order_release);
    }
    insertedLevel.store(kTestEraseInsertions, std::memory_order_release);
    return nullptr;
}

void* testEraseEraseThread(void*) {
    for (int i = 0; i < kTestEraseInsertions; ++i) {
        /*
        * Make sure that we don't get ahead of the insert thread, because
        * part of the condition for this unit test succeeding is that the
        * map ends up empty.
        *
        * Note, there is a subtle case here when a new submap is
        * allocated: the erasing thread might get 0 from count(key)
        * because it hasn't seen numSubMaps_ update yet.  To avoid this
        * race causing problems for the test (it's ok for real usage), we
        * lag behind the inserter by more than just element.
        */
        const int lag = 10;
        int currentLevel;
        do {
            currentLevel = insertedLevel.load(std::memory_order_acquire);
            if (currentLevel == kTestEraseInsertions) currentLevel += lag + 1;
        } while (currentLevel - lag < i);

        KeyT key = randomizeKey(i);
        while (globalAHM->exists(key))
            if (globalAHM->erase(key))
                break;
    }
    return nullptr;
}

}

// Here we have a single thread inserting some values, and several threads
// racing to delete the values in the order they were inserted.
BOOST_AUTO_TEST_CASE( test_atomic_hash_map_thread_erase_insert_race) {
    const int kInsertThreads = 1;
    const int kEraseThreads = utxx::detail::cpu_count()-1;

    BOOST_MESSAGE("Testing insertion and erase with " << kInsertThreads
        << " thread inserting and " << kEraseThreads << " threads erasing.");

    globalAHM.reset(new AHMapT(kTestEraseInsertions / 4, config));

    vector<pthread_t> threadIds;
    for (int64_t j = 0; j < kInsertThreads + kEraseThreads; j++) {
        pthread_t tid;
        void *(*thread)(void*) =
            (j < kInsertThreads ? testEraseInsertThread : testEraseEraseThread);
        if (pthread_create(&tid, nullptr, thread, (void*) j) == 0)
            threadIds.push_back(tid);
        else {
            BOOST_MESSAGE("Could not start thread");
            BOOST_REQUIRE(false);
        }
    }
    for (size_t i = 0; i < threadIds.size(); i++)
        pthread_join(threadIds[i], nullptr);

    BOOST_CHECK(globalAHM->empty());
    BOOST_CHECK_EQUAL(globalAHM->size(), 0u);

    BOOST_MESSAGE("Ended up with " << globalAHM->num_submaps() << " submaps");
}

// Repro for T#483734: Duplicate AHM inserts due to incorrect AHA return value.
using AHA = atomic_hash_array<int32_t, int32_t>;
AHA::config configRace;
auto atomicHashArrayInsertRaceArray = AHA::create(2, allocator, configRace);
void* atomicHashArrayInsertRaceThread(void* j) {
    AHA*      arr         = atomicHashArrayInsertRaceArray.get();
    uintptr_t numInserted = 0;
    while (!runThreadsCreatedAllThreads.load());
    for (int i = 0; i < 2; i++)
        if (arr->insert(RecordT(randomizeKey(i), 0)).first != arr->end())
            numInserted++;
    pthread_exit((void *) numInserted);
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_map_atomic_hash_array_insert_race) {
    AHA*  arr        = atomicHashArrayInsertRaceArray.get();
    int   iterations = 50000, threads = utxx::detail::cpu_count();
    void* statuses[threads];
    for (int i = 0; i < iterations; i++) {
        arr->clear();
        runThreads(atomicHashArrayInsertRaceThread, threads, statuses);
        BOOST_CHECK(arr->size() > 1);
        for (int j = 0; j < threads; j++)
            BOOST_CHECK_EQUAL(arr->size(), uintptr_t(statuses[j]));
    }
}