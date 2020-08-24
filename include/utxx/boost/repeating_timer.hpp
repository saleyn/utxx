//----------------------------------------------------------------------------
/// \file   repeating_timer.hpp
/// \author David C. Wyles
//----------------------------------------------------------------------------
/// \brief Extends Boost basic_deadline_timer to make it repeatitive.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2007
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source library.

Copyright (C) 2007 David C. Wyles (http:///www.codegorilla.co.uk)

Use, modification and distribution are subject to the Boost Software
License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
at http://www.boost.org/LICENSE_1_0.txt)

***** END LICENSE BLOCK *****
*/

#ifndef BOOST_ASIO_REPEATING_TIMER_H_INCLUDED
#define BOOST_ASIO_REPEATING_TIMER_H_INCLUDED

#if (BOOST_VERSION < 107000)

#include <boost/bind/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/asio/detail/deadline_timer_service.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/version.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace boost {
namespace asio {

    namespace detail {
        template <typename Time, typename TimeTraits, typename TimerService>
        class basic_repeating_timer;
    }

    /// The handler is being passed this event so that it can know
    /// when this is the first time / repeating / last time the timer
    /// is firing as per timer startup instructions provided by
    /// the start() function arguments.
    enum timer_event_type {
          TIMER_STARTING
        , TIMER_REPEATING
        , TIMER_ENDING
    };


    static const char* timer_event_type_string(timer_event_type x) {
        const char* s_types[] = { "starting", "repeating", "ending" };
        return s_types[static_cast<int>(x)];
    }

    // Rasic repeating timer with start stop semantics.
    typedef detail::basic_repeating_timer<
        boost::posix_time::ptime,
        boost::asio::time_traits<boost::posix_time::ptime>,
        boost::asio::deadline_timer_service<boost::posix_time::ptime>
    > repeating_timer;


    namespace detail {

        template <typename Time, typename TimeTraits, typename TimerService>
        class basic_repeating_timer : public boost::asio::basic_io_object<TimerService>
        {
        public:
            typedef boost::asio::basic_deadline_timer<Time> timer_type;
            typedef typename timer_type::duration_type      duration_type;
            typedef typename timer_type::time_type          time_type;

            explicit basic_repeating_timer( boost::asio::io_service& io_service )
                : boost::asio::basic_io_object<TimerService>(io_service)
            {
            }

            ~basic_repeating_timer()
            {
                stop();
            }

            /// Start a timer given the repeat interval and its window of operation
            template <typename WaitHandler>
            bool start( int id, const duration_type& repeat_interval, 
                WaitHandler handler,
                const time_type& start_at = time_type(),
                const time_type& stop_at  = time_type())
            {
                boost::recursive_mutex::scoped_lock guard(lock_);
                {
                    // cleanup code, cancel any existing timer
                    handler_.reset();
                    if( timer_ )
                    {
                        timer_->cancel();
                    }
                    timer_.reset();
                }

                // create new handler.
                handler_.reset( new handler_impl<WaitHandler>( handler ) );
                // create new timer
                timer_ = internal_timer::create(
// method io_service renamed to get_io_service in commit r40176 (19 Oct 2007),
// so this change is probably released in 1.35.0 (August 14th, 2008)
                    #if BOOST_VERSION >= 103500
                    this->get_io_service(),
                    #else
                    this->io_service(),
                    #endif
                    id, handler_, repeat_interval, start_at, stop_at );
                return !!timer_;
            }

            void stop()
            {
                boost::recursive_mutex::scoped_lock guard(lock_);
                {
                    // cleanup code.
                    handler_.reset();
                    if( timer_ )
                    {
                        timer_->cancel();
                    }
                    timer_.reset();
                }
            }

            void cancel() {stop();}

            // changes the interval the next time the timer is fired.
            void change_interval( const duration_type& repeat_interval )
            {
                boost::recursive_mutex::scoped_lock guard(lock_);
                if( timer_ )
                {
                    timer_->change_interval( repeat_interval );
                }
            }

        private:

            boost::recursive_mutex  lock_;
            class internal_timer;
            typedef boost::shared_ptr<internal_timer>   internal_timer_ptr;
            internal_timer_ptr      timer_;

            class handler_base;
            boost::shared_ptr<handler_base> handler_;

            class handler_base
            {
            public:
                virtual ~handler_base() {}
                virtual void handler( int, timer_event_type, const time_type&,
                    boost::system::error_code const & ) = 0;
            };

            template <typename HandlerFunc>
            class handler_impl : public handler_base
            {
            public:
                handler_impl( HandlerFunc func ) : handler_func_(func) {}
                virtual void handler(
                    int id, timer_event_type ev, const time_type& now,
                    boost::system::error_code const & result )
                {
                    handler_func_(id, ev, now, result);
                }
                HandlerFunc handler_func_;
            };

            class internal_timer
                : public boost::enable_shared_from_this<internal_timer>
            {
            public:
                static internal_timer_ptr create(
                    boost::asio::io_service& io_service,
                    int id,
                    boost::shared_ptr<handler_base> const & handler,
                    duration_type const & repeat_interval, 
                    time_type const & start_at = time_type(),
                    time_type const & stop_at = time_type())
                {
                    internal_timer_ptr timer( new internal_timer( io_service ) );
                    bool rc = timer->start( id, repeat_interval, handler, start_at, stop_at );
                    if (!rc)
                        timer.reset();
                    return timer;
                }

                void cancel()
                {
                    boost::recursive_mutex::scoped_lock guard( lock_ );
                    timer_.cancel();
                }

                void change_interval(
                    duration_type const& repeat_interval )
                {
                    boost::recursive_mutex::scoped_lock guard( lock_ );
                    interval_ = repeat_interval;
                }

            private:
                timer_type                      timer_;
                boost::weak_ptr<handler_base>   handler_;
                int                             id_;
                bool                            first_time_;
                time_type                       start_at_;
                time_type                       stop_at_;
                duration_type                   interval_;
                boost::recursive_mutex          lock_;

                internal_timer( boost::asio::io_service& io_service )
                    : timer_(io_service)
                {
                }

                bool start( int id,
                            duration_type const & repeat_interval, 
                            boost::shared_ptr<handler_base> const & handler,
                            time_type const & start_at,
                            time_type const & stop_at )
                {
                    // only EVER called once, via create
                    id_         = id;
                    interval_   = repeat_interval;
                    handler_    = handler;
                    start_at_   = start_at;
                    stop_at_    = stop_at;
                    first_time_ = true;

                    if (start_at == time_type())
                        first_time_ = false;
                    else {
                        time_type next =
                            next_interval( TimeTraits::now() );
                        if ((stop_at_ != time_type() &&
                             start_at > stop_at)
                            || (next == time_type()))
                        {
                            return false;
                        }
                    }

                    if (first_time_)
                        timer_.expires_at( start_at_ );
                    else
                        timer_.expires_from_now( interval_ );
                    timer_.async_wait( boost::bind(
                            &internal_timer::handle_timeout,
                            this->shared_from_this(),
                            boost::asio::placeholders::error ) );
                    return true;
                }

                time_type
                next_interval(time_type const & now)
                {
                    if (stop_at_ != time_type() && stop_at_ <= now)
                        return time_type();
                    return TimeTraits::add(now, interval_);
                }

                void handle_timeout(boost::system::error_code const& error)
                {
                    time_type now  = TimeTraits::now();
                    time_type next = next_interval( now );

                    timer_event_type ev = first_time_
                        ? TIMER_STARTING : (next == time_type()
                                   ? TIMER_ENDING : TIMER_REPEATING);

                    first_time_ = false;

                    // we lock in the timeout to block the cancel operation
                    // until this timeout completes
                    boost::recursive_mutex::scoped_lock guard( lock_ );
                    {
                        // do the fire.
                        boost::shared_ptr<handler_base> Handler = handler_.lock();
                        if( Handler )
                        {
                            try
                            {
                                Handler->handler(id_, ev, now, error);
                            }
                            catch( std::exception const & e )
                            {
                                // consume for now, no much else we can do,
                                // we don't want to damage the io_service thread
                                (void)e;
                            }
                        }
                    }
                    
                    if ( ev == TIMER_ENDING )
                        timer_.cancel();
                    else if( !error )
                    {
                        // check if we need to reschedule.
                        boost::shared_ptr<handler_base> Handler = handler_.lock();
                        if( Handler )
                        {
                            timer_.expires_at( next );
                            //m_Timer.expires_at( m_Timer.expires_at() + m_Repeat_Interval );
                            timer_.async_wait(
                                boost::bind( &internal_timer::handle_timeout,
                                    this->shared_from_this(),
                                    boost::asio::placeholders::error ) );
                        }
                    }
                }
            };
        };
}
}
}

#endif // BOOST_VERSION

#endif // BOOST_ASIO_REPEATING_TIMER_H_INCLUDED


