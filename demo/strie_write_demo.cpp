// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief strie write-to-file demo
 *
 * \author Dmitriy Kargapolov
 * \since 14 November 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <utxx/simple_node_store.hpp>
#include <utxx/svector.hpp>
#include <utxx/pnode.hpp>
#include <utxx/ptrie.hpp>

// offset type in external data representation
typedef uint32_t offset_t;

// external string representation will be an offset in file
// referencing actual null-terminated string
//
struct exportable_string : public std::string {
    exportable_string() {}
    exportable_string(const char *s) : std::string(s) {}

    // data header of external string representation
    template<typename OffsetType> struct ext_header {
        // offset to variable-length payload
        OffsetType offset;
        // write data header to file
        void write_to_file(std::ofstream& a_ofs) const {
            a_ofs.write((const char *) &offset, sizeof(offset));
        }
    };

    // write nested data payload to file, fill header
    template<typename OffsetType, typename Store>
    void write_to_file(ext_header<OffsetType>& hdr, const Store&,
            std::ofstream& f) const {
        size_t n = size();
        if (n > 0) {
            hdr.offset =
                boost::numeric_cast<OffsetType, std::streamoff>(f.tellp());
            f.write((const char *) c_str(), n + 1);
        } else
            hdr.offset = 0;
    }
};

// payload type
typedef exportable_string data_t;

// trie node type
typedef utxx::pnode<utxx::simple_node_store<>, data_t, utxx::svector<> > node_t;

// trie type
typedef utxx::ptrie<node_t> trie_t;

int main() {
    trie_t trie;

    // store some data
    trie.store("123", "three");
    trie.store("1234", "four");
    trie.store("12345", "five");

    // write (export) trie to the file in external format
    trie.write_to_file<offset_t>("trie.bin");

    return 0;
}
