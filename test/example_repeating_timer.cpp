// compile with:
// g++ -o t repeating_timer_example.cpp -I. -I${BOOST_ROOT}/include -L${BOOST_ROOT}/lib -lboost_date_time -lboost_system -lboost_thread

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/ptr_container/ptr_array.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>

#include <util/boost/repeating_timer.hpp>

using namespace  boost::posix_time;

// callback hander for out timers
void handle_timeout( int id, boost::asio::timer_event_type ev, 
    const ptime& now,
    boost::system::error_code const& error )
{
    std::stringstream s;
    s << now << " timer " << id << " fired (event type="
        << boost::asio::timer_event_type_string(ev) << ")" << std::endl;
    std::cout << s.str();
}

int main(int argc, char* argv[])
{
    size_t ntimers  = argc > 1 ? atoi(argv[1]) : 10;
    size_t nthreads = argc > 2 ? atoi(argv[2]) : 5;

    if (ntimers  > 10) ntimers = 10;
    if (nthreads > 10) nthreads = 10;

    // example shows io_service running in multiple thread, with multiple timers
    boost::thread_group         threads;
    boost::asio::io_service     my_io_service;
    boost::asio::strand         strand(my_io_service);

    boost::ptr_array<boost::asio::repeating_timer,10> timers;

    // create timers
    for( size_t i = 0; i < ntimers; ++i )
    {
        timers.replace( i, new boost::asio::repeating_timer(my_io_service) );
    }

    typedef boost::date_time::c_local_adjustor<boost::posix_time::ptime> local_adj;

    // start them with their callbacks referencing the next timer
    for( size_t i = 0; i < ntimers; ++i )
    {
        boost::posix_time::ptime 
            start_time = boost::posix_time::second_clock::universal_time()
                       + boost::posix_time::seconds(i+3);
        boost::posix_time::ptime 
            stop_time = start_time + boost::posix_time::seconds(i + 30);

        std::cout << "Timer " << i << " will start at " << 
            local_adj::utc_to_local(start_time)
            << " and end at " << local_adj::utc_to_local(stop_time) << " interval " 
            << boost::posix_time::seconds(i+1) << std::endl;

        bool res = timers[i].start( i, 
                         strand.wrap(&handle_timeout),
                         boost::posix_time::seconds(i+1),
                         start_time, stop_time);
        if (!res)
            std::cout << "Couldn't start timer " << i << std::endl;            
    }

    std::cout << "Press enter to stop the timers...\n";

    // finally we start the service running. If the service has nothing to work
    // with it will exit immediately.
    for( size_t i = 0; i < nthreads; ++i )
    {
        threads.create_thread(
                boost::bind(&boost::asio::io_service::run, &my_io_service)
        );
    }

    /*
    // wait for the enter/return key to be pressed
    std::cin.get();

    // explicitly cancel the timers
    // could just call timers.release() which would destroy the timer objects but
    // for completeness I wanted to show the cancel.
    for( size_t i = 0; i < ntimers; ++i )
    {
        timers[i].cancel();
    }
    */

    // and then wait for all threads to complete
    threads.join_all();

    return 0;
}
