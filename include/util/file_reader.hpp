/**
 * \file
 * \brief file reader
 *
 * \author Dmitriy Kargapolov <dmitriy dot kargapolov at gmail dot com>
 * \since 27 Jul 2012
 *
 * Copyright (C) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Copyright (C) 2012 Serge Aleynikov <saleyn@gmail.com>
 *
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTIL_FILE_READER_HPP_
#define _UTIL_FILE_READER_HPP_

#include <iostream>
#include <fstream>
#include <boost/noncopyable.hpp>
#include <util/buffer.hpp>

namespace util {

namespace detail {

/**
 * \brief Basic file reader with pre-allocated buffer.
 */
template<size_t BufSize = 1024 * 1024>
class basic_file_reader : private boost::noncopyable {
    std::string m_fname;
    std::ifstream m_file;
    util::basic_io_buffer<BufSize> m_buf;
    size_t m_offset;
    bool m_open;

public:
    /// default constructor
    basic_file_reader()
        : m_offset(0), m_open(false)
    {}

    /// constructor opening file for reading
    basic_file_reader(const std::string& a_fname)
        : m_offset(0), m_open(false)
    {
        open(a_fname);
    }

    /// destructor
    ~basic_file_reader() {
        if (!m_open) return;
        try {
            m_file.close();
        } catch (...) {
        }
    }

    /// underlying filename
    const std::string& filename() const { return m_fname; }

    /// open file for reading
    void open(const std::string& a_fname) {
        if (m_open) return;
        // make sure exception thrown in case of error
        m_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        m_file.open(a_fname.c_str(), std::ios::in | std::ios::binary);
        // further read can set failbit in case there is not enough data
        // this case should not throw an exception in our class
        m_file.exceptions(std::ifstream::badbit);
        m_open = true;
        m_fname = a_fname;
        m_buf.reset();
        BOOST_ASSERT(m_buf.capacity() > 0);
    }

    /// set initial read position
    void seek(size_t a_offset) {
        if (!m_open) return;
        m_file.seekg(a_offset, std::ios::beg);
        m_offset = m_file.tellg();
    }

    /// clear error control state so read could be resumed
    void clear() { if (m_open) m_file.clear(); }

    /// offset at which read start
    size_t offset() const { return m_offset; }

    /// current number of bytes available to read.
    size_t size() const { return m_buf.size(); }

    /// current read pointer.
    const char* rd_ptr() const { return m_buf.rd_ptr(); }

    /// confirm consuming of n bytes
    void commit(size_t n) { m_buf.read(n); }

    /// read portion of file into internal buffer
    /// if a_crunch == true, crunch buffer before reading
    bool read(bool a_crunch = true) {
        if (!m_open || !m_file.is_open())
            return false;
        if (a_crunch)
            m_buf.crunch();
        while (true) {
            BOOST_ASSERT(m_buf.capacity() > 0);
            m_file.read(m_buf.wr_ptr(), m_buf.capacity());
            int n = m_file.gcount();
            if (n == 0) {
                if (m_file.good()) continue;
                if (m_file.eof()) return false;
                // this should never happen since we have set badbit
                throw io_error(errno, "Unexpected error reading ", m_fname);
            }
            m_buf.commit(n);
            return true;
        }
    }
};

} // namespace detail

/**
 * \brief File reader with data payload codec and input iterator.
 */
template<typename Codec, size_t BufSize = 1024 * 1024>
class data_file_reader : public detail::basic_file_reader<BufSize> {

    typedef Codec codec_t;
    typedef typename codec_t::data_t data_t;
    typedef detail::basic_file_reader<BufSize> base;

    codec_t m_codec;
    size_t m_data_offset;
    data_t m_data;
    bool m_empty;  // no data in m_data
    bool m_end;    // eof reached

    friend class iterator;

public:
    /// create file reader with specific or default codec object
    data_file_reader(const codec_t& a_codec = codec_t())
        : m_codec(a_codec)
        , m_data_offset(base::offset())
        , m_empty(true)
        , m_end(false)
    {}

    /// create reader object and open file for reading
    data_file_reader(const std::string& a_fname,
                     const codec_t& a_codec = codec_t())
        : base(a_fname), m_codec(a_codec)
        , m_data_offset(base::offset())
        , m_empty(true)
        , m_end(false)
    {}

    /// create reader object and open file for reading at given offset
    data_file_reader(const std::string& a_fname, size_t a_offset,
                     const codec_t& a_codec = codec_t())
        : base(a_fname), m_codec(a_codec)
        , m_data_offset(base::offset())
        , m_empty(true)
        , m_end(false)
    {
        seek(a_offset);
    }

    /// set initial read position
    void seek(size_t a_offset) {
        base::seek(a_offset);
        m_data_offset = base::offset();
    }

    /// offset for the next record to decode
    size_t data_offset() const { return m_data_offset; }

    /// clear error control state so read could be resumed
    void clear() {
        base::clear();
        m_end = false;
    }

    void read_data() {
        while (!m_end) {
            ssize_t n = m_codec.decode(m_data,
                base::rd_ptr(), base::size(), m_data_offset);
            if (n > 0) {
                m_data_offset += n;
                base::commit(n);
                m_empty = false;
                break;
            }
            if (n < 0)
                throw io_error("decode error", n, " at ", m_data_offset,
                        " when reading ", base::filename());
            // n == 0: not enough data in buffer
            if (!base::read()) {
                m_end = true;
                m_empty = true;
                break;
            }
        }
    }

    /// iterator for reading sequence of records
    class iterator : public std::iterator<std::input_iterator_tag, data_t> {

        data_file_reader& m_freader;
        bool m_end;

    public:
        iterator(data_file_reader& a_freader, bool a_end = false)
            : m_freader(a_freader), m_end(a_end)
        {
            if (m_end) return;
            if (m_freader.m_empty) {
                m_freader.clear();
                m_freader.read_data();
                m_end |= m_freader.m_end;
            }
        }

        iterator& operator++() {
            m_freader.read_data();
            m_end |= m_freader.m_end;
            return *this;
        }

        bool operator==(const iterator& rhs) const {
            return (m_end == true && m_end == rhs.m_end)
                || (m_end == false && m_end == rhs.m_end &&
                    m_freader.data_offset() == rhs.m_freader.data_offset());
        }

        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }

        data_t& operator*() { return m_freader.m_data; }
        const data_t& operator*() const { return m_freader.m_data; }
        data_t* operator->() { return &m_freader.m_data; }
        const data_t* operator->() const { return &m_freader.m_data; }

    };

    typedef const iterator const_iterator;

    /// get input iterator at current read position
    iterator begin() { return iterator(*this); }
    const_iterator begin() const { return iterator(*this); }

    /// end-of-read position iterator
    iterator end() { return iterator(*this, true); }
    const_iterator end() const { return iterator(*this, true); }
};

} // namespace util

#endif // _UTIL_FILE_READER_HPP_
