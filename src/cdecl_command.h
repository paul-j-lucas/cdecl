/*
**      cdecl -- C gibberish translator
**      src/cdecl_command.h
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

#ifndef cdecl_command_H
#define cdecl_command_H

/**
 * @file
 * Declares macros and functions for cdecl commands.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

// standard
#include <stddef.h>                     /* for NULL */

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

/**
 * The kind of cdecl command in least-to-most restrictive order.
 */
enum cdecl_command_kind {
  CDECL_COMMAND_ANYWHERE,               ///< Command is OK anywhere.
  CDECL_COMMAND_FIRST_ARG,              ///< `$ cdecl` _command_ _args_
  CDECL_COMMAND_PROG_NAME,              ///< `$` _command_ _args_
  CDECL_COMMAND_LANG_ONLY               ///< `cdecl>` _command_ _args_
};

/**
 * A cdecl command.
 */
struct cdecl_command {
  char const           *literal;        ///< The command literal.
  cdecl_command_kind_t  kind;           ///< The kind of command.
  c_lang_id_t           lang_ids;       ///< Language(s) command is in.
};

////////// extern functions ///////////////////////////////////////////////////

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

#endif /* cdecl_command */
/* vim:set et sw=2 ts=2: */
