//----------------------------------------------------------------------------
/// \file config_validator.hpp
//----------------------------------------------------------------------------
/// This file is a part of configuration verification framework.  Nearly every
/// application requires an ability to read configuration information from a
/// file.  The framework uses a util::config_tree class derived from
/// boost::property_tree.  Note that in order for the util::config_tree to
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
/// util::confg::validator that overrides the init() function, which populates
/// the validator with rules used for validating configuration options.
///   An application that uses a configuration file needs to read its
/// configuration in the following manner (assuming that the name of the
/// config file is "app.info", the name of the auto-generated header file
/// is "app_config.hpp", and the name of the validating class in the
/// "app_config.hpp" is "test::app_config_validator":
///
/// <code>
///     #include "app_config.hpp"
///     ...
///     util::config_tree l_config;
///     boost::property_tree::read_info("app.info", l_config);
///     test::app_config_validator l_validator;
///     l_validator.validate(l_config, true); /* When the second parameter
///                                              is true the l_config will be
///                                              populated with default values
///                                              of missing options */
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
///   <tt>/config@name</tt> (derived from util::config::validator) will be
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
///     - <tt>value_type</tt> - the type of option's value: "string", "int",
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

This file may be included in various open-source projects.

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

/* TODO: following schema is incomplete

    <?xml version="1.0"?>
    <xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">

    <xs:element name="config">
      <xs:complexType>
        <xs:attribute name="namespace" type="xs:string"/>
        <xs:attribute name="name"      type="xs:string"/>
        <xs:element name="options">
          <xs:complexType>
            <xs:sequence>
              <xs:element name="option" type="xs:string"/>
                <xs:complexType>
                  <xs:attribute name="name" type="xs:string" use="required"/>
                  <xs:attribute name="type">
                    <xs:simpleType>
                      <xs:restriction base="xs:string">
                        <xs:enumeration value="string"/>
                        <xs:enumeration value="bool"/>
                      </xs:restriction>
                    </xs:simpleType>
                  </xs:attribute>
                  <xs:attribute name="val_type">
                    <xs:simpleType>
                      <xs:restriction base="xs:string">
                        <xs:enumeration value="string"/>
                        <xs:enumeration value="int"/>
                        <xs:enumeration value="float"/>
                        <xs:enumeration value="bool"/>
                      </xs:restriction>
                    </xs:simpleType>
                  </xs:attribute>
                  <xs:attribute name="description"  type="xs:string"/>
                  <xs:attribute name="unique"       type="xs:boolean"/>
                  <xs:attribute name="default"/>
                  <xs:attribute name="min"    />
                  <xs:attribute name="max"    />
                  <xs:element name="choices">
                    <xs:complexType>
                      <xs:choice maxOccurs="unbounded">
                        <xs:choice name="name"/>
                        <xs:choice name="value"/>
                      </xs:choice>
                    </xs:complexType>
                  </xs:element>
                  <xs:element ref="options" minOccurs="0" maxOccurs="unbounded"/>
                </xs:complexType>
              </xs:element>
            </xs:sequence>
          </xs:complexType>
        </xs:element>
      </xs:complexType>
    </xs:element>

    </xs:schema>

*/

#ifndef _UTIL_CONFIG_VALIDATOR_HPP_
#define _UTIL_CONFIG_VALIDATOR_HPP_

#include <util/config_tree.hpp>
#include <util/error.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <set>
#include <list>
#include <map>

namespace util {
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

    typedef std::set<variant>               variant_set;
    typedef std::map<std::string, option>   option_map;
    typedef std::set<std::string>           string_set;

    const char* type_to_string(option_type_t a_type);

    struct option {

        std::string     name;
        option_type_t   opt_type;   // Only STRING & ANONYMOUS are allowed
        string_set      name_choices;
        variant_set     value_choices;

        option_type_t   value_type;
        variant         default_value;
        variant         min_value;
        variant         max_value;

        std::string     description;
        option_map      children;
        bool            required;
        bool            unique;

        option() : opt_type(UNDEF), value_type(UNDEF), required(true), unique(true) {}

        option( const std::string& a_name,
                option_type_t a_type, option_type_t a_value_type,
                const std::string& a_desc = std::string(),
                bool a_unique = true,
                bool a_required = true,
                const variant& a_def = variant(),
                const variant& a_min = variant(),
                const variant& a_max = variant(),
                const string_set&   a_names   = string_set(),
                const variant_set&  a_values  = variant_set(),
                const option_map&   a_options = option_map())
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
        {}

        bool operator== (const option& a_rhs) const { return name == a_rhs.name; }

        std::string to_string() const;
    };

    template <class Derived>
    class validator {
        void check_option(const config_path& a_root, config_tree::value_type& a_vt,
            const option& a_opt, bool a_fill_defaults) const throw(config_error);

        void check_unique(const config_path& a_root, const config_tree& a_config,
            const option_map& a_opts) const throw(config_error);

        void check_required(const config_path& a_root, const config_tree& a_config,
            const option_map& a_opts) const throw (config_error);

        static option_type_t to_option_type(variant::value_type a_type);

        config_path format_name(const config_path& a_root, const option& a_opt,              
            const std::string& a_cfg_opt   = std::string(),
            const std::string& a_cfg_value = std::string()) const;

        bool has_required_child_options(const option_map& a_opts,
            config_path& a_req_option_path) const;

        static std::ostream& dump(std::ostream& a_out, const std::string& a_indent,
            int a_level, const option_map& a_opts);

        static inline bool all_anonymous(const option_map& a_opts) {
            BOOST_FOREACH(const typename option_map::value_type& ovt, a_opts)
                if (ovt.second.opt_type != ANONYMOUS)
                    return false;
            return true;
        }

        static const variant& find_default(
            const config_path& a_option, config_path& a_suffix,
            const option_map& a_vec) throw (config_error);

    protected:
        config_path m_root;       // Path from configuration root
        option_map  m_options;

        static Derived init_once() { Derived v; return v.init(); }

        void validate(const config_path& a_root, config_tree& a_config,
            const option_map& a_opts, bool fill_defaults) const throw(config_error);

        static void add_option(option_map& a, const option& a_opt) {
            a.insert(std::make_pair(a_opt.name, a_opt));
        }

        validator() {}
    public:
        virtual ~validator() {}

        static const Derived& instance() {
            static Derived s_instance = init_once();
            return s_instance;
        }

        virtual const Derived& init() = 0;

        /// @return config option details
        std::string usage(const std::string& a_indent=std::string()) const;

        /// Get default value for a named option.
        /// @return default value for a given option's path.
        const variant& default_value(const config_path& a_path)
            const throw (config_error);

        /// Get configuration a_option of type <tt>T</tt> from \a a_config tree.
        /// If the option is not found and there's a default value, that
        /// value will be returned.  Otherwise if the option is required and there is
        /// no default, an exception is thrown.
        template <class T>
        T get(const config_path& a_option, const config_tree& a_config) const
                throw(config_error) {
            try { return a_config.get<T>(a_option.dump()); } catch (config_bad_path&) {
                return default_value(a_option).get<T>();
            }
        }

        inline void clear() { m_options.clear(); }

        /// @return vector of configuration options
        const option_map& options() const { return m_options; }

        void validate(config_tree& a_config, bool a_fill_defaults,
                const config_path& a_root = config_path()) const throw(config_error) {
            validate(a_root, a_config, m_options, a_fill_defaults);
        }

        void validate(const config_tree& a_config,
                const config_path& a_root = config_path()) const throw(config_error) {
            config_tree l_config(a_config);
            validate(l_config, false, a_root);
        }
    };

} // namespace config
} // namespace util

#include <util/config_validator.ipp>

#endif // _UTIL_CONFIG_VALIDATOR_HPP_

