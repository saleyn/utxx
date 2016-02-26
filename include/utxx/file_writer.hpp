//----------------------------------------------------------------------------
/// \file   file_writer.hpp
/// \author Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
//----------------------------------------------------------------------------
/// \brief Basic UDP packet receiver.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Omnibius, LLC
// Created: 2012-08-12
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2012 Omnibius, LLC
Author: Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>

Use, modification and distribution are subject to the Boost Software
License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
at http://www.boost.org/LICENSE_1_0.txt)

***** END LICENSE BLOCK *****
*/
#ifndef _UTXX_FILE_WRITER_HPP_
#define _UTXX_FILE_WRITER_HPP_

#include <iostream>
#include <fstream>
#include <boost/noncopyable.hpp>
#include <utxx/buffer.hpp>

namespace utxx {

namespace detail {

/**
 * \brief Basic file writer with pre-allocated buffer.
 */
template<size_t BufSize = 1024 * 1024>
class basic_file_writer : private boost::noncopyable {
    std::string m_fname;
    std::ofstream m_file;
    utxx::basic_io_buffer<BufSize> m_buf;
    size_t m_offset;
    bool m_open;

protected:
    size_t offset() const { return m_offset; }

    size_t capacity() const { return m_buf.capacity(); }

    char* wr_ptr() { return m_buf.wr_ptr(); }

    void commit(size_t n) { m_buf.commit(n); }

public:
    basic_file_writer() : m_offset(0), m_open(false) {}

    basic_file_writer(const std::string& a_fname, bool a_append = false)
        : m_offset(0), m_open(false)
    {
        open(a_fname, a_append);
    }

    ~basic_file_writer() {
        if (!m_open) return;
        try {
            m_file.close();
        } catch (...) {
        }
    }

    void open(const std::string& a_fname, bool a_append = false) {
        using std::ios;
        using std::ios_base;
        if (m_open) return;
        m_file.exceptions(ios::failbit | ios::badbit);
        ios_base::openmode l_mode = ios::out | ios::binary;
        if (a_append)
            l_mode |= ios::app;
        else
            l_mode |= ios::trunc;
        m_file.open(a_fname.c_str(), l_mode);
        m_offset = m_file.tellp();
        m_open = true;
        m_fname = a_fname;
        m_buf.reset();
        BOOST_ASSERT(m_buf.capacity() > 0);
    }

    void flush() {
        if (!m_open) return;
        size_t n;
        if ((n = m_buf.size()) > 0) {
            m_file.write(m_buf.read(n), n);
            m_buf.crunch();
        }
    }

    void sync() {
        if (!m_open) return;
        m_file.flush();
    }
};

} // namespace detail

/**
 * \brief File writer with payload codec
 */
template<typename Codec, size_t BufSize = 1024 * 1024>
class data_file_writer : public detail::basic_file_writer<BufSize> {

    typedef Codec codec_t;
    typedef typename codec_t::data_t data_t;
    typedef detail::basic_file_writer<BufSize> base;

    codec_t m_codec;
    size_t  m_data_offset;

    bool try_write(const data_t& a_data) {
        size_t n;
        if ((n = m_codec(a_data, base::wr_ptr(), base::capacity())) > 0) {
            base::commit(n);
            m_data_offset += n;
            return true;
        }
        return false;
    }

public:
    /// create file writer with specific or default codec object
    data_file_writer(const codec_t& a_codec = codec_t())
        : m_codec(a_codec)
        , m_data_offset(base::offset())
    {}

    /// create writer object and open file for writing
    data_file_writer(const std::string& a_fname, bool a_append = false,
                     const codec_t& a_codec = codec_t())
        : base(a_fname, a_append)
        , m_codec(a_codec)
        , m_data_offset(base::offset())
    {}

    ~data_file_writer() {
        try {
            base::flush();
        } catch (...) {
        }
    }

    /// write record at current offset
    void push_back(const data_t& a_data) {
        if (try_write(a_data)) return;
        base::flush();
        if (try_write(a_data)) return;
        throw runtime_error("encode error", "short buffer: ", base::capacity());
    }

    /// offset for the next record to encode
    size_t data_offset() const { return m_data_offset; }
};

} // namespace utxx

#endif // _UTXX_FILE_WRITER_HPP_
