/*
**      cdecl -- C gibberish translator
**      src/english.h
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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

#ifndef cdecl_english_H
#define cdecl_english_H

/**
 * @file
 * Defines functions for printing in pseudo-English.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @defgroup printing-english-group Printing English
 * Functions for printing in pseudo-English.
 * @{
 */

////////// extern functions ///////////////////////////////////////////////////

/**
 * Explains \a ast in pseudo-English.
 *
 * @param ast The AST to explain.
 * @param eout The `FILE` to print to.
 *
 * @note A newline _is_ printed.
 *
 * @sa c_ast_gibberish()
 * @sa c_ast_list_english()
 * @sa c_typedef_english()
 * @sa print_type()
 * @sa show_type()
 */
void c_ast_english( c_ast_t const *ast, FILE *eout );

/**
 * Explains \a ast_list in pseudo-English.
 *
 * @param ast_list The list of AST nodes to explain.
 * @param eout The `FILE` to print to.
 *
 * @note A newline _is_ printed.
 *
 * @sa c_ast_english()
 * @sa c_ast_gibberish()
 * @sa c_typedef_english()
 */
void c_ast_list_english( c_ast_list_t const *ast_list, FILE *eout );

/**
 * Given \a kind, gets the associated English literal.
 *
 * @param kind The cast kind to get the literal for.
 * @return Returns said literal.
 *
 * @sa c_cast_gibberish()
 */
NODISCARD
char const* c_cast_english( c_cast_kind_t kind );

/**
 * Prints \a sname in pseudo-English.
 *
 * @param sname The name to print.
 * @param eout The `FILE` to print to.
 *
 * @note A newline is _not_ printed.
 *
 * @sa c_sname_full_name()
 */
void c_sname_english( c_sname_t const *sname, FILE *eout );

/**
 * Explains \a tdef as a type in pseudo-English.
 *
 * @param tdef The type to explain.
 * @param eout The `FILE` to print to.
 *
 * @sa c_ast_english()
 * @sa c_typedef_gibberish()
 * @sa print_type()
 * @sa show_type()
 */
void c_typedef_english( c_typedef_t const *tdef, FILE *eout );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_english_H */
/* vim:set et sw=2 ts=2: */
