#include <boost/test/unit_test.hpp>
#include <utxx/concurrent_mpsc_queue.hpp>

#include <vector>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace utxx {

BOOST_AUTO_TEST_CASE( test_concurrent_mpsc ) {
    concurrent_mpsc_queue<int> queue;

    BOOST_REQUIRE(queue.empty());
    queue.push(1);
    BOOST_REQUIRE(!queue.empty());

    queue.push(2);
    queue.push(3);

    typedef typename concurrent_mpsc_queue<int>::node node;

    node* m, *n = queue.pop_all();

    BOOST_REQUIRE(queue.empty());
    
    BOOST_REQUIRE_EQUAL(1, n->data);
    m = n->next;
    BOOST_REQUIRE(m);
    BOOST_REQUIRE_EQUAL(2, m->data);
    queue.free(n);
    n = m->next;
    BOOST_REQUIRE(n);
    BOOST_REQUIRE_EQUAL(3, n->data);
    queue.free(m);
    m = n->next;
    BOOST_REQUIRE(!m);
}

} // namespace utxx
