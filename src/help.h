/*
**      cdecl -- C gibberish translator
**      src/help.h
**
**      Copyright (C) 2021-2024  Paul J. Lucas
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

#ifndef cdecl_help_H
#define cdecl_help_H

/**
 * @file
 * Declares functions for printing help.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

// standard
#include <stdbool.h>

/**
 * @defgroup printing-help-group Printing Help
 * Functions for printing help.
 * @{
 */

/**
 * Convenience macro for iterating over all **cdecl** `help` options.
 *
 * @param VAR The `char const *const*`  loop variable.
 *
 * @sa help_option_next()
 */
#define FOREACH_HELP_OPTION(VAR) \
  for ( char const *const *VAR = NULL; (VAR = help_option_next( VAR )) != NULL; )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Iterates to the next **cdecl** `help` option.
 *
 * @param opt A pointer to the current option. For the first iteration, NULL
 * should be passed.
 * @return Returns the next **cdecl** `help` option or NULL for none.
 *
 * @note This function isn't normally called directly; use the
 * #FOREACH_HELP_OPTION() macro instead.
 *
 * @sa #FOREACH_HELP_OPTION()
 */
NODISCARD
char const* const* help_option_next( char const *const *opt );

/**
 * Prints **cdecl** help.
 *
 * @param what
 * @parblock
 * What to print help for, one of:
 *  + `command`, `commands`, or NULL: All **cdecl** commands.
 *  + `english`: Pseudo-English.
 *  + `options`: **cdecl** `set` command options.
 *  + A **cdecl** command.
 *
 * If \a what isn't any of those or is a command that's not supported in the
 * current language, an error message is printed.
 * @endparblock
 * @param what_loc The location of \a what.
 * @return Returns `true` only if \a what is either NULL or valid and help was
 * printed or `false` otherwise.
 */
NODISCARD
bool print_help( char const *what, c_loc_t const *what_loc );

/**
 * Prints `; use --help or -h for help` to stderr.
 */
void print_use_help( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_help_H */
/* vim:set et sw=2 ts=2: */
