/*
**      cdecl -- C gibberish translator
**      src/prompt.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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
#include "cdecl.h"                      /* must go first */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * The prompt strings:
 *
 *  + 0 = The primary prompt.
 *  + 1 = The secondary prompt (used for continuation lines).
 *
 * @note These may contain SGR color codes.
 */
extern char const  *prompt[2];

////////// extern functions ///////////////////////////////////////////////////

/**
 * Enables or disables the prompt.
 *
 * @param enable If `true`, enables the prompt; else disables it.
 */
void cdecl_prompt_enable( bool enable );

/**
 * Initializes the prompt.
 *
 * @note This is called `cdecl_prompt_init` and not `prompt_init` so as not to
 * conflict with the latter function in `libedit`.
 */
void cdecl_prompt_init( void );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_prompt_H */
/* vim:set et sw=2 ts=2: */
