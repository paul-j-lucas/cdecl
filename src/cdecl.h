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
 * Declares miscellaneous macros, global variables, and the main parsing
 * function.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/** Default configuration file name. */
#define CONF_FILE_NAME_DEFAULT    "." CDECL "rc"

/** Program name when composing or deciphering C. */
#define CDECL                     PACKAGE

/** Program name when composing or deciphering C++. */
#define CPPDECL                   "c++decl"

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern bool         cdecl_initialized;  ///< Initialized (read conf. file)?
extern bool         cdecl_interactive;  ///< Interactive (connected to a tty)?
extern cdecl_mode_t cdecl_mode;         ///< Converting English or gibberish?
extern char const  *me;                 ///< Program name.

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

#endif /* cdecl_cdecl_H */
/* vim:set et sw=2 ts=2: */
