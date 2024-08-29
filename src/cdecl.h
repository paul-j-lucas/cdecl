/*
**      cdecl -- C gibberish translator
**      src/cdecl.h
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

#ifndef cdecl_cdecl_H
#define cdecl_cdecl_H

/**
 * @file
 * Declares miscellaneous macros, global variables, and functions.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

/**
 * @defgroup misc-globals Miscellaneous Globals
 * Miscellaneous global macros, variables, and functions.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/** Default configuration file (not path) name. */
#define CONF_FILE_NAME_DEFAULT    "." CDECL "rc"

/**
 * Program name when composing or deciphering C.
 *
 * @sa #CPPDECL
 * @sa is_cdecl()
 */
#define CDECL                     PACKAGE

/**
 * **cdecl** latest copyright year.
 */
#define CDECL_COPYRIGHT_YEAR      "2024"

/**
 * The name of the **cdecl** environment variable.
 */
#define CDECL_ENV_VAR_NAME        "CDECLRC"

/**
 * **cdecl** license.
 *
 * @sa #CDECL_LICENSE_URL
 */
#define CDECL_LICENSE             "GPLv3+: GNU GPL version 3 or later"

/**
 * **cdecl** license URL.
 *
 * @sa #CDECL_LICENSE
 */
#define CDECL_LICENSE_URL         "https://gnu.org/licenses/gpl.html"

/**
 * **cdecl** primary author.
 */
#define CDECL_PRIMARY_AUTHOR      "Paul J. Lucas"

/**
 * Program name when composing or deciphering C++.
 *
 * @sa #CDECL
 * @sa is_cppdecl()
 */
#define CPPDECL                   "c++decl"

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern bool         cdecl_is_initialized; ///< Initialized (read conf. file)?
extern char const  *cdecl_input_path;     ///< Current input file path, if any.
extern bool         cdecl_is_interactive; ///< Interactive (connected to a tty)?
extern bool         cdecl_is_testing;     ///< Is **cdecl** being tested?
extern char const  *me;                   ///< Program name.

////////// extern functions ///////////////////////////////////////////////////

/**
 * Checks whether we're **c++decl** or a variant.
 *
 * @returns Returns `true` only if we are.
 *
 * @sa is_cdecl()
 */
NODISCARD
bool is_cppdecl( void );

/**
 * Implements the **cdecl** `quit` command.
 *
 * @note This should be marked `_Noreturn` but isn't since that would generate
 * a warning that a `break` in the Bison-generated code won't be executed.
 */
void cdecl_quit( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_cdecl_H */
/* vim:set et sw=2 ts=2: */
