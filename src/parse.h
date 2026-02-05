/*
**      cdecl -- C gibberish translator
**      src/parse.h
**
**      Copyright (C) 2017-2026  Paul J. Lucas, et al.
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

#ifndef cdecl_parse_H
#define cdecl_parse_H

/**
 * @file
 * Declares functions for parsing input.
 */

// local
#include "pjl_config.h"                 /* IWYU pragma: keep */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @ingroup parser-group
 * @defgroup parser-api-group Parser API
 * Functions for parsing and running **cdecl** commands.
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Parses the command-line.
 *
 * @param cli_count The size of \a cli_value.
 * @param cli_value The command-line argument values, if any.  Note that,
 * unlike `main()`'s `argv`, this contains _only_ the command-line arguments
 * _after_ the program name.
 * @return Returns `EX_OK` upon success or another value upon failure.
 *
 * @note The parameters are _not_ named `argc` and `argv` intentionally to
 * avoid confusion since they're not the same.
 */
NODISCARD
int cdecl_parse_cli( size_t cli_count, char const *const cli_value[cli_count] );

/**
 * Parses **cdecl** commands from \a fin until either an error occurs or until
 * EOF.
 *
 * @param fin The `FILE` to read from.
 * @return Returns `EX_OK` upon success or another value upon failure.
 */
NODISCARD
int cdecl_parse_file( FILE *fin );

/**
 * Parses a **cdecl** command from a string.
 *
 * @param s The string to parse.
 * @param s_len The length of \a s.
 * @return Returns `EX_OK` upon success or another value upon failure.
 *
 * @note This is the main parsing function (the only one that calls Bison).
 * All other `cdecl_parse_*()` functions ultimately call this function.
 */
NODISCARD
int cdecl_parse_string( char const *s, size_t s_len );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_parse_H */
/* vim:set et sw=2 ts=2: */
