/*
**      cdecl -- C gibberish translator
**      src/lexer.h
**
**      Copyright (C) 2017-2026  Paul J. Lucas
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
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
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
 * For the lexer, specifies what to find when an identifier is lex'd.
 *
 * @sa lexer_find
 */
enum lexer_find_kind {
  /**
   * Find everything (the default).
   */
  LEXER_FIND_ANY            = ~0,

  /**
   * Find C/C++ keywords.
   */
  LEXER_FIND_C_KEYWORDS     = 1 << 0,

  /**
   * Find **cdecl** keywords.
   *
   * @remarks This also controls the overall "mode" of **cdecl**:
   *  + When set, **cdecl** converts English to gibberish (C/C++ declarations);
   *  + When not set, **cdecl** deciphers gibberish to English.
   *
   * @sa is_english_to_gibberish()
   * @sa is_gibberish_to_english()
   */
  LEXER_FIND_CDECL_KEYWORDS = 1 << 1,

  /**
   * Find `typedef`'d names.
   */
  LEXER_FIND_TYPES          = 1 << 2
};
typedef enum lexer_find_kind lexer_find_kind_t;

////////// extern variables ///////////////////////////////////////////////////

/**
 * For the lexer, specifies what to find when an identifier is lex'd.
 *
 * @remarks Defaults to #LEXER_FIND_ANY, but other values can be turned off
 * either individually or in combination via bitwise-and'ing the complement to
 * find all _but_ those things.  For example:
 *
 *      lexer_find &= ~LEXER_FIND_CDECL_KEYWORDS;
 *
 * would find all _but_ **cdecl** keywords so they'd be returned as ordinary
 * identifiers.
 *
 * @sa is_english_to_gibberish()
 * @sa is_gibberish_to_english()
 */
extern lexer_find_kind_t  lexer_find;

/**
 * Set to `true` only when were currently "in" a C-style comment, that is we
 * encountered <code>/\*</code> but not its closing
 * <code>\*</code><code>/</code> (yet).
 *
 * @remarks Ideally, we'd just be able to check `yy_start` `==` `X_C_COMMENT`,
 * but the former is declared `static` and the latter is a macro declared in
 * `lexer.c`.
 */
extern bool               lexer_in_c_comment;

/**
 * Lets the lexer know whether we're currently parsing a C/C++ function-like
 * parameter list.
 */
extern bool               lexer_is_param_list_decl;

/**
 * Lexer keyword context.
 */
extern c_keyword_ctx_t    lexer_keyword_ctx;

#pragma GCC diagnostic push
// Declare yylineno and yytext so they can be accessed from anywhere.  However,
// Flex declares these in the generated .c file before it #includes headers, so
// we'd get a redundant declaration warning -- so suppress that.
#pragma GCC diagnostic ignored "-Wredundant-decls"

/**
 * Flex's current line number.
 */
extern int                yylineno;

/**
 * Flex's current token.
 *
 * @sa printable_yytext()
 * @sa set_yytext()
 */
extern char              *yytext;

#pragma GCC diagnostic pop

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets whether we're converting English to gibberish (C/C++ declarations).
 *
 * @return Returns `true` only if we are.
 *
 * @sa is_gibberish_to_english()
 * @sa lexer_find
 * @sa #LEXER_FIND_CDECL_KEYWORDS
 */
NODISCARD
inline bool is_english_to_gibberish( void ) {
  return (lexer_find & LEXER_FIND_CDECL_KEYWORDS) != 0;
}

/**
 * Gets whether we're deciphering gibberish (C/C++ declarations) to English.
 *
 * @return Returns `true` only if we are.
 *
 * @sa is_english_to_gibberish()
 * @sa #LEXER_FIND_CDECL_KEYWORDS
 */
NODISCARD
inline bool is_gibberish_to_english( void ) {
  return !is_english_to_gibberish();
}

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
 * Gets the lexer's current location within the source string or file.
 *
 * @return Returns said location.
 */
NODISCARD
c_loc_t lexer_loc( void );

/**
 * Makes \a s the input source for the lexer so that subsequent tokens lex'd
 * via yylex() will be from \a s.
 *
 * @param s The string to push.
 * @param s_len The length of \a s.
 * @param line_no The source line number whence \a s came.
 *
 * @note This _must_ be balanced by calling lexer_pop_string() eventually.
 *
 * @sa lexer_pop_string()
 */
void lexer_push_string( char const *s, size_t s_len, c_loc_num_t line_no );

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
 * Gets a printable version of \ref yytext.
 *
 * @return If \ref yytext is either the empty string or a newline, returns
 * NULL; otherwise returns \ref yytext.
 */
NODISCARD
char const* printable_yytext( void );

/**
 * Sets Flex's \ref yytext to \a s.
 *
 * @remarks This is a convenience function to set \ref yytext to a constant
 * string without having to explicitly cast away `const`.
 *
 * @param s The string to set \ref yytext to.
 */
inline void set_yytext( char const *s ) {
  yytext = CONST_CAST( char*, s );
}

/**
 * Flex: gets the next token ID.
 *
 * @return Returns the token ID.
 *
 * @note The definition is provided by Flex.
 */
NODISCARD
int yylex( void );

#pragma GCC diagnostic push
// Declare yyrestart() so it can be called from anywhere.  However, Flex
// declares this in the generated .c file before it #includes headers, so we'd
// get a redundant declaration warning -- so suppress that.
#pragma GCC diagnostic ignored "-Wredundant-decls"

/**
 * Flex: immediately switch to reading \a file.
 *
 * @param in_file The `FILE` to read from.
 *
 * @note The definition is provided by Flex.
 */
void yyrestart( FILE *in_file );

#pragma GCC diagnostic pop

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_lexer_H */
/* vim:set et sw=2 ts=2: */
