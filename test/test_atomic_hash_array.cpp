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

#include <sys/mman.h>

#include <cstddef>
#include <map>
#include <stdexcept>
#include <type_traits>

#include <boost/test/unit_test.hpp>
#include <utxx/atomic_hash_array.hpp>

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

using namespace std;
using namespace utxx;

namespace std {
    template <class T, class V>
    ostream& operator<< (ostream& out, const pair<T, V>& a) {
        return out << '{' << a.first << ", " << a.second << '}' << endl;
    }
}

template <class T>
class MmapAllocator {
public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;

    typedef ptrdiff_t difference_type;
    typedef size_t size_type;

    T*       address(T& x) const { return std::addressof(x); }

    const T* address(const T& x) const { return std::addressof(x); }

    constexpr size_t max_size() const { return std::numeric_limits<size_t>::max(); }

    bool operator!=(const MmapAllocator<T>& a) const { return !(*this == a); }
    bool operator==(const MmapAllocator<T>& a) const { return true; }

    template <class... Args>
    void construct(T* p, Args&&... args) { new (p) T(std::forward<Args>(args)...); }

    void destroy(T* p) { p->~T(); }

    T* allocate(size_t n) {
        void* p = mmap(nullptr, n * sizeof(T), PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) throw std::bad_alloc();
        return (T*)p;
    }

    void deallocate(T* p, size_t n) { munmap(p, n * sizeof(T)); }
};

template<class KeyT, class ValueT>
typename std::enable_if<std::is_integral<ValueT>::value,
                        pair<KeyT,ValueT>>::
type create_entry(int i) {
    return std::make_pair((KeyT)(std::hash<int>()(i) % 1000), (ValueT)(i + 3));
}

template<class KeyT, class ValueT>
typename std::enable_if<std::is_same<ValueT, std::string>::value,
                        pair<KeyT,ValueT>>::
type create_entry(int i) {
    return std::make_pair((KeyT)(std::hash<int>()(i) % 1000),
                          (ValueT)(std::to_string(i + 3)));
}

template<class KeyT, class ValueT, class Allocator = std::allocator<char>>
void test_map() {
    using MyArr = atomic_hash_array<KeyT, ValueT, std::hash<KeyT>,
                                    std::equal_to<KeyT> /*, Allocator */>;
    Allocator alloc;
    auto arr = MyArr::create(150, alloc);
    map<KeyT, ValueT> ref;
    for (int i = 0; i < 100; ++i) {
        auto e = create_entry<KeyT, ValueT>(i);
        auto ret = arr->insert(e);
        BOOST_CHECK(!ref.count(e.first) == ret.second);  // succeed if not in ref
        ref.insert(e);
        BOOST_CHECK_EQUAL(ref.size(), arr->size());
        if (ret.first == arr->end()) {
            BOOST_TEST_MESSAGE("AHA should not have run out of space.");
            BOOST_CHECK(false);
            continue;
        }
        BOOST_CHECK_EQUAL(e.first, ret.first->first);
        BOOST_CHECK_EQUAL(ref.find(e.first)->second, ret.first->second);
    }

    for (int i = 125; i > 0; i -= 10) {
        auto e = create_entry<KeyT, ValueT>(i);
        auto ret    = arr->erase(e.first);
        auto refRet = ref.erase(e.first);
        BOOST_CHECK_EQUAL(ref.size(), arr->size());
        BOOST_CHECK_EQUAL(refRet, ret);
    }

    for (int i = 155; i > 0; i -= 10) {
        auto e = create_entry<KeyT, ValueT>(i);
        auto ret    = arr->insert(e);
        auto refRet = ref.insert(e);
        BOOST_CHECK_EQUAL(ref.size(), arr->size());
        BOOST_CHECK_EQUAL(*refRet.first, *ret.first);
        BOOST_CHECK_EQUAL(refRet.second, ret.second);
    }

    for (const auto& e : ref) {
        auto ret = arr->find(e.first);
        if (ret == arr->end()) {
            BOOST_TEST_MESSAGE("Key " << e.first << " was not in AHA");
            BOOST_CHECK(false);
            continue;
        }
        BOOST_CHECK_EQUAL(e.first,  ret->first);
        BOOST_CHECK_EQUAL(e.second, ret->second);
    }
}

template<class KeyT, class ValueT, class Allocator = std::allocator<char>>
void test_noncopyable_map() {
    using MyArr = atomic_hash_array<KeyT, std::unique_ptr<ValueT>,
                                    std::hash<KeyT>, std::equal_to<KeyT>>;
    Allocator alloc;
    auto arr = MyArr::create(150, alloc);
    for (int i = 0; i < 100; i++)
        arr->insert(make_pair(i,std::unique_ptr<ValueT>(new ValueT(i))));

    for (int i = 0; i < 100; i++) {
        auto ret = arr->find(i);
        BOOST_CHECK_EQUAL(*(ret->second), i);
    }
}


BOOST_AUTO_TEST_CASE( test_atomic_hash_array_insert_i32_i32 ) {
    test_map<int32_t, int32_t>();
    test_map<int32_t, int32_t, MmapAllocator<char>>();
    test_noncopyable_map<int32_t, int32_t>();
    test_noncopyable_map<int32_t, int32_t, MmapAllocator<char>>();
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_array_insert_erase_i64_i32 ) {
    test_map<int64_t, int32_t>();
    test_map<int64_t, int32_t, MmapAllocator<char>>();
    test_noncopyable_map<int64_t, int32_t>();
    test_noncopyable_map<int64_t, int32_t, MmapAllocator<char>>();
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_array_insert_erase_i64_i64 ) {
    test_map<int64_t, int64_t>();
    test_map<int64_t, int64_t, MmapAllocator<char>>();
    test_noncopyable_map<int64_t, int64_t>();
    test_noncopyable_map<int64_t, int64_t, MmapAllocator<char>>();
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_array_insert_erase_i32_i64 ) {
    test_map<int32_t, int64_t>();
    test_map<int32_t, int64_t, MmapAllocator<char>>();
    test_noncopyable_map<int32_t, int64_t>();
    test_noncopyable_map<int32_t, int64_t, MmapAllocator<char>>();
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_array_insert_erase_i32_str ) {
    test_map<int32_t, string>();
    test_map<int32_t, string, MmapAllocator<char>>();
}

BOOST_AUTO_TEST_CASE( test_atomic_hash_array_insert_erase_i64_str ) {
    test_map<int64_t, string>();
    test_map<int64_t, string, MmapAllocator<char>>();
}
