/*
**      cdecl -- C gibberish translator
**      src/cdecl.h
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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
 * Declares miscellaneous macros and global variables.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/** Program name when composing or deciphering C. */
#define CDECL                     PACKAGE

/** Default configuration file name. */
#define CONF_FILE_NAME_DEFAULT    "." CDECL "rc"

/** Program name when composing or deciphering C++. */
#define CPPDECL                   "c++decl"

/**
 * Convenience  macro for iterating over all cdecl commands.
 *
 * @param VAR The `c_command` loop variable.
 */
#define FOREACH_COMMAND(VAR) \
  for ( c_command_t const *VAR = CDECL_COMMANDS; VAR->literal != NULL; ++VAR )

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern c_command_t const
                    CDECL_COMMANDS[];   ///< cdecl commands.
extern c_mode_t     c_mode;             ///< Converting English or gibberish?
extern bool         c_initialized;      ///< Initialized (read conf. file)?
extern bool         is_input_a_tty;     ///< Is our input from a TTY?
extern char const  *me;                 ///< Program name.

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_cdecl_H */
/* vim:set et sw=2 ts=2: */
