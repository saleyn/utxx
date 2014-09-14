//----------------------------------------------------------------------------
/// \file  test_registrar.hpp
//----------------------------------------------------------------------------
/// \brief Test implementation of registrar.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#include <boost/test/unit_test.hpp>
#include <utxx/test_helper.hpp>
#include <utxx/registrar.hpp>

using namespace utxx;
using namespace std;

struct Base {
    string m_name;

    Base(const string& a_inst) : m_name(a_inst) {}
    virtual ~Base() {}
};

struct A : public Base {
    int& m_data;
    A(const char* a_inst, int& x) : Base(a_inst), m_data(x) {}
};

struct B : public Base {
    string& m_data;
    B(const char* a_inst, string& x) : Base(a_inst), m_data(x) {}
};

struct C : public Base {
    shared_ptr<A> m_a;
    shared_ptr<B> m_b;
    double m_data;

    C(const shared_ptr<A>& a, const shared_ptr<B>& b,
      const char* a_inst, double x) : Base(a_inst), m_a(a), m_b(b), m_data(x)
    {}
};

BOOST_AUTO_TEST_CASE( test_registrar )
{
    typed_registrar reg;
    int     x = 10;
    string  s = "abc";

    reg.reg_class<A>("a", ref(x));
    reg.reg_class<B>("b", ref(s));

    shared_ptr<Base> a0;
    {
        a0 = reg.get_and_register<Base>("A", "instance-of-A");
        BOOST_CHECK_EQUAL(2, a0.use_count());
        reg.erase("A", "instance-of-A");
        BOOST_CHECK_EQUAL(1, a0.use_count());
        a0.reset();
    }
    BOOST_CHECK_EQUAL(0, a0.use_count());

    a0 = reg.get_and_register<Base>("A", "instance-of-A");

    UTXX_CHECK_NO_THROW(reg.get<Base>("A", "instance-of-A"));
    BOOST_CHECK_THROW  (reg.get<B>   ("A", "instance-of-A"), utxx::badarg_error);
    UTXX_CHECK_NO_THROW(reg.get<A>("instance-of-A"));
    UTXX_CHECK_NO_THROW(reg.get<B>("instance-of-B"));
    BOOST_CHECK_THROW  (reg.get<C>("instance-of-C"), utxx::badarg_error);
    BOOST_CHECK_THROW  (reg.reg_class<A>("singleton-A", ref(x)), utxx::badarg_error);

    auto b1 = reg.get<B>("instance-of-B");
    auto b2 = reg.get<B>("instance-of-B", [&]() { return new B("b2", s); });

    BOOST_CHECK(b1 != b2);
    BOOST_CHECK_EQUAL("b",  b1->m_name);
    BOOST_CHECK_EQUAL("b2", b2->m_name);

    auto b3 = reg.get_and_register<B>("inst3-of-B");
    auto b4 = reg.get<B>("inst3-of-B");
    auto b5 = reg.get_and_register<B>("inst3-of-B", [&]() { return new B("b4", s); });

    BOOST_CHECK(b2 != b3);
    BOOST_CHECK(b4 == b3);
    BOOST_CHECK(b5 == b3);

    BOOST_CHECK_EQUAL("b",  b3->m_name);

    auto a1 = reg.get_singleton<A>([&x]() { return new A("xxx", x); });
    auto a2 = reg.get_singleton<A>();
    auto a3 = reg.get_singleton<Base>("A");

    BOOST_CHECK(a1 == a2);
    BOOST_CHECK(a1 == a3);
    BOOST_CHECK_EQUAL("xxx", a1->m_name);

    reg.get_singleton<C>([&]() { return new C(a1, b1, "c", 100.0); });

    // This error is due to the fact that registered instance is of type A
    // which can't be dynamicly casted to Base:
    //BOOST_CHECK_THROW(reg.get<Base>("A", "instance-of-A"), utxx::badarg_error);

    auto base = reg.get<A>("instance-of-A");
    BOOST_CHECK(a0 == base);
    BOOST_CHECK_EQUAL("a", base->m_name);

    {
        auto a = reg.get<A>("instance-of-A");
        auto b = reg.get<B>("instance-of-B");
        auto c = reg.get_singleton<C>();

        BOOST_CHECK_EQUAL(10,    a->m_data);
        BOOST_CHECK_EQUAL("abc", b->m_data);
        BOOST_CHECK_EQUAL(100.0, c->m_data);
        BOOST_CHECK_EQUAL(10,    c->m_a->m_data);
        BOOST_CHECK_EQUAL("abc", c->m_b->m_data);

        x = 20;
        s = "xxx";

        BOOST_CHECK_EQUAL(20,    a->m_data);
        BOOST_CHECK_EQUAL("xxx", b->m_data);
        BOOST_CHECK_EQUAL(100.0, c->m_data);
        BOOST_CHECK_EQUAL(20,    c->m_a->m_data);
        BOOST_CHECK_EQUAL("xxx", c->m_b->m_data);
    }

    BOOST_CHECK(reg.is_instance_registered<B>("inst3-of-B"));
    reg.erase<B>("inst3-of-B");
    BOOST_CHECK(!reg.is_instance_registered<B>("inst3-of-B"));
}
