// vim:ts=4:sw=4:et
//----------------------------------------------------------------------------
/// \file   md5.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief MD5 encoding function
//----------------------------------------------------------------------------
// Copyright (c) 2020 Serge Aleynikov <saleyn@gmail.com>
// Created: 2020-12-10
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
#pragma once

#if BOOST_VERSION >= 106600
#  include <boost/uuid/detail/sha1.hpp>
#  include <boost/uuid/detail/md5.hpp>
#else
#  include <boost/uuid/sha1.hpp>
#endif
#include <utxx/string.hpp>
#include <utxx/endian.hpp>

namespace utxx {

#if BOOST_VERSION >= 106600

/// Calculates MD5 hash of the passed string.
std::string md5(const std::string& str, bool a_lower=true)
{
    using boost::uuids::detail::md5;

    md5 hash;
    md5::digest_type digest;

    hash.process_bytes(str.data(), str.size());
    hash.get_digest(digest);

    char     ccdigest[sizeof(md5::digest_type)];
    uint8_t* pc = reinterpret_cast<uint8_t*>(ccdigest);

    for (unsigned i=0; i < std::extent<md5::digest_type>::value; i++)
        put_be(pc, digest[i]);

    std::string res;
    res = hex((const char*)ccdigest, sizeof(md5::digest_type), a_lower);
    return res;
}

#endif

/// Calculates SHA1 hash of the passed string.
std::string sha1(const std::string& str, bool a_lower=true)
{
    using boost::uuids::detail::sha1;

    sha1 hash;
#if BOOST_VERSION >= 106600
    sha1::digest_type digest;
#else
    unsigned int buf[5];
    sha1::digest_type digest = buf;
#endif

    hash.process_bytes(str.data(), str.size());
    hash.get_digest(digest);

    char  ccdigest[sizeof(digest)];
    uint8_t* pc = reinterpret_cast<uint8_t*>(ccdigest);

    for (unsigned i=0; i < std::extent<decltype(digest)>::value; i++)
        put_be(pc, digest[i]);

    std::string res;
    res = hex((const char*)ccdigest, sizeof(digest), a_lower);
    return res;
}

} // namespace utxx
