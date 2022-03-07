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
 * Declares miscellaneous macros and global variables.
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

/** Program name when composing or deciphering C. */
#define CDECL                     PACKAGE

/** Default configuration file name. */
#define CONF_FILE_NAME_DEFAULT    "." CDECL "rc"

/** Program name when composing or deciphering C++. */
#define CPPDECL                   "c++decl"

/**
 * Convenience macro for iterating over all cdecl commands.
 *
 * @param VAR The cdecl_command loop variable.
 *
 * @sa cdecl_command_next()
 */
#define FOREACH_CDECL_COMMAND(VAR) \
  for ( cdecl_command_t const *VAR = NULL; (VAR = cdecl_command_next( VAR )) != NULL; )

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern FILE        *cdecl_fin;          ///< File in.
extern FILE        *cdecl_fout;         ///< File out.
extern bool         cdecl_initialized;  ///< Initialized (read conf. file)?
extern cdecl_mode_t cdecl_mode;         ///< Converting English or gibberish?
extern bool         is_input_a_tty;     ///< Is our input from a TTY?
extern char const  *me;                 ///< Program name.

////////// extern functions ///////////////////////////////////////////////////

/**
 * Parses a cdecl command from a string.
 *
 * @note This is the main parsing function (the only one that calls Bison).
 * All other `cdecl_parse_*()` functions ultimately call this function.
 *
 * @param s The null-terminated string to parse.
 * @param s_len The length of \a s.
 * @return Returns `EX_OK` upon success or another value upon failure.
 */
PJL_WARN_UNUSED_RESULT
int cdecl_parse_string( char const *s, size_t s_len );

/**
 * Iterates to the next cdecl command.
 *
 * @note This function isn't normally called directly; use the
 * #FOREACH_CDECL_COMMAND() macro instead.
 *
 * @param command A pointer to the previous command. For the first iteration,
 * NULL should be passed.
 * @return Returns the next command or NULL for none.
 *
 * @sa #FOREACH_CDECL_COMMAND
 */
PJL_WARN_UNUSED_RESULT
cdecl_command_t const* cdecl_command_next( cdecl_command_t const *command );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_cdecl_H */
/* vim:set et sw=2 ts=2: */
