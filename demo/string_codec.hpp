// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief string serialization demo
 *
 * \author Dmitriy Kargapolov
 * \since 08 December 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _STRING_CODEC_HPP_
#define _STRING_CODEC_HPP_

#include <boost/numeric/conversion/cast.hpp>

namespace {

// this class combines stuff related to external string representation
//
template<typename AddrType>
struct codec_impl {

    // external string representation - offset in file pointing to actual string
    // used by readers based on utxx::container::mmap_ptrie
    class data {
        AddrType ptr;

    public:
        bool empty() const { return ptr == 0; }

        template<typename Store>
        const char *str(const Store& store) const {
            if (empty())
                return "";
            const char *s = store.template native_pointer<const char>(ptr);
            if (s == NULL)
                throw std::runtime_error("bad store pointer");
            return s;
        }
    };

    // data writer - used by utxx::container::ptrie writer
    struct writer {
        typedef std::pair<const void *, size_t> buf_t;

        // encoder always initialized with parent state
        template<typename T> writer(T&) {}

        template<typename StoreIn, typename StoreOut>
        void store(const std::string& str, const StoreIn&, StoreOut& out) {
            size_t n = str.size();
            addr = n > 0 ? out.store(buf_t(str.c_str(), n + 1)) : out.null();
            buf.first = &addr;
            buf.second = sizeof(addr);
        }

        const buf_t& buff() const { return buf; }

    private:
        AddrType addr;
        buf_t buf;
    };

};

}

// public codec interface
// opaque data_type is used in mmap-ed trie instantiation
// encoder must expose method: void store(Data, Store, Out)
// (Out must have store(Buff) and AddrType null() methods)
//
struct string_codec {
    template<typename AddrType>
    struct bind {
        typedef typename codec_impl<AddrType>::data data_type;
        typedef typename codec_impl<AddrType>::writer encoder;
    };
};

#endif // _STRING_CODEC_HPP_
