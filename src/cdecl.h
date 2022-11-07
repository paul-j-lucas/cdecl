/*
**      cdecl -- C gibberish translator
**      src/cdecl.h
**
**      Copyright (C) 2017-2022  Paul J. Lucas, et al.
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

#ifndef cdecl_cdecl_H
#define cdecl_cdecl_H

/**
 * @file
 * Declares miscellaneous macros, global variables, and the main parsing
 * function.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdio.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/** Default configuration file name. */
#define CONF_FILE_NAME_DEFAULT    CDECL "rc"

/** Program name when composing or deciphering C. */
#define CDECL                     PACKAGE

/** Program name when composing or deciphering C++. */
#define CPPDECL                   "c++decl"

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern FILE        *cdecl_fout;         ///< File out.
extern bool         cdecl_initialized;  ///< Initialized (read conf. file)?
extern bool         cdecl_interactive;  ///< Interactive (connected to a tty)?
extern cdecl_mode_t cdecl_mode;         ///< Converting English or gibberish?
extern char const  *me;                 ///< Program name.

////////// extern functions ///////////////////////////////////////////////////

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

/**
 * Checks whether \a prog_name is **cdecl**.
 *
 * @param prog_name The name of the program.
 * @returns Returns `true` only if we are.
 *
 * @sa is_cppdecl()
 */
NODISCARD
bool is_cdecl( char const *prog_name );

/**
 * Checks whether \a prog_name is **c++decl** or a variant.
 *
 * @param prog_name The name of the program.
 * @returns Returns `true` only if we are.
 *
 * @sa is_cdecl()
 */
NODISCARD
bool is_cppdecl( char const *prog_name );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_cdecl_H */
/* vim:set et sw=2 ts=2: */
