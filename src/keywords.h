/*
**      cdecl -- C gibberish translator
**      src/keywords.h
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

#ifndef cdecl_keywords_H
#define cdecl_keywords_H

/**
 * @file
 * Declares types and functions for looking up C/C++ keyword information.
 */

// local
#include "lang.h"
#include "types.h"

///////////////////////////////////////////////////////////////////////////////

/**
 * C/C++ language keyword information.
 */
struct c_keyword {
  char const *literal;                  // C string literal of the keyword
  int         y_token;                  // yacc token number
  c_type_t    type;                     // type, if any
};
typedef struct c_keyword c_keyword_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Gets the c_keyword for the given literal.
 *
 * @param literal The literal to find.
 * @return Returns a pointer to the corresponding c_keyword or null for none.
 */
c_keyword_t const* c_keyword_find_literal( char const *literal );

/**
 * Gets the c_keyword for the given YACC token.
 *
 * @param y_token The YACC token to find.
 * @return Returns a pointer to the corresponding c_keyword or null for none.
 */
c_keyword_t const* c_keyword_find_token( int y_token );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_keywords_H */
/* vim:set et sw=2 ts=2: */
