#include <boost/test/unit_test.hpp>
#include <utxx/concurrent_mpsc_queue.hpp>

#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace utxx {

BOOST_AUTO_TEST_CASE( test_concurrent_mpsc_queue ) {
    {
        concurrent_mpsc_queue<int> queue;

        BOOST_REQUIRE(queue.empty());
        queue.push(1);
        BOOST_REQUIRE(!queue.empty());

        queue.push(2);
        queue.push(3);

        typedef typename concurrent_mpsc_queue<int>::node node;

        node* m, *n = queue.pop_all();

        BOOST_REQUIRE(queue.empty());
        
        BOOST_REQUIRE_EQUAL(1, n->data());
        m = n->next();
        BOOST_REQUIRE(m);
        BOOST_REQUIRE_EQUAL(2, m->data());
        queue.free(n);
        n = m->next();
        BOOST_REQUIRE(n);
        BOOST_REQUIRE_EQUAL(3, n->data());
        queue.free(m);
        m = n->next();
        BOOST_REQUIRE(!m);
        queue.free(n);
    }
    {
        typedef typename concurrent_mpsc_queue<char>::node node;

        concurrent_mpsc_queue<char> queue;

        BOOST_REQUIRE(queue.empty());
        bool res = queue.push(3, [](char* a_data, size_t) { strcpy(a_data, "ab"); });

        BOOST_REQUIRE(!queue.empty());         BOOST_REQUIRE(res);

        res = queue.push("xyz");               BOOST_REQUIRE(res);
        res = queue.push(std::string("test")); BOOST_REQUIRE(res);
        res = queue.emplace<int>(123);         BOOST_REQUIRE(res);

        node* m, *n = queue.pop_all();

        BOOST_REQUIRE(queue.empty());
        
        BOOST_REQUIRE_EQUAL("ab", n->data());
        m = n->next();
        BOOST_REQUIRE(m);
        BOOST_REQUIRE_EQUAL("xyz", m->data());
        queue.free(n);
        n = m->next();
        BOOST_REQUIRE(n);
        BOOST_REQUIRE_EQUAL("test", n->data());
        queue.free(m);
        m = n->next();
        BOOST_REQUIRE(m);
        BOOST_REQUIRE_EQUAL(123, m->to<int>());
        queue.free(n);
        n = m->next();
        BOOST_REQUIRE(!n);
        queue.free(m);
    }
}

} // namespace utxx
