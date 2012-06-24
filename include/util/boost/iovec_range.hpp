/// This class allows to use iovec as an argument to send() functions in boost::asio

// Core taken from:
// http://sourceforge.net/tracker/?func=detail&aid=3195720&group_id=122478&atid=694040

#ifndef _UTIL_BOOST_IOVEC_RANGE_HPP_
#define _UTIL_BOOST_IOVEC_RANGE_HPP_

#include <boost/asio.hpp>

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

#if 0
int main()
{
    iovec vec[10] = { { 0, 0 } };
    iovec_range vec_range(&vec[0], &vec[10]);

    boost::asio::io_service io_service;
    boost::asio::ip::udp::socket socket(io_service, boost::asio::ip::udp::v4());
    socket.send(vec_range);
}
#endif

#endif // _UTIL_BOOST_IOVEC_RANGE_HPP_
