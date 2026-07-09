#include <utxx/test_helper.hpp>


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
    std::string to_string(int i) const { return std::to_string(i); }
};

struct Implementation2 : printer<Implementation> {
    std::string to_string(int i) const { return std::to_string(i); }
};

TEST_CASE( test_static_polymorphism )
{
    printer<Implementation> p;
    std::string r = p.to_string(1);

    REQUIRE_EQUAL("1", r);

    //Interface* pi = reinterpret_cast<Interface*>(&p);

    //std::string s = pi->to_string(2);
    //REQUIRE_EQUAL("2", r);

    //printer<Implementation2> p2;
}
