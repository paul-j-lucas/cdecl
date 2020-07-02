/*
**      cdecl -- C gibberish translator
**      src/c_keyword.h
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
 * Declares types and functions for looking up C/C++ keyword or C++11 (or
 * later) attribute information.
 */

// local
#include "types.h"

/**
 * @defgroup c-keywords-group C/C++ Keywords
 * Types and functions for C/C++ keywords or C++11 (or later) attributes.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * C/C++ language keyword or C++11 (or later) attribute information.
 */
struct c_keyword {
  char const   *literal;                ///< C string literal of the keyword.
  int           y_token;                ///< Bison token number.
  c_type_id_t   type_id;                ///< Type the keyword maps to.
  c_lang_id_t   lang_ids;               ///< Language(s) OK in.
};

////////// extern functions ///////////////////////////////////////////////////

/**
 * Given a literal, gets the `c_keyword` for the corresponding C++11 (or later)
 * attribute, e.g., `[[deprecated]]`.
 * @note The search is _sensitive_ to the current language.
 *
 * @param literal The literal to find.
 * @return Returns a pointer to the corresponding attribute (`c_keyword`) or
 * null for none.
 */
C_WARN_UNUSED_RESULT
c_keyword_t const* c_attribute_find( char const *literal );

/**
 * Given a literal, gets the `c_keyword` for the corresponding C/C++ keyword.
 * @note The search is _insensitive_ to the current language.
 *
 * @param literal The literal to find.
 * @param lang_id The bitwise-or of language(s) to look for the keyword in.
 * @return Returns a pointer to the corresponding `c_keyword` or null for none.
 */
C_WARN_UNUSED_RESULT
c_keyword_t const* c_keyword_find( char const *literal, c_lang_id_t lang_id );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_c_keyword_H */
/* vim:set et sw=2 ts=2: */
