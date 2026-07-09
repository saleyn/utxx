#include <utxx/test_helper.hpp>
#include <utxx/concurrent_mpsc_queue.hpp>

#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace utxx {

TEST_CASE( test_concurrent_mpsc_queue ) {
    {
        concurrent_mpsc_queue<int> queue;

        REQUIRE(queue.empty());
        queue.push(1);
        REQUIRE(!queue.empty());

        queue.push(2);
        queue.push(3);

        typedef typename concurrent_mpsc_queue<int>::node node;

        node* m, *n = queue.pop_all();

        REQUIRE(queue.empty());
        
        REQUIRE_EQUAL(1, n->data());
        m = n->next();
        REQUIRE(m);
        REQUIRE_EQUAL(2, m->data());
        queue.free(n);
        n = m->next();
        REQUIRE(n);
        REQUIRE_EQUAL(3, n->data());
        queue.free(m);
        m = n->next();
        REQUIRE(!m);
        queue.free(n);
    }
    {
        typedef typename concurrent_mpsc_queue<char>::node node;

        concurrent_mpsc_queue<char> queue;

        REQUIRE(queue.empty());
        bool res = queue.push(3, [](char* a_data, size_t) { strcpy(a_data, "ab"); });

        REQUIRE(!queue.empty());         REQUIRE(res);

        res = queue.push("xyz");               REQUIRE(res);
        res = queue.push(std::string("test")); REQUIRE(res);
        res = queue.emplace<int>(123);         REQUIRE(res);

        node* m, *n = queue.pop_all();

        REQUIRE(queue.empty());
        
        REQUIRE_EQUAL("ab", n->data());
        m = n->next();
        REQUIRE(m);
        REQUIRE_EQUAL("xyz", m->data());
        queue.free(n);
        n = m->next();
        REQUIRE(n);
        REQUIRE_EQUAL("test", n->data());
        queue.free(m);
        m = n->next();
        REQUIRE(m);
        REQUIRE_EQUAL(123, m->to<int>());
        queue.free(n);
        n = m->next();
        REQUIRE(!n);
        queue.free(m);
    }
}

} // namespace utxx
