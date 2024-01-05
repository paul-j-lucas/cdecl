/*
**      cdecl -- C gibberish translator
**      src/lexer.h
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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
 * Declares types, global variables, and functions for interacting with the
 * lexical analyzer.
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
 * Types, global variables, and functions for interacting with the lexical
 * analyzer.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * For the lexer, specifies what to look-up when an identifier is lex'd.
 */
enum lexer_find_kind {
  LEXER_FIND_ANY            = ~0,       ///< Find everything (the default).
  LEXER_FIND_C_KEYWORDS     = (1 << 0), ///< Find C/C++ keywords.
  LEXER_FIND_CDECL_KEYWORDS = (1 << 1), ///< Find **cdecl** keywords.
  LEXER_FIND_TYPES          = (1 << 2)  ///< Find `typedef`'d names.
};
typedef enum lexer_find_kind lexer_find_kind_t;

////////// extern variables ///////////////////////////////////////////////////

/**
 * For the lexer, specifies what to look-up when an identifier is lex'd.
 *
 * @remarks Defaults to #LEXER_FIND_ANY, but other values can be turned off
 * either individually or in combination via bitwise-and'ing the complement to
 * find all _but_ those things.  For example:
 *
 *      lexer_find &= ~LEXER_FIND_CDECL_KEYWORDS;
 *
 * would find all _but_ **cdecl** keywords so they'd be returned as ordinary
 * identifiers.
 */
extern lexer_find_kind_t  lexer_find;

/**
 * Lexer keyword context.
 */
extern c_keyword_ctx_t    lexer_keyword_ctx;

/**
 * Text of current token.
 *
 * @sa lexer_printable_token()
 */
extern char const        *lexer_token;

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
 * Makes \a s the input source for the lexer so that subsequent tokens lex'd
 * via yylex() will be from \a s.
 *
 * @param s The string to push.
 * @param s_len The length of \a s.
 *
 * @note This _must_ be balanced by calling lexer_pop_string() eventually.
 *
 * @sa lexer_pop_string()
 */
void lexer_push_string( char const *s, size_t s_len );

/**
 * Pops a previously pushed string from the lexer's input.
 *
 * @sa lexer_push_string()
 */
void lexer_pop_string( void );

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
