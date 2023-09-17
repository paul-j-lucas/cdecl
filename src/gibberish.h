/*
**      cdecl -- C gibberish translator
**      src/gibberish.h
**
**      Copyright (C) 2019-2023  Paul J. Lucas
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

////////// extern functions ///////////////////////////////////////////////////

/**
 * @defgroup printing-gibberish-group Printing Gibberish
 * Functions for printing in gibberish, aka, a C/C++ declaration.
 * @{
 */

/**
 * If \ref opt_alt_tokens is `true`, gets the alternative token of a C++
 * operator \a token.
 *
 * @param token The C++ operator token to get the alternative token for.
 * @return If \ref opt_alt_tokens is `true` and if \a token is a token that has
 * an alternative token, returns said token; otherwise returns \a token as-is.
 *
 * @sa c_oper_token_c()
 * @sa graph_token_c()
 * @sa opt_alt_tokens
 */
NODISCARD
char const* alt_token_c( char const *token );

/**
 * Prints \a ast as gibberish, aka, a C/C++ declaration or cast.
 *
 * @param ast The AST to print.
 * @param gib_flags The gibberish flags to use; _must_ include one of
 * #C_GIB_PRINT_CAST, #C_GIB_PRINT_DECL, or #C_GIB_USING.
 * @param gout The `FILE` to print to.
 *
 * @sa c_ast_english()
 * @sa c_typedef_gibberish()
 * @sa print_ast_type_aka()
 * @sa print_type_decl()
 * @sa show_type()
 */
void c_ast_gibberish( c_ast_t const *ast, unsigned gib_flags, FILE *gout );

/**
 * Given \a kind, gets the associated C++ literal.
 *
 * @param kind The cast kind to get the literal for.  _Must only_ be one of
 * #C_CAST_CONST, #C_CAST_DYNAMIC, #C_CAST_REINTERPRET, or #C_CAST_STATIC.
 * @return Returns said literal.
 *
 * @sa c_cast_english()
 */
NODISCARD
char const* c_cast_gibberish( c_cast_kind_t kind );

/**
 * Prints \a tdef as a C/C++ type declaration.
 *
 * @param tdef The type to print.
 * @param gib_flags The gibberish flags to use; _must_ include either
 * #C_GIB_TYPEDEF or #C_GIB_USING.
 * @param gout The `FILE` to print to.
 *
 * @sa c_ast_gibberish()
 * @sa c_typedef_english()
 * @sa print_type_decl()
 * @sa show_type()
 */
void c_typedef_gibberish( c_typedef_t const *tdef, unsigned gib_flags,
                          FILE *gout );

/**
 * Gets the digraph or trigraph (collectively, "graph") equivalent of \a token.
 *
 * @param token The token to get the graph token for.
 * @return If we're \ref opt_graph "emitting graphs" and \a token contains one
 * or more characters that have a graph equivalent, returns \a token with said
 * characters replaced by their graphs; otherwise returns \a token as-is.
 *
 * @sa alt_token_c()
 * @sa c_oper_token_c()
 * @sa opt_graph
 */
NODISCARD
char const* graph_token_c( char const *token );

/** @} */

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_gibberish_H */
/* vim:set et sw=2 ts=2: */
