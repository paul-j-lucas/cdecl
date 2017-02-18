/*
**      cdecl -- C gibberish translator
**      src/keywords.h
*/

#ifndef cdecl_keywords_H
#define cdecl_keywords_H

// local
#include "lang.h"
#include "types.h"

///////////////////////////////////////////////////////////////////////////////

/**
 * C/C++ language keyword info.
 */
struct c_keyword {
  char const *literal;                  // C string literal of the keyword
  int         y_token;                  // yacc token number
  c_type_t    type;                     // type, if any
  lang_t      not_ok;                   // which language(s) it's not OK in
};
typedef struct c_keyword c_keyword_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * TODO
 *
 * @param s TODO
 * @return TODO
 */
c_keyword_t const* c_keyword_find( char const *s );

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_keywords_H */
/* vim:set et sw=2 ts=2: */
