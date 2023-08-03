/*
**      cdecl -- C gibberish translator
**      src/lexer.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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
#include "pjl_config.h"                 /* must go first */
#include "c_keyword.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @defgroup lexer-group Lexical Analyzer
 * Macros, global variables, and functions interacting with the lexical
 * analyzer.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/** For \ref lexer_find, look-up everything (the default). */
#define LEXER_FIND_ANY            (~0u)

/** For \ref lexer_find, look-up C/C++ keywords. */
#define LEXER_FIND_C_KEYWORDS     (1u << 0)

/** For \ref lexer_find, look-up **cdecl** keywords. */
#define LEXER_FIND_CDECL_KEYWORDS (1u << 1)

/** For \ref lexer_find, look-up type names. */
#define LEXER_FIND_TYPES          (1u << 2)

// extern variables

/**
 * The bitwise-or of what to look up.
 * Defaults to #LEXER_FIND_ANY, but #LEXER_FIND_C_KEYWORDS,
 * #LEXER_FIND_CDECL_KEYWORDS, or #LEXER_FIND_TYPES can be turned off to find
 * all _but_ that.
 *
 * @sa #LEXER_FIND_ANY
 * @sa #LEXER_FIND_C_KEYWORDS
 * @sa #LEXER_FIND_CDECL_KEYWORDS
 * @sa #LEXER_FIND_TYPES
 */
extern unsigned         lexer_find;

/**
 * Keyword context.
 */
extern c_keyword_ctx_t  lexer_keyword_ctx;

/**
 * Text of current token.
 *
 * @sa lexer_printable_token()
 */
extern char const      *lexer_token;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Initializes the lexer.
 *
 * @note This function must be called exactly once.
 */
void lexer_init( void );

/**
 * Gets the current input line.
 *
 * @param rv_len Receives the length of said line.
 * @return Returns said line.
 */
NODISCARD
char const* lexer_input_line( size_t *rv_len );

/**
 * Gets the lexer's current location.
 *
 * @return Returns said location.
 */
NODISCARD
c_loc_t lexer_loc( void );

/**
 * Gets a printable string of \ref lexer_token.
 *
 * @return Returns said string or NULL if \ref lexer_token is the empty string.
 */
NODISCARD
char const* lexer_printable_token( void );

/**
 * Resets the lexer to its initial state.
 *
 * @param hard_reset If `true`, does a "hard" reset that currently resets the
 * EOF flag also.
 */
void lexer_reset( bool hard_reset );

/**
 * Flex: gets the next token ID.
 *
 * @return Returns the token ID.
 *
 * @note The definition is provided by Flex.
 */
NODISCARD
int yylex( void );

#ifdef __GNUC__
# pragma GCC diagnostic push
  // Declare yyrestart() so it can be called from anywhere.  However, Flex
  // declares yyrestart() in the generated .c file before it #includes headers,
  // so we'd get a redundant declaration warning -- so suppress that.
# pragma GCC diagnostic ignored "-Wredundant-decls"
#endif /* __GNUC__ */

/**
 * Flex: immediately switch to reading \a file.
 *
 * @param in_file The `FILE` to read from.
 *
 * @note The definition is provided by Flex.
 */
void yyrestart( FILE *in_file );

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif /* __GNUC__ */

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_lexer_H */
/* vim:set et sw=2 ts=2: */
