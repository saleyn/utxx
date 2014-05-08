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
#include <utxx/variant_tree_parser.hpp>

namespace utxx {

/**
 * Configuration object combining configuration tree and validator
 */
template<typename Validator>
class valcfg {
    typedef typename Validator::cfg_validator val_t;
    config_tree m_config;

public:
    /// Root configuration constructor
    valcfg(const std::string& a_file, const config_path& a_root = config_path())
        : m_config(a_root, val_t::instance())
    {
        try {
            read_config(a_file, m_config);
        } catch (config_error& e) {
            std::cerr << "Configuration error in " << e.path() << ": "
                      << e.what() << std::endl;
            const val_t* l_validator = val_t::instance();
            if (l_validator != NULL)
                std::cerr << "Configuration schema to follow:\n\n"
                          << l_validator->usage() << std::endl;
            throw;
        }
    }

    /// Child configuration constructor
    valcfg(const valcfg& a_root, const config_path& a_root_path)
        : m_config(a_root.m_config, a_root_path)
    {}

    /// Get configuration option using default root path for the context
    template<typename T>
    T get(const config_path& a_option) const throw(config_error) {
        return m_config.get<T>(a_option);
    }

    /// Get internal configuration tree reference
    const config_tree& conf_tree() { return m_config; }
};

} // namespace utxx

#endif // _UTXX_VALCFG_HPP_
