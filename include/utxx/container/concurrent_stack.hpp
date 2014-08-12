//----------------------------------------------------------------------------
/// \file   concurrent_stack.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Concurrent lock-free stack.
/// 
/// Note: this implementation has ABA problem.
//----------------------------------------------------------------------------
// Created: 2010-01-06
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _UTXX_CONCURRENT_STACK_HPP_
#define _UTXX_CONCURRENT_STACK_HPP_

#include <boost/type_traits.hpp>
#include <utxx/meta.hpp>
#include <utxx/atomic.hpp>
#include <utxx/synch.hpp>
#include <time.h>

namespace utxx {
namespace container {

//-----------------------------------------------------------------------------
// VERSIONED STACK
//-----------------------------------------------------------------------------

/// @class container::versioned_stack
/// Versioned stack is a concurrent structure that can be used for lock-free
/// temporary storage of data. Each item pushed to stack has to be inherited
/// from the <versioned_stack::node_t> type. The stack doesn't own data (no 
/// copying is performed on items pushed to the stack). A node managed by
/// the stack must be allocated/deallocated externally in the following form
/// (where N = sizeof(long)):
/// <code>
///            0         N-1     2*N-1         2*N+(1<<SizeClass)-1
///     Node:  +-----------+---------+--------------+
///            | SizeClass | NextPtr | ... Data ... |
///            +-----------+---------+--------------+
///            \-----------+--------/
///                        V
///                 sizeof(node_t)
/// </code>
/// The SizeClass is used to store a magic number and optional size 
/// information about the node's data.  NextPtr is used by the stack to 
/// refer to the next item in the list.
/// For example usage of this stack see <alloc_cached.cpp>, which holds
/// size-class-specific free lists represented by versioned_stack.
/// Data pushed to the stack must be aligned on the 4-byte boudary since
/// the stack uses version masking of lower three bits of the head pointer
/// for ABA problem prevention.
class versioned_stack {
public:
    class node_t {
        const unsigned int m_size_class;

        // Magic value s_magic is used to check validity of managed pointers.
        // Version value s_version_mask is used to prevent the ABA problem.
        static const unsigned int  s_magic_mask   = 0xFFFFFF00;
        static const unsigned int  s_magic_unmask = 0x000000FF;
    public:
        static const unsigned int  s_magic        = 0xFEDCBA00;
        static const unsigned long s_version_mask = 0x7;

        node_t*      next;

        void*        data()             { return static_cast<void*>(this + 1); }
        unsigned int size_class() const { return s_magic_unmask & m_size_class; }
        unsigned int magic()      const { return s_magic_mask   & m_size_class; }
        bool         valid()      const { return magic() == s_magic; }

        static unsigned int size_class(unsigned int n) {
            return s_magic | (s_magic_unmask & n);
        }

        static node_t* to_node(void* p) { return reinterpret_cast<node_t*>(p)-1; }
        static node_t* no_version(node_t* p) {
            return reinterpret_cast<node_t*>(
                reinterpret_cast<unsigned long>(p) & ~node_t::s_version_mask);
        }
        static node_t* inc_version(node_t* p, node_t* versioned) { 
            unsigned long version = 
                reinterpret_cast<unsigned long>(versioned) & s_version_mask;
            return reinterpret_cast<node_t*>(
                ((version + 1) & node_t::s_version_mask) |
                (reinterpret_cast<unsigned long>(p) & ~node_t::s_version_mask));
        }

        node_t() : m_size_class(s_magic), next(NULL) 
        {}
        node_t(size_t a_size_class, node_t* a_next = NULL)
            : m_size_class(size_class(a_size_class)), next(a_next) 
        {}
    };

    versioned_stack() : m_head(NULL) {}

    static size_t header_size() { return sizeof(node_t); }

    /// Push a node <nd> to stack.
    void push(node_t* nd) {
        node_t *curr, *new_head;
        do {
            curr     = m_head;
            nd->next = node_t::no_version(curr);
            new_head = node_t::inc_version(nd, curr);
        } while (!atomic::cas(&m_head, curr, new_head));
    }

    /// Pop a node from stack in the LIFO order.
    node_t* pop() { return pop(false); }

    /// Reset replaces the head pointer with NULL and returns 
    /// the old content of the head optionally reversing
    /// the chain of nodes from LIFO to FIFO order.
    /// @param <reverse> when true, the old content of stack
    ///        is reversed prior to returning.
    /// @return old content of the stack (optionally reversed)
    node_t* reset(bool reverse = false) {
        node_t *old_head, *new_head, *curr = pop(true);

        if (reverse) {
            for (old_head = NULL; curr; curr = new_head) {
                new_head   = curr->next;
                curr->next = old_head;
                old_head   = curr;
            }
            curr = old_head;
        }
        return curr;
    }

    /// @return true if stack is empty
    bool empty() const { return node_t::no_version(m_head) == NULL; }

    /// Returns number of items in the stack. O(n) complexity.
    /// Use only for debugging.
    int unsafe_size() const {
        int len = 0;
        for (node_t* tmp = node_t::no_version(m_head); tmp; ++len, tmp = tmp->next);
        return len;
    }

protected:
    node_t* m_head;

    node_t* pop(bool empty_head) {
        node_t *old_head, *curr, *new_head;
        do {
            old_head = m_head;
            curr     = node_t::no_version(old_head);
            if (curr == NULL) return NULL;
            new_head = node_t::inc_version(empty_head ? NULL : curr->next, old_head);
        } while (!atomic::cas(&m_head, old_head, new_head));

        if (!empty_head)
            curr->next = NULL;
        return curr;
    }
};

//-----------------------------------------------------------------------------
// VIRSIONED STACK
//-----------------------------------------------------------------------------

template <typename EventT = futex>
class blocking_versioned_stack: public versioned_stack {
    EventT not_empty_condition;
    //char __pad[CACHELINE_SIZE - sizeof(versioned_stack) - sizeof(EventT)];
public:
    typedef versioned_stack::node_t node_t;

    blocking_versioned_stack() : not_empty_condition(true) {}
    ~blocking_versioned_stack() {
        signal();
    }

    /// Push an item to stack
    void push(node_t* nd) {
        versioned_stack::push(nd);
        not_empty_condition.signal();
    }

    /// Pop an item from stack
    /// @return NULL if stack is empty or a node pointer otherwise.
    node_t* try_pop() {
        return versioned_stack::pop();
    }

    /// Pop an item waiting for up to <timeout> in case the stack is empty.
    /// @param <timeout>  - max time to wait (NULL <=> infinity)
    node_t* pop(struct timespec* timeout) {
        int sync_val = not_empty_condition.value();
        node_t* p = try_pop();
        return p ? p 
                 : not_empty_condition.wait(timeout, &sync_val) == 0
                     ? try_pop() : NULL;
    }

    /// Reset the stack's head pointer by replacing it with NULL and 
    /// returning stacked chain of nodes optionally reversed in order.
    /// @param <timeout>  - max time to wait (NULL <=> infinity)
    node_t* try_reset(bool reverse = false) {
        return versioned_stack::reset(reverse);
    }

    /// Reset the stack's head pointer by replacing it with NULL and 
    /// returning stacked chain of nodes optionally reversed in order.
    //  The call will block for up to <timeout> in case the stack is empty.
    //  If <timeout> is NULL blocking is infinite until a node is pushed to the stack.
    /// @param <timeout>  - max time to wait (NULL <=> infinity)
    node_t* reset(struct timespec* timeout, bool reverse = false) {
        int sync_val = not_empty_condition.value();
        node_t* p = try_reset(reverse);
        return p
             ? p
             : not_empty_condition.wait(timeout, &sync_val) == wakeup_result::SIGNALED
             ? try_reset(reverse) : NULL;
    }

    /// Signals all waiting threads calling pop() or reset() forcing them to unblock.
    void signal() {
        not_empty_condition.signal_all();
    }
};

} // namespace container
} // namespace utxx

#endif  // _UTXX_CONCURRENT_STACK_HPP_
