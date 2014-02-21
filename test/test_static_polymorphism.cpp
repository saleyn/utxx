#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

struct Interface {
    std::string to_string(int i) const { throw std::runtime_error("Not implemented!"); }
};

template <class Derived>
struct printer {

    ~printer() {
        static_assert(
            std::is_base_of<Interface, Derived>::value,
            "Derived must implement Interface!");
        static_assert(
            std::is_base_of<printer, Derived>::value,
            "Derived must derive from printer!");
        static_assert(
            std::is_base_of<Interface, Derived>::value,
            "Derived must not be Interface!");
    }

    std::string to_string(int i) const { return static_cast<const Derived*>(this)->to_string(i); }
};

struct Implementation : printer<Implementation>, Interface {
    std::string to_string(int i) const { return boost::lexical_cast<std::string>(i); }
};

struct Implementation2 : printer<Implementation> {
    std::string to_string(int i) const { return boost::lexical_cast<std::string>(i); }
};

BOOST_AUTO_TEST_CASE( test_static_polymorphism )
{
    printer<Implementation> p;
    std::string r = p.to_string(1);

    BOOST_REQUIRE_EQUAL("1", r);

    //printer<Implementation2> p2;
}
