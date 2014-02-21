//----------------------------------------------------------------------------
/// \file   logger_impl.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Supplementary classes for the <logger> class.
//----------------------------------------------------------------------------
// Copyright (C) 2003-2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-25
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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
#ifndef _UTXX_LOGGER_IMPL_HPP_
#define _UTXX_LOGGER_IMPL_HPP_

#include <stdarg.h>
#include <stdio.h>
#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>
#include <utxx/delegate.hpp>
#include <utxx/event.hpp>
#include <utxx/time_val.hpp>
#include <utxx/timestamp.hpp>
#include <utxx/meta.hpp>
#include <utxx/error.hpp>
#include <utxx/singleton.hpp>
#include <utxx/logger.hpp>
#include <utxx/path.hpp>
#include <utxx/variant_tree.hpp>
#include <vector>

namespace utxx { 

/// Temporarily stores msg source location information given to the logger.
class log_msg_info {
    logger*         m_logger;
    log_level       m_level;
    std::string     m_category;
    size_t          m_src_file_len;
    const char*     m_src_file;
    size_t          m_src_line;
public:

    template <int N>
    log_msg_info(logger& a_logger, log_level lv,
                 const char (&filename)[N], size_t ln);

    template <int N>
    log_msg_info(log_level a_lv, const std::string& a_category,
                 const char (&a_filename)[N], size_t a_ln);

    log_msg_info(log_level a_lv, const std::string& a_category);

    logger*             get_logger()        const { return m_logger; }
    log_level           level()             const { return m_level; }
    const std::string&  category()          const { return m_category; }
    const char*         src_file()          const { return m_src_file; }
    size_t              src_file_len()      const { return m_src_file_len; }
    size_t              src_line()          const { return m_src_line; }
    bool                has_src_location()  const { return m_src_file; }

    void category(const std::string& a_category) { m_category = a_category; }

    std::string src_location() const {
        return has_src_location()
            ? (boost::format("[%s:%d]") % src_file() % src_line()).str() : std::string();
    }

    // Helper function for LOG_* macros. See logger_impl.ipp for implementation.
    void inline log(const char* fmt, ...);
};

// Logger back-end implementations must derive from this class.
struct logger_impl {
    enum {
        NLEVELS = log<(int)LEVEL_ALERT, 2>::value
                - log<(int)LEVEL_TRACE, 2>::value + 1
    };

    logger_impl();
    virtual ~logger_impl();

    /// Name of the logger
    virtual const std::string& name() const = 0;

    virtual bool init(const variant_tree& a_config)
        throw(badarg_error, io_error) = 0;

    /// Dump all settings to stream
    virtual std::ostream& dump(std::ostream& out, const std::string& a_prefix) const = 0;

    /// Called by logger upon reading initialization from configuration
    void set_log_mgr(logger* a_log_mgr) { m_log_mgr = a_log_mgr; }

    /// on_msg_delegate calls this function which performs content formatting
    /// @param buf is the buffer that will contain formatted message content
    /// @param size is the buffer size
    /// @param add_new_line if true '\n' will be added at the end of the output.
    /// @param a_show_ident if true <a_logger.ident()> will be included in the output.
    /// @param a_show_location if true <info.src_location> will be included in
    ///                        the output.
    /// @param a_ts current timestamp value
    /// @param a_logger the logger object calling on_message callback.
    /// @param info msg log level and source line details
    /// @param fmt format string
    /// @param args formatting arguments.
    int format_message(
        char* buf, size_t size, bool add_new_line,
        bool a_show_ident, bool a_show_location,
        const timeval* a_ts, const log_msg_info& info,
        const char* fmt, va_list args
    ) throw (badarg_error);

    typedef delegate<
        void (const log_msg_info&,
              const timeval*,
              const char*, /* format */
              va_list      /* args */) throw(std::runtime_error)
    > on_msg_delegate_t;

    typedef delegate<
        void (const std::string& /* category */,
              const char*        /* msg */,
              size_t             /* size */) throw(std::runtime_error)
    > on_bin_delegate_t;

    /// To be called by <logger_impl> child to register a delegate to be
    /// invoked on a call to LOG_*() macros.
    /// @return Id assigned to the message logger, which is to be used
    ///         in the remove_msg_logger call to release the event sink.
    void add_msg_logger(log_level level, on_msg_delegate_t subscriber);

    /// To be called by <logger_impl> child to register a delegate to be
    /// invoked on a call to logger::log(msg, size).
    /// @return Id assigned to the message logger, which is to be used
    ///         in the remove_msg_logger call to release the event sink.
    void add_bin_logger(on_bin_delegate_t subscriber);

    friend bool operator==(const logger_impl& a, const logger_impl& b) {
        return a.name() == b.name();
    };
    friend bool operator!=(const logger_impl& a, const logger_impl& b) {
        return a.name() != b.name();
    };
protected:
    logger* m_log_mgr;
    int     m_msg_sink_id[NLEVELS]; // Message sink identifiers in the loggers' signal
    int     m_bin_sink_id;          // Message sink identifier in the loggers' signal
};

/// Log implementation manager. It handles registration of
/// logging back-ends, so that they can be instantiated automatically
/// based on configuration information. The manager contains a list
/// of logger backend creation functions mapped by name.
struct logger_impl_mgr {
    typedef boost::function<logger_impl*(const char*)>  impl_callback_t;
    typedef std::map<std::string, impl_callback_t>      impl_map_t;

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

} // namespace utxx

#endif  // _UTXX_LOGGER_IMPL_HPP_
