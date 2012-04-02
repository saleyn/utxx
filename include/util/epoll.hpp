/*
 * Copyright (c) 2010 Peter Simons <simons@cryp.to>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTIL_EPOLL_HPP_
#define _UTIL_EPOLL_HPP_

#include <sys/socket.hpp>
#include <util/signal.hpp>
#include <util/string.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple.hpp>
#include <algorithm>
#include <limits>
#include <iosfwd>
#include <sys/epoll.h>

namespace util {
namespace detail {

typedef size_t milliseconds_t;
typedef int native_socket_t;

/**
 * \brief I/O reactor implementation based on \c epoll(7).
 *
 * \sa http://www.kernel.org/doc/man-pages/online/pages/man7/epoll.7.html
 */
class epoll : private boost::noncopyable {
    native_socket_t                     m_epoll_fd;
    std::vector<epoll_event>            m_events;
    std::vector<epoll_event>::iterator  m_current;
    size_t                              m_nevents;
public:
    class socket {
        native_socket_t m_sock_handle;
        epoll&          m_epoll;
    protected:
        epoll& reactor() { return m_epoll; }
    public:
        enum event_set {
              no_events = 0
            , readable  = EPOLLIN
            , writable  = EPOLLOUT
            , pridata   = EPOLLPRI
            , epollet   = EPOLLET
        };

        socket(epoll& a_reactor, native_socket_t a_sock, event_set a_ev = no_events)
            : m_sock_handle(a_sock), m_epoll(a_reactor)
        {
            BOOST_ASSERT(a_sock >= 0);
            epoll_event e;
            e.data.fd = sock_handle();
            e.events  = a_ev;
            if (!epoll_ctl(m_epoll.m_epoll_fd, epoll_ctl(m_epoll.epoll_fd, EPOLL_CTL_ADD, sock_handle(), &e)))
                throw sys_error(errno, "Add socket into epoll");
        }

        ~socket()
        {
            epoll_event e;
            e.data.fd = sock_handle();
            e.events  = 0;
            if (!epoll_ctl(m_epoll.m_epoll_fd, epoll_ctl(m_epoll.epoll_fd, EPOLL_CTL_DEL, sock_handle(), &e)))
                throw sys_error(errno, "Remove socket from epoll");
        }

        int sock_handle() const { return m_sock_handle; }

        void update(event_set ev) {
            epoll_event e;
            e.data.fd = sock_handle();
            e.events  = ev;
            if (!epoll_ctl(m_epoll.m_epoll_fd, epoll_ctl(m_epoll.epoll_fd, EPOLL_CTL_MOD, sock_handle(), &e)))
                throw sys_error(errno, "Modify epoll socket");
        }

        friend inline event_set & operator|= (event_set& lhs, event_set rhs) {
            return lhs = (event_set)((int)(lhs) | (int)(rhs));
        }
        friend inline event_set&  operator&= (event_set& lhs, event_set rhs) {
            return lhs = (event_set)((int)(lhs) & (int)(rhs));
        }
        friend inline event_set   operator|  (event_set lhs, event_set rhs) {
            return lhs |= rhs;
        }
        friend inline event_set   operator&  (event_set lhs, event_set rhs) {
            return lhs &= rhs;
        }
        friend inline std::ostream & operator<< (std::ostream& os, event_set ev) {
            std::string s;
            os << "events(";
            if (ev == no_events) os << "none";
            if (ev & readable)   s += "|read";
            if (ev & writable)   s += "|write";
            if (ev & pridata)    s += "|pridata";
            return os << (s.size() ? s.c_str()+1 : "") << ')';
        }
    };

    static seconds_t max_timeout() {
        return static_cast<seconds_t>(std::numeric_limits<int>::max() / 1000);
    }

    explicit epoll(size_t a_size_hint = 128u) : m_nevents(0u) {
        a_size_hint = std::min(a_size_hint, static_cast<size_t>(std::numeric_limits<int>::max()));
        m_events.reserve(a_size_hint);
        m_current = m_events.end();
        m_epoll_fd = ::epoll_create(static_cast<int>(a_size_hint));
    }

    ~epoll() {
        ::close(m_epoll_fd);
    }

    bool empty() const { return m_nevents == 0u; }

    boost::tuple<native_socket_t, socket::event_set>
    pop_event(socket::event_set& a_ev) {
        if (m_nevents <= 0)
            return boost::tuple<native_socket_t, socket::event_set>(
                static_cast<native_socket_t>(-1), socket::no_events);
        native_socket_t sock = m_current->data.fd;
        int ev = m_current->events;
        if (ev & EPOLLRDNORM) ev |= socket::readable;
        if (ev & EPOLLWRNORM) ev |= socket::writable;
        if (ev & EPOLLRDBAND) ev |= socket::pridata;
        socket::event_set evout = static_cast<socket::event_set>(ev)
             & (socket::readable | socket::writable | socket::pridata);
        BOOST_ASSERT(evout != socket::no_events);
        --m_nevents; ++m_current;
        return boost::tuple<native_socket_t, socket::event_set>(sock, evout);
    }

    void wait(milliseconds_t a_timeout, int& rc) {
        BOOST_ASSERT(a_timeout <= max_timeout());
        BOOST_ASSERT(m_nevents <= 0);
        {
            #if defined(HAVE_EPOLL_PWAIT) && HAVE_EPOLL_PWAIT
            sigset_t unblock_all;
            ::sigemptyset(&unblock_all);
            rc = ::epoll_pwait(m_epoll_fd, &m_events.front(), m_events.size(), static_cast<int>(a_timeout)
                    , &unblock_all);
            #else
            signal_unblock signals;
            rc = ::epoll_wait(m_epoll_fd, &m_events.front(), m_events.size(), static_cast<int>(a_timeout));
            #endif
        }
        m_nevents = static_cast<size_t>(rc);
        m_current = m_events.begin();
    }

    void wait(milliseconds_t a_timeout) {
        int rc;
        wait(a_timeout, rc);
        if (rc < 0) {
            if (errno == EINTR) return;
            throw sys_error(errno, "epoll_wait(2)");
        }
    }
};

} // namespace detail
} // namespace util

#endif // _UTIL_EPOLL_HPP_
