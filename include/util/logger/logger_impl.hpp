/**
 * Supplementary classes for the <logger> class.
 *-----------------------------------------------------------------------------
 * Copyright (c) 2003-2009 Serge Aleynikov <serge@aleynikov.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *-----------------------------------------------------------------------------
 * Created: 2009-11-25
 * $Id$
 */
#ifndef _UTIL_LOGGER_IMPL_HPP_
#define _UTIL_LOGGER_IMPL_HPP_

#include <stdarg.h>
#include <stdio.h>
#include <boost/thread/mutex.hpp>
#include <util/delegate.hpp>
#include <util/event.hpp>
#include <util/time_val.hpp>
#include <util/timestamp.hpp>
#include <util/meta.hpp>
#include <util/error.hpp>
#include <util/singleton.hpp>
#include <util/logger.hpp>
#include <util/path.hpp>
#include <vector>

namespace util { 

/// Temporary stores msg source location information given to the logger.
class log_msg_info {
    logger&         m_logger;
    log_level       m_level;
    char            m_src_location[128];
    size_t          m_src_location_len;
public:

    template <int N>
    log_msg_info(logger& a_logger, log_level lv, const char (&filename)[N], size_t ln);

    logger&     get_logger()        const { return m_logger; }
    log_level   level()             const { return m_level; }
    const char* src_location()      const { return m_src_location; }
    size_t      src_location_len()  const { return m_src_location_len; }
    bool        has_src_location()  const { return m_src_location_len > 0; }

    // See logger_impl.hxx for implementation.
    void inline log(const char* fmt, ...);
};

// Logger back-end implementations must derive from this class.
struct logger_impl {
    enum {
        NLEVELS = log<(int)LEVEL_ALERT, 2>::value
                - log<(int)LEVEL_TRACE, 2>::value + 1
    };

    logger_impl() : m_log_mgr(NULL) {}
    virtual ~logger_impl() {}

    virtual bool init(const boost::property_tree::ptree& a_config)
        throw(badarg_error, io_error) = 0;

    /// Called by logger upon reading initialization from configuration
    void set_log_mgr(logger* a_log_mgr) { m_log_mgr = a_log_mgr; }

    /// on_msg_delegate calls this function which performs content formatting
    /// @param <buf> is the buffer that will contain formatted message content
    /// @param <size> is the buffer size
    /// @param <add_new_line> if true '\n' will be added at the end of the output.
    /// @param <a_show_ident> if true <a_logger.ident()> will be included in the output.
    /// @param <a_show_location> if true <info.src_location> will be included in
    ///                          the output.
    /// @param <timestamp> current timestamp value
    /// @param <a_logger> the logger object calling on_message callback.
    /// @param <info> msg log level and source line details
    /// @param <fmt> format string
    /// @param <args> formatting arguments.
    int format_message(
        char* buf, size_t size, bool add_new_line,
        bool a_show_ident, bool a_show_location,
        const timestamp& a_ts, const log_msg_info& info,
        const char* fmt, va_list args
    ) throw (badarg_error);

    typedef delegate<
        void (const log_msg_info&,
              const timestamp&,
              const char*, /* format */
              va_list      /* args */) throw(std::runtime_error)
    > on_msg_delegate_t;

    typedef delegate<
        void (const char* /* msg */,
              size_t      /* size */) throw(std::runtime_error)
    > on_bin_delegate_t;

protected:
    logger* m_log_mgr;

    // Binders for binding event_source's to event_sink's
    event_binder<on_msg_delegate_t> msg_binder[NLEVELS];
    event_binder<on_bin_delegate_t> bin_binder;
};

/// Log implementation manager. It handles registration of
/// logging back-ends, so that they can be instantiated automatically
/// based on configuration information.
struct logger_impl_mgr {
    typedef boost::function<logger_impl*(void)>    impl_callback_t;
    typedef std::map<std::string, impl_callback_t> impl_map_t;

    static logger_impl_mgr& instance() { 
        return singleton<logger_impl_mgr>::instance();
    } 

    void register_impl(const char* config_name, impl_callback_t& factory);
    void unregister_impl(const char* config_name);
    impl_callback_t* get_impl(const char* config_name);

    /// A static instance of the registrar must be created by
    /// each back-end in order to be automatically registered with
    /// the implementation manager.
    struct registrar {
        registrar(const char* config_name, impl_callback_t& factory)
            : name(config_name)
        {
            instance().register_impl(config_name, factory);
        }
        ~registrar() {
            instance().unregister_impl(name);
        }
    private:
        const char* name;
    };

    impl_map_t&   implementations()  { return m_implementations; }
    boost::mutex& mutex()            { return m_mutex; }

private:
    impl_map_t   m_implementations;
    boost::mutex m_mutex;
};

} // namespace util

#endif  // _UTIL_LOGGER_IMPL_HPP_
