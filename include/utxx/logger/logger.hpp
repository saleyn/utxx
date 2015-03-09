//----------------------------------------------------------------------------
/// \file   logger.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Light-weight logging of messages to configured targets.
///
/// The client supports pluggable back-ends that can write messages
/// synchronously and asyncronously to different targets.  Unsafe printf() like 
/// interface of the logger (vs. stream-based) is chosen primarily for 
/// simplisity and speed. 
/// The idea behind the logger is hargely based on the asynchronous logging 
/// project called LAMA in the jungerl.sf.net repository.
///
/// The following back-end logger plugins are currently implemented:
///  * console writer
///  * file writer
///  * asynchronous file writer
///  * syslog writer
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
#pragma  once

#include <stdarg.h>
#include <stdio.h>
#include <utxx/function.hpp>
#include <utxx/delegate.hpp>
#include <utxx/delegate/event.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/config_tree.hpp>
#include <utxx/concurrent_mpsc_queue.hpp>
#include <utxx/logger/logger_enums.hpp>
#include <utxx/synch.hpp>
#include <thread>
#include <mutex>

#ifndef _MSC_VER
#   include <utxx/synch.hpp>
#   include <utxx/high_res_timer.hpp>
#   include <utxx/timestamp.hpp>
#include <utxx/persist_array.hpp>
#endif

#ifndef UTXX_SKIP_LOG_MACROS
#   define UTXX_LOG_TRACE4( Fmt, ...)     UTXX_CLOG(LEVEL_TRACE4 , "",  Fmt, ##__VA_ARGS__)
#   define UTXX_LOG_TRACE3( Fmt, ...)     UTXX_CLOG(LEVEL_TRACE3 , "",  Fmt, ##__VA_ARGS__)
#   define UTXX_LOG_TRACE2( Fmt, ...)     UTXX_CLOG(LEVEL_TRACE2 , "",  Fmt, ##__VA_ARGS__)
#   define UTXX_LOG_TRACE1( Fmt, ...)     UTXX_CLOG(LEVEL_TRACE1 , "",  Fmt, ##__VA_ARGS__)
#   define UTXX_LOG_DEBUG(  Fmt, ...)     UTXX_CLOG(LEVEL_DEBUG  , "",  Fmt, ##__VA_ARGS__)
#   define UTXX_LOG_INFO(   Fmt, ...)     UTXX_CLOG(LEVEL_INFO   , "",  Fmt, ##__VA_ARGS__)
#   define UTXX_LOG_WARNING(Fmt, ...)     UTXX_CLOG(LEVEL_WARNING, "",  Fmt, ##__VA_ARGS__)
#   define UTXX_LOG_ERROR(  Fmt, ...)     UTXX_CLOG(LEVEL_ERROR  , "",  Fmt, ##__VA_ARGS__)
#   define UTXX_LOG_FATAL(  Fmt, ...)     UTXX_CLOG(LEVEL_FATAL  , "",  Fmt, ##__VA_ARGS__)
#   define UTXX_LOG_ALERT(  Fmt, ...)     UTXX_CLOG(LEVEL_ALERT  , "",  Fmt, ##__VA_ARGS__)

#   define UTXX_CLOG_TRACE4( Cat,Fmt, ...) UTXX_CLOG(LEVEL_TRACE4 , Cat, Fmt, ##__VA_ARGS__)
#   define UTXX_CLOG_TRACE3( Cat,Fmt, ...) UTXX_CLOG(LEVEL_TRACE3 , Cat, Fmt, ##__VA_ARGS__)
#   define UTXX_CLOG_TRACE2( Cat,Fmt, ...) UTXX_CLOG(LEVEL_TRACE2 , Cat, Fmt, ##__VA_ARGS__)
#   define UTXX_CLOG_TRACE1( Cat,Fmt, ...) UTXX_CLOG(LEVEL_TRACE1 , Cat, Fmt, ##__VA_ARGS__)
#   define UTXX_CLOG_DEBUG(  Cat,Fmt, ...) UTXX_CLOG(LEVEL_DEBUG  , Cat, Fmt, ##__VA_ARGS__)
#   define UTXX_CLOG_INFO(   Cat,Fmt, ...) UTXX_CLOG(LEVEL_INFO   , Cat, Fmt, ##__VA_ARGS__)
#   define UTXX_CLOG_WARNING(Cat,Fmt, ...) UTXX_CLOG(LEVEL_WARNING, Cat, Fmt, ##__VA_ARGS__)
#   define UTXX_CLOG_ERROR(  Cat,Fmt, ...) UTXX_CLOG(LEVEL_ERROR  , Cat, Fmt, ##__VA_ARGS__)
#   define UTXX_CLOG_FATAL(  Cat,Fmt, ...) UTXX_CLOG(LEVEL_FATAL  , Cat, Fmt, ##__VA_ARGS__)
#   define UTXX_CLOG_ALERT(  Cat,Fmt, ...) UTXX_CLOG(LEVEL_ALERT  , Cat, Fmt, ##__VA_ARGS__)

#ifndef  UTXX_LOGGER_RESTRICT_NAMESPACE_PREFIX
#   define LOG_TRACE4   UTXX_LOG_TRACE4
#   define LOG_TRACE3   UTXX_LOG_TRACE3
#   define LOG_TRACE2   UTXX_LOG_TRACE2
#   define LOG_TRACE1   UTXX_LOG_TRACE1
#   define LOG_DEBUG    UTXX_LOG_DEBUG
#   define LOG_INFO     UTXX_LOG_INFO
#   define LOG_WARNING  UTXX_LOG_WARNING
#   define LOG_ERROR    UTXX_LOG_ERROR
#   define LOG_FATAL    UTXX_LOG_FATAL
#   define LOG_ALERT    UTXX_LOG_ALERT

#   define CLOG_TRACE4  UTXX_CLOG_TRACE4
#   define CLOG_TRACE3  UTXX_CLOG_TRACE3
#   define CLOG_TRACE2  UTXX_CLOG_TRACE2
#   define CLOG_TRACE1  UTXX_CLOG_TRACE1
#   define CLOG_DEBUG   UTXX_CLOG_DEBUG
#   define CLOG_INFO    UTXX_CLOG_INFO
#   define CLOG_WARNING UTXX_CLOG_WARNING
#   define CLOG_ERROR   UTXX_CLOG_ERROR
#   define CLOG_FATAL   UTXX_CLOG_FATAL
#   define CLOG_ALERT   UTXX_CLOG_ALERT
#endif  // UTXX_LOGGER_RESTRICT_NAMESPACE_PREFIX

#endif  // UTXX_SKIP_LOG_MACROS

/// We use the macro trickery below to implemenet a UTXX_LOG() macro that
/// can optionally take the category as the second macro argument. The idea
/// is to use a macro chooser that takes __VA_ARGS__ to select the
/// UTXX_LOG_N_ARGS() macro depending on whether it was called with one
/// or two arguments:
#define UTXX_LOG_1_ARGS(Level) \
    utxx::logger::msg_streamer(LEVEL_##Level, "", UTXX_FILE_SRC_LOCATION)
#define UTXX_LOG_2_ARGS(Level, Cat) \
    utxx::logger::msg_streamer(LEVEL_##Level, Cat, UTXX_FILE_SRC_LOCATION)

#define UTXX_GET_3RD_ARG(arg1, arg2, arg3, ...) arg3
#define UTXX_LOG_MACRO_CHOOSER(...) \
        UTXX_GET_3RD_ARG(__VA_ARGS__, UTXX_LOG_2_ARGS, UTXX_LOG_1_ARGS)

//------------------------------------------------------------------------------
/// In all <LOG_*> macros <FmtArgs> are parameter lists with signature of
/// the <printf> function: <(const char* fmt, ...)>
//------------------------------------------------------------------------------
#define UTXX_CLOG(Level, Cat, Fmt, ...) \
    utxx::logger::instance().logfmt(Level, Cat, UTXX_FILE_SRC_LOCATION, Fmt, ##__VA_ARGS__)

//------------------------------------------------------------------------------
/// Suppost for streaming version of the logger
//------------------------------------------------------------------------------
#define UTXX_LOG(Level, ...) \
    UTXX_LOG_MACRO_CHOOSER(Level, ##__VA_ARGS__)(Level, ##__VA_ARGS__)

namespace utxx {

struct logger_impl;

/// Logging class that supports pluggable back-ends responsible for handling
/// log messages. Examples of backends are implemented in the logger_impl_console,
/// logger_impl_file, logger_impl_async_file classes.
struct logger : boost::noncopyable {
    friend class logger_impl;
    enum {
        NLEVELS = log<(int)LEVEL_ALERT, 2>::value
                - log<(int)LEVEL_TRACE, 2>::value + 1
    };

    static const char* log_level_to_str(log_level level) noexcept;
    static size_t      log_level_size  (log_level level) noexcept;
    static std::string log_levels_to_str(int a_levels)   noexcept;
    /// Convert a <level> to the slot number in the <m_sig_msg> array
    static int         level_to_signal_slot(log_level level) noexcept;
    /// Convert a <level> to the slot number in the <m_sig_msg> array
    static log_level   signal_slot_to_level(int slot) noexcept;

    typedef boost::shared_ptr<logger_impl>  impl;
    typedef std::vector<impl>               implementations_vector;

    using char_function  = function<int (char* a_buf, size_t a_size)>;
    using str_function   = function
        <std::string (const char* pfx, size_t plen, const char* sfx, size_t slen)>;

    enum class payload_t { STR_FUN, CHAR_FUN, STR };

    class msg {
        time_val      m_timestamp;
        log_level     m_level;
        std::string   m_category;
        std::size_t   m_src_loc_len;
        const char*   m_src_location;
        payload_t     m_type;

        union U {
            char_function  cf;
            str_function   sf;
            std::string    str;
            U() : cf(nullptr) {}
            U(const char_function& f) : cf(f)  {}
            U(const str_function&  f) : sf(f)  {}
            U(const std::string&   f) : str(f) {}
            ~U() {}
        } m_fun;

        friend class logger;

        template <typename Fun>
        msg(log_level a_ll, const std::string& a_category, payload_t a_type,
            const Fun& a_fun, const char* a_src_loc, std::size_t a_sloc_len)
            : m_timestamp   (now_utc())
            , m_level       (a_ll)
            , m_category    (a_category)
            , m_src_loc_len (a_sloc_len)
            , m_src_location(a_src_loc)
            , m_type        (a_type)
            , m_fun         (a_fun)
        {}

    public:

        msg(log_level a_ll, const std::string& a_category,
            const char_function& a_fun, const char* a_src_loc, std::size_t a_sloc_len)
            : msg(a_ll, a_category, payload_t::CHAR_FUN, a_fun, a_src_loc, a_sloc_len)
        {}

        msg(log_level a_ll, const std::string& a_category,
            const str_function& a_fun, const char* a_src_loc, std::size_t a_sloc_len)
            : msg(a_ll, a_category, payload_t::STR_FUN, a_fun, a_src_loc, a_sloc_len)
        {}

        template <int N>
        msg(log_level a_ll, const std::string& a_category,
            const str_function& a_fun, const char (&a_src_loc)[N])
            : msg(a_ll, a_category, payload_t::STR_FUN, a_fun, a_src_loc, N-1)
        {}

        template <int N>
        msg(log_level a_ll, const std::string& a_category,
            const std::string& a_str, const char (&a_src_loc)[N])
            : msg(a_ll, a_category, payload_t::STR, a_str, a_src_loc, N-1)
        {}

        msg(log_level a_ll, const std::string& a_category,
            const std::string& a_str, const char* a_src_loc, std::size_t a_sloc_len)
            : msg(a_ll, a_category, payload_t::STR, a_str, a_src_loc, a_sloc_len)
        {}

        ~msg() {
            switch (m_type) {
                case payload_t::STR_FUN:  m_fun.sf = nullptr;  break;
                case payload_t::CHAR_FUN: m_fun.cf = nullptr;  break;
                case payload_t::STR:      m_fun.str.~basic_string(); break;
            }
        }

        time_val      timestamp   () const { return m_timestamp;    }
        log_level     level       () const { return m_level;        }
        std::string   category    () const { return m_category;     }
        std::size_t   src_loc_len () const { return m_src_loc_len;  }
        const char*   src_location() const { return m_src_location; }
        payload_t     type        () const { return m_type;         }
    };

    struct msg_streamer {
        detail::basic_buffered_print<512> data;
        log_level                         level;
        std::string                       category;
        const char*                       src_loc;
        size_t                            src_loc_len;

        template <int N>
        msg_streamer(log_level a_ll, const std::string& a_cat, const char (&a_src_loc)[N])
            : level(a_ll), category(a_cat), src_loc(a_src_loc), src_loc_len(N-1)
        {}

        /// Helper class used to implement streaming support in the logger.
        /// It makes it possible to write:
        /// \code
        /// msg_streamer(LEVEL_DEBUG, "") << 1 << "test" << std::endl;
        /// \endcode
        class helper {
            msg_streamer* m_ms;
            mutable bool  m_last;
        public:
            helper(msg_streamer* a)
                : m_ms  (a)
                , m_last(true)
            {}

            helper(const helper* a_rhs) noexcept
                : m_ms  (a_rhs->m_ms)
                , m_last(a_rhs->m_last)
            {
                a_rhs->m_last = false;
            }

            ~helper() {
                if (!m_last)
                    return;
                m_ms->data.chop('\n');
                logger::instance().dolog
                    (m_ms->level,       m_ms->category,
                     m_ms->data.c_str(),m_ms->data.size(),
                     m_ms->src_loc,     m_ms->src_loc_len);
            }

            template <typename T>
            helper operator<< (T&& a) {
                m_ms->data.print(std::forward<T>(a));
                return helper(this);
            }

            template <typename T>
            helper operator<< (const T& a) {
                m_ms->data.print(a);
                return helper(this);
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

    typedef delegate
        <void (const msg& a_msg, const char* a_buf, size_t a_size)
               throw(io_error)>
        on_msg_delegate_t;

    // Maps macros to values that can be used in configuration
    typedef std::map<std::string, std::string> macro_var_map;

private:
    using concurrent_queue = concurrent_mpsc_queue<msg>;
    using signal_delegate  = signal<on_msg_delegate_t>;

    std::unique_ptr<std::thread>    m_thread;
    concurrent_queue                m_queue;
    bool                            m_abort;
    bool                            m_initialized;
    futex                           m_event;
    std::mutex                      m_mutex;
    struct timespec                 m_wait_timeout;

    signal_delegate                 m_sig_slot[NLEVELS];
    unsigned int                    m_level_filter;
    implementations_vector          m_implementations;
    stamp_type                      m_timestamp_type;
    char                            m_src_location[256];
    bool                            m_show_location;
    bool                            m_show_ident;
    std::string                     m_ident;
    bool                            m_silent_finish;
    macro_var_map                   m_macro_var_map;


    std::function<void (const char* a_reason)> m_error;

    void  do_finalize();

    char* format_header(const msg& a_msg, char* a_buf, const char* a_end);
    char* format_footer(const msg& a_msg, char* a_buf, const char* a_end);

    /// @return <true> if log <level> is enabled.
    bool is_enabled(log_level level) const {
        return (m_level_filter & (unsigned int)level) != 0;
    }

    void set_timestamp(char* buf, time_t seconds) const;

    /// To be called by <logger_impl> child to register a delegate to be
    /// invoked on a call to LOG_*() macros.
    /// @return Id assigned to the message logger, which is to be used
    ///         in the remove_msg_logger call to release the event sink.
    int add(log_level level, on_msg_delegate_t subscriber);

    /// To be called by <logger_impl> child to unregister a delegate
    void remove(log_level a_lvl, int a_id);

    void dolog_msg(const msg& a_msg);

    void run();

    template<typename Fun>
    bool dolog(log_level a_level, const std::string& a_cat, const Fun& a_fun,
              const char* a_src_loc, std::size_t a_src_loc_len);

    bool dolog(log_level   a_ll,  const std::string& a_cat,
               const char* a_buf,     std::size_t    a_size,
               const char* a_src_loc, std::size_t    a_src_loc_len);

    friend class log_msg_info;

public:
    static logger& instance() {
        static logger s_logger;
        return s_logger;
    }

    logger()
        : m_abort(false), m_initialized(false)
        , m_level_filter(LEVEL_NO_DEBUG), m_timestamp_type(TIME)
        , m_show_location(true), m_show_ident(false), m_silent_finish(false)
    {}
    ~logger() { finalize(); }

    /// @return vector of active back-end logging implementations
    const implementations_vector&  implementations() const;

    /// \brief Call to initialize the logger by reading configuration from file.
    /// Supported file formats: {scon, info, xml}. The file format is determined
    /// by the extension (".config|.conf" - SCON; ".info" - INFO; ".xml" - XML).
    void init(const char* filename);

    /// Call to initialize the logger from a configuration container.
    void init(const config_tree& a_cfg);

    /// Called on destruction/reinitialization of the logger.
    void finalize();

    /// Delete logger back-end implementation identified by \a a_name
    ///
    /// This method is not thread-safe!
    void delete_impl(const std::string& a_name);

    /// Set program identifier to be used in the log output.
    void set_ident(const char* ident) { m_ident = ident; }

    /// Set filter mask to allow only selected log_levels to be included
    /// in the log.
    void set_level_filter(log_level a_level);

    /// Set filter mask to allow log_levels above or equal to \a a_level
    /// to be included in the log.
    void set_min_level_filter(log_level a_level);

    /// Set an error handler delegate to fire when there is an error
    /// instead of throwing run-time exceptions.  Note that the handler
    /// may be called from different threads so it has to be thread-safe.
    void set_error_handler(boost::function<void (const char*)>& eh) { m_error = eh; }

    // FIXME: macros are temporary experimental feature that will be
    // replaces in a future release

    /// Macro name->value mapping that can be used in configuration
    const macro_var_map& macros() const { return m_macro_var_map; }
    /// Add a macro value
    void  add_macro(const std::string& a_macro, const std::string& a_value);
    /// Replace all macros in a string that are found in macros() dictionary.
    std::string replace_macros(const std::string& a_value) const;

    /// Set the timestamp type to use in log files.
    /// @param ts the timestamp type.
    void timestamp_type(stamp_type ts) { m_timestamp_type = ts; }

    /// @return format type of timestamp written to log
    stamp_type  timestamp_type() const { return m_timestamp_type; }

    /// @return true if ident display is enabled by default.
    bool        show_ident()     const { return m_show_ident; }
    /// @return true if source location display is enabled by default.
    bool        show_location()  const { return m_show_location; }

    /// Get program identifier to be used in the log output.
    const std::string&  ident()  const { return m_ident; }
    /// Set program identifier to be used in the log output.
    void  ident(const std::string& a_ident) { m_ident = a_ident; }

    /// Converts a string (e.g. "DEBUG | INFO | WARNING") sizeof(m_timestamp)-1to a bitmask of
    /// corresponding levels.  This method is used for configuration parsing
    static int parse_log_levels(const std::string& levels) throw(std::runtime_error);
    /// String representation of log levels enabled by default.  Used in config
    /// parsing.
    static const char* default_log_levels;

    /// Dump internal settings
    std::ostream& dump(std::ostream& out) const;

    /// Log a message of given log level to the registered implementations.
    /// \a a_msg will be copied to std::string and passed to another context
    /// for logging.
    /// Use the provided <LOG_*> macros instead of calling it directly.
    /// @param a_level is the log level to record
    /// @param a_category is a category of the message (use NULL if undefined).
    /// @param a_buf is the message to be logged
    /// @param a_size is the message size
    template <int N>
    bool logcs(log_level a_level, const std::string& a_category,
               const char* a_msg, size_t a_size, const char (&a_src_log)[N] = "");

    /// Log a message of given log level to the registered implementations.
    /// Logged message will be limited in size to 1024 bytes.
    /// Formatting of the resulting string to be logged happens in the caller's
    /// context, but actual message logging is handled asynchronously.
    /// Use the provided <LOG_*> macros instead of calling it directly.
    /// @param a_level is the log level to record
    /// @param a_cat is a category of the message (use NULL if undefined).
    /// @param a_fmt is the format string passed to <sprintf()>
    /// @param args is the list of optional arguments passed to <args>
    template<int N, typename... Args>
    bool logfmt(log_level a_level, const std::string& a_cat,
              const char (&a_src_log)[N], const char* a_fmt, Args&&... a_args);

    /// Log a message of given log level to the registered implementations.
    /// Formatting of the resulting string to be logged happens in the caller's
    /// context, but actual message logging is handled asynchronously.
    /// Use the provided <LOG_*> macros instead of calling it directly.
    /// @param a_level is the log level to record
    /// @param a_cat is a category of the message (use NULL if undefined).
    /// @param a_fmt is the format string passed to <sprintf()>
    /// @param args is the list of optional arguments passed to <args>
    template<int N, typename... Args>
    bool logs(log_level a_level, const std::string& a_cat,
              const char (&a_src_log)[N], Args&&... a_args);

    /// Log a message of given log level to registered implementations.
    /// Use the provided <LOG_*> macros instead of calling it directly.
    /// @param a_level is the log level to record
    /// @param a_category is a category of the message (use NULL if undefined).
    /// @param a_msg is the message to be logged
    template <int N>
    bool log(log_level a_level, const std::string& a_category,
             const std::string& a_msg, const char (&a_src_log)[N] = "");

    /// Log a message of given log level to registered implementations.
    /// Invocation of \a a_fun happens in the context different from the caller's.
    /// Use the provided <LOG_*> macros instead of calling it directly.
    /// @param a_level   is the log level to record
    /// @param a_cat     is a category of the message (use NULL if undefined).
    /// @param a_fun     lambda that formats captured expression to a
    ///                  string/buffer (see char_funciton or str_function types).
    ///                  The function will be called in the context of another
    ///                  thread, so capture any variables by reference!
    /// @param a_src_loc is the source file/line location which is obtained by
    ///                  using UTXX_SRC_LOCATION macro.
    template<typename Fun, int N>
    bool async_logf(log_level a_level, const std::string& a_cat, const Fun& a_fun,
              const char (&a_src_loc)[N])
    { return dolog(a_level, a_cat, a_fun, a_src_loc, N); }

    /// Log a message of given log level message to registered implementations.
    /// Arguments \a args are copied by value to a lambda that is executed
    /// in the context different from the caller's.
    /// Use the provided <LOG_*> macros instead of calling it directly.
    /// @param a_level is the log level to record
    /// @param a_cat   is a category of the message (use NULL if undefined).
    /// @param args are the arguments to be converted to buffer and logged as string
    template<typename... Args>
    bool async_logs(log_level a_level, const std::string& a_cat, Args&&... args)
    { return async_logs(a_level, a_cat, "", std::forward<Args>(args)...); }

    /// Log a message of given log level message to registered implementations.
    /// Arguments \a args are copied by value to a lambda that is executed
    /// in the context different from the caller's.
    /// Use the provided <LOG_*> macros instead of calling it directly.
    /// @param a_level is the log level to record
    /// @param a_category is a category of the message (use NULL if undefined).
    /// @param args are the arguments to be converted to buffer and logged as string
    template<int N, typename... Args>
    bool async_logs(log_level a_level, const std::string& a_category,
                    const char (&a_src_log)[N], Args&&... args);

    /// Asynchronously log a message of given log level message to registered
    /// implementations.
    /// Use the provided <LOG_*> macros instead of calling it directly.
    /// Arguments \a args are copied by value to a lambda that is executed
    /// in the context different from the caller's.
    /// @param a_level is the log level to record
    /// @param a_cat is a category of the message (use NULL if undefined).
    /// @param a_fmt is the format string passed to <sprintf()>
    /// @param args is the list of optional arguments passed to <args>
    template<int N, typename... Args>
    bool async_logfmt(log_level a_level, const std::string& a_cat,
                      const char (&a_src_log)[N], const char* a_fmt,
                      Args&&... a_args);
};

// Logger back-end implementations must derive from this class.
struct logger_impl {
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

    /// To be called by <logger_impl> child to register a delegate to be
    /// invoked on a call to LOG_*() macros.
    /// @return Id assigned to the message logger, which is to be used
    ///         in the remove_msg_logger call to release the event sink.
    void add(log_level level, logger::on_msg_delegate_t subscriber);

    friend bool operator==(const logger_impl& a, const logger_impl& b) {
        return a.name() == b.name();
    };
    friend bool operator!=(const logger_impl& a, const logger_impl& b) {
        return a.name() != b.name();
    };
protected:
    logger* m_log_mgr;
    int     m_msg_sink_id[logger::NLEVELS]; // Message sink identifiers in the loggers' signal

    //void do_log(const log_msg_info<>& a_info);
};

} // namespace utxx

namespace std {

    /// Write a newline to the buffered_print object
    inline utxx::logger::msg_streamer::helper&
    endl(utxx::logger::msg_streamer::helper& a_out)
    { a_out << '\n'; return a_out; }

    inline utxx::logger::msg_streamer::helper&
    ends(utxx::logger::msg_streamer::helper& a_out)
    { a_out << '\0'; return a_out; }

    inline utxx::logger::msg_streamer::helper&
    flush(utxx::logger::msg_streamer::helper& a_out)
    { return a_out; }
}

#include <utxx/logger/logger.ipp> // Logger implementation
