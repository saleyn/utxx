#ifndef _UTIL_CONCURRENT_PRI_QUEUE_HPP_
#define _UTIL_CONCURRENT_PRI_QUEUE_HPP_

#include <boost/cstdint.hpp>
#include <util/bitmap.hpp>

template <typename T, int Priorities, typename Queue>
class concurrent_priority_queue {
    union versioned_bitmask {
        uint64_t index;
        struct {
            bitmap_low<Priorities> bitmask : Priorities;
            uint32_t version               : 64 - Priorities;
        } s;
    };

    volatile versioned_bitmask m_idx;
    char __pad[64 - sizeof(versioned_bitmask)];
    Queue m_queues[N];

    static const int s_max_version = 64 - Priorities - 1;

    static uint64_t inc_version(uint64_t value) {
        versioned_bitmask u.index = value;
        u.s.version = (u.s.version+1) & s_max_version;
        return u.index;
    }
public:
    static const int max_priority = Priorities - 1;

    concurrent_priority_queue() {
        STATIC_ASSERT(0 < Priorities && Priorities <= 56, Invalid_priority_bound);
    }

    bool get(T& item) {
        versioned_bitmask old = m_idx;
        int pri = old.s.bitmask.first();
        if (pri == old.s.bitmask.end())
            return false;
        if (m_queues[pri].get(item)) {
            while(1) {
                if (!m_queues[pri].empty())
                    break;
                old.s.bitmask.clear(pri);
                if (atomic::cas(m_idx.index, old.index, inc_version(old.index)))
                    break;
                old = m_idx;
            } 
            return true;
        }
        return false;
    }

    bool put(int priority, T& item) {
        versioned_bitmask old = m_idx;
        if (!m_queues[priority].put(item))
            return false;
        while(1) {
            old.s.bitmask.set(priority);
            if (atomic::cas(m_idx.index, old.index, inc_version(old.index)))
                break;
            old = m_idx;
        }
        return true;
    }
};

#endif // _UTIL_CONCURRENT_PRI_QUEUE_HPP_
