/*
**      cdecl -- C gibberish translator
**      src/gibberish.h
**
**      Copyright (C) 2019-2026  Paul J. Lucas
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
 * Prints \a ast as gibberish, aka, a C/C++ declaration or cast.
 *
 * @param ast The AST to print.
 * @param gib_flags The gibberish flags to use; _must_ include one of
 * #C_GIB_PRINT_CAST, #C_GIB_PRINT_DECL, or #C_GIB_USING.
 * @param fout The `FILE` to print to.
 *
 * @sa c_ast_english()
 * @sa c_ast_sname_list_gibberish()
 * @sa c_typedef_gibberish()
 * @sa print_ast_type_aka()
 * @sa print_type_decl()
 * @sa show_type()
 */
void c_ast_gibberish( c_ast_t const *ast, decl_flags_t gib_flags, FILE *fout );

/**
 * Prints the names in \a sname_list as gibberish, aka, C/C++ declarations.
 * For example, if \a sname_list is [ `"x"`, `"y"` ] and \a ast is:
 *
 *      $$_ast: {
 *        sname: null,
 *        kind: { value: 0x400, string: "pointer" },
 *        ...
 *        ptr_ref: {
 *          to_ast: {
 *            sname: null,
 *            kind: { value: 0x2, string: "built-in type" },
 *            ...
 *            type: { btid: 0x4001, stid: 0x2, atid: 0x4, string: "int" },
 *            ...
 *          }
 *        }
 *      }
 *
 * prints:
 *
 *      int *x, *y;
 *
 * @param ast The AST that is the type to print.
 * @param sname_list The names to print as \a ast.
 * @param fout The `FILE` to print to.
 *
 * @sa c_ast_gibberish()
 */
void c_ast_sname_list_gibberish( c_ast_t *ast, slist_t const *sname_list,
                                 FILE *fout );

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
 * Gets the fully scoped name of \a sname.
 *
 * @param sname The scoped name to get the full name of; may be NULL.
 * @return Returns said name or the empty string if \a sname is empty or NULL.
 *
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_local_name()
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_gibberish()
 * @sa c_sname_scope_sname()
 */
NODISCARD
char const* c_sname_gibberish( c_sname_t const *sname );

/**
 * Gets just the scope name of \a sname.
 * Examples:
 *  + For `a::b::c`, returns `a::b`.
 *  + For `c`, returns the empty string.
 *
 * @param sname The scoped name to get the scope name of; may be NULL.
 * @return Returns said name or the empty string if \a sname is empty, NULL, or
 * not within a scope.
 *
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_gibberish()
 * @sa c_sname_global_name()
 * @sa c_sname_local_name()
 * @sa c_sname_name_atr()
 * @sa c_sname_scope_sname()
 */
NODISCARD
char const* c_sname_scope_gibberish( c_sname_t const *sname );

/**
 * Prints \a tdef as a C/C++ type declaration.
 *
 * @param tdef The type to print.
 * @param gib_flags The gibberish flags to use; _must_ include either
 * #C_GIB_TYPEDEF or #C_GIB_USING.
 * @param fout The `FILE` to print to.
 *
 * @sa c_ast_gibberish()
 * @sa c_typedef_english()
 * @sa print_type_decl()
 * @sa show_type()
 */
void c_typedef_gibberish( c_typedef_t const *tdef, decl_flags_t gib_flags,
                          FILE *fout );

/**
 * Gets either the altertative or "graph" token, if any, of \a token.
 *
 * @param token The C or C++ token to get the other token for.
 * @return
 *  + If \ref opt_alt_tokens is `true`, returns the alternative token of \a
 *    token, if any; or:
 * + If \ref opt_graph is not #C_GRAPH_NONE, returns the "graph" token of \a
 *   token, if any; or:
 * + Returns \a token as-is.
 *
 * @sa alt_token_c()
 * @sa c_op_token_c()
 * @sa graph_token_c()
 */
NODISCARD
char const* other_token_c( char const *token );

/** @} */

///////////////////////////////////////////////////////////////////////////////

#endif /* cdecl_gibberish_H */
/* vim:set et sw=2 ts=2: */
