//----------------------------------------------------------------------------
/// \file  async_wait_timeout.hpp
//----------------------------------------------------------------------------
/// \brief Implements an extension of boost::asio::deadline_timer with
///        async_wait_timeout function.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-31
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the util open-source library.

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

#ifndef _UTIL_ASYNC_WAIT_TIMEOUT_HPP_
#define _UTIL_ASYNC_WAIT_TIMEOUT_HPP_

#include <boost/asio.hpp>

namespace boost {

namespace asio {
namespace error {

    enum timer_errors {
        timeout = ETIMEDOUT
    };

    namespace detail {

        struct timer_category : public boost::system::error_category {
            const char* name() const { return "asio.timer"; }

            std::string message(int value) const {
                if (value == error::timeout)
                    return "Operation timed out";
                return "asio.timer error";
            }
        };

    } // namespace detail

    inline const boost::system::error_category& get_timer_category() {
        static detail::timer_category instance;
        return instance;
    }

    static const boost::system::error_category& timer_category
          = boost::asio::error::get_timer_category();

} // namespace error
} // namespace asio

namespace system {

    template<> struct is_error_code_enum<boost::asio::error::timer_errors> {
        static const bool value = true;
    };

    inline boost::system::error_code make_error_code(boost::asio::error::timer_errors e) {
        return boost::system::error_code(
            static_cast<int>(e), boost::asio::error::get_timer_category());
    }

} // namespace system

namespace asio {

    /// Derives from boost::asio::deadline_timer by adding an
    /// async_wait_timeout() function that will call a handler after a given
    /// number of milliseconds.
    class deadline_timer_ex : public basic_deadline_timer<boost::posix_time::ptime>
    {
        typedef basic_deadline_timer<boost::posix_time::ptime> super;

        template <typename Handler>
        struct async_wait_handler_wrapper {
            async_wait_handler_wrapper(Handler h): m_h(h) {}

            void operator() (const system::error_code& ec) {
                system::error_code e =
                    ec == system::error_code()
                        ? system::make_error_code(error::timeout)
                        : ec;
                m_h(e);
            }
            Handler m_h;
        };

    public:
        deadline_timer_ex(boost::asio::io_service& a_svc)
            : basic_deadline_timer<boost::posix_time::ptime>(a_svc)
        {}

        /// Asynchronous wait without expiration. The expiration time
        /// is set to <tt>boost::posix_time::pos_infin</tt>.
        template<typename Handler>
        void async_wait(Handler h)
        {
            expires_at(boost::posix_time::pos_infin);
            //expires_at(boost::posix_time::not_a_date_time);
            async_wait_handler_wrapper<Handler> wrapper(h);
            super::async_wait(wrapper);
        }

        /// Returns <tt>boost::asio::error::operation_aborted</tt> on cancel,
        /// and <tt>boost::asio::error::timeout</tt> on timeout.
        template<typename Handler>
        void async_wait_timeout(Handler h, long millisecs)
        {
            expires_from_now( boost::posix_time::milliseconds(millisecs) );
            async_wait_handler_wrapper<Handler> wrapper(h);
            super::async_wait(wrapper);
        }
    };

    } // namespace asio

} // namespace boost

#endif // _UTIL_ASYNC_WAIT_TIMEOUT_HPP_
