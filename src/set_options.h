/*
**      cdecl -- C gibberish translator
**      src/set_options.h
**
**      Copyright (C) 2021-2026  Paul J. Lucas, et al.
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

#ifndef cdecl_set_options_H
#define cdecl_set_options_H

/**
 * @file
 * Declares macros, types, and functions that implement the **cdecl** `set`
 * command.
 *
 * @sa cli_options.h
 * @sa options.h
 */

// local
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <getopt.h>                     /* {no|required|optional}_argument */
#include <stdbool.h>
#include <stddef.h>                     /* for NULL */

/// @endcond

/**
 * @defgroup set-options-group Set Options
 * Macros, types, global variables, and functions for **cdecl** `set` command
 * options.
 *
 * @sa \ref cdecl-options-group
 * @sa \ref cli-options-group
 * @{
 */

/**
 * Convenience macro for iterating over all **cdecl** `set` command options.
 *
 * @param VAR The \ref set_option loop variable.
 *
 * @sa #FOREACH_CLI_OPTION()
 * @sa set_option_next()
 */
#define FOREACH_SET_OPTION(VAR) \
  for ( set_option_t const *VAR = NULL; (VAR = set_option_next( VAR )) != NULL; )

///////////////////////////////////////////////////////////////////////////////

typedef struct set_option_fn_args set_option_fn_args_t;

/**
 * The signature for a `set` option function.
 *
 * @param args The `set` option function arguments.
 * @return Returns `true` only if the option was set.
 */
typedef bool (*set_option_fn_t)( set_option_fn_args_t const *args );

/**
 * **Cdecl** `set` option kind.
 */
enum set_option_kind {
  SET_OPTION_TOGGLE,                    ///< Toggle, e.g., `foo` & `nofoo`.
  SET_OPTION_AFF_ONLY,                  ///< Affirmative only, e.g., `foo`.
  SET_OPTION_NEG_ONLY                   ///< Negative only, e.g., `nofoo`.
};
typedef enum set_option_kind set_option_kind_t;

/**
 * **Cdecl** `set` option.
 */
struct set_option {
  char const       *name;               ///< Option name.
  set_option_kind_t kind;               ///< Option kind.

  /**
   * Has the same meaning as in `struct option` used by `getopt_long()`, one of
   * `no_argument`, `required_argument`, or `optional_argument`.
   */
  int               has_arg;

  set_option_fn_t   set_fn;             ///< Set function.
};
typedef struct set_option set_option_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Implements the **cdecl** `set` command.
 *
 * @param opt_name The name of the option to set. If NULL or `"options"`,
 * displays the current values of all options.
 * @param opt_name_loc The location of \a opt_name if not NULL.
 * @param opt_value The option value, if any.
 * @param opt_value_loc The location of \a opt_value if not NULL.
 * @return Returns `true` only upon success.
 */
NODISCARD
bool set_option( char const *opt_name, c_loc_t const *opt_name_loc,
                 char const *opt_value, c_loc_t const *opt_value_loc );

/**
 * Iterates to the next **cdecl** `set` option.
 *
 * @param opt A pointer to the previous option. For the first iteration, NULL
 * should be passed.
 * @return Returns the next `set` option or NULL for none.
 *
 * @note This function isn't normally called directly; use the
 * #FOREACH_SET_OPTION() macro instead.
 *
 * @sa #FOREACH_SET_OPTION()
 */
NODISCARD
set_option_t const* set_option_next( set_option_t const *opt );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_set_options_H */
/* vim:set et sw=2 ts=2: */
