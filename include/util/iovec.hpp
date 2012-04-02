/*
 * @file
 * @author Serge Aleynikov
 * Since 24 September 2011
 *
 * BEGIN LICENSE
 * END LICENSE
 */

#ifndef _UTIL_IOVEC_HPP_
#define _UTIL_IOVEC_HPP_

#include <sys/uio.h>
#include <string.h>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>

namespace util {

/// @return length of data stored in the \a a iovec array of size \a n.
inline size_t length(const iovec* a, size_t n) {
    size_t m = 0;
    for (const iovec* e = a + n; a < e; a++)
        m += a->iov_len;
    return m;
}

template <int N>
inline size_t length(const iovec (&a)[N]) { return length(a, N); }

template<int N>
class basic_iovector {
    iovec  m_data[N];
    size_t m_length;
    iovec* m_begin;
    iovec* m_end;

public:
    basic_iovector() : m_length(0), m_begin(m_data), m_end(m_begin)
    {}

    template<int M>
    basic_iovector(const iovec(&a_data)[M])
        : m_begin(m_data), m_end(m_data + M), m_length(0)
    {
        BOOST_STATIC_ASSERT(M <= N);
        for (int i = 0; i < M; m_length += a_data[i].iov_len, i++)
            m_data[i] = a_data[i];
    }

    basic_iovector(const iovec *a_data, size_t a_size)
        : m_begin(m_data), m_end(m_data + a_size), m_length(0)
    {
        BOOST_ASSERT(a_size <= N);
        for (int i = 0; i < a_size; m_length += a_data[i++].iov_len)
            m_data[i] = a_data[i];
    }

    const iovec & operator[](int i) const {
        BOOST_ASSERT (m_begin + i < m_end);
        return *(m_begin + i);
    }

    /// Push \a a_bytes to the end of the vector array
    void push_back(const char *a_bytes, size_t a_size) {
        BOOST_ASSERT(m_end < m_data + N);
        m_end->iov_len = a_size;
        m_end->iov_base = (void *) a_bytes;
        m_length += a_size;
        m_end++;
    }

    /// @return pointer to beginning of data
    const iovec*    data()   const { return m_begin; }

    /// Number of items in the vector
    size_t          size()   const { return m_end - m_begin; }
    /// Total data length
    size_t          length() const { return m_length; }
    /// @return true if the vector is empty
    bool            empty()  const { return m_begin == m_end; }


    /// Erase \a a_bytes from the beginning of the vector
    void erase(int a_bytes) {
        BOOST_ASSERT(a_bytes <= m_length);
        for (iovec *p = m_begin; p < m_end && a_bytes >= p->iov_len;
                m_length -= p->iov_len, a_bytes -= p->iov_len,
                        m_begin = ++p) {}
        if (a_bytes) {
            m_begin->iov_len  -= a_bytes;
            m_begin->iov_base += a_bytes;
        }
    }

    /// Reset vector to zero length
    void reset() {
        m_begin = m_end = m_data;
        m_length = 0;
    }

    /// Copy data from the vector to given buffer
    /// @return -1 if a_size is insufficient to copy data to, or else the
    ///            length of copied data.
    int copy_to(char *a_buf, size_t a_size) const {
        const char *begin = a_buf;
        if (a_size < m_length)
            return -1;
        for (const iovec *p = m_begin; p < m_end; a_buf += p->iov_len, p++)
            memcpy(a_buf, p->iov_base, p->iov_len);
        BOOST_ASSERT(a_buf - begin == m_length);
        return m_length;
    }
};

} // namespace util

#endif // _UTIL_IOVEC_HPP_
