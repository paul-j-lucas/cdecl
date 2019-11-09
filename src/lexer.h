/*
**      cdecl -- C gibberish translator
**      src/lexer.h
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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

#ifndef cdecl_lexer_H
#define cdecl_lexer_H

/**
 * @file
 * Declares global variables and functions interacting with the lexical
 * analyzer.
 */

// local
#include "cdecl.h"                      /* must go first */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

/// @endcond

/**
 * @defgroup lexer-group Lexical Analyzer
 * Global variables and functions interacting with the lexical analyzer.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

// extern variables
extern bool         lexer_find_typedef; ///< Look-up `typedef` names?
extern char const  *lexer_token;        ///< Text of current token.

/**
 * Gets the current input line.
 *
 * @param plen If not null, sets the value pointed at to be the length of said
 * line.
 * @return Returns said line.
 */
char const* lexer_input_line( size_t *plen );

/**
 * Gets the lexer's current location.
 *
 * @param pline A pointer to a variable to receive the lexer's line.
 * @param pcolumn A pointer to a variable to receive the lexer's column.
 */
void lexer_loc( int *pline, int *pcolumn );

/**
 * Resets the lexer to its initial state.
 *
 * @param hard_reset If `true`, does a "hard" reset that currently resets the
 * EOF flag also.
 */
void lexer_reset( bool hard_reset );

/**
 * The main Bison entrypoint for the lexical analyzer.
 *
 * @return Returns the token ID.
 */
int yylex( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_lexer_H */
/* vim:set et sw=2 ts=2: */
