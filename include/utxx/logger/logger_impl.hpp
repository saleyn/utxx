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
#include <utxx/delegate.hpp>
#include <utxx/event.hpp>
#include <utxx/time_val.hpp>
#include <utxx/timestamp.hpp>
#include <utxx/meta.hpp>
#include <utxx/error.hpp>
#include <utxx/singleton.hpp>
#include <utxx/logger/logger_enums.hpp>
#include <utxx/logger.hpp>
#include <utxx/path.hpp>
#include <utxx/convert.hpp>
#include <utxx/variant_tree.hpp>
#include <utxx/print.hpp>
#include <vector>
struct T;
namespace utxx {

/// Temporarily stores msg source location information given to the logger.
class log_msg_info {
    enum { MAX_LOG_MESSAGE_SIZE = 512 };

    using data_buffer = detail::basic_buffered_print
                        <MAX_LOG_MESSAGE_SIZE, std::allocator<char>>;

    logger*       m_logger;
    time_val      m_timestamp;
    log_level     m_level;
    std::string   m_category;
    size_t        m_src_loc_len;
    const char*   m_src_location;
    data_buffer   m_data;

    /// log() calls this function which performs content formatting
    void format_header();
    void format_footer();
public:

    template <int N>
    log_msg_info(logger& a_logger, log_level lv,
                 const char (&a_src_location)[N]);

    template <int N>
    log_msg_info(log_level a_lv, const std::string& a_category,
                 const char (&a_src_location)[N]);

    log_msg_info(log_level a_lv, const std::string& a_category);

    const logger*       get_logger()        const;
    logger*             get_logger();

    time_val const&     msg_time()          const { return m_timestamp;}
    log_level           level()             const { return m_level;    }
    const std::string&  category()          const { return m_category; }
    const char*         src_loc()           const { return m_src_location; }
    size_t              src_loc_len()       const { return m_src_loc_len;  }
    bool                has_src_location()  const { return m_src_location; }

    const char*         data()              const { return m_data.str(); }
    size_t              data_len()          const { return m_data.size();  }

    void category(const std::string& a_category) { m_category = a_category; }

    std::string src_location() const {
        detail::basic_buffered_print<128> buf;
        buf << '[' << src_loc() << ']';
        return buf.to_string();
    }

    void format(const char* a_fmt, ...)
#ifdef __GNUC__
        // Since this is a member function, first argument is 'this', hence '2,3':
        __attribute__((format(printf,2,3)))
#endif
    ;
    void format(const char* a_fmt, va_list a_args);

    // Helper function for LOG_* macros. See logger_impl.ipp for implementation.
    void log();
    void log(const char* a_fmt, ...)
#ifdef __GNUC__
        // Since this is a member function, first argument is 'this', hence '2,3':
        __attribute__((format(printf,2,3)))
#endif
    ;

    /// Helper class used to implement streaming support in the logger.
    /// It makes it possible to write:
    /// \code
    /// log_msg_info(LEVEL_DEBUG, "") << 1 << "test" << std::endl;
    /// \endcode
    class helper {
        log_msg_info* m_owner;
        mutable bool  m_last;
    public:
        helper(log_msg_info* a)
            : m_owner(a)
            , m_last(true)
        {}

        helper(const helper* a_rhs) noexcept
            : m_owner(a_rhs->m_owner)
            , m_last(a_rhs->m_last)
        {
            a_rhs->m_last = false;
        }

        ~helper() {
            if (!m_last)
                return;
            m_owner->log();
        }

        template <typename T>
        helper operator<< (T&& a) {
            m_owner->m_data.print(std::forward<T>(a));
            return helper(*this);
        }

        template <typename T>
        helper operator<< (const T& a) {
            m_owner->m_data.print(a);
            return helper(*this);
        }

        helper operator<<(helper& (*Manipulator)(helper&)) {
            Manipulator(*this);
            return helper(this);
        }
    };

    template <class T>
    helper operator<< (T&& a) {
        return helper(this) << a;
    }

};

/*
/// Interface for manipulators.
/// Manipulators such as @c std::endl and @c std::hex use these
/// functions in constructs like "std::cout << std::endl".
/// For more information, see the iomanip header.
template <typename Alloc>
inline typename log_msg_info::helper&
operator<<(typename log_msg_info::helper& a,
           typename log_msg_info::helper&
            (*Manipulator)(typename log_msg_info::helper&)) {
    return typename log_msg_info::helper(Manipulator(a));
}
*/
/*
    helper operator<<(std::ios_type& (*manip)(std::ios_type&)) {
        return helper(manip(*this));
    }

    helper operator<<(std::ios_base& (*manip)(std::ios_base&)) {
        return helper(manip(*this));
    }
*/

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

    typedef delegate<void (const log_msg_info&) throw(io_error)>
        on_msg_delegate_t;

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

    void do_log(const log_msg_info& a_info);
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

namespace std {

    /// Write a newline to the buffered_print object
    inline typename utxx::log_msg_info::helper&
    endl(typename utxx::log_msg_info::helper& a_out)
    { a_out << '\n'; return a_out; }

    template <class Alloc>
    inline typename utxx::log_msg_info::helper&
    ends(typename utxx::log_msg_info::helper& a_out)
    { a_out << '\0'; return a_out; }

    template <class Alloc>
    inline typename utxx::log_msg_info::helper&
    flush(typename utxx::log_msg_info::helper& a_out)
    { return a_out; }
}

#endif  // _UTXX_LOGGER_IMPL_HPP_
