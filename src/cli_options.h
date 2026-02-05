/*
**      cdecl -- C gibberish translator
**      src/cli_options.h
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

#ifndef cdecl_cli_options_H
#define cdecl_cli_options_H

/**
 * @file
 * Declares macros and functions for command-line options.
 *
 * @sa options.h
 * @sa set_options.h
 */

// local
#include "pjl_config.h"                 /* IWYU pragma: keep */

/// @cond DOXYGEN_IGNORE

// standard
#include <getopt.h>
#include <stddef.h>                     /* for NULL */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup cli-options-group Command-Line Options
 * Macros and functions for command-line options.
 *
 * @sa \ref cdecl-options-group
 * @sa \ref set-options-group
 *
 * @{
 */

/**
 * Convenience macro for iterating over all **cdecl** command-line options.
 *
 * @param VAR The `struct option` loop variable.
 *
 * @sa cli_option_next()
 * @sa #FOREACH_SET_OPTION()
 */
#define FOREACH_CLI_OPTION(VAR) \
  for ( struct option const *VAR = NULL; (VAR = cli_option_next( VAR )) != NULL; )

////////// extern functions ///////////////////////////////////////////////////

/**
 * Iterates to the next **cdecl** command-line option.
 *
 * @param opt A pointer to the previous option. For the first iteration, NULL
 * should be passed.
 * @return Returns the next command-line option or NULL for none.
 *
 * @note This function isn't normally called directly; use the
 * #FOREACH_CLI_OPTION() macro instead.
 *
 * @sa #FOREACH_CLI_OPTION()
 */
NODISCARD
struct option const* cli_option_next( struct option const *opt );

/**
 * Initializes **cdecl** options from the command-line.
 *
 * @param pargc A pointer to the argument count from main().
 * @param pargv A pointer to the argument values from main().
 *
 * @note This function must be called exactly once.
 * @note On return, `*pargc` and `*pargv` are updated to reflect the remaining
 * command-line with the options removed.
 */
void cli_options_init( int *const pargc, char const *const *pargv[] );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_cli_options_H */
/* vim:set et sw=2 ts=2: */
