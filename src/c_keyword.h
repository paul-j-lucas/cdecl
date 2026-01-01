/*
**      cdecl -- C gibberish translator
**      src/c_keyword.h
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

#ifndef cdecl_c_keyword_H
#define cdecl_c_keyword_H

/**
 * @file
 * Declares types and functions for looking up C/C++ keyword or C23/C++11 (or
 * later) attribute information.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast.h"
#include "types.h"
#include "cdecl_parser.h"               /* must go last */

/**
 * @defgroup c-keywords-group C/C++ Keywords
 * Types and functions for C/C++ keywords or C23/C++11 (or later) attributes.
 * @{
 */

/**
 * Convenience macro for iterating over all C/C++ keywords.
 *
 * @param VAR The \ref c_keyword loop variable.
 *
 * @sa c_keyword_next()
 */
#define FOREACH_C_KEYWORD(VAR) \
  for ( c_keyword_t const *VAR = NULL; (VAR = c_keyword_next( VAR )) != NULL; )

///////////////////////////////////////////////////////////////////////////////

/**
 * C++ keyword contexts.
 *
 * @remarks A context specifies where particular literals are recognized as
 * keywords in gibberish.  For example, `final` and `override` are recognized
 * as keywords only within C++ member function declarations.
 *
 * @note These matter only when converting gibberish to pseudo-English.
 */
enum c_keyword_ctx {
  C_KW_CTX_DEFAULT,                     ///< Default context.
  C_KW_CTX_ATTRIBUTE,                   ///< Attribute declaration.
  C_KW_CTX_MBR_FUNC                     ///< Member function declaration.
};
typedef enum c_keyword_ctx c_keyword_ctx_t;

/**
 * C/C++ language keyword or C23/C++11 (or later) attribute information.
 */
struct c_keyword {
  char const     *literal;              ///< C string literal of the keyword.
  yytoken_kind_t  y_token_id;           ///< Bison token (`Y_xxx`).
  c_keyword_ctx_t kw_ctx;               ///< Keyword context.
  c_tid_t         tid;                  ///< Type the keyword maps to, if any.
  c_lang_id_t     lang_ids;             ///< Language(s) OK in.
#ifdef WITH_READLINE
  /**
   * Language(s) autocompletable in.
   *
   * @remarks
   * @parblock
   * Relative to `lang_ids`, this field:
   *
   *  1. Is exactly the same in which case it's autocompletable in all (and
   *     only) those language(s) in which it's valid.
   *
   *  2. Is a subset.  This is for a case like `restrict` where it's a C-only
   *     keyword, but we also allow it to be recognized in C++ so we can give a
   *     better error message (saying it's not supported in C++).  However,
   *     even though we also allow it to be recognized in C++, we do _not_
   *     allow it to be autocompletable in C++.
   *
   *  3. Is #LANG_NONE.  This is for a case like `break` where it's a keyword
   *     in all languages, but it's not used in declarations; hence there's no
   *     reason to allow it to be autocompletable.
   * @endparblock
   */
  c_lang_id_t     ac_lang_ids;
#endif /* WITH_READLINE */
};
typedef struct c_keyword c_keyword_t;

////////// extern functions ///////////////////////////////////////////////////

/**
 * Given a literal, gets the c_keyword for the corresponding C/C++ keyword or
 * C23/C++11 (or later) attribute, if any.
 *
 * @param literal The literal to find.
 * @param lang_ids The bitwise-or of language(s) to look for the keyword or
 * attribute in.
 * @param kw_ctx The keyword context to limit to.
 * @return Returns a pointer to the corresponding c_keyword or NULL if not
 * found.
 */
NODISCARD
c_keyword_t const* c_keyword_find( char const *literal, c_lang_id_t lang_ids,
                                   c_keyword_ctx_t kw_ctx );

/**
 * Iterates to the next C/C++ keyword or or C23/C++11 (or later) attribute
 *
 * @param k A pointer to the current keyword or attribute. For the first
 * iteration, NULL should be passed.
 * @return Returns the next C/C++ keyword or C23/C++11 (or later) attribute, or
 * NULL for none.
 *
 * @note This function isn't normally called directly; use the
 * #FOREACH_C_KEYWORD() macro instead.
 *
 * @sa #FOREACH_C_KEYWORD()
 */
NODISCARD
c_keyword_t const* c_keyword_next( c_keyword_t const *k );

/**
 * Initializes \ref c_keyword data.
 *
 * @note This function must be called exactly once.
 */
void c_keywords_init( void );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_c_keyword_H */
/* vim:set et sw=2 ts=2: */
