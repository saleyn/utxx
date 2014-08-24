//----------------------------------------------------------------------------
/// \file  config_validator.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of configuration validation class.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-09-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/variant_tree.hpp>
#include <utxx/config_validator.hpp>
#include <utxx/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <regex>
#include <iomanip>

namespace utxx {
namespace config {

static std::string value(const variant& v) {
    return (v.type() == variant::TYPE_STRING)
        ? std::string("\"") + v.to_string() + "\""
        : v.to_string();
}

const char* type_to_string(option_type_t a_type) {
    switch (a_type) {
        case STRING:    return "string";
        case INT:       return "int";
        case BOOL:      return "bool";
        case FLOAT:     return "float";
        case ANONYMOUS: return "anonymous";
        case BRANCH:    return "branch";
        default:        return "undefined";
    }
}

std::string option::to_string() const {
    std::stringstream s;
    s   << "option{name=" << name
        << ",type=" << type_to_string(opt_type);
    if (!description.empty())
        s << ",desc=\"" << description << '"';
    if (name_choices.size()) {
        s << ",names=["; bool l_first = true;
        BOOST_FOREACH(const string_val& v, name_choices) {
            if (!l_first) s << ";";
            l_first = false;
            s << '"' << v.value() << '"';
        }
        s << ']';
    }
    if (value_choices.size()) {
        s << ",values=["; bool l_first = true;
        BOOST_FOREACH(const variant_val& v, value_choices) {
            if (!l_first) s << ";";
            l_first = false;
            s << value(v.value());
        }
        s << "]";
    }
    if (!default_value.data().is_null())
        s << ",default=" << value(default_value.data());
    if (!min_value.is_null())
        s << (value_type == STRING ? ",min_length=" : ",min=") << value(min_value);
    if (!max_value.is_null())
        s << (value_type == STRING ? ",max_length=" : ",max=") << value(max_value);
    s << ",required="   << utxx::to_string(required);
    s << ",unique="     << utxx::to_string(unique);
    if (children.size()) {
        bool l_first = true;
        s << ",children=[";
        BOOST_FOREACH(const option_map::value_type& o, children) {
            if (!l_first) s << ",";
            l_first = false;
            s << "\n  " << o.second.to_string();
        }
        s << "\n]";
    }
    s << "}";
    return s.str();
}

std::string option::substitute_vars(const std::string& a_value) {
    namespace pt = boost::posix_time;
    struct tm tm = pt::to_tm(pt::second_clock::local_time());
    return path::replace_env_vars(a_value, &tm);
}

variant option::default_subst_value() const {
    return default_value.data().is_string()
         ? variant(substitute_vars(default_value.data().to_str()))
         : default_value.data();
}

tree_path validator::
strip_root(const tree_path& a_path, const tree_path& a_root) const throw(variant_tree_error)
{
    /// Example: m_root = a.b.c
    ///          a_path = a.b.c.d.e
    ///          Return ->      d.e

    /// Example: m_root = a.b
    ///          a_root = a.b.c
    ///          a_path = a.b.c.d.e
    ///          Return ->      d.e

    char sep = a_path.separator();
    std::string s(a_path.dump());
    std::string r(a_root.empty() ? m_root.dump() : a_root.dump());
    if (r.empty())
        return a_path;
    if (s.substr(0, r.size()) == r) {
        size_t n = r.size() + (s[r.size()] == sep ? 1 : 0);
        return s.erase(0, n);
    }
    if (s.size() < r.size() && r.substr(0, s.size()) == s)
        throw variant_tree_error(a_path, "Path is shorter than root!");

    return a_path;
}

const option* validator::
find(tree_path& a_suffix, const option_map& a_options) throw ()
{
    if (a_suffix.empty())
        return NULL;

    std::string s = a_suffix.reduce();
    size_t n = s.find_first_of('[');
    if (n != std::string::npos)
        s.erase(n);

    option_map::const_iterator it = a_options.find(s);
    if (it == a_options.end())
        return NULL;
    else if (a_suffix.empty())
        return &it->second;

    return find(a_suffix, it->second.children);
}

const option* validator::find
(
    const tree_path& a_path,
    const tree_path& a_root_path
)
    const throw ()
{
    try {
        tree_path p = strip_root(a_path, a_root_path);
        return find(p, m_options);
    } catch (variant_tree_error&) {
        return NULL;
    }
}

const option& validator::get(const tree_path& a_path, const tree_path& a_root) const
{
    tree_path p(strip_root(a_path, a_root));
    const option* o = find(p, m_options);
    if (o) return *o;
    tree_path ep(p.dump());
    throw variant_tree_error(
        (a_root.empty() ? m_root : a_root) / ep, "Configuration option not found!");
}

const variant_tree_base& validator::def
(
    const tree_path& a_path,
    const tree_path& a_root_path
)
    const throw (variant_tree_error)
{
    return get(a_path, a_root_path).default_value;
}

tree_path validator::format_name
(
    const tree_path& a_root,
    const option& a_opt, const std::string& a_cfg_opt,
    const std::string& a_cfg_value
)
    const
{
    tree_path s = a_root / a_opt.name;
    if (!a_cfg_opt.empty() && a_cfg_opt != a_opt.name) // && a_opt.opt_type == ANONYMOUS)
        s /= a_cfg_opt;
    if (!a_cfg_value.empty()) // && !a_opt.unique)
        s = s.dump() + '[' + a_cfg_value + ']';
    return s;
}

void validator::validate
(
    variant_tree&           a_config,
    bool                    a_fill_defaults,
    const custom_validator& a_custom_validator
)
    const throw(variant_tree_error)
{
    validate(a_config, m_options, a_fill_defaults, a_custom_validator);
}

void validator::validate
(
    const variant_tree&     a_config,
    const custom_validator& a_custom_validator
)
    const throw(variant_tree_error)
{
    variant_tree l_config(a_config);
    validate(l_config, false, a_custom_validator);
}

void validator::validate(const custom_validator& a_custom_validator)
    const throw(variant_tree_error)
{
    if (!m_config || m_config->empty())
        throw variant_tree_error(std::string(), "validator: unassigned field 'config()'!");
    // Note that the following const cast is safe - since fill_defaults is false
    // the tree will not be updated
    validate(m_root, *const_cast<variant_tree_base*>(m_config),
             m_options, false, a_custom_validator);
}

void validator::validate
(
    variant_tree&           a_config,
    const option_map&       a_opts,
    bool                    a_fill_defaults,
    const custom_validator& a_custom_validator
)
    const throw(variant_tree_error)
{
    validate(a_config.root_path(), a_config.to_base(), a_opts, a_fill_defaults,
             a_custom_validator);
}

void validator::validate
(
    const tree_path& a_root, variant_tree_base& a_config,
    const option_map& a_opts, bool a_fill_defaults,
    const custom_validator& a_custom_validator
)
    const throw (variant_tree_error)
{
    check_unique(a_root, a_config, a_opts);
    check_required(a_root, a_config, a_opts);

    // If the options tree is arranged such that all options are children
    // of a single branch, then don't validate non-matching named options
    // at the branch level. E.g.:
    //   Suppose we have a signle config tree, and try to validate a component
    //   called 'strategy' that has its own validator, which doesn't include
    //   definition of 'logger' that is defined using a separate validator:
    //      strategy { a=1,   b=2 }
    //      logger   { file="abc" }
    //
    //   then the variable below detects the options tree layout and prevents
    //   the validation error that the 'logger' sub-tree is undefined.
    bool single_nested_tree =
        (m_options.size() == 1 && m_options.begin()->second.children.size() > 0);

    BOOST_FOREACH(variant_tree::value_type& vt, a_config) {
        bool l_match = false;
        BOOST_FOREACH(const typename option_map::value_type& ovt, a_opts) {
            const option& opt = ovt.second;
            if (opt.opt_type == ANONYMOUS) {
                if (!all_anonymous(a_opts))
                    throw variant_tree_error(format_name(a_root, opt,
                        vt.first, vt.second.data().to_string()),
                        "Check XML spec. Cannot mix anonymous and named options "
                        "in one section!");
                if (opt.validate)
                    check_option(a_root, vt, opt, a_fill_defaults, a_custom_validator);
                l_match = true;
                break;
            } else if (opt.name == vt.first) {
                if (opt.validate)
                    check_option(a_root, vt, opt, a_fill_defaults, a_custom_validator);
                l_match = true;
                break;
            } else if (single_nested_tree) {
                l_match = true;
                break;
            }
        }
        if (!l_match) {
            tree_path p; p /= vt.first;
            if (a_custom_validator)
                l_match = a_custom_validator(a_root, vt.first, vt.second.data());
            if (!l_match)
                throw variant_tree_error(p, "Unsupported config option!");
        }
    }
}

void validator::check_required
(
    const tree_path& a_root,
    const variant_tree_base& a_config, const option_map& a_opts
)
    const throw (variant_tree_error)
{
    #ifdef TEST_CONFIG_VALIDATOR
    std::cout << "check_required(" << a_root << ", cfg.count=" << a_config.size()
        << ", opts.count=" << a_opts.size() << ')' << std::endl;
    #endif
    BOOST_FOREACH(const typename option_map::value_type& ovt, a_opts) {
        const option& opt = ovt.second;
        if (opt.required && opt.default_value.data().is_null()) {
            #ifdef TEST_CONFIG_VALIDATOR
            std::cout << "  checking_option(" << format_name(a_root, opt) << ")"
                << (opt.opt_type == ANONYMOUS ? " [anonymous]" : "")
                << (opt.unique ? " [unique]" : "")
                << " [required, no default]"
                << std::endl;
            #endif
            if (opt.opt_type == ANONYMOUS) {
                if (a_config.empty())
                    throw variant_tree_error(format_name(a_root, opt),
                        "Check XML spec. Missing required value of anonymous option!");
            } else {
                bool l_found = false;
                BOOST_FOREACH(const variant_tree::value_type& vt, a_config)
                    if (vt.first == opt.name) {
                        #ifdef TEST_CONFIG_VALIDATOR
                        std::cout << "    found: "
                            << format_name(a_root, opt, vt.first, vt.second.data().to_string())
                            << ", value=" << vt.second.data().to_string()
                            << ", type=" << type_to_string(opt.opt_type)
                            << std::endl;
                        #endif

                        if (opt.opt_type == BRANCH) {
                            l_found = true;
                            break;
                        }

                        if (vt.second.data().is_null())
                            throw variant_tree_error(format_name(a_root, opt,
                                    vt.first, vt.second.data().is_null()
                                                ? std::string()
                                                : vt.second.data().to_string()),
                                "Missing value of the required option "
                                "and no default provided!");
                        l_found = true;
                        if (opt.unique)
                            break;
                    }

                if (!l_found && (opt.opt_type != BRANCH || opt.children.empty()))
                    throw variant_tree_error(format_name(a_root, opt),
                        "Missing required ", (opt.opt_type == BRANCH ? "branch" : "option"),
                        " with no default!");
            }
        }
        #ifdef TEST_CONFIG_VALIDATOR
        else {
            std::cout << "  option(" << format_name(a_root, opt)
                << ") is " << (opt.required ? "required" : "not required")
                << " and " << (opt.default_value.data().is_null()
                    ? "no default"
                    : (std::string("has default=") + opt.default_value.data().to_string()))
                << std::endl;
        }
        #endif

        if (opt.opt_type == ANONYMOUS) {
            #ifdef TEST_CONFIG_VALIDATOR
            if (a_config.size())
                std::cout << "  Checking children of anonymous node "
                    << format_name(a_root, opt) << std::endl;
            #endif
            BOOST_FOREACH(const variant_tree::value_type& vt, a_config)
                check_required(
                    format_name(a_root, opt, vt.first, vt.second.data().to_string()),
                    vt.second, opt.children);
        } else {
            #ifdef TEST_CONFIG_VALIDATOR
            if (opt.children.size())
                std::cout << "  Checking children of " << format_name(a_root, opt) << std::endl;
            #endif
            tree_path l_req_name;
            bool l_has_req = has_required_child_options(opt.children, l_req_name);
            bool l_found   = false;

            BOOST_FOREACH(const variant_tree::value_type& vt, a_config)
                if (vt.first == opt.name) {
                    l_found = true;
                    if (l_has_req) {
                        if (!vt.second.size())
                            throw variant_tree_error(format_name(a_root, opt,
                                    vt.first, vt.second.data().to_string()),
                                    "Option is missing required child option: ",
                                    l_req_name.dump());
                        check_required(
                            format_name(a_root, opt, vt.first, vt.second.data().to_string()),
                            vt.second, opt.children);
                    }
                    if (!opt.children.size() && vt.second.size())
                        throw variant_tree_error(format_name(a_root, opt, vt.first,
                                vt.second.data().to_string()),
                            "Option is not allowed to have child nodes!");
                }

            if (!l_found && l_has_req)
                throw variant_tree_error(format_name(a_root, opt),
                                         "Missing a required child option: ",
                                         l_req_name.dump());
        }
    }
}


void validator::
check_option
(
    const tree_path& a_root,
    typename variant_tree_base::value_type& a_vt,
    const option& a_opt, bool a_fill_defaults,
    const custom_validator& a_custom_validator
)
    const throw(std::invalid_argument, variant_tree_error)
{
    try {
        // Populate default value
        if (!a_opt.required && a_vt.second.data().is_null()) {
            if (a_opt.default_value.data().is_null() && a_opt.opt_type != BRANCH)
                throw std::invalid_argument("Check XML spec. Required option is missing default value!");
            BOOST_ASSERT(
                (a_opt.opt_type == BRANCH && a_opt.default_value.data().is_null()) ||
                (to_option_type(a_opt.default_value.data().type()) == a_opt.value_type));
            if (a_fill_defaults && !a_opt.default_value.data().is_null())
                a_vt.second.data() = a_opt.default_value.data();
        }

        switch (a_opt.value_type) {
            case STRING:
                if (!a_vt.second.data().is_null() && a_vt.second.data().type() != variant::TYPE_STRING)
                    throw std::invalid_argument("Wrong type - expected string!");
                if (!a_opt.min_value.is_null() &&
                     a_vt.second.data().to_str().size() < (size_t)a_opt.min_value.to_int())
                    throw std::invalid_argument("String value too short!");
                if (!a_opt.max_value.is_null() &&
                     a_vt.second.data().to_str().size() > (size_t)a_opt.max_value.to_int())
                    throw std::invalid_argument("String value too long!");
                break;
            case INT:
                if (!a_vt.second.data().is_null() && a_vt.second.data().type() != variant::TYPE_INT)
                    throw std::invalid_argument("Wrong type - expected integer!");
                if (!a_opt.min_value.is_null() && a_opt.min_value > a_vt.second.data())
                    throw std::invalid_argument("Value too small!");
                if (!a_opt.max_value.is_null() && a_opt.max_value < a_vt.second.data())
                    throw std::invalid_argument("Value too large!");
                break;
            case BOOL:
                if (!a_vt.second.data().is_null() && a_vt.second.data().type() != variant::TYPE_BOOL)
                    throw std::invalid_argument("Wrong type - expected boolean true/false!");
                break;
            case FLOAT:
                if (!a_vt.second.data().is_null() && a_vt.second.data().type() != variant::TYPE_DOUBLE)
                    throw std::invalid_argument("Wrong type - expected float!");
                if (!a_opt.min_value.is_null() && a_opt.min_value > a_vt.second.data())
                    throw std::invalid_argument("Value too small!");
                if (!a_opt.max_value.is_null() && a_opt.max_value < a_vt.second.data())
                    throw std::invalid_argument("Value too large!");
                break;
            default: {
                // Allow anonymous options to have no value (since the
                // name defines the value)
                if (a_opt.opt_type == ANONYMOUS || a_opt.opt_type == BRANCH)
                    break;
                throw variant_tree_error(format_name(a_root, a_opt, a_vt.first,
                        a_vt.second.data().to_string()),
                    "Check XML spec. Option's value_type '",
                    type_to_string(a_opt.value_type),
                    "' is invalid!");
            }
        }

        if (a_opt.required &&
                a_opt.opt_type != ANONYMOUS &&
                a_opt.opt_type != BRANCH &&
                a_vt.second.data().is_null())
            throw std::invalid_argument("Required value missing!");
        if (a_vt.first.empty())
            throw std::invalid_argument("Expected non-empty name!");

        switch (a_opt.opt_type) {
            case STRING:    break;
            case ANONYMOUS: break;
            case BRANCH:    break;
            case INT:       break;
            case FLOAT:     break;
            case BOOL:      break;
            default: {
                throw variant_tree_error(format_name(a_root, a_opt, a_vt.first,
                        a_vt.second.data().to_string()),
                    "Check XML spec. Unsupported type of option: ",
                    type_to_string(a_opt.opt_type));
            }
        }

        if (!a_opt.name_choices.empty()) {
            if (a_opt.opt_type != ANONYMOUS)
                throw variant_tree_error(format_name(a_root, a_opt, a_vt.first,
                        a_vt.second.data().to_string()),
                    "Check XML spec. Non-anonymous option cannot have name choices!");
            if (a_opt.name_choices.find(a_vt.first) == a_opt.name_choices.end())
                throw std::invalid_argument("Invalid name given to anonymous option!");
        }

        if (!a_opt.value_choices.empty())
            if (a_opt.value_choices.find(a_vt.second.data()) == a_opt.value_choices.end()) {
                throw variant_tree_error(format_name(a_root, a_opt,
                        a_vt.first, a_vt.second.data().to_string()),
                    "Value is not allowed for option!");
            }
        if (!a_opt.children.empty())
            validate(a_root / a_opt.name, a_vt.second, a_opt.children, a_fill_defaults,
                a_custom_validator);
    } catch (std::invalid_argument& e) {
        throw variant_tree_error(format_name(a_root, a_opt, a_vt.first,
                a_vt.second.data().to_string()),
                e.what());
    }
}

option_type_t validator::to_option_type(variant::value_type a_type) {
    switch (a_type) {
        case variant::TYPE_STRING:  return STRING;
        case variant::TYPE_INT:     return INT;
        case variant::TYPE_BOOL:    return BOOL;
        case variant::TYPE_DOUBLE:  return FLOAT;
        default:                    return UNDEF;
    }
}

std::string validator::usage(const std::string& a_indent, bool a_colorize) const {
    std::stringstream s;
    dump(s, a_indent, 0, m_options, a_colorize);
    return s.str();
}

std::ostream& validator::dump
(
    std::ostream& out, const std::string& a_indent,
    int a_level, const option_map& a_opts, bool a_colorize
) {
    static const char GREEN[]  = "\E[1;32;40m";
    static const char YELLOW[] = "\E[1;33;40m";
    static const char NORMAL[] = "\E[0m";
    std::regex eol_re("\n *");

    std::string       l_indent = a_indent + std::string(a_level, ' ');
    const std::string l_nl_15  = l_indent + std::string(15, ' ');

    BOOST_FOREACH(const typename option_map::value_type& ovt, a_opts) {
        const option& opt = ovt.second;
        out << l_indent << (a_colorize ? GREEN  : "")
            << opt.name << (a_colorize ? NORMAL : "")
            << (opt.opt_type == ANONYMOUS ? " (anonymous): " : ": ")
            << (a_colorize ? YELLOW : "")
            << type_to_string(opt.value_type) << (a_colorize ? NORMAL : "")
            << std::endl;
        if (!opt.description.empty()) {
            auto desc = std::regex_replace(opt.description, eol_re, "\n"+l_nl_15,
                                           std::regex_constants::match_any);
            out << l_indent << "  Description: " << desc << std::endl;
        }
        if (!opt.unique)
            out << l_indent << "       Unique: true" << std::endl;
        if (!opt.required) {
            if (!opt.default_value.data().is_null())
                out << l_indent << "      Default: "
                    << value(opt.default_value.data()) << std::endl;
        } else
            out << l_indent << "     Required: true" << std::endl;

        if (!opt.validate)
            out << l_indent << "     Validate: false" << std::endl;

        if (!opt.min_value.is_null() || !opt.max_value.is_null()) {
            out << l_indent << "         "
                << (!opt.min_value.is_null()
                        ? std::string(opt.value_type == STRING
                                       ? "MinLength: " : " Min: ")
                            + value(opt.min_value)
                        : std::string())
                << (!opt.max_value.is_null()
                        ? std::string(opt.value_type == STRING
                                       ? "MaxLength: " : " Max: ")
                            + value(opt.max_value)
                        : std::string())
                << std::endl;
        }
        if (!opt.name_choices.empty()) {
            std::vector<string_val> r(opt.name_choices.size());
            std::copy(opt.name_choices.begin(), opt.name_choices.end(), r.begin());
            std::sort(r.begin(), r.end());
            out << l_indent << "        Names: ";
            bool first    = true;
            bool has_desc = false;
            int  max_len  = 0;
            for (const auto& v : r) {
                has_desc |= !v.desc().empty();
                max_len   = std::max<int>(max_len, v.value().size());
            }
            for (const auto& v : r) {
                out << (first ? "" : l_nl_15);
                if (has_desc) out << std::setw(max_len+1) << std::left;
                out << v.value();
                if (has_desc) out << "|";
                if (!v.desc().empty()) {
                    std::string pfx =
                        "\n" + l_indent + std::string(max_len+2,' ') + "| ";
                    out << " " << std::regex_replace(v.desc(), eol_re, pfx,
                                                     std::regex_constants::match_any);
                }
                out << std::endl;
                first = false;
            }
        }
        if (!opt.value_choices.empty()) {
            std::vector<variant_val> r(opt.value_choices.size());
            std::copy(opt.value_choices.begin(), opt.value_choices.end(), r.begin());
            std::sort(r.begin(), r.end());
            out << l_indent << "       Values: ";
            bool first    = true;
            bool has_desc = false;
            int  max_len  = 0;
            for (const auto& v : r) {
                has_desc |= !v.desc().empty();
                max_len   = std::max<int>(max_len, v.value().to_string().size());
            }
            for (const auto& v : opt.value_choices) {
                out << (first ? "" : l_nl_15);
                if (has_desc) out << std::setw(max_len+1) << std::left;
                out << v.value().to_string();
                if (has_desc) out << "|";
                if (!v.desc().empty()) {
                    std::string pfx =
                        "\n" + l_indent + std::string(max_len+2,' ') + "| ";
                    out << " " << std::regex_replace(v.desc(), eol_re, pfx,
                                                     std::regex_constants::match_any);
                }
                out << std::endl;
                first = false;
            }
        }
        if (opt.children.size())
            dump(out, l_indent, a_level+2, opt.children, a_colorize);
    }
    return out;
}

void validator::check_unique
(
    const tree_path& a_root,
    const variant_tree_base& a_config,
    const option_map& a_opts
) const throw(variant_tree_error) {
    string_set l_names;
    BOOST_ASSERT(a_opts.size() > 0);
    BOOST_FOREACH(const variant_tree::value_type& vt, a_config) {
        if (l_names.find(vt.first) == l_names.end())
            l_names.insert(vt.first);
        else {
            BOOST_FOREACH(const typename option_map::value_type& ovt, a_opts) {
                const option& o = ovt.second;
                if (o.name == vt.first && o.unique)
                    throw variant_tree_error(format_name(a_root, o, vt.first,
                          vt.second.data().to_string()),
                          "Non-unique config option found!");
            }
        }
    }
}

bool validator::has_required_child_options
(
    const option_map& a_opts,
    tree_path& a_req_option_path
) const
{
    BOOST_FOREACH(const typename option_map::value_type& ovt, a_opts) {
        const option& opt = ovt.second;
        tree_path l_path = a_req_option_path / opt.name;
        if (opt.required) {
            a_req_option_path = l_path;
            return true;
        }
        if (has_required_child_options(opt.children, l_path)) {
            a_req_option_path = l_path;
            return true;
        }
    }
    return false;
}


} // namespace config
} // namespace utxx
