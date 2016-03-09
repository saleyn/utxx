// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace utxx {

// This allocator can be used with STL containers to provide a stack buffer
// from which to allocate memory and overflows onto the heap. This stack buffer
// would be allocated on the stack and allows us to avoid heap operations in
// some situations.
//
// STL likes to make copies of allocators, so the allocator itself can't hold
// the data. Instead, we make the creator responsible for creating a
// stack_allocator::storage which contains the data. Copying the allocator
// merely copies the pointer to this shared source, so all allocators created
// based on our allocator will share the same stack buffer.
//
// This stack buffer implementation is very simple. The first allocation that
// fits in the stack buffer will use the stack buffer. Any subsequent
// allocations will not use the stack buffer, even if there is unused room.
// This makes it appropriate for array-like containers, but the caller should
// be sure to reserve() in the container up to the stack buffer size. Otherwise
// the container will allocate a small array which will "use up" the stack
// buffer.
template<typename T, size_t stack_capacity>
class stack_allocator : public std::allocator<T>
{
public:
    typedef typename std::allocator<T>::pointer pointer;
    typedef typename std::allocator<T>::size_type size_type;

    // Backing store for the allocator. The container owner is responsible for
    // maintaining this for as long as any containers using this allocator are
    // live.
    struct storage
    {
        storage() : m_used_stack_buffer(false) {}

        // Casts the buffer in its right type.
        T* stack_buffer()
        {
            return reinterpret_cast<T*>(m_stack_buffer);
        }
        const T* stack_buffer() const
        {
            return reinterpret_cast<const T*>(m_stack_buffer);
        }

        //
        // IMPORTANT: Take care to ensure that stack_buffer_ is aligned
        // since it is used to mimic an array of T.
        // Be careful while declaring any unaligned types (like bool)
        // before stack_buffer_.
        //

        // The buffer itself. It is not of type T because we don't want the
        // constructors and destructors to be automatically called. Define a POD
        // buffer of the right size instead.
        char m_stack_buffer[sizeof(T[stack_capacity])];

        // Set when the stack buffer is used for an allocation. We do not track
        // how much of the buffer is used, only that somebody is using it.
        bool m_used_stack_buffer;
    };

    // Used by containers when they want to refer to an allocator of type U.
    template<typename U>
    struct rebind
    {
        typedef stack_allocator<U, stack_capacity> other;
    };

    // For the straight up copy c-tor, we can share storage.
    stack_allocator(const stack_allocator<T, stack_capacity>& rhs)
        : std::allocator<T>(), m_src(rhs.m_src)
    {}

    // ISO C++ requires the following constructor to be defined,
    // and std::vector in VC++2008SP1 Release fails with an error
    // in the class _Container_base_aux_alloc_real (from <xutility>)
    // if the constructor does not exist.
    // For this constructor, we cannot share storage; there's
    // no guarantee that the storage buffer of Ts is large enough
    // for Us.
    // TODO: If we were fancy pants, perhaps we could share storage
    // iff sizeof(T) == sizeof(U).
    template<typename U, size_t other_capacity>
    stack_allocator(const stack_allocator<U, other_capacity>& other)
        : m_src(NULL)
    {}

    // Don't use this allocator directly - it's for compatibility with GCC < 5.1
    // when using with std::string that has __a == __Alloc() comparison
    stack_allocator() : m_src(NULL)
    {}

    explicit stack_allocator(storage* source) : m_src(source)
    {}

    // Actually do the allocation. Use the stack buffer if nobody has used it yet
    // and the size requested fits. Otherwise, fall through to the standard
    // allocator.
    pointer allocate(size_type n, void* hint = 0)
    {
        if (m_src != NULL && !m_src->m_used_stack_buffer && n <= stack_capacity)
        {
            m_src->m_used_stack_buffer = true;
            return m_src->stack_buffer();
        }
        else
        {
            return std::allocator<T>::allocate(n, hint);
        }
    }

    // Free: when trying to free the stack buffer, just mark it as free. For
    // non-stack-buffer pointers, just fall though to the standard allocator.
    void deallocate(pointer p, size_type n)
    {
        if (m_src != NULL && p == m_src->stack_buffer())
            m_src->m_used_stack_buffer = false;
        else
            std::allocator<T>::deallocate(p, n);
    }

    bool operator==(const stack_allocator& rhs) const {
        return m_src == rhs.m_src;
    }

    bool used_stack() const { return m_src && m_src->m_used_stack_buffer; }
private:
    storage* m_src;
};

// A wrapper around STL containers that maintains a stack-sized buffer that the
// initial capacity of the vector is based on. Growing the container beyond the
// stack capacity will transparently overflow onto the heap. The container must
// support reserve().
//
// WATCH OUT: the ContainerType MUST use the proper stack_allocator for this
// type. This object is really intended to be used only internally. You'll want
// to use the wrappers below for different types.
template<typename TContainerType, int stack_capacity>
class stack_container
{
public:
    typedef TContainerType ContainerType;
    typedef typename ContainerType::value_type value_type;
    typedef stack_allocator<value_type, stack_capacity> Allocator;

    // Allocator must be constructed before the container!
    explicit stack_container(size_t capacity = stack_capacity)
        : m_allocator(&m_stack_data), m_container(m_allocator)
    {
        // Make the container use the stack allocation by reserving our buffer size
        // before doing anything else.
        m_container.reserve(capacity);
    }

    // Getters for the actual container.
    //
    // Danger: any copies of this made using the copy constructor must have
    // shorter lifetimes than the source. The copy will share the same allocator
    // and therefore the same stack buffer as the original. Use std::copy to
    // copy into a "real" container for longer-lived objects.
    ContainerType&       container()        { return m_container; }
    const ContainerType& container()  const { return m_container; }

    // Support operator-> to get to the container. This allows nicer syntax like:
    //   stack_container<...> foo;
    //   std::sort(foo->begin(), foo->end());
    ContainerType*       operator->()       { return &m_container; }
    const ContainerType* operator->() const { return &m_container; }

#ifdef UNIT_TEST
    // Retrieves the stack source so that that unit tests can verify that the
    // buffer is being used properly.
    const typename Allocator::storage& stack_data() const { return m_stack_data; }
#endif

protected:
    typename Allocator::storage m_stack_data;
    Allocator                   m_allocator;
    ContainerType               m_container;

    stack_container(stack_container const&)            = delete;
    stack_container& operator=(stack_container const&) = delete;
};

// basic_stack_string
template<size_t stack_capacity>
class basic_stack_string
    : public stack_container<
        std::basic_string<char, std::char_traits<char>,
                          stack_allocator<char, stack_capacity>>,
        stack_capacity
      >
{
    using base = stack_container<
        std::basic_string<char, std::char_traits<char>,
                          stack_allocator<char, stack_capacity>>,
        stack_capacity
      >;
public:
    // Note: reserving capacity-1 because string implementation does +1 to
    // provision for terminating '\0'.
    basic_stack_string() : base(stack_capacity-1) {}

private:
    basic_stack_string(basic_stack_string const&)            = delete;
    basic_stack_string& operator=(basic_stack_string const&) = delete;
};

// basic_stack_wstring
template<size_t stack_capacity>
class basic_stack_wstring
    : public stack_container<
        std::basic_string<wchar_t, std::char_traits<wchar_t>,
                          stack_allocator<wchar_t, stack_capacity>>,
        stack_capacity>
{
    using base = stack_container<
        std::basic_string<wchar_t, std::char_traits<wchar_t>,
                          stack_allocator<wchar_t, stack_capacity>>,
        stack_capacity>;
public:
    basic_stack_wstring() : base(stack_capacity - sizeof(wchar_t)) {}

private:
    basic_stack_wstring(basic_stack_wstring const&)            = delete;
    basic_stack_wstring& operator=(basic_stack_wstring const&) = delete;
};

// basic_stack_vector
//
// Example:
//   basic_stack_vector<int, 16> foo;
//   foo->push_back(22);  // we have overloaded operator->
//   foo[0] = 10;         // as well as operator[]
template<typename T, size_t stack_capacity>
class basic_stack_vector : public stack_container<
    std::vector<T, stack_allocator<T, stack_capacity> >,
    stack_capacity>
{
public:
    basic_stack_vector() : stack_container<
        std::vector<T, stack_allocator<T, stack_capacity> >,
        stack_capacity>()
    {}

    // We need to put this in STL containers sometimes, which requires a copy
    // constructor. We can't call the regular copy constructor because that will
    // take the stack buffer from the original. Here, we create an empty object
    // and make a stack buffer of its own.
    basic_stack_vector(const basic_stack_vector<T, stack_capacity>& other)
        : stack_container<
        std::vector<T, stack_allocator<T, stack_capacity> >,
        stack_capacity>()
    {
        this->container().assign(other->begin(), other->end());
    }

    basic_stack_vector<T, stack_capacity>& operator=(
        const basic_stack_vector<T, stack_capacity>& other)
    {
        this->container().assign(other->begin(), other->end());
        return *this;
    }

    // Vectors are commonly indexed, which isn't very convenient even with
    // operator-> (using "->at()" does exception stuff we don't want).
    T&       operator[](size_t i)       { return this->container().operator[](i); }
    const T& operator[](size_t i) const { return this->container().operator[](i); }
};

} // namespace utxx