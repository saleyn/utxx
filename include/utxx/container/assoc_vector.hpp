//------------------------------------------------------------------------------
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any
//     purpose is hereby granted without fee, provided that the above copyright
//     notice appear in all copies and that both that copyright notice and this
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the
//     suitability of this software for any purpose. It is provided "as is"
//     without express or implied warranty.
//------------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <functional>
#include <vector>
#include <utility>

namespace utxx {
    //--------------------------------------------------------------------------
    /// class template assoc_vector_compare
    /// Used by assoc_vector
    //--------------------------------------------------------------------------
    namespace detail {
        template <class Value, class C>
        class assoc_vector_compare : public C
        {
            using data      = std::pair<typename C::first_argument_type, Value>;
            using first_argument_type = typename C::first_argument_type;

        public:
            assoc_vector_compare() {}

            assoc_vector_compare(const C& src) : C(src) {}
            assoc_vector_compare(C&&      src) : C(std::move(src)) {}

            bool operator()(const first_argument_type& lhs,
                            const first_argument_type& rhs) const
            { return C::operator()(lhs, rhs); }

            bool operator()(const data& lhs, const data& rhs) const
            { return operator()(lhs.first, rhs.first); }

            bool operator()(const data& lhs, const first_argument_type& rhs) const
            { return operator()(lhs.first, rhs); }

            bool operator()(const first_argument_type& lhs, const data& rhs) const
            { return operator()(lhs, rhs.first); }
        };
    }

    //--------------------------------------------------------------------------
    /// An associative vector built as a drop-in replacement for std::map.
    /// BEWARE: assoc_vector doesn't respect all map's guarantees, the most
    /// important being:
    /// * iterators are invalidated by insert and erase operations
    /// * the complexity of insert/erase is O(N) not O(log N)
    /// * value_type is std::pair<K, V> not std::pair<const K, V>
    /// * iterators are random
    //--------------------------------------------------------------------------
    template
    <
        class K,
        class V,
        class C = std::less<K>,
        class A = std::allocator<std::pair<K, V>>
    >
    class assoc_vector
        : private std::vector<std::pair<K, V>, A>
        , private detail::assoc_vector_compare<V, C>
    {
        using base                  = std::vector<std::pair<K, V>, A>;
        using my_compare            = detail::assoc_vector_compare<V, C>;

    public:
        using key_type              = K;
        using mapped_type           = V;
        using value_type            = typename base::value_type;

        using key_compare           = C;
        using allocator_type        = A;
        using reference             = typename A::value_type&;
        using const_reference       = const typename A::value_type&;
        using iterator              = typename base::iterator;
        using const_iterator        = typename base::const_iterator;
        using size_type             = typename base::size_type;
        using difference_type       = typename base::difference_type;
        using pointer               = typename A::value_type*;
        using const_pointer         = const typename A::value_type*;
        using reverse_iterator      = typename base::reverse_iterator;
        using const_reverse_iterator= typename base::const_reverse_iterator;

        class value_compare
            : public  std::binary_function<value_type, value_type, bool>
            , private key_compare
        {
            friend class assoc_vector;

        protected:
            value_compare(key_compare pred) : key_compare(pred)
            {}

        public:
            bool operator()(const value_type& lhs, const value_type& rhs) const
            { return key_compare::operator()(lhs.first, rhs.first); }
        };

        // 23.3.1.1 construct/copy/destroy

        explicit assoc_vector(const key_compare& comp = key_compare(), const A& alloc = A())
            : base(alloc), my_compare(comp)
        {}

        explicit assoc_vector(assoc_vector&& rhs)
            : base(std::move(static_cast<base&&>(rhs)))
            , my_compare(std::move(static_cast<my_compare&&>(rhs)))
        {}

        explicit assoc_vector(std::initializer_list<std::pair<K,V>> items,
            const key_compare& comp = key_compare(), const A& alloc = A())
            : base(items, alloc)
            , my_compare(comp)
        {}

        template <class InputIterator>
        assoc_vector(InputIterator first, InputIterator last,
            const key_compare&  comp  = key_compare(),
            const A&            alloc = A())
            : base(first, last, alloc), my_compare(comp)
        {
            std::sort(begin(), end(), static_cast<my_compare&>(*this));
        }

        assoc_vector& operator=(assoc_vector&& rhs) {
            swap(std::move(rhs));
            return *this;
        }
        assoc_vector& operator=(const assoc_vector& rhs) {
            assoc_vector(rhs).swap(*this);
            return *this;
        }

        // iterators:
        // The following are here because MWCW gets 'using' wrong
        iterator                begin()         { return base::begin();    }
        const_iterator          cbegin()  const { return base::begin();    }
        iterator                end()           { return base::end();      }
        const_iterator          cend()    const { return base::end();      }
        reverse_iterator        rbegin()        { return base::rbegin();   }
        const_reverse_iterator  crbegin() const { return base::rbegin();   }
        reverse_iterator        rend()          { return base::rend();     }
        const_reverse_iterator  crend()   const { return base::rend();     }

        // capacity:
        bool                    empty()   const { return base::empty();    }
        size_type               size()    const { return base::size();     }
        size_type               max_size()      { return base::max_size(); }

        // 23.3.1.2 element access:
        mapped_type& operator[](const key_type& key)
        { return insert(value_type(key, mapped_type())).first->second; }

        // modifiers:
        std::pair<iterator, bool> insert(const value_type& val) {
            bool found(true);
            iterator i(lower_bound(val.first));

            if (i == end() || this->operator()(val.first, i->first)) {
                i = base::insert(i, val);
                found = false;
            }
            return std::make_pair(i, !found);
        }

        iterator insert(iterator pos, const value_type& val) {
            if (pos != end() && this->operator()(*pos, val) &&
               (pos == end()-1  ||
                   (!this->operator()(val, pos[1]) &&
                     this->operator()(pos[1], val))))
                return base::insert(pos, val);
            return insert(val).first;
        }

        template <class InputIterator>
        void insert(InputIterator first, InputIterator last)
        { for (; first != last; ++first) insert(*first); }

        void erase(iterator pos)
        { base::erase(pos); }

        size_type erase(const key_type& k) {
            iterator i(find(k));
            if (i == end()) return 0;
            erase(i);
            return 1;
        }

        void erase(iterator first, iterator last)
        { base::erase(first, last); }

        void swap(assoc_vector& other) {
            using std::swap;
            base::swap(other);
            my_compare& me  = *this;
            my_compare& rhs = other;
            swap(me, rhs);
        }

        void swap(assoc_vector&& other) {
            using std::swap;
            base::swap(std::move(static_cast<base&&>(other)));
            static_cast<my_compare&>(*this) =
                std::move(static_cast<my_compare&&>(other));
        }

        void clear() { base::clear(); }

        // observers:
        key_compare key_comp() const { return *this; }

        value_compare value_comp() const {
            const  key_compare&  comp = *this;
            return value_compare(comp);
        }

        // 23.3.1.3 map operations:
        iterator find(const key_type& k) {
            iterator i(lower_bound(k));
            if (i != end() && this->operator()(k, i->first))
                i =  end();
            return i;
        }

        const_iterator find(const key_type& k) const {
            const_iterator i(lower_bound(k));
            if (i != end() && this->operator()(k, i->first))
                i =  end();
            return i;
        }

        size_type count(const key_type& k) const
        { return find(k) != end(); }

        /// Return first element that doesn't compare less than \a k.
        iterator lower_bound(const key_type& k) {
            return std::lower_bound(begin(), end(), k,
                                    static_cast<my_compare&>(*this));
        }

        /// Return first element that doesn't compare less than \a k.
        const_iterator  lower_bound(const key_type& k) const {
            return std::lower_bound(begin(), end(), k,
                                    static_cast<const my_compare&>(*this));
        }

        iterator upper_bound(const key_type& k) {
            return std::upper_bound(begin(), end(), k,
                                    static_cast<my_compare&>(*this));
        }

        const_iterator upper_bound(const key_type& k) const {
            return std::upper_bound(begin(), end(), k,
                                    static_cast<const my_compare&>(*this));
        }

        std::pair<iterator, iterator>
        equal_range(const key_type& k) {
            return std::equal_range(begin(), end(), k,
                                    static_cast<my_compare&>(*this));
        }

        std::pair<const_iterator, const_iterator>
        equal_range(const key_type& k) const {
            return std::equal_range(begin(), end(), k,
                                    static_cast<const my_compare&>(*this));
        }

        friend bool operator==(const assoc_vector& lhs, const assoc_vector& rhs) {
            return static_cast<base&>(lhs) == rhs;
        }

        bool operator<(const assoc_vector& rhs) const {
            return static_cast<const base&>(*this) < static_cast<const base&>(rhs);
        }

        friend bool operator!=(const assoc_vector& lhs, const assoc_vector& rhs)
        { return !(lhs == rhs); }

        friend bool operator> (const assoc_vector& lhs, const assoc_vector& rhs)
        { return rhs < lhs; }

        friend bool operator>=(const assoc_vector& lhs, const assoc_vector& rhs)
        { return !(lhs < rhs); }

        friend bool operator<=(const assoc_vector& lhs, const assoc_vector& rhs)
        { return !(rhs < lhs); }
    };

    // specialized algorithms:
    template <class K, class V, class C, class A>
    void swap(assoc_vector<K, V, C, A>& lhs, assoc_vector<K, V, C, A>& rhs)
    { lhs.swap(rhs); }

} // namespace utxx
