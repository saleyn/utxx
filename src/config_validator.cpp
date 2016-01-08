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
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <regex>
#include <iomanip>

using namespace std;

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
        for (auto& v : name_choices) {
            if (!l_first) s << ";";
            l_first = false;
            s << '"' << v.value() << '"';
        }
        s << ']';
    }
    if (value_choices.size()) {
        s << ",values=["; bool l_first = true;
        for (auto& v : value_choices) {
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
        for (auto& o : children) {
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

void validator::set_validator(
    const tree_path& a_path, const validator* a_validator
) const
{
    const option* opt = find(a_path);
    if (!opt)
        throw variant_tree_error(a_path, "Option path is not found!");
    opt->set_validator(a_validator);
}

void validator::set_validator(
    const tree_path& a_path, const custom_validator& a_val
) const
{
    const option* opt = find(a_path);
    if (!opt)
        throw variant_tree_error(a_path, "Option path is not found!");
    opt->set_validator(a_val);
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
    const variant& a_cfg_value
)
    const
{
    tree_path s = a_root / a_opt.name;
    if (!a_cfg_opt.empty() && a_cfg_opt != a_opt.name) // && a_opt.opt_type == ANONYMOUS)
        s /= a_cfg_opt;
    if (!a_cfg_value.is_null()) // && !a_opt.unique)
        s = s.dump() + '[' + a_cfg_value.to_string() + ']';
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
    config_level_list stack;
    stack.push(m_root, *const_cast<variant_tree_base*>(m_config), nullptr, m_options);

    recursive_validate(stack, false, a_custom_validator);
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
    config_level_list stack;
    stack.push(a_config.root_path(), a_config.to_base(), nullptr, a_opts);
    recursive_validate(stack, a_fill_defaults, a_custom_validator);
}

void validator::internal_fill_fallback_defaults
    (stack_t& stack, option_map& a_scope, std::string const& a_def_branch)
{
    auto thrower = [&, this](std::string const& a_opt, std::string const& a_path, int n) {
        auto  joiner = [](auto& pair){ return pair.first; };
        auto  path   = join(stack, string("/"), joiner) + a_opt;
        throw runtime_error("Config option '", path,
                            "' has invalid 'defaults' fallback branch (ref",
                            n, "): ", a_path);
    };

    auto get_def_branch = [&, this](std::string const& a_path, const option_map& a_scope) {
        const option* res = nullptr;
        if (a_path.empty()) return res;

        vector<string> parts;
        boost::split(parts, a_path, boost::is_any_of("/"), boost::token_compress_on);

        auto* map = &a_scope;

        for (auto pit = parts.begin(); pit != parts.end(); ++pit) {
            auto cit  = map->find(*pit);
            if  (cit == map->end())
                throw runtime_error("not_found");
            res = &cit->second;
            map = &res->children;
        }

        return res;
    };

    for (auto& vt : a_scope) {
        auto& opt = vt.second;
        opt.fallback_defaults_branch = nullptr;

        if (!opt.fallback_defaults_branch_path.empty() && !opt.fallback_defaults_branch) {
            auto it = stack.end();
            auto s  = opt.fallback_defaults_branch_path;
            while (s.substr(0, 3) == "../") {
                s.erase(0, 3);
                if (it == stack.begin())
                    thrower(vt.first, opt.fallback_defaults_branch_path, 1);
                --it;
            }

            if (s.empty()) {
                if (opt.fallback_defaults_branch_path.empty())
                    thrower(vt.first, opt.fallback_defaults_branch_path, 2);
                s = opt.name;
            }

            bool stack_pop = false;

            if (it == stack.end()) {
                auto pair = split(s, "/");
                auto sit  = a_scope.find(pair.first);
                if  (sit == a_scope.end())
                    thrower(vt.first, opt.fallback_defaults_branch_path, 3);
                stack.emplace_back(make_pair(pair.first, &a_scope));
                stack_pop = true;
                it = stack.end(); --it;
                if (pair.second.empty())
                    s = opt.children.empty() ? strjoin(pair.first, opt.name, "/") : pair.first;
                else
                    s = pair.second;
            }

            // If the path ends with '/', it is a branch name => append opt.name
            // Otherwise, the 'defaults' branch path name is the one to be
            // searched for this option:
            auto pair = split<RIGHT>(s, "/");
            if (pair.second.empty()) {
                if (!s.empty() && s.back() != '/') s += "/";
                s += opt.name;
            }

            try {
                opt.fallback_defaults_branch = get_def_branch(s, *it->second);
            } catch (...) {
                thrower(vt.first, opt.fallback_defaults_branch_path, 4);
            }

            string     root_path;
            vector<string> paths;
            if  (it   != stack.end()) ++it;
            for (; it != stack.end(); ++it) paths.push_back(it->first);
            paths.push_back(s);
            root_path = join(paths, "/");
            opt.fallback_defaults_branch_path = root_path;

            if (stack_pop)
                stack.pop_back();
        }

        auto def_branch = !opt.fallback_defaults_branch_path.empty()
                        ? opt.fallback_defaults_branch_path
                        : a_def_branch.empty()
                        ? ""
                        : strjoin(a_def_branch, opt.name, "/");

        std::replace(def_branch.begin(), def_branch.end(), '/', '.');

        opt.fallback_defaults_branch_path = def_branch;

        if (!opt.children.empty()) {
            stack.emplace_back(make_pair(vt.first, &a_scope));
            internal_fill_fallback_defaults(stack, opt.children, def_branch);
            stack.pop_back();
        }
    }
}

void validator::preprocess()
{
    if (m_preprocessed) return;

    stack_t stack;

    internal_fill_fallback_defaults(stack, m_options, "");

    //dump(std::cerr, "  ", 2, m_options, true, true);
}

void validator::recursive_validate
(
    config_level_list&      a_stack,
    bool                    a_fill_defaults,
    const custom_validator& a_custom_validator
)
    const throw (variant_tree_error)
{
    const tree_path&         a_root   = a_stack.back().path();
    const variant_tree_base& a_config = a_stack.back().cfg();
    const option_map&        a_opts   = a_stack.back().options();

    auto  old_opt = a_stack.back().opt();

    check_unique(a_root, a_config, a_opts);
    check_required(a_stack);

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

    for (auto& vt : a_config) {
        bool l_match = false;
        for (auto& ovt : a_opts) {
            const option& opt = ovt.second;
            bool  check = false;
            if (opt.opt_type == ANONYMOUS) {
                if (!all_anonymous(a_opts))
                    throw variant_tree_error(format_name(a_root, opt,
                        vt.first, vt.second.data()),
                        "Check XML spec. Cannot mix anonymous and named options "
                        "in one section!");
                check = true;
            } else if (opt.name == vt.first) {
                check = true;
            } else if (single_nested_tree) {
                l_match = true;
                break;
            }

            if (check) {
                if (!opt.validate)
                    l_match = true;
                else if (opt.m_validator) {
                    a_stack.push(a_root / opt.name, vt.second, &opt, opt.children);
                    opt.m_validator->recursive_validate
                        (a_stack, a_fill_defaults, opt.m_custom_validator);
                    a_stack.pop();
                } else {
                    a_stack.back().opt(&opt);
                    check_option(a_stack,
                        const_cast<typename variant_tree_base::value_type&>(vt),
                        a_fill_defaults,
                        opt.m_custom_validator ? opt.m_custom_validator : a_custom_validator);
                }
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

    a_stack.back().opt(old_opt);
}

void validator::check_required(config_level_list& a_stack)
    const throw (missing_required_option_error, variant_tree_error)
{
    const tree_path&         root = a_stack.back().path();
    const variant_tree_base& cfg  = a_stack.back().cfg();
    const option_map&        opts = a_stack.back().options();

    #ifdef TEST_CONFIG_VALIDATOR
    std::cout << "check_required(" << root << ", cfg.count=" << cfg.size()
        << ", opts.count=" << a_stack.back().options().size() << ')' << std::endl;
    #endif
    for (auto& ovt : opts) {
        const option& opt = ovt.second;
        if (opt.required && opt.default_value.data().is_null()) {
            #ifdef TEST_CONFIG_VALIDATOR
            std::cout << "  checking_option(" << format_name(root, opt) << ")"
                << (opt.opt_type == ANONYMOUS ? " [anonymous]" : "")
                << (opt.unique ? " [unique]" : "")
                << " [required, no default]"
                << std::endl;
            #endif
            if (opt.opt_type == ANONYMOUS) {
                if (cfg.empty())
                    throw missing_required_option_error(format_name(root, opt),
                        "Check XML spec. Missing required value of anonymous option!");
            } else {
                bool found = false;
                for (auto& vt : cfg)
                    if (vt.first == opt.name) {
                        #ifdef TEST_CONFIG_VALIDATOR
                        std::cout << "    found: "
                            << format_name(root, opt, vt.first, vt.second.data())
                            << ", value=" << vt.second.data().to_string()
                            << ", type=" << type_to_string(opt.opt_type)
                            << std::endl;
                        #endif

                        if (opt.opt_type == BRANCH) {
                            found = true;
                            break;
                        }

                        if (vt.second.data().is_null())
                            throw missing_required_option_error
                                (format_name(root, opt, vt.first, vt.second.data()),
                                 "Missing value of the required option "
                                 "and no default provided!");
                        found = true;
                        if (opt.unique)
                            break;
                    }

                if (!found && (opt.opt_type != BRANCH || opt.children.empty())) {
                    // If the option points to some other node with a fallback
                    // default, then we check if that path has a value, in which
                    // case the value is considered to be found:
                    if (!opt.fallback_defaults_branch_path.empty())
                        if (a_stack.front().cfg().get_child_optional(opt.fallback_defaults_branch_path))
                            found = true;
                    if (!found) {
                        // Go up the stack of options tree and see if
                        // there's any parent branch that has
                        // 'required=false' and 'recursive=true':
                        for (auto it=a_stack.rbegin(); it != a_stack.rend(); ++it) {
                            auto* br = it->opt();
                            if (!br) break;
                            if (br->opt_type != BRANCH || br->required || !br->recursive)
                                continue;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        throw missing_required_option_error(format_name(root, opt),
                            "Missing required ", (opt.opt_type == BRANCH ? "branch" : "option"),
                            " with no default!");
                }
            }
        }
        #ifdef TEST_CONFIG_VALIDATOR
        else {
            std::cout << "  option(" << format_name(root, opt)
                << ") is " << (opt.required ? "required" : "not required")
                << " and " << (opt.default_value.data().is_null()
                    ? "no default"
                    : (std::string("has default=") + opt.default_value.data().to_string()))
                << std::endl;
        }
        #endif

        if (opt.opt_type == ANONYMOUS) {
            #ifdef TEST_CONFIG_VALIDATOR
            if (cfg.size())
                std::cout << "  Checking children of anonymous node "
                    << format_name(root, opt) << std::endl;
            #endif
            for (auto& vt : cfg) {
                auto path = format_name(root, opt, vt.first, vt.second.data());
                a_stack.push(path, vt.second, &opt, opt.children);
                check_required(a_stack);
                a_stack.pop();
            }
        } else {
            tree_path l_req_name;
            bool l_has_req = has_required_child_options(opt.children, l_req_name);
            bool found   = false;

            #ifdef TEST_CONFIG_VALIDATOR
            if (opt.children.size())
                std::cout << "  Checking children of " << format_name(root, opt)
                          << " (hasreq=" << (l_has_req ? l_req_name.dump() : "") << ")"
                          << std::endl;
            #endif

            for (auto& vt : cfg)
                if (vt.first == opt.name) {
                    found = true;
                    if (l_has_req) {
                        if (!vt.second.size())
                            throw missing_required_option_error
                                (format_name(root, opt, vt.first, vt.second.data()),
                                 "Option is missing required child option: ",
                                 l_req_name.dump());
                        auto path = format_name(root, opt, vt.first, vt.second.data());
                        try {
                            a_stack.push(path, vt.second, &opt, opt.children);
                            check_required(a_stack);
                            a_stack.pop();
                        } catch (missing_required_option_error const& e) {
                            a_stack.pop();
                            // If this branch option is not required, and the
                            // exception is due to missing child options, we
                            // can ignore it
                            if (!opt.required && opt.opt_type == BRANCH)
                                continue;
                            throw;
                        }
                    }
                    if (!opt.children.size() && vt.second.size() && opt.validate)
                        throw missing_required_option_error
                            (format_name(root, opt, vt.first, vt.second.data()),
                             "Option is not allowed to have child nodes!");
                }

            if (!found && l_has_req && opt.validate) {
                if (!opt.required && !opt.children.empty()) {
                    // Check if all children have values in default fallback branches
                    bool ok = true;
                    for (auto& vt : opt.children)
                        if (vt.second.required && !vt.second.fallback_defaults_branch_path.empty()) {
                            auto s = vt.second.fallback_defaults_branch_path;
                            auto c = a_stack.front().cfg().get_child_optional(s);
                            if (!c) {
                                ok = false;
                                break;
                            }
                        }
                    if (ok)
                        found = true;
                }

                if (!found)
                    throw missing_required_option_error
                        (format_name(root, opt), "Missing a required child option: ",
                        l_req_name.dump());
            }
        }
    }
}


void validator::
check_option
(
    config_level_list&                      a_stack,
    typename variant_tree_base::value_type& a_vt,
    bool                                    a_fill_defaults,
    const custom_validator&                 a_custom_validator
)
    const throw(std::invalid_argument, variant_tree_error)
{
    assert(a_stack.back().opt());
    const tree_path& root =  a_stack.back().path();
    const option&    opt  = *a_stack.back().opt();
    try {
        // Populate default value
        if (!opt.required && a_vt.second.data().is_null()) {
            if (opt.default_value.data().is_null() && opt.opt_type != BRANCH)
                throw std::invalid_argument("Check XML spec. Required option is missing default value!");
            BOOST_ASSERT(
                (opt.opt_type == BRANCH && opt.default_value.data().is_null()) ||
                (to_option_type(opt.default_value.data().type()) == opt.value_type));
            if (a_fill_defaults && !opt.default_value.data().is_null())
                a_vt.second.data() = opt.default_value.data();
        }

        switch (opt.value_type) {
            case STRING:
                if (!a_vt.second.data().is_null() && a_vt.second.data().type() != variant::TYPE_STRING)
                    throw std::invalid_argument("Wrong type - expected string!");
                if (!opt.min_value.is_null() &&
                     a_vt.second.data().to_str().size() < (size_t)opt.min_value.to_int())
                    throw std::invalid_argument("String value too short!");
                if (!opt.max_value.is_null() &&
                     a_vt.second.data().to_str().size() > (size_t)opt.max_value.to_int())
                    throw std::invalid_argument("String value too long!");
                break;
            case INT:
                if (!a_vt.second.data().is_null() && a_vt.second.data().type() != variant::TYPE_INT)
                    throw std::invalid_argument("Wrong type - expected integer!");
                if (!opt.min_value.is_null() && opt.min_value > a_vt.second.data())
                    throw std::invalid_argument("Value too small!");
                if (!opt.max_value.is_null() && opt.max_value < a_vt.second.data())
                    throw std::invalid_argument("Value too large!");
                break;
            case BOOL:
                if (!a_vt.second.data().is_null() && a_vt.second.data().type() != variant::TYPE_BOOL)
                    throw std::invalid_argument("Wrong type - expected boolean true/false!");
                break;
            case FLOAT:
                if (!a_vt.second.data().is_null() && a_vt.second.data().type() != variant::TYPE_DOUBLE)
                    throw std::invalid_argument("Wrong type - expected float!");
                if (!opt.min_value.is_null() && opt.min_value > a_vt.second.data())
                    throw std::invalid_argument("Value too small!");
                if (!opt.max_value.is_null() && opt.max_value < a_vt.second.data())
                    throw std::invalid_argument("Value too large!");
                break;
            default: {
                // Allow anonymous options to have no value (since the
                // name defines the value)
                if (opt.opt_type == ANONYMOUS || opt.opt_type == BRANCH)
                    break;
                throw variant_tree_error(format_name(root, opt, a_vt.first,
                        a_vt.second.data()),
                    "Check XML spec. Option's value_type '",
                    type_to_string(opt.value_type),
                    "' is invalid!");
            }
        }

        if
        (
            opt.required &&
            opt.opt_type != ANONYMOUS &&
            opt.opt_type != BRANCH &&
            a_vt.second.data().is_null()
        )
            throw std::invalid_argument("Required value missing!");
        if (a_vt.first.empty())
            throw std::invalid_argument("Expected non-empty name!");

        switch (opt.opt_type) {
            case STRING:    break;
            case ANONYMOUS: break;
            case BRANCH:    break;
            case INT:       break;
            case FLOAT:     break;
            case BOOL:      break;
            default: {
                throw variant_tree_error(format_name(root, opt, a_vt.first,
                        a_vt.second.data()),
                    "Check XML spec. Unsupported type of option: ",
                    type_to_string(opt.opt_type));
            }
        }

        if (!opt.name_choices.empty()) {
            if (opt.opt_type != ANONYMOUS)
                throw variant_tree_error(format_name(root, opt, a_vt.first,
                        a_vt.second.data()),
                    "Check XML spec. Non-anonymous option cannot have name choices!");
            if (opt.name_choices.find(a_vt.first) == opt.name_choices.end())
                throw std::invalid_argument("Invalid name given to anonymous option!");
        }

        if (!opt.value_choices.empty())
            if (opt.value_choices.find(a_vt.second.data()) == opt.value_choices.end()) {
                throw variant_tree_error(format_name(root, opt,
                        a_vt.first, a_vt.second.data()),
                    "Value is not allowed for option!");
            }
        if (!opt.children.empty()) {
            a_stack.push(root / opt.name, a_vt.second, &opt, opt.children);
            recursive_validate(a_stack, a_fill_defaults, opt.m_custom_validator);
            a_stack.pop();
        }
    } catch (std::invalid_argument& e) {
        throw variant_tree_error(format_name(root, opt, a_vt.first,
                a_vt.second.data()),
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

std::string validator::
usage(const std::string& a_indent, bool a_colorize, bool a_braces) const {
    std::stringstream s;
    dump(s, a_indent, 0, m_options, a_colorize, a_braces);
    return s.str();
}

std::ostream& validator::dump
(
    std::ostream& out, const std::string& a_indent,
    int a_level, const option_map& a_opts, bool a_colorize, bool a_braces
) {
    static const char GREEN[]  = "\E[1;32;40m";
    static const char YELLOW[] = "\E[1;33;40m";
    static const char NORMAL[] = "\E[0m";
    std::regex eol_re("\n *");

    std::string       l_indent = a_indent + std::string(a_level, ' ');
    const std::string l_nl_15  = l_indent + std::string(15, ' ');

    for (auto& ovt : a_opts) {
        const option& opt = ovt.second;
        out << l_indent << (a_colorize ? GREEN  : "")
            << opt.name << (a_colorize ? NORMAL : "")
            << (opt.opt_type == ANONYMOUS ? " (anonymous): " : ": ")
            << (a_colorize ? YELLOW : "")
            << type_to_string(opt.value_type) << (a_colorize ? NORMAL : "")
            << (a_braces ? " {" : "") << std::endl;
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
        if (!opt.fallback_defaults_branch_path.empty())
            out << l_indent << " Fallback Def: " << opt.fallback_defaults_branch_path << endl;
        if (opt.children.size())
            dump(out, l_indent, a_level+2, opt.children, a_colorize, a_braces);
        if (a_braces)
            out << l_indent << "}" << endl;
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
    for (auto& vt : a_config) {
        if (l_names.find(vt.first) == l_names.end())
            l_names.insert(vt.first);
        else {
            for (auto& ovt : a_opts) {
                const option& o = ovt.second;
                if (o.name == vt.first && o.unique)
                    throw variant_tree_error(format_name(a_root, o, vt.first,
                          vt.second.data()),
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
    for (auto& ovt : a_opts) {
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