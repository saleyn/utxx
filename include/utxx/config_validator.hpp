//------------------------------------------------------------------------------
/// \file   config_validator.hpp
/// \author Serge Aleynikov
//------------------------------------------------------------------------------
/// \brief Configuration verification framework.
///
/// Nearly every
/// application requires an ability to read configuration information from a
/// file.  The framework uses a utxx::config_tree class derived from
/// boost::property_tree.  Note that in order for the utxx::config_tree to
/// work correctly, the BOOST property_tree library needs to be patched as
/// per https://svn.boost.org/trac/boost/ticket/4786.
///   Configuration data is stored in any one of these formats that can be
/// parsed by boost::property_tree::read_[xml,json,info] functions:
///   * XML
///   * JSON
///   * INFO (we recommend using this formet for its simplicity)
///   An application developer should provide an XML file describing the
/// format of options used in the config file.  It's conceptually similar to
/// XLS schema.  The framework includes config_validator.xsl file that needs
/// to be used to do a source code generation of the header file (matching
/// XML file's name) that is included in the developer's project.  That file
/// implements a structure derived from config_validator.hpp:
/// utxx::confg::validator that overrides the init() function, which populates
/// the validator with rules used for validating configuration options.
///   An application that uses a configuration file needs to read its
/// configuration in the following manner (assuming that the name of the
/// config file is "app.info", the name of the auto-generated header file
/// is "app_config.hpp", and the name of the validating class in the
/// "app_config.hpp" is "test::app_config_validator":
///
/// <code>
///     #include "app_config.hpp" /* Auto-generated from user's app_config.xml */
///     ...
///     utxx::config_tree cfg;
///     boost::property_tree::read_info("app.info", cfg);
///     try {
///         test::app_config_validator::instance().
///             validate(cfg, true); /* When the second parameter
///                                     is true the l_config will be
///                                     populated with default values
///                                     of missing options */
///     } catch (util::variant_tree_error& e) {
///         /* Output config usage information */
///         std::cerr << "Configuration error in " << e.path() << ": "
///                   << e.what() << std::endl
///                   << test::app_config_validator::instance().usage()
///                   << std::endl;
///         exit(1);
///     }
/// </code>
///
/// The format of the XML file with validation rules is provided below:
///
/// <code>
///   <xml>
///     <config namespace="NAMESPACE" name="NAME">
///       OPTIONS_SECTION
///     </config>
///   </xml>
/// </code>
///
/// * <tt>/config@namespace</tt> is the namespace where the class
///   <tt>/config@name</tt> (derived from utxx::config::validator) will be
///   put.
/// * <tt>/config/option</tt> is an element that may contain an
///   optional list of <tt>/config/option/value</tt> values and the
///   following attributes:
///     - <tt>name</tt> - the name of an option.
///     - <tt>type</tt> - the type of the option's name: "string" (default),
///                       "anonymous", "branch", or "defaults". The "anonymous"
///                       option permits to have a variable value.  It may be
///                       useful to have options where the option's name
///                       signifies a group name of options. "branch" means that
///                       this option has children. "defaults" option means that
///                       this is a branch option that contains children that
///                       serve the purpose of defaults (i.e. other nodes in
///                       other branches may point to them via
///                       "../path/to/default/node" syntax).
///     - <tt>val-type</tt> - the type of option's value: "string", "int",
///                       "float", "bool".
///     - <tt>description</tt> - description of an option (used for
///                       outputting usage details).
///     - <tt>unique</tt> - true/false value indicating if the option
///                       entry by this name must be unique in the current
///                       'options' section. Default: true.
///     - <tt>default</tt> - default value for the option. If an option
///                       doesn't have a default, it is assumed to require
///                       a value. Otherwise the option is optional.
///     - <tt>min</tt> -  min value of the option (valid only for int/float).
///     - <tt>max</tt> -  max value of the option (valid only for int/float).
///     - <tt>min_length</tt> - min value length (valid only for string type).
///     - <tt>max_length</tt> - max value length (valid only for string type).
///     - <tt>validate</tt>   - boolean "true/false" indicating whether or
///                             not to perform validation of this node
/// </code>
///
/// An option may have an optional 'value' child tag. If provided, the body
/// of the tag may have at least one 'value' or 'name' entries. The 'name'
/// entry is only allowed for anonymous options used to restrict the name of
/// the options to the ones enumerated by the tag.  The 'value' entry
/// restricts the value of the option to a pre-defined list of values.
///
/// An option may have child options in its body encapsulated in the 'options'
/// tag. There is no limit in the depth of the child hierarchy of options.
//------------------------------------------------------------------------------
// Copyright (c) 2010 Omnibius, LLC
// Author:  Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-06-21
//------------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of utxx open-source project.

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
#pragma once

#include <utxx/path.hpp>
#include <utxx/error.hpp>
#include <utxx/variant.hpp>
#include <utxx/variant_tree_error.hpp>
#include <utxx/variant_tree_fwd.hpp>
#include <utxx/variant_tree_path.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <type_traits>
#include <set>
#include <list>
#include <map>

namespace utxx {
namespace config {

    class option;

    enum option_type_t {
          UNDEF
        , STRING
        , INT
        , BOOL
        , FLOAT
        , ANONYMOUS // Doesn't have a fixed name
        , BRANCH    // May not have value, but may have children
    };

    class option;

    //--------------------------------------------------------------------------
    /// Typed value with description
    //--------------------------------------------------------------------------
    template <typename T>
    class typed_val {
        T           m_value;
        std::string m_desc;

    public:
        bool operator<(const typed_val<T>& a_rhs) const {
            return m_value < a_rhs.m_value;
        };

        typed_val() {}

        typed_val(const T& a_val, const std::string& a_desc = std::string())
            : m_value(a_val), m_desc(a_desc)
        {}

        const T&            value() const { return m_value; }
        const std::string&  desc()  const { return m_desc;  }
    };

    //--------------------------------------------------------------------------
    /// Set of typed values
    //--------------------------------------------------------------------------
    template <typename T>
    struct typed_val_set
        : public std::set<typed_val<T>>
    {
        typedef std::set<typed_val<T>> base;
        typedef typename base::iterator         iterator;
        typedef typename base::const_iterator   const_iterator;

        void insert(const T& a_val, const std::string& a_desc = std::string()) {
            base::insert(typed_val<T>(a_val, a_desc));
        }
        iterator find(const T& a_val) const {
            typed_val<T> x(a_val);
            return base::find(x);
        }
    };

    typedef typed_val<std::string>        string_val;
    typedef typed_val<variant>            variant_val;
    typedef typed_val_set<std::string>    string_set;
    typedef typed_val_set<variant>        variant_set;
    typedef std::map<std::string, option> option_map;

    const char* type_to_string(option_type_t a_type);

    struct validator;

    /// Performs custom validation of unrecognized options
    /// @return true if given option is valid.
    typedef std::function
        <bool (const tree_path&     a_path,
               const std::string&   a_name,
               const variant&       a_value)> custom_validator;

    //--------------------------------------------------------------------------
    /// Error raised when required option is missing
    //--------------------------------------------------------------------------
    class missing_required_option_error : public variant_tree_error {
        using variant_tree_error::variant_tree_error;
    };

    //--------------------------------------------------------------------------
    /// Configuration option metadata
    //--------------------------------------------------------------------------
    class option {
    public:
        std::string                 name;
        option_type_t               opt_type;   // STRING | ANONYMOUS
        string_set                  name_choices;
        variant_set                 value_choices;

        option_type_t               value_type;
        // NB: Default value is a tree because variant_tree.get_child()
        // may need to return a default child tree node as a const expression
        variant_tree_base           default_value;
        variant                     min_value;
        variant                     max_value;

        std::string                 description;
        option_map                  children;
        bool                        required;
        bool                        unique;
        bool                        validate;

        /// A 'default' branch may have required=false applied recursively to all children
        bool                        recursive;

        /// Optional branch name that will be used to check for missing
        /// required options
        mutable std::string         fallback_defaults_branch_path;
        mutable const option*       fallback_defaults_branch;

        /// Optional validator for this node
        mutable const validator*    m_validator;
        /// Optional custom validator for this node
        mutable custom_validator    m_custom_validator;

        option()
            : opt_type(UNDEF), value_type(UNDEF), required(true), unique(true)
            , recursive(false)
        {}

        option
        (
            const std::string&  a_name,
            option_type_t       a_type,
            option_type_t       a_value_type,
            const std::string&  a_desc = std::string(),
            bool                a_unique = true,
            bool                a_required = true,
            bool                a_validate = true,
            const variant&      a_def = variant(),
            const variant&      a_min = variant(),
            const variant&      a_max = variant(),
            const string_set&   a_names   = string_set(),
            const variant_set&  a_values  = variant_set(),
            const option_map&   a_options = option_map(),
            const std::string&  a_defaults_fallback = std::string(),
            bool                a_recursive = false
        )
            : name(a_name), opt_type(a_type)
            , name_choices(a_names)
            , value_choices(a_values)
            , value_type(a_value_type)
            , default_value(a_def)
            , min_value(a_min), max_value(a_max)
            , description(a_desc)
            , children(a_options)
            , required(!a_required ? false : a_def.type() == variant::TYPE_NULL)
            , unique(a_unique)
            , validate(a_validate)
            , recursive(a_recursive)
            , fallback_defaults_branch_path(a_defaults_fallback)
            , fallback_defaults_branch(nullptr)
            , m_validator(nullptr)
            , m_custom_validator(nullptr)
        {}

        bool operator== (const option& a_rhs) const { return name == a_rhs.name; }

        std::string to_string() const;

        void set_validator(const validator* a_validator)  const { m_validator = a_validator;  }
        void set_validator(const custom_validator& a_val) const { m_custom_validator = a_val; }

        static std::string substitute_vars(const std::string& a_value);

        variant default_subst_value() const;
    };

    //--------------------------------------------------------------------------
    /// Configuration validator
    //--------------------------------------------------------------------------
    struct validator {
        /// Derived classes must implement this method
        template <class Derived>
        static const Derived* instance
            (const tree_path& a_root = tree_path(), const variant_tree* a_config = nullptr)
        {
            static Derived s_instance;
            if (!a_root.empty())
                s_instance.m_root = a_root;
            if (a_config)
                s_instance.m_config = a_config;
            return &s_instance;
        }

        /// @return config option details
        std::string usage
        (
            const std::string&  a_indent   = std::string(),
            bool                a_colorize = true,
            bool                a_braces   = false
        ) const;

        /// @return default variant for a given option's path.
        const variant_tree_base& def(
             const tree_path& a_path,
             const tree_path& a_root_path  = tree_path()
         ) const throw (variant_tree_error);
 
        /// Get default value for a named option.
        /// @return default value for a given option's path.
        template <class T>
        T def_value(
            const tree_path& a_path,
            const tree_path& a_root_path  = tree_path()
        ) const throw (variant_tree_error)
        {
            return def(a_path, a_root_path).data().get<T>();
        }

        /// Find option's metadata
        const option* find(const tree_path& a_path,
            const tree_path& a_root_path = tree_path()) const throw ();

        /// Find option's metadata or throw if not found
        const option& get(const tree_path& a_path,
            const tree_path& a_root_path = tree_path()) const;

        /// @return vector of configuration options
        const option_map& options() const { return m_options; }

        /// @return root path of this configuration from the XML /config/@namespace
        ///              property
        const tree_path& root()                      const { return m_root;     }
        void  root(const tree_path& a_root)          const { m_root = a_root;   }

        const variant_tree_base* config()            const { return m_config;   }
        void  config(const variant_tree_base& a_cfg) const { m_config = &a_cfg; }

        void set_validator(const tree_path& a_path, const validator* a_validator)  const;
        void set_validator(const tree_path& a_path, const custom_validator& a_val) const;

        void validate(variant_tree& a_config, bool a_fill_defaults,
            const custom_validator& a_custom_validator = NULL
        ) const throw(variant_tree_error);

        void validate(const variant_tree& a_config,
            const custom_validator& a_custom_validator = NULL
        ) const throw(variant_tree_error);

        // Validate configuration tree stored in config()
        void validate(const custom_validator& a_custom_validator = NULL)
            const throw(variant_tree_error);

    protected:
        mutable tree_path                m_root;   // Path from configuration root
        mutable const variant_tree_base* m_config; // If set, the derived class can use typed
                                                   // accessor methods as syntactic sugar to
                                                   // access named option values
        option_map                       m_options;

        /// No direct construction - always derive implementations from this class
        validator() : m_config(nullptr) {}
        virtual ~validator() {}

        void validate(variant_tree& a_config,
            const option_map& a_opts, bool fill_defaults,
            const custom_validator& a_custom_validator
        ) const throw(variant_tree_error);

        static void add_option(option_map& a, const option& a_opt) {
            a.insert(std::make_pair(a_opt.name, a_opt));
        }

        void preprocess();

    private:
        bool m_preprocessed = false;

        struct config_level {
            config_level(const tree_path&  a_path, const variant_tree_base& a_cfg,
                         const option*     a_opt,  const option_map&        a_opts)
                : m_path(a_path),  m_config(a_cfg)
                , m_option(a_opt), m_options(a_opts)
            {}

            tree_path  const&         path()    const { return m_path;    }
            variant_tree_base const&  cfg()     const { return m_config;  }
            option     const*         opt()     const { return m_option;  }
            option_map const&         options() const { return m_options; }

            void opt(const option* a_opt) { m_option = a_opt; }
        private:
            tree_path                 m_path;
            const variant_tree_base&  m_config;
            const option*             m_option;
            const option_map&         m_options;
        };

        struct config_level_list : public std::list<config_level> {
            config_level_list&
            push(const tree_path&  a_root, const variant_tree_base& a_config,
                 const option*     a_opt,  const option_map&        a_opts) {
                this->emplace_back(a_root, a_config, a_opt, a_opts);
                return *this;
            }
            void pop() { if (!this->empty()) this->pop_back(); }
        };

        using stack_t = std::list<std::pair<std::string, option_map*>>;

        void recursive_validate(
            config_level_list&      a_stack,
            bool                    a_fill_defaults,
            const custom_validator& a_custom_validator
        ) const throw(variant_tree_error);

        void check_option(
            config_level_list&                      a_stack,
            typename variant_tree_base::value_type& a_vt,
            bool                                    a_fill_defaults,
            const custom_validator&                 a_custom_validator
        ) const throw(std::invalid_argument, variant_tree_error);

        void check_unique(const tree_path& a_root, const variant_tree_base& a_config,
            const option_map& a_opts) const throw(variant_tree_error);

        void check_required(config_level_list& a_stack) const
            throw (missing_required_option_error, variant_tree_error);

        static option_type_t to_option_type(variant::value_type a_type);

        tree_path format_name(const tree_path& a_root, const option& a_opt,
            const std::string& a_cfg_opt   = std::string(),
            const variant&     a_cfg_value = variant()) const;

        bool has_required_child_options(const option_map& a_opts,
            tree_path& a_req_option_path) const;

        static std::ostream& dump(std::ostream& a_out, const std::string& a_indent,
            int a_level, const option_map& a_opts, bool a_colorize = true,
            bool a_braces = false);

        static inline bool all_anonymous(const option_map& a_opts) {
            BOOST_FOREACH(const typename option_map::value_type& ovt, a_opts)
                if (ovt.second.opt_type != ANONYMOUS)
                    return false;
            return true;
        }

        tree_path strip_root(const tree_path& a_root_path,
                             const tree_path& a_root = tree_path()) const
            throw(variant_tree_error);

        static const option* find(tree_path& a_suffix, const option_map& a_options)
            throw ();

        void internal_fill_fallback_defaults
            (stack_t& a_stack, option_map& a_scope, std::string const& a_def_branch);
    };

} // namespace config
} // namespace utxx