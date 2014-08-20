//----------------------------------------------------------------------------
/// \file  test_type_traits.cpp
//----------------------------------------------------------------------------
/// \brief This file contains test cases for static metafunctions
//----------------------------------------------------------------------------
// Copyright (c) 2011 Travis Gocke
// Created: 2011-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****
*
*   Copyright 2011 Travis Gockel
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*  
***** END LICENSE BLOCK *****
*/

#include <boost/test/unit_test.hpp>
#include <utxx/type_traits.hpp>
#include <iostream>

using namespace utxx;

struct check_has_print {
    template <typename T, void (T::*)(std::ostream&) const = &T::print>
    struct get {};
};

struct check_has_derived_print {
    template <typename T, void (T::derived::*)(std::ostream&) const = &T::derived::print>
    struct get {};
};

template <typename T>
struct has_print : has_member<T, check_has_print>
{};

template <typename T>
struct has_derived_print : has_member<T, check_has_derived_print>
{};

struct check_has_x {
    template <typename T, int (T::*) = &T::x>
    struct get {};
};

template <typename T>
struct has_x : has_member<T, check_has_x>
{};

struct Yes {
    void print(std::ostream&) const;
    int x;
};

struct No {};

struct WrongSig {
    void print(std::ostream&);
    float x;
};

template <class Derived>
struct DerivedYes : public Derived {
    using derived = Derived;
};

template <typename T>
typename std::enable_if<has_print<T>::value, std::ostream&>::type
operator<< (std::ostream& stream, const T& value) {
    value.print(stream);
    return stream;
}

BOOST_AUTO_TEST_CASE( test_type_traits )
{
    static_assert(has_print<Yes>::value,       "Yes: has print()");
    static_assert(!has_print<No>::value,       "No: doesn't have print()");
    static_assert(!has_print<WrongSig>::value, "WrongSig: wrong signature of print()");

    static_assert(has_x<Yes>::value,           "Yes: doesn't have x");
    static_assert(!has_x<No>::value,           "No: has x");
    static_assert(!has_x<WrongSig>::value,     "WrongSig: has x of improper type");

    static_assert(has_derived_print<DerivedYes<Yes>>::value, "DerivedYes doesn't have x");

    BOOST_REQUIRE(true);
}
