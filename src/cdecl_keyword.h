/*
**      cdecl -- C gibberish translator
**      src/cdecl_keyword.h
**
**      Copyright (C) 2021  Paul J. Lucas
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

#ifndef cdecl_cdecl_keyword_H
#define cdecl_cdecl_keyword_H

/**
 * @file
 * Declares types and functions for looking up cdecl keyword information.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast.h"
#include "types.h"
#include "parser.h"                     /* must go last */

// standard
#include <stdbool.h>

/**
 * cdecl keyword info.
 */
struct cdecl_keyword {
  char const         *literal;          ///< C string literal of the keyword.
  bool                allow_explain;    ///< Allow when explaining C/C++?
  yytokentype         yy_token_id;      ///< Bison token number; or:

  /**
   * Array of language(s)/synonym-keyword pair(s).  The array is terminated by
   * an element that has #LANG_ANY for lang_ids; hence subset(s) of language(s)
   * cases come first and, failing to match opt_lang against any of those,
   * matches the last (default) element.
   */
  c_lang_lit_t const *lang_syn;
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Finds a cdecl keyword matching \a s, if any.
 *
 * @param s The string to find.
 * @return Returns a pointer to the corresponding keyword or NULL if not found.
 */
PJL_WARN_UNUSED_RESULT
cdecl_keyword_t const* cdecl_keyword_find( char const *s );

/**
 * Iterates to the next cdecl keyword.
 *
 * @param k A pointer to the previous keyword. For the first iteration, NULL
 * should be passed.
 * @return Returns the next cdecl keyword or NULL for none.
 *
 * @sa #FOREACH_CDECL_KEYWORD
 */
PJL_WARN_UNUSED_RESULT
cdecl_keyword_t const* cdecl_keyword_next( cdecl_keyword_t const *k );

/**
 * Convenience macro for iterating over all cdecl keywords.
 *
 * @param VAR The `cdecl_keyword` loop variable.
 *
 * @sa cdecl_keyword_next()
 */
#define FOREACH_CDECL_KEYWORD(VAR) \
  for ( cdecl_keyword_t const *VAR = NULL; (VAR = cdecl_keyword_next( VAR )) != NULL; )

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_cdecl_keyword_H */
/* vim:set et sw=2 ts=2: */
