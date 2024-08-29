/*
**      cdecl -- C gibberish translator
**      src/config_file.h
**
**      Copyright (C) 2017-2024  Paul J. Lucas, et al.
**
**      This program is free software: you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation, either version 3 of the License, or
**      (at your option) any later version.
**
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
**
**      You should have received a copy of the GNU General Public License
**      along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef cdecl_config_file_H
#define cdecl_config_file_H

/**
 * @file
 * Declares a function for reading **cdecl**'s configuration file.
 */

/**
 * @defgroup config-file-group Configuration File
 * Functions for reading **cdecl**'s configuration file.
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Reads the configuration file, if any.
 *
 * @remarks
 * @parblock
 * The path of the configuration file is determined as follows (in priority
 * order):
 *
 *  1. The value of either the `--config` or `-c` command-line option; or:
 *  2. The value of #CDECL_ENV_VAR_NAME or:
 *  3. <code>~/</code>#CONF_FILE_NAME_DEFAULT.
 * @endparblock
 *
 * @note This function must be called as most once.
 */
void config_init( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_config_file_H */
/* vim:set et sw=2 ts=2: */
