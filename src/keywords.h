/*
**      cdecl -- C gibberish translator
**      src/keywords.h
**
**      Paul J. Lucas
*/

#ifndef cdecl_keywords_H
#define cdecl_keywords_H

/**
 * @file
 * Contains types and functions for looking up C/C++ keyword information.
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
  c_lang_t    not_ok;                   // which language(s) it's not OK in
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
