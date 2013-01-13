//----------------------------------------------------------------------------
/// \file   valcfg.hpp
/// \author Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
//----------------------------------------------------------------------------
/// \brief Template class combining configuration tree and validator.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
// Created: 2012-07-12
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2012 Omnibius, LLC
Author: Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>

Use, modification and distribution are subject to the Boost Software
License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
at http://www.boost.org/LICENSE_1_0.txt)

***** END LICENSE BLOCK *****
*/

#ifndef _UTXX_VALCFG_HPP_
#define _UTXX_VALCFG_HPP_

#include <iostream>
#include <utxx/config_tree.hpp>

namespace utxx {

/**
 * Configuration object combining configuration tree and validator
 */
template<typename Validator>
class valcfg {
    config_tree  m_config_root;
    config_tree& m_config;
    const Validator& m_validator;
    config_path m_root_path;

public:
    /// Root configuration constructor
    valcfg(const std::string& a_fname, const config_path& a_root_path = config_path())
        : m_config(m_config_root)
        , m_validator(Validator::cfg_validator::instance())
        , m_root_path(a_root_path)
    {
        config_tree::read_info(a_fname, m_config);
        try {
            m_validator.validate(m_config, true);
        } catch (config_error& e) {
            std::cerr << "Configuration error in " << e.path() << ": "
                      << e.what() << std::endl;
            std::cerr << "Configuration schema to follow:\n\n"
                      << m_validator.usage() << std::endl;
            throw;
        }
    }

    /// Child configuration constructor
    valcfg(const valcfg& a_root, const config_path& a_root_path)
        : m_config(a_root.m_config.get_child(a_root_path))
        , m_validator(a_root.m_validator)
        , m_root_path(a_root.m_root_path / a_root_path)
    {}

    /// Get configuration option using default root path for the context
    template<typename T>
    T get(const config_path& a_option) const throw(config_error) {
        return m_validator.get<T>(a_option, m_config, m_root_path);
    }

    config_tree     & conf_tree() { return m_config;    }
    const Validator & validator() { return m_validator; }
};

} // namespace utxx

#endif // _UTXX_VALCFG_HPP_
