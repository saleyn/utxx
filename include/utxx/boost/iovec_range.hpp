//----------------------------------------------------------------------------
/// \file   iovec_range.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Helper functions to use iovec as an argument to send() functions
///        in boost::asio.
///
/// Core taken from:
/// http://sourceforge.net/tracker/?func=detail&aid=3195720&group_id=122478&atid=694040
//----------------------------------------------------------------------------
// Created: 2009-11-21
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

#ifndef _UTXX_BOOST_IOVEC_RANGE_HPP_
#define _UTXX_BOOST_IOVEC_RANGE_HPP_

#include <boost/asio.hpp>

namespace boost {
namespace asio {

class iovec_range {
    struct to_buffer {
        typedef boost::asio::mutable_buffer result_type;
        result_type operator()(iovec i) const { return result_type(i.iov_base, i.iov_len); }
    };

    const iovec *m_begin, *m_end;
public:
    typedef boost::asio::mutable_buffer value_type;
    typedef boost::transform_iterator<to_buffer, const iovec*> const_iterator;

    iovec_range(const iovec* begin, const iovec* end) : m_begin(begin), m_end(end) {}
    const_iterator begin() const { return const_iterator(m_begin); }
    const_iterator end() const { return const_iterator(m_end); }

};

} // namespace asio
} // namespace boost

#if 0
using boost::asio::iovec_range;

int main()
{
    iovec vec[10] = { { 0, 0 } };
    iovec_range vec_range(&vec[0], &vec[10]);

    boost::asio::io_service io_service;
    boost::asio::ip::udp::socket socket(io_service, boost::asio::ip::udp::v4());
    socket.send(vec_range);
}
#endif

#endif // _UTXX_BOOST_IOVEC_RANGE_HPP_
