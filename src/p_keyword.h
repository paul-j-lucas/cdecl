/*
**      cdecl -- C gibberish translator
**      src/p_keyword.h
**
**      Copyright (C) 2023-2025  Paul J. Lucas
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

#ifndef cdecl_p_keyword_H
#define cdecl_p_keyword_H

/**
 * @file
 * Declares types and functions for looking up C preprocessor keyword
 * information.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "cdecl_parser.h"               /* must go last */

/**
 * @defgroup p-keywords-group C Preprocessor Keywords
 * Types and functions for C preprocessor keywords.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * C preprocessor keyword information.
 */
struct p_keyword {
  char const     *literal;              ///< C string literal of the keyword.
  yytoken_kind_t  y_token_id;           ///< Bison token (`Y_xxx`).
};
typedef struct p_keyword p_keyword_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Given a literal, gets the p_keyword for the corresponding C preprocessor
 * keyword, if any.
 *
 * @param literal The literal to find.
 * @return Returns a pointer to the corresponding p_keyword or NULL if not
 * found.
 */
NODISCARD
p_keyword_t const* p_keyword_find( char const *literal );

/**
 * Initializes \ref p_keyword data.
 *
 * @note This function must be called exactly once.
 */
void p_keywords_init( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_p_keyword_H */
/* vim:set et sw=2 ts=2: */
