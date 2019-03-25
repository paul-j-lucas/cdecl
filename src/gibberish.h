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
#include "c_sname.h"
#include "typedefs.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdio.h>                      /* for FILE */

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_GIBBERISH_INLINE
# define CDECL_GIBBERISH_INLINE _GL_INLINE
#endif /* CDECL_GIBBERISH_INLINE */
/// @endcond

// Gibberish declaration flags.
#define G_DECL_NONE           0u        /**< None. */
#define G_DECL_TYPEDEF        (1u << 0) /**< Is a typedef. */

///////////////////////////////////////////////////////////////////////////////

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
 * Gets the fully scoped name of \a sname.
 *
 * @param sname The `c_sname_t` to get the full name of.
 * @return Returns said name or the empty string if \a sname is empty.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_local
 * @sa c_sname_scope_c
 */
char const* c_sname_full_c( c_sname_t const *sname );

/**
 * Gets just the scope name of \a sname.
 * Examples:
 *  + For `a::b::c`, returns `a::b`.
 *  + For `c`, returns the empty string.
 *
 * @param sname The `c_sname_t` to get the scope name of.
 * @return Returns said name or the empty string if \a sname is empty or the
 * name is not within a scope.
 * @warning The pointer returned is to a static buffer, so you can't do
 * something like call this twice in the same `printf()` statement.
 *
 * @sa c_sname_full_c
 * @sa c_sname_local
 */
char const* c_sname_scope_c( c_sname_t const *sname );

/**
 * Gets the local (last) name of \a sname.
 *
 * @param sname The `c_sname_t` to get the local name of.
 * @return Returns said name or the empty string if \a sname is empty.
 *
 * @sa c_sname_full_c
 * @sa c_sname_scope_c
 */
CDECL_GIBBERISH_INLINE char const* c_sname_local( c_sname_t const *sname ) {
  return c_sname_empty( sname ) ? "" : SLIST_TAIL( char const*, sname );
}

/**
 * Gets the digraph or trigraph (collectively, "graph") equivalent of \a token.
 *
 * @param token The token to get the equivalent name for.
 * @return If \a token contains one or more characters that have a graph
 * equivalent and we're emitting graphs, returns \a token with said characters
 * replaced by their graphs; otherwise returns \a token as-is.
 */
char const* graph_name_c( char const *token );

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_gibberish_H */
/* vim:set et sw=2 ts=2: */
