// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief simple output file store for persistent trie
 *
 * \author Dmitriy Kargapolov
 * \since 17 February 2014
 *
 */

/*
 * Copyright (C) 2014 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_CONTAINER_DETAIL_FILE_STORE_HPP_
#define _UTXX_CONTAINER_DETAIL_FILE_STORE_HPP_

#include <fstream>
#include <boost/numeric/conversion/cast.hpp>

namespace utxx {
namespace container {
namespace detail {

// plain file writable data store
//
template<typename AddrType>
class file_store {
    std::ofstream m_ofs;

public:
    typedef AddrType pointer_t;
    typedef std::pair<const void *, size_t> buf_t;

    file_store(const char *a_fname) {
        m_ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        m_ofs.open(a_fname, std::ofstream::out |
            std::ofstream::binary | std::ofstream::trunc);
        char l_fake = 'F';
        m_ofs.write((const char *)&l_fake, sizeof(l_fake));
    }

    ~file_store() {
        try {
            m_ofs.close();
        } catch (...) {
        }
    }

    static pointer_t null() { return 0; }

    pointer_t store(const buf_t& b) {
        if (b.first == 0 || b.second == 0)
            return 0;
        pointer_t ret =
            boost::numeric_cast<pointer_t, std::streamoff>(m_ofs.tellp());
        m_ofs.write((const char *)b.first, b.second);
        return ret;
    }

    pointer_t store(const buf_t& b1, const buf_t& b2) {
        pointer_t ret1 = store(b1);
        pointer_t ret2 = store(b2);
        return ret1 ? ret1 : ret2;
    }

    pointer_t store(const buf_t& b1, const buf_t& b2, const buf_t& b3) {
        pointer_t ret = store(b1);
        pointer_t tmp;
        tmp = store(b2); if (!ret) ret = tmp;
        tmp = store(b3); if (!ret) ret = tmp;
        return ret;
    }

    pointer_t store(const buf_t& b1, const buf_t& b2, const buf_t& b3,
            const buf_t& b4) {
        pointer_t ret = store(b1);
        pointer_t tmp;
        tmp = store(b2); if (!ret) ret = tmp;
        tmp = store(b3); if (!ret) ret = tmp;
        tmp = store(b4); if (!ret) ret = tmp;
        return ret;
    }

    void store_at(pointer_t addr, pointer_t off, const buf_t& buff) {
        std::streamoff save = m_ofs.tellp();
        m_ofs.seekp(addr + off);
        store(buff);
        m_ofs.seekp(save);
    }
};

} // namespace detail
} // namespace container
} // namespace utxx

#endif // _UTXX_CONTAINER_DETAIL_FILE_STORE_HPP_
