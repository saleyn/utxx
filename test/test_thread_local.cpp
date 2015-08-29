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
#include <utxx/thread_local.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>

#include <boost/thread/tss.hpp>

using namespace utxx;

struct Widget {
    static int m_total_val;
    int m_val;
    ~Widget() {
        m_total_val += m_val;
    }

    static void customDeleter(Widget* w, tlp_destruct_mode mode) {
        m_total_val += (mode == tlp_destruct_mode::ALL_THREADS) * 1000;
        delete w;
    }
};
int Widget::m_total_val = 0;

BOOST_AUTO_TEST_CASE( test_thread_local_basic_destructor2 ) {
    Widget::m_total_val = 0;
    thr_local_ptr<Widget> w;
    std::thread([&w]() {
        w.reset(new Widget());
        w.get()->m_val += 10;
    }).join();
    BOOST_CHECK_EQUAL(10, Widget::m_total_val);
}

BOOST_AUTO_TEST_CASE( test_thread_local_custom_deleter1 ) {
    Widget::m_total_val = 0;
    {
        thr_local_ptr<Widget> w;
        std::thread([&w]() {
            w.reset(new Widget(), Widget::customDeleter);
            w.get()->m_val += 10;
        }).join();
        BOOST_CHECK_EQUAL(10, Widget::m_total_val);
    }
    BOOST_CHECK_EQUAL(10, Widget::m_total_val);
}

BOOST_AUTO_TEST_CASE( test_thread_local_reset_null ) {
    thr_local_ptr<int> tl;
    BOOST_CHECK(!tl);
    tl.reset(new int(4));
    BOOST_CHECK(static_cast<bool>(tl));
    BOOST_CHECK_EQUAL(*tl.get(), 4);
    tl.reset();
    BOOST_CHECK(!tl);
}

BOOST_AUTO_TEST_CASE( test_thread_local_test_release ) {
    Widget::m_total_val = 0;
    thr_local_ptr<Widget> w;
    std::unique_ptr<Widget> wPtr;
    std::thread([&w, &wPtr]() {
        w.reset(new Widget());
        w.get()->m_val += 10;

        wPtr.reset(w.release());
    }).join();
    BOOST_CHECK_EQUAL(0, Widget::m_total_val);
    wPtr.reset();
    BOOST_CHECK_EQUAL(10, Widget::m_total_val);
}

// Test deleting the thr_local_ptr object
BOOST_AUTO_TEST_CASE( test_thread_local_custom_deleter2) {
    Widget::m_total_val = 0;
    std::thread t;
    std::mutex mutex;
    std::condition_variable cv;
    enum class State {
        START,
        DONE,
        EXIT
    };
    State state = State::START;
    {
        thr_local_ptr<Widget> w;
        t = std::thread([&]() {
            w.reset(new Widget(), Widget::customDeleter);
            w.get()->m_val += 10;

            // Notify main thread that we're done
            {
                std::unique_lock<std::mutex> lock(mutex);
                state = State::DONE;
                cv.notify_all();
            }

            // Wait for main thread to allow us to exit
            {
                std::unique_lock<std::mutex> lock(mutex);
                while (state != State::EXIT)
                    cv.wait(lock);
            }
        });

        // Wait for main thread to start (and set w.get()->m_val)
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (state != State::DONE)
                cv.wait(lock);
        }

        // Thread started but hasn't exited yet
        BOOST_CHECK_EQUAL(0, Widget::m_total_val);

        // Destroy thr_local_ptr<Widget> (by letting it go out of scope)
    }

    BOOST_CHECK_EQUAL(1010, Widget::m_total_val);

    // Allow thread to exit
    {
        std::unique_lock<std::mutex> lock(mutex);
        state = State::EXIT;
        cv.notify_all();
    }
    t.join();

    BOOST_CHECK_EQUAL(1010, Widget::m_total_val);
}

BOOST_AUTO_TEST_CASE( test_thread_local_basic_destructor ) {
    Widget::m_total_val = 0;
    thr_local<Widget> w;
    std::thread([&w]() { w->m_val += 10; }).join();
    BOOST_CHECK_EQUAL(10, Widget::m_total_val);
}

BOOST_AUTO_TEST_CASE( test_thread_local_simple_repeat_destructor ) {
    Widget::m_total_val = 0;
    {
        thr_local<Widget> w;
        w->m_val += 10;
    }
    {
        thr_local<Widget> w;
        w->m_val += 10;
    }
    BOOST_CHECK_EQUAL(20, Widget::m_total_val);
}

BOOST_AUTO_TEST_CASE( test_thread_local_interleaved_destructors ) {
    Widget::m_total_val = 0;
    std::unique_ptr<thr_local<Widget>> w;
    int wVersion = 0;
    const int wVersionMax = 2;
    int thIter = 0;
    std::mutex lock;
    auto th = std::thread([&]() {
        int wVersionPrev = 0;
        while (true) {
            while (true) {
                std::lock_guard<std::mutex> g(lock);
                if (wVersion > wVersionMax)
                    return;
                if (wVersion > wVersionPrev) {
                    // We have a new version of w, so it should be initialized to zero
                    BOOST_CHECK_EQUAL((*w)->m_val, 0);
                    break;
                }
            }
            std::lock_guard<std::mutex> g(lock);
            wVersionPrev = wVersion;
            (*w)->m_val  += 10;
            ++thIter;
        }
    });
    for (int i=0; i < wVersionMax; ++i) {
        int thIterPrev = 0;
        {
            std::lock_guard<std::mutex> g(lock);
            thIterPrev = thIter;
            w.reset(new thr_local<Widget>());
            ++wVersion;
        }
        while (true) {
            std::lock_guard<std::mutex> g(lock);
            if (thIter > thIterPrev)
                break;
        }
    }
    {
        std::lock_guard<std::mutex> g(lock);
        wVersion = wVersionMax + 1;
    }
    th.join();
    BOOST_CHECK_EQUAL(wVersionMax * 10, Widget::m_total_val);
}

class SimpleThreadCachedInt {
    class NewTag;
    thr_local<int,NewTag> m_val;
public:
    void add(int val) { *m_val += val; }

    int read() {
        int ret = 0;
        for (const auto& i : m_val.access_all_threads())
            ret += i;
        return ret;
    }
};

BOOST_AUTO_TEST_CASE( test_thread_local_access_all_threads_counter ) {
    const int kNumThreads = 10;
    SimpleThreadCachedInt stci;
    std::atomic<bool> run(true);
    std::atomic<int> totalAtomic(0);
    std::vector<std::thread> threads;
    for (int i = 0; i < kNumThreads; ++i)
        threads.push_back(std::thread([&,i]() {
            stci.add(1);
            totalAtomic.fetch_add(1);
            while (run.load()) { usleep(100); }
        }));
    while (totalAtomic.load() != kNumThreads) { usleep(100); }
    BOOST_CHECK_EQUAL(kNumThreads, stci.read());
    run.store(false);
    for (auto& t : threads)
        t.join();
}

BOOST_AUTO_TEST_CASE( test_thread_local_reset_null2 ) {
    thr_local<int> tl;
    tl.reset(new int(4));
    BOOST_CHECK_EQUAL(*tl.get(), 4);
    tl.reset();
    BOOST_CHECK_EQUAL(*tl.get(), 0);
    tl.reset(new int(5));
    BOOST_CHECK_EQUAL(*tl.get(), 5);
}

namespace {
    struct Tag {};

    struct Foo {
        utxx::thr_local<int, Tag> tl;
    };
}  // namespace

BOOST_AUTO_TEST_CASE( test_thread_local_movable1 ) {
    Foo a;
    Foo b;
    BOOST_CHECK(a.tl.get() != b.tl.get());

    a = Foo();
    b = Foo();
    BOOST_CHECK(a.tl.get() != b.tl.get());
}

BOOST_AUTO_TEST_CASE( test_thread_local_movable2 ) {
    std::map<int, Foo> map;

    map[42];
    map[10];
    map[23];
    map[100];

    std::set<void*> tls;
    for (auto& m : map)
        tls.insert(m.second.tl.get());

    // Make sure that we have 4 different instances of *tl
    BOOST_CHECK_EQUAL(4u, tls.size());
}

namespace {

    constexpr size_t kFillObjectSize = 300;

    std::atomic<uint64_t> gDestroyed;

    /**
    * Fill a chunk of memory with a unique-ish pattern that includes the thread id
    * (so deleting one of these from another thread would cause a failure)
    *
    * Verify it explicitly and on destruction.
    */
    class FillObject {
    public:
        explicit FillObject(uint64_t idx) : idx_(idx) {
            uint64_t v = val();
            for (size_t i = 0; i < kFillObjectSize; ++i)
                data_[i] = v;
        }

        void check() {
            uint64_t v = val();
            for (size_t i = 0; i < kFillObjectSize; ++i)
                BOOST_CHECK(v == data_[i]);
        }

        ~FillObject() {
            ++gDestroyed;
        }

    private:
        uint64_t val() const {
            return (idx_ << 40) | uint64_t(pthread_self());
        }

        uint64_t idx_;
        uint64_t data_[kFillObjectSize];
    };

}  // namespace

// Yes, threads and fork don't mix
// (http://cppwisdom.quora.com/Why-threads-and-fork-dont-mix) but if you're
// stupid or desperate enough to try, we shouldn't stand in your way.
namespace {
    class HoldsOne {
    public:
        HoldsOne() : value_(1) { }
        // Do an actual access to catch the buggy case where this == nullptr
        int value() const { return value_; }
    private:
        int value_;
    };

    struct HoldsOneTag {};

    thr_local<HoldsOne, HoldsOneTag> ptr;

    int totalValue() {
        int value = 0;
        for (auto& p : ptr.access_all_threads())
            value += p.value();
        return value;
    }

}  // namespace

BOOST_AUTO_TEST_CASE( test_thread_local_fork ) {
    BOOST_CHECK_EQUAL(1, ptr->value());  // ensure created
    BOOST_CHECK_EQUAL(1, totalValue());
    // Spawn a new thread

    std::mutex mutex;
    bool started = false;
    std::condition_variable startedCond;
    bool stopped = false;
    std::condition_variable stoppedCond;

    std::thread t([&] () {
        BOOST_CHECK_EQUAL(1, ptr->value());  // ensure created
        {
            std::unique_lock<std::mutex> lock(mutex);
            started = true;
            startedCond.notify_all();
        }
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (!stopped)
                stoppedCond.wait(lock);
        }
    });

    {
        std::unique_lock<std::mutex> lock(mutex);
        while (!started)
            startedCond.wait(lock);
    }

    BOOST_CHECK_EQUAL(2, totalValue());

    pid_t pid = fork();
    if (pid == 0) {
        // in child
        int v = totalValue();

        // exit successfully if v == 1 (one thread)
        // diagnostic error code otherwise :)
        switch (v) {
            case 1: _exit(0);
            case 0: _exit(1);
        }
        _exit(2);
    } else if (pid > 0) {
        // in parent
        int status;
        BOOST_CHECK_EQUAL(pid, waitpid(pid, &status, 0));
        BOOST_CHECK(WIFEXITED(status));
        BOOST_CHECK_EQUAL(0, WEXITSTATUS(status));
    } else {
        BOOST_TEST_MESSAGE("Fork failed");
        BOOST_CHECK(false);
    }

    BOOST_CHECK_EQUAL(2, totalValue());

    {
        std::unique_lock<std::mutex> lock(mutex);
        stopped = true;
        stoppedCond.notify_all();
    }

    t.join();

    BOOST_CHECK_EQUAL(1, totalValue());
}

struct HoldsOneTag2 {};

BOOST_AUTO_TEST_CASE( test_thread_local_fork2 ) {
    // A thread-local tag that was used in the parent from a *different* thread
    // (but not the forking thread) would cause the child to hang in a
    // thr_local_ptr's object destructor. Yeah.
    thr_local<HoldsOne, HoldsOneTag2> p;
    {
        // use tag in different thread
        std::thread t([&p] { p.get(); });
        t.join();
    }
    pid_t pid = fork();
    if (pid == 0) {
        {
            thr_local<HoldsOne, HoldsOneTag2> q;
            q.get();
        }
        _exit(0);
    } else if (pid > 0) {
        int status;
        BOOST_CHECK_EQUAL(pid, waitpid(pid, &status, 0));
        BOOST_CHECK(WIFEXITED(status));
        BOOST_CHECK_EQUAL(0, WEXITSTATUS(status));
    } else {
        BOOST_TEST_MESSAGE("Fork failed");
        BOOST_CHECK(false);
    }
}

// Simple reference implementation using pthread_get_specific
template<typename T>
class PThreadGetSpecific {
public:
    PThreadGetSpecific() : key_(0) {
        pthread_key_create(&key_, OnThreadExit);
    }

    T* get() const { return static_cast<T*>(pthread_getspecific(key_)); }

    void reset(T* t) {
        delete get();
        pthread_setspecific(key_, t);
    }
    static void OnThreadExit(void* obj) {
        delete static_cast<T*>(obj);
    }
private:
    pthread_key_t key_;
};
