/*
**      cdecl -- C gibberish translator
**      src/lexer.h
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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
#include "types.h"

_GL_INLINE_HEADER_BEGIN
#ifndef C_LEXER_INLINE
# define C_LEXER_INLINE _GL_INLINE
#endif /* C_LEXER_INLINE */

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>
#include <stddef.h>                     /* for size_t */

/// @endcond

/**
 * @defgroup lexer-group Lexical Analyzer
 * Declares global variables and functions interacting with the lexical
 * analyzer.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/** For \ref lexer_find, look-up everything (the default). */
#define LEXER_FIND_ALL            (~0u)

/** For \ref lexer_find, also look-up C/C++ keywords. */
#define LEXER_FIND_C_KEYWORDS     (1u << 0)

/** For \ref lexer_find, also look-up cdecl keywords. */
#define LEXER_FIND_CDECL_KEYWORDS (1u << 1)

/** For \ref lexer_find, also look-up `typedef`s. */
#define LEXER_FIND_TYPEDEFS       (1u << 2)

// extern variables
extern unsigned         lexer_find;         ///< What to look up.
extern c_keyword_ctx_t  lexer_keyword_ctx;  ///< Keyword context.
extern char const      *lexer_token;        ///< Text of current token.

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the current input line.
 *
 * @param plen If not null, sets the value pointed at to be the length of said
 * line.
 * @return Returns said line.
 */
PJL_WARN_UNUSED_RESULT
char const* lexer_input_line( size_t *plen );

/**
 * Convenience function for getting whether we're currently parsing pseudo-
 * English rather than gibberish.
 *
 * @return Returns `true` only if we're parsing pseudo-English.
 */
C_LEXER_INLINE PJL_WARN_UNUSED_RESULT
bool lexer_is_english() {
  return (lexer_find & LEXER_FIND_CDECL_KEYWORDS) != 0;
}

/**
 * Gets the lexer's current location.
 *
 * @return Returns said location.
 */
PJL_WARN_UNUSED_RESULT
c_loc_t lexer_loc( void );

/**
 * Resets the lexer to its initial state.
 *
 * @param hard_reset If `true`, does a "hard" reset that currently resets the
 * EOF flag also.
 */
void lexer_reset( bool hard_reset );

/**
 * Gets the next token ID.
 *
 * @note The definition is provided by Flex.
 *
 * @return Returns the token ID.
 */
PJL_WARN_UNUSED_RESULT
int yylex( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_lexer_H */
/* vim:set et sw=2 ts=2: */
