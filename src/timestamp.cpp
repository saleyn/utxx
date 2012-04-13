#include <util/convert.hpp>
#include <util/timestamp.hpp>
#include <util/compiler_hints.hpp>
#include <stdio.h>

namespace util {

boost::mutex            timestamp::s_mutex;
__thread hrtime_t       timestamp::s_last_hrtime;
__thread struct timeval timestamp::s_last_time = {0,0};
__thread time_t         timestamp::s_midnight_seconds = 0;
__thread time_t         timestamp::s_utc_offset = 0;
__thread char           timestamp::s_timestamp[16];

#ifdef DEBUG_TIMESTAMP
volatile long timestamp::s_hrcalls;
volatile long timestamp::s_syscalls;
#endif

void timestamp::update_midnight_seconds(const time_val& a_now)
{
    // FIXME: it doesn't seem like the mutex is needed here at all
    // since s_timestamp lives in TLS storage
    boost::mutex::scoped_lock guard(s_mutex);
    struct tm tm;
    ::localtime_r(&a_now.timeval().tv_sec, &tm);

    // FIXME: use strftime()
    sprintf(s_timestamp, "%4d-%02d-%02d ",
            tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);

    s_utc_offset = tm.tm_gmtoff;
    s_midnight_seconds  = a_now.sec() - a_now.sec() % 86400;
}

const time_val& timestamp::now() {
    // thread safe - we use TLV storage
    s_last_time   = time_val::universal_time().timeval();
    s_last_hrtime = high_res_timer::gettime();
    return last_time();
}

void timestamp::update_slow()
{
    now();
    #ifdef DEBUG_TIMESTAMP
    atomic::inc(&s_syscalls);
    #endif

    // FIXME: the method below will produce incorrect time stamps during
    // switch to/from daylight savings time because of the unaccounted
    // utc_offset change.
    if (unlikely(s_midnight_seconds == 0 ||
                 last_time().sec() > s_midnight_seconds)) {
        update_midnight_seconds(last_time());
    }
}

int timestamp::format(stamp_type a_tp,
    const struct timeval* tv, char* a_buf, size_t a_sz)
{
    BOOST_ASSERT(a_sz > 26);

    if (unlikely(a_tp == NO_TIMESTAMP)) {
        a_buf[0] = '\0';
        return 0;
    }

    if (unlikely(s_midnight_seconds == 0)) {
        timestamp ts; ts.update();
    }

    time_t sec = tv->tv_sec + s_utc_offset;
    // If small time is given, it's a relative value.
    if (tv->tv_sec < 86400) sec = tv->tv_sec;

    switch (a_tp) {
        case TIME:
            write_timestamp(a_buf, sec, 8);
            return 8;
        case TIME_WITH_USEC:
            write_timestamp(a_buf, sec, 15);
            a_buf[8] = '.';
            itoa(a_buf+9, 6, tv->tv_usec, '0');
            return 15;
        case TIME_WITH_MSEC:
            write_timestamp(a_buf, sec, 12);
            a_buf[8] = '.';
            itoa(a_buf+9, 3, tv->tv_usec / 1000, '0');
            return 12;
        case DATE_TIME:
            strncpy(a_buf, s_timestamp, 12);
            write_timestamp(a_buf+11, sec, 8);
            return 19;
        case DATE_TIME_WITH_USEC:
            strncpy(a_buf, s_timestamp, 12);
            write_timestamp(a_buf+11, sec, 15);
            a_buf[19] = '.';
            itoa(a_buf+20, 6, tv->tv_usec, '0');
            return 26;
        case DATE_TIME_WITH_MSEC:
            strncpy(a_buf, s_timestamp, 12);
            write_timestamp(a_buf+11, sec, 12);
            a_buf[19] = '.';
            itoa(a_buf+20, 3, tv->tv_usec / 1000, '0');
            return 23;
        default:
            strcpy(a_buf, "UNDEFINED");
            return -1;
    }
}

} // namespace util
