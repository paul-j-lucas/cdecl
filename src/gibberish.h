/*
**      cdecl -- C gibberish translator
**      src/gibberish.h
**
**      Copyright (C) 2019  Paul J. Lucas, et al.
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

#ifndef cdecl_gibberish_H
#define cdecl_gibberish_H

/**
 * @file
 * Declares functions for printing an AST in gibberish, aka, a C/C++
 * declaration.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "typedefs.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @defgroup printing-gibberish-group Printing Gibberish
 * Declares functions for printing an AST in gibberish, aka, a C/C++
 * declaration.
 * @{

 */
// Gibberish declaration flags.
#define G_DECL_NONE           0u        /**< None. */
#define G_DECL_TYPEDEF        (1u << 0) /**< Is a typedef. */

///////////////////////////////////////////////////////////////////////////////

/**
 * Gets the alternate token of \a token.
 *
 * @param token The token to get the alternate token for.
 * @return If we're emitting alternate tokens and if \a token is a token that
 * has an alternate token, returns said token; otherwise returns \a token as-
 * is.
 */
char const* alt_token_c( char const *token );

/**
 * Prints \a ast as a C/C++ cast.
 *
 * @param ast The `c_ast` to print.
 * @param gout The `FILE` to print to.
 *
 * @sa c_ast_gibberish_declare()
 */
void c_ast_gibberish_cast( c_ast_t const *ast, FILE *gout );

/**
 * Prints \a ast as a C/C++ declaration.
 *
 * @param ast The `c_ast` to print.
 * @param flags The bitwise-or of gibberish declaration flags.
 * @param gout The `FILE` to print to.
 *
 * @sa c_ast_gibberish_cast()
 */
void c_ast_gibberish_declare( c_ast_t const *ast, unsigned flags, FILE *gout );

/**
 * Gets the digraph or trigraph (collectively, "graph") equivalent of \a token.
 *
 * @param token The token to get the graph token for.
 * @return If we're emitting graphs and \a token contains one or more
 * characters that have a graph equivalent, returns \a token with said
 * characters replaced by their graphs; otherwise returns \a token as-is.
 */
char const* graph_token_c( char const *token );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_gibberish_H */
/* vim:set et sw=2 ts=2: */
