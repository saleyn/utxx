//----------------------------------------------------------------------------
/// \file  variant_tree_parser.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Stream/file reader/writer of the variant tree class.
//----------------------------------------------------------------------------
// Created: 2014-02-22
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/detail/variant_tree_parser_impl.hpp>
#include <utxx/error.hpp>

namespace utxx {

    enum config_format {
        FORMAT_UNDEFINED = -1,
        FORMAT_SCON,
        FORMAT_INI,
        FORMAT_XML
    };

    inline const char* fmt_to_string(config_format fmt) {
        switch (fmt) {
            case FORMAT_SCON: return "SCON";
            case FORMAT_INI:  return "INI";
            case FORMAT_XML:  return "XML";
            default:          return "UNDEFINED";
        }
    }

    /**
     * @brief Read SCON/INI/XML format from stream
     * @param a_stream   is a filename associated with stream in case of exceptions
     * @param a_tree     is the tree
     * @param a_inc_filename_resolver is the resolver of files included in the
     *                                configuration via '#include "filename"'
     *                                clause (given that configuration format, such
     *                                as SCON supports this feature)
     * @param a_flags are the optional flags for reading XML file
     *                (see boost::property_tree::xml_parser::read_xml())
     * @note Replaces the existing contents. Strong exception guarantee.
     * @throw file_parser_error If the stream cannot be read, doesn't contain
     *                          valid format, or a conversion fails.
     */
    template<class Ch>
    void read_config
    (
        std::basic_istream<Ch>&      a_stream,
        basic_variant_tree<Ch>&      a_tree,
        config_format                a_format,
        const std::basic_string<Ch>& a_filename  = std::basic_string<Ch>(),
        const boost::function<bool (std::basic_string<Ch>& a_filename)>
                                     a_resolver = inc_file_resolver<Ch>(),
        int                          a_flags = 0
    ) {
        switch (a_format) {
            case FORMAT_SCON:
                detail::read_scon(a_stream, a_tree, a_filename, a_resolver);
                break;
            case FORMAT_INI:
#ifndef UTXX_VARIANT_TREE_NO_INI_PARSER
                detail::read_ini(a_stream, a_tree, a_flags);
#else
                UTXX_THROW_BADARG_ERROR("INI format reading is disabled!");
#endif
                break;
            case FORMAT_XML:
#ifndef UTXX_VARIANT_TREE_NO_XML_PARSER
                detail::read_xml(a_stream, a_tree, a_flags);
#else
                UTXX_THROW_BADARG_ERROR("XML format reading is disabled!");
#endif
                break;
            default:
                UTXX_THROW_BADARG_ERROR
                    ("Reading of this file format not implemented (", a_filename, ")!");
        }

        if (a_tree.validator())
            a_tree.validate();
    }

    /**
     * @brief Read SCON/INI/XML/INFO file format by guessing content type by extension
     * @param a_filename is a filename associated with stream in case of exceptions
     * @param a_tree     is the tree
     * @param a_inc_filename_resolver is the resolver of files included in the
     *                                configuration via '#include "filename"'
     *                                clause (given that configuration format, such
     *                                as SCON supports this feature)
     * @param a_flags are the optional flags for reading XML file
     *                (see boost::property_tree::xml_parser::read_xml())
     * @note Replaces the existing contents. Strong exception guarantee.
     * @throw file_parser_error If the stream cannot be read, doesn't contain
     *                          valid format, or a conversion fails.
     */
    template<class Ch>
    void read_config
    (
        const std::basic_string<Ch>& a_filename,
        basic_variant_tree<Ch>&      a_tree,
        const boost::function<bool (std::basic_string<Ch>& a_filename)>
                                     a_resolver = inc_file_resolver<Ch>(),
        int                          a_flags = 0,
        const std::locale &          a_loc   = std::locale(),
        config_format                a_fmt   = FORMAT_UNDEFINED
    ) {
        std::basic_string<Ch> ext = boost::filesystem::extension(a_filename);

        if (a_fmt == FORMAT_UNDEFINED) {
            if (ext == ".config" || ext == ".conf" || ext == ".cfg" || ext == ".scon")
                a_fmt = FORMAT_SCON;
            else if (ext == ".ini")
                a_fmt = FORMAT_INI;
            else if (ext == ".xml")
                a_fmt = FORMAT_XML;
            else
                UTXX_THROW_BADARG_ERROR
                    ("Configuration file extension not supported (", a_filename, "!");
        }

        std::basic_ifstream<Ch> stream(a_filename.c_str());
        if (!stream)
            UTXX_THROW_BADARG_ERROR("Cannot open file for reading", a_filename, 0);
        stream.imbue(a_loc);
        read_config(stream, a_tree, a_fmt, a_filename, a_resolver, a_flags);
    }

    template<class Ch>
    void read_config
    (
        const Ch*               a_filename,
        basic_variant_tree<Ch>& a_tree,
        const boost::function<bool (std::basic_string<Ch>& a_filename)>
                                a_resolver = inc_file_resolver<Ch>(),
        int                     a_flags = 0,
        const std::locale &     a_loc   = std::locale(),
        config_format           a_fmt   = FORMAT_UNDEFINED
    ) {
        read_config(std::basic_string<Ch>(a_filename),
                    a_tree, a_resolver, a_flags, a_loc, a_fmt);
    }


    /**
     * Writes a tree to the stream in SCON format.
     * @throw file_parser_error If the stream cannot be written to, or a
     *                          conversion fails.
     * @param settings The settings to use when writing the SCON data.
     */
    template <class Stream, class Ch, class Settings>
    void write_config
    (
        Stream&                       a_stream,
        const basic_variant_tree<Ch>& a_tree,
        config_format                 a_format,
        const Settings&               a_settings = Settings()
    ) {
        switch (a_format) {
            case FORMAT_SCON:
                detail::write_scon(a_stream, a_tree, a_settings);
                break;
            default:
                UTXX_THROW_BADARG_ERROR
                    ("Writing to ", fmt_to_string(a_format), " format not implemented!");
        }
    }

    /**
     * Writes a tree to the file in a given format
     *
     * The tree's key type must be a string type, i.e. it must have a
     * nested value_type typedef that is a valid parameter for basic_ofstream.
     * @throw file_parser_error If the file cannot be written to, or a
     *                          conversion fails.
     * @param settings The settings to use when writing the INFO data.
     */
    template <class Ch, class Settings>
    void write_config
    (
        const std::basic_string<Ch>&    a_filename,
        const basic_variant_tree<Ch>&   a_tree,
        config_format                   a_format,
        const Settings&                 a_settings = Settings(),
        const std::locale &             a_loc      = std::locale()
    ) {
        std::basic_ofstream<Ch> stream(a_filename.c_str());
        if (!stream) {
            UTXX_THROW_RUNTIME_ERROR
                ("Cannot open file for writing", a_filename);
        }
        stream.imbue(a_loc);
        write_config(stream, a_tree, a_format, a_settings);
        if (!stream.good())
            UTXX_THROW_IO_ERROR(errno, "Config write error ", a_filename);
    }

    template <class Ch, class Settings>
    void write_config
    (
        const Ch*                       a_filename,
        const basic_variant_tree<Ch>&   a_tree,
        config_format                   a_format,
        const Settings&                 a_settings = Settings(),
        const std::locale &             a_loc      = std::locale()
    ) {
        write_config(std::basic_string<Ch>(a_filename),
                     a_tree, a_format, a_settings, a_loc);
    }

    template <class Stream, class Ch>
    void write_scon
    (
        Stream&                         a_stream,
        const basic_variant_tree<Ch>&   a_tree,
        const scon_writer_settings<Ch>& a_settings = scon_writer_settings<Ch>()
    ) {
        detail::write_scon(a_stream, a_tree, a_settings);
    }

    template <class Stream, class Ch>
    void write_scon
    (
        const std::basic_string<Ch>&    a_filename,
        const basic_variant_tree<Ch>&   a_tree,
        const scon_writer_settings<Ch>& a_settings = scon_writer_settings<Ch>(),
        const std::locale &             a_loc      = std::locale()
    ) {
        detail::write_scon(a_filename, a_tree, a_settings, a_loc);
    }

} // namespace utxx
