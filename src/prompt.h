/*
**      cdecl -- C gibberish translator
**      src/prompt.h
**
**      Copyright (C) 2017-2025  Paul J. Lucas
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

#ifndef cdecl_prompt_H
#define cdecl_prompt_H

/**
 * @file
 * Declares global variables and functions for the prompt.
 */

// local
#include "pjl_config.h"                 /* must go first */

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

/**
 * @defgroup prompt-group Prompt
 * Global variables and functions for the prompt.
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Enables or disables the prompt based on \ref opt_prompt.
 *
 * @sa cdecl_prompt_init()
 */
void cdecl_prompt_enable( void );

/**
 * Gets the current prompt.
 *
 * @param is_cont_line `true` only if the current line is a "continued line"
 * from the previous one that ended with a `\`.
 * @return Returns the string to use as the prompt.
 */
NODISCARD
char const* cdecl_prompt( bool is_cont_line );

/**
 * Initializes the prompt for \ref opt_lang_id.
 *
 * @note This may be called more than once, specifically whenever \ref
 * opt_lang_id changes to update the prompt.
 * @note This is called `cdecl_prompt_init` and not `prompt_init` so as not to
 * conflict with the latter function in `libedit`.
 *
 * @sa cdecl_prompt_enable()
 */
void cdecl_prompt_init( void );

/**
 * Gets the length of the current prompt, if any.
 *
 * @return Returns said length.
 */
NODISCARD
size_t cdecl_prompt_len( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_prompt_H */
/* vim:set et sw=2 ts=2: */
