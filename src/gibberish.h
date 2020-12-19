/*
**      cdecl -- C gibberish translator
**      src/gibberish.h
**
**      Copyright (C) 2019-2020  Paul J. Lucas, et al.
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
 * Declares functions for printing in gibberish, aka, a C/C++ declaration.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>                      /* for FILE */

/// @endcond

/**
 * @defgroup printing-gibberish-group Printing Gibberish
 * Declares functions for printing in gibberish, aka, a C/C++ declaration.
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Kind of gibberish to print.  The gibberish printed varies slightly depending
 * on the kind.
 */
enum c_gib_kind {
  C_GIB_NONE    = 0u,                   ///< Not gibberish (hence, English).
  C_GIB_DECL    = (1u << 1),            ///< Gibberish is a declaration.
  C_GIB_CAST    = (1u << 2),            ///< Gibberish is a cast.
  C_GIB_TYPEDEF = (1u << 3),            ///< Gibberish is a `typedef`.
  C_GIB_USING   = (1u << 4)             ///< Gibberish is a `using`.
};

///////////////////////////////////////////////////////////////////////////////


/**
 * Prints \a ast as a C/C++ declaration or cast.
 *
 * @param ast The AST to print.
 * @param kind The kind of gibberish to print as; must only be either
 * #C_GIB_CAST or #C_GIB_DECL.
 * @param gout The `FILE` to print to.
 *
 * @sa c_typedef_gibberish()
 */
void c_ast_gibberish( c_ast_t const *ast, c_gib_kind_t kind, FILE *gout );

/**
 * Prints \a type as a C/C++ type declaration.
 *
 * @param type The type to print.
 * @param kind The kind of gibberish to print as; must only be either
 * #C_GIB_TYPEDEF or #C_GIB_USING.
 * @param gout The `FILE` to print to.
 *
 * @sa c_ast_gibberish()
 */
void c_typedef_gibberish( c_typedef_t const *type, c_gib_kind_t kind,
                          FILE *gout );

/**
 * Gets the digraph or trigraph (collectively, "graph") equivalent of \a token.
 *
 * @param token The token to get the graph token for.
 * @return If we're emitting graphs and \a token contains one or more
 * characters that have a graph equivalent, returns \a token with said
 * characters replaced by their graphs; otherwise returns \a token as-is.
 */
PJL_WARN_UNUSED_RESULT
char const* graph_token_c( char const *token );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_gibberish_H */
/* vim:set et sw=2 ts=2: */
