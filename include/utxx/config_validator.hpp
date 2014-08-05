//----------------------------------------------------------------------------
/// \file   config_validator.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
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
///     - <tt>type</tt> - the type of the option's name: "string" (default)
///                       or "anonymous". The former is a regular named
///                       option.  The later is an option with a variable
///                       name.  It may be useful to have options where the
///                       option's name signifies a group name of options.
///     - <tt>val_type</tt> - the type of option's value: "string", "int",
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
///     - <tt>macros</tt>     - perform environment substitution in the value.
///                       Only valid for string type. Valid values are:
///                       'false', 'true' (same as 'env'),
///                       'env', 'env-date', 'env-date-utc'.
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
//----------------------------------------------------------------------------
// Copyright (c) 2010 Omnibius, LLC
// Author:  Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-06-21
//----------------------------------------------------------------------------
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

#ifndef _UTXX_CONFIG_VALIDATOR_HPP_
#define _UTXX_CONFIG_VALIDATOR_HPP_

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

    /// Environment subsctitution type
    enum subst_env_type {
          ENV_NONE                  // No substitution
        , ENV_VARS                  // Substitute environment variables only
        , ENV_VARS_AND_DATETIME     // Substitute environment variables and datetime macros
        , ENV_VARS_AND_DATETIME_UTC // Same as ENV_VARS_AND_DATETIME but using UTC clock
    };

    typedef std::set<variant>               variant_set;
    typedef std::map<std::string, option>   option_map;
    typedef std::set<std::string>           string_set;

    const char* type_to_string(option_type_t a_type);

    struct option {

        std::string         name;
        option_type_t       opt_type;   // STRING | ANONYMOUS
        string_set          name_choices;
        variant_set         value_choices;

        option_type_t       value_type;
        // NB: Default value is a tree because variant_tree.get_child()
        // may need to return a default child tree node as a const expression
        variant_tree_base   default_value;
        variant             min_value;
        variant             max_value;

        std::string         description;
        option_map          children;
        bool                required;
        bool                unique;
        subst_env_type      subst_env;  // If true, the value may have environment
                                        // variable references that require substitution

        option() : opt_type(UNDEF), value_type(UNDEF), required(true), unique(true) {}

        option
        (
            const std::string&  a_name,
            option_type_t       a_type,
            option_type_t       a_value_type,
            const std::string&  a_desc = std::string(),
            bool                a_unique = true,
            bool                a_required = true,
            subst_env_type      a_subst_env = ENV_NONE,
            const variant&      a_def = variant(),
            const variant&      a_min = variant(),
            const variant&      a_max = variant(),
            const string_set&   a_names   = string_set(),
            const variant_set&  a_values  = variant_set(),
            const option_map&   a_options = option_map()
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
            , subst_env(a_subst_env)
        {}

        bool operator== (const option& a_rhs) const { return name == a_rhs.name; }

        std::string to_string() const;

        static std::string substitute_vars(
            subst_env_type a_type, const std::string& a_value);

        variant default_subst_value() const;
    };

    /// Performs custom validation of unrecognized options
    /// @return true if given option is valid.
    typedef std::function
        <bool (const tree_path&     a_path,
               const std::string&   a_name,
               const variant&       a_value)> custom_validator;

    class validator {
        void check_option(
            const tree_path&                        a_root,
            typename variant_tree_base::value_type& a_vt,
            const option&                           a_opt,
            bool                                    a_fill_defaults,
            const custom_validator&                 a_custom_validator
        ) const throw(std::invalid_argument, variant_tree_error);

        void check_unique(const tree_path& a_root, const variant_tree_base& a_config,
            const option_map& a_opts) const throw(variant_tree_error);

        void check_required(const tree_path& a_root, const variant_tree_base& a_config,
            const option_map& a_opts) const throw (variant_tree_error);

        static option_type_t to_option_type(variant::value_type a_type);

        tree_path format_name(const tree_path& a_root, const option& a_opt,
            const std::string& a_cfg_opt   = std::string(),
            const std::string& a_cfg_value = std::string()) const;

        bool has_required_child_options(const option_map& a_opts,
            tree_path& a_req_option_path) const;

        static std::ostream& dump(std::ostream& a_out, const std::string& a_indent,
            int a_level, const option_map& a_opts);

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

    protected:
        mutable tree_path m_root;       // Path from configuration root
        option_map        m_options;

        void validate(const tree_path& a_root, variant_tree_base& a_config,
            const option_map& a_opts, bool fill_defaults,
            const custom_validator& a_custom_validator
        ) const throw(variant_tree_error);

        void validate(variant_tree& a_config,
            const option_map& a_opts, bool fill_defaults,
            const custom_validator& a_custom_validator
        ) const throw(variant_tree_error);

        static void add_option(option_map& a, const option& a_opt) {
            a.insert(std::make_pair(a_opt.name, a_opt));
        }

        validator() {}
        virtual ~validator() {}

    public:

        /// Derived classes must implement this method
        template <class Derived>
        static const Derived* instance(const tree_path& a_root = tree_path()) {
            static Derived s_instance;
            if (!a_root.empty())
                s_instance.m_root = a_root;
            return &s_instance;
        }

        /// @return config option details
        std::string usage(const std::string& a_indent=std::string()) const;

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
        const tree_path& root()             const { return m_root;   }
        void  root(const tree_path& a_root) const { m_root = a_root; }

        void validate(variant_tree& a_config, bool a_fill_defaults,
            const custom_validator& a_custom_validator = NULL
        ) const throw(variant_tree_error);

        void validate(const variant_tree& a_config,
            const custom_validator& a_custom_validator = NULL
        ) const throw(variant_tree_error);
    };

} // namespace config
} // namespace utxx

#endif // _UTXX_CONFIG_VALIDATOR_HPP_

