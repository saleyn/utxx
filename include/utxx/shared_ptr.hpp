//----------------------------------------------------------------------------
/// \file   shared_ptr.ipp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Shared pointer like std::shared_ptr() but having size of a pointer.
/// Implementation inspired by boost::intrusive_ptr().
//----------------------------------------------------------------------------
// Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-10-25
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
#pragma once

#include <utility>
#include <atomic>

namespace utxx {

// A shared_ptr containing a pointer and a reference count.
template<typename T>
struct shared_ptr
{
    shared_ptr()                        noexcept : m_data(nullptr) {}

    shared_ptr(const shared_ptr& a_rhs) noexcept : m_data(a_rhs.m_data) {
        if (m_data) m_data->inc();
    }

    shared_ptr(shared_ptr&& a_rhs)      noexcept : m_data(a_rhs.m_data) {
        a_rhs.m_data = nullptr;
    }

    ~shared_ptr() noexcept {
        if (m_data && m_data->dec())
            delete m_data;
    }

    shared_ptr& operator=(shared_ptr a_rhs) noexcept { swap(a_rhs); return *this; }

    T*   get()           const noexcept { return m_data->ptr();    }
    void reset()               noexcept { *this = shared_ptr();    }
    void reset(std::nullptr_t) noexcept { *this = shared_ptr();    }
    void reset(T&&    a_value) noexcept { *this = shared_ptr(std::forward<T>(a_value)); }

    int  use_count()     const noexcept { return m_data ? m_data->use_count() : 0; }
    void swap (shared_ptr&  a) noexcept { std::swap(m_data, a.m_data); }

    T&   operator*()     const noexcept { return m_data->value();  }
    T*   operator->()    const noexcept { return get();            }

    bool operator==(const shared_ptr& a_rhs) const noexcept { return m_data == a_rhs.m_data; }
    bool operator==(std::nullptr_t)          const noexcept { return m_data == nullptr; }
    bool operator!=(std::nullptr_t)          const noexcept { return m_data != nullptr; }
    bool operator!=(const shared_ptr& a_rhs) const noexcept { return m_data != a_rhs.m_data; }
    bool operator< (const shared_ptr& a_rhs) const noexcept { return m_data  < a_rhs.m_data; }
    bool operator<=(const shared_ptr& a_rhs) const noexcept { return m_data <= a_rhs.m_data; }
    bool operator> (const shared_ptr& a_rhs) const noexcept { return m_data  > a_rhs.m_data; }
    bool operator>=(const shared_ptr& a_rhs) const noexcept { return m_data >= a_rhs.m_data; }

    explicit operator bool()                 const noexcept { return m_data != nullptr; }

    friend inline bool operator==(std::nullptr_t, const shared_ptr<T>& a_rhs)
    { return bool(a_rhs); }
    friend inline bool operator!=(std::nullptr_t, const shared_ptr<T>& a_rhs)
    { return !bool(a_rhs); }

    friend inline void swap      (shared_ptr<T>& a_lhs, shared_ptr<T>& a_rhs)
    { a_lhs.swap(a_rhs); }

private:
    struct ptr_data {
        struct refc {
            refc() : m_rc(1) {}

            void inc() { m_rc.fetch_add(1, std::memory_order_relaxed); }
            bool dec() {
                // taken from boost intrusive_ptr:
                if (m_rc.fetch_sub(1, std::memory_order_release) == 1) {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    return true;
                }
                return false;
            }

            int use_count() const { return m_rc.load(std::memory_order_relaxed); }
        private:
            std::atomic<int> m_rc;
        };

        template<typename... Args>
        ptr_data(Args&&... args) : m_value(std::forward<Args>(args)...) {}

        void     inc()             { m_ref_cnt.inc(); }
        bool     dec()             { return m_ref_cnt.dec(); }
        T*       ptr()             { return &m_value; }
        const T* ptr()       const { return &m_value; }
        T&       value()           { return m_value;  }
        const T& value()     const { return m_value;  }
        int      use_count() const { return m_ref_cnt.use_count(); }
    private:
        refc  m_ref_cnt;
        T     m_value;
    };

    shared_ptr(ptr_data* a_data) : m_data(a_data) {}
    shared_ptr(T&&       a_data) : m_data(new ptr_data(std::move(a_data))) {}

    template<typename... Args>
    static shared_ptr construct(Args&&... args) {
        return shared_ptr(new ptr_data(std::forward<Args>(args)...));
    }

    template<typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args&&... args);

    ptr_data* m_data;
};

template<typename T, typename... Args>
inline shared_ptr<T> make_shared(Args&&... args)
{ return shared_ptr<T>::construct(std::forward<Args>(args)...); }

} // namespace utxx