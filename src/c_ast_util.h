/*
**      cdecl -- C gibberish translator
**      src/c_ast_util.h
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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

#ifndef cdecl_c_ast_util_H
#define cdecl_c_ast_util_H

/**
 * @file
 * Declares functions implementing various cdecl-specific algorithms for
 * construcing an Abstract Syntax Tree (AST) for parsed C/C++ declarations.
 */

// local
#include "config.h"                     /* must go first */
#include "c_ast.h"
#include "c_type.h"
#include "typedefs.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

_GL_INLINE_HEADER_BEGIN
#ifndef CDECL_AST_UTIL_INLINE
# define CDECL_AST_UTIL_INLINE _GL_INLINE
#endif /* CDECL_AST_UTIL_INLINE */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * A pair of `c_ast` pointers used as one of the synthesized attribute types in
 * the parser.
 */
struct c_ast_pair {
  /**
   * A pointer to the AST being built.
   */
  c_ast_t *ast;

  /**
   * Array and function (or block) declarations need a separate `c_ast` pointer
   * that points to their `of_ast` or `ret_ast` (respectively) to be the
   * "target" of subsequent additions to the AST.
   */
  c_ast_t *target_ast;
};

/**
 * The kind of semantic checks to perform on an AST.
 */
enum c_check {
  CHECK_CAST,                           ///< Perform checks for casts.
  CHECK_DECL                            ///< Perform checks for declarations.
};

///////////////////////////////////////////////////////////////////////////////

/**
 * A visitor function used to find an AST node of a particular kind.
 *
 * @param ast The `c_ast` to check.
 * @param data The bitwise-or of <code>\ref c_kind</code> (cast to `void*`) to
 * find.
 * @return Returns `true` only if the kind of \a ast is one of \a data.
 */
bool c_ast_vistor_kind( c_ast_t *ast, void *data );

/**
 * A visitor function used to find a name.
 *
 * @param ast The `c_ast` to check.
 * @param data Not used.
 * @return Returns `true` only if \a ast has a name.
 */
bool c_ast_visitor_name( c_ast_t *ast, void *data );

/**
 * A visitor function used to find a type.
 *
 * @param ast The `c_ast` to check.
 * @param data The bitwise-or of <code>\ref c_type_id_t</code> (cast to
 * `void*`) to find.
 * @return Returns `true` only if the type of \a ast is one of \a data.
 */
bool c_ast_vistor_type( c_ast_t *ast, void *data );

/**
 * Adds an array to the AST being built.
 *
 * @param ast The `c_ast` to append to.
 * @param array The array `c_ast` to append.  Its "of" type must be null.
 * @return Returns the AST to be used as the grammar production's return value.
 */
c_ast_t* c_ast_add_array( c_ast_t *ast, c_ast_t *array );

/**
 * Adds a function (or block) to the AST being built.
 *
 * @param ast The `c_ast` to append to.
 * @param ret_ast The `c_ast` of the return-type of the function (or block).
 * @param func The function (or block) `c_ast` to append.  Its "of" type must
 * be null.
 * @return Returns the AST to be used as the grammar production's return value.
 */
c_ast_t* c_ast_add_func( c_ast_t *ast, c_ast_t *ret_ast, c_ast_t *func );

/**
 * Checks an entire AST for semantic errors and warnings.
 *
 * @param ast The `c_ast` to check.
 * @param check The kind of checks to perform.
 * @return Returns `true` only if \a ast  error-free.
 */
bool c_ast_check( c_ast_t const *ast, c_check_t check );

/**
 * Checks whether \a ast is an AST for a builtin type.
 *
 * @param ast The `c_ast` to check.
 * @param type_id The bitwise-or of the type(s) \a ast can be or
 * <code>\ref T_NONE</code> for any builtin type.
 * @return Returns `true` only if \a ast is a builtin type.
 */
bool c_ast_is_builtin( c_ast_t const *ast, c_type_id_t type_id );

/**
 * Checks whether \a ast is an AST for an `enum`, `class`, `struct`, or `union`
 * or a reference or rvalue reference thereto.
 *
 * @param ast The `c_ast` to check.
 * @return Returns `true` only if \a ast is an `enum`, `class`, `struct`, or
 * `union` or a reference or rvalue reference thereto.
 */
bool c_ast_is_ecsu( c_ast_t const *ast );

/**
 * Checks whether \a ast is an AST for a pointer to \a type_id.
 *
 * @param ast The `c_ast` to check.
 * @param type_id The bitwise-or of type(s) to check against.
 * @return Returns `true` only if \a ast is a pointer to one of the types.
 */
bool c_ast_is_ptr_to( c_ast_t const *ast, c_type_id_t type_id );

/**
 * Prints \a ast as pseudo-English.
 *
 * @param ast The `c_ast` to print.  May be null.
 * @param fout The `FILE` to print to.
 */
void c_ast_english( c_ast_t const *ast, FILE *fout );

/**
 * Traverses \a ast attempting to find an AST node having \a kind.
 *
 * @param ast The `c_ast` to begin at.
 * @param dir The direction to visit.
 * @param kind The bitwise-or of <code>\ref c_kind</code> to find.
 * @return Returns a pointer to an AST node having \a kind or null if none.
 */
CDECL_AST_UTIL_INLINE c_ast_t* c_ast_find_kind( c_ast_t *ast,
                                                v_direction_t dir,
                                                c_kind_t kind ) {
  void *const data = REINTERPRET_CAST( void*, kind );
  return c_ast_visit( ast, dir, c_ast_vistor_kind, data );
}

/**
 * Traverses \a ast attempting to find an AST node having \a type_id.
 *
 * @param ast The `c_ast` to begin at.
 * @param dir The direction to visit.
 * @param type_id The bitwise-or of of <code>\ref c_type_id_t</code> to find.
 * @return Returns a pointer to an AST node having \a type_id or null if none.
 */
CDECL_AST_UTIL_INLINE c_ast_t* c_ast_find_type( c_ast_t *ast,
                                                v_direction_t dir,
                                                c_type_id_t type_id ) {
  void *const data = REINTERPRET_CAST( void*, type_id );
  return c_ast_visit( ast, dir, c_ast_vistor_type, data );
}

/**
 * Prints \a ast as a C/C++ cast.
 *
 * @param ast The `c_ast` to print.
 * @param fout The `FILE` to print to.
 */
void c_ast_gibberish_cast( c_ast_t const *ast, FILE *fout );

/**
 * Prints \a ast as a C/C++ declaration.
 *
 * @param ast The `c_ast` to print.
 * @param fout The `FILE` to print to.
 */
void c_ast_gibberish_declare( c_ast_t const *ast, FILE *fout );

/**
 * Gets the name from \a ast.
 *
 * @param ast The `c_ast` to begin the search at.
 * @param dir The direction to search.
 * @return Returns said name or null if none.
 */
char const* c_ast_name( c_ast_t const *ast, v_direction_t dir );

/**
 * "Patches" \a type_ast into \a decl_ast only if:
 *  + \a type_ast has no parent.
 *  + The depth of \a type_ast is less than that of \a decl_ast.
 *  + \a decl_ast still contains an AST node of type
 *    <code>\ref K_PLACEHOLDER</code>.
 *
 * @param type_ast The `c_ast` of the initial type.
 * @param decl_ast The `c_ast` of a declaration.  May be null.
 * @return Returns the final `c_ast`.
 */
c_ast_t* c_ast_patch_placeholder( c_ast_t *type_ast, c_ast_t *decl_ast );

/**
 * Takes the name, if any, away from \a ast
 * (with the intent of giving it to another `c_ast`).
 *
 * @param ast The `c_ast` (or one of its child nodes) to take from.
 * @return Returns said name or null.  The caller is responsible for freeing
 * the string.
 */
char const* c_ast_take_name( c_ast_t *ast );

/**
 * Checks \a ast to see if it contains a `typedef`.
 * If so, removes it.
 * This is used in cases like:
 * @code
 *  explain typedef int *p
 * @endcode
 * that should be explained as:
 * @code
 *  declare p as type pointer to int
 * @endcode
 * and \e not:
 * @code
 *  declare p as pointer to typedef int
 * @endcode
 *
 * @param ast The `c_ast` to check.
 * @return Returns `true` only if \a ast contains a `typedef`.
 */
bool c_ast_take_typedef( c_ast_t *ast );

/**
 * Un-references \a ast, i.e., if \a ast is a <code>\ref K_REFERENCE</code> or
 * <code>\ref K_RVALUE_REFERENCE</code> returns the AST of the underlying type.
 *
 * @param ast The `c_ast` to un-reference.
 * @return Returns the AST of the underlying type.
 */
c_ast_t const* c_ast_unreference( c_ast_t const *ast );

/**
 * Un-typedefs \a ast, i.e., if \a ast is a <code>\ref K_TYPEDEF</code>,
 * returns the AST of the underlying type.
 *
 * @param ast The `c_ast` to un-typedef.
 * @return Returns the AST of the underlying type.
 */
c_ast_t const* c_ast_untypedef( c_ast_t const *ast );

///////////////////////////////////////////////////////////////////////////////

_GL_INLINE_HEADER_END

#endif /* cdecl_c_ast_util_H */
/* vim:set et sw=2 ts=2: */
