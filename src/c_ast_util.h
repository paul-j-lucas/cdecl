/*
**      cdecl -- C gibberish translator
**      src/c_ast_util.h
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

#ifndef cdecl_c_ast_util_H
#define cdecl_c_ast_util_H

/**
 * @file
 * Declares functions implementing various cdecl-specific algorithms for
 * construcing an Abstract Syntax Tree (AST) for parsed C/C++ declarations.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "c_ast.h"
#include "c_type.h"
#include "gibberish.h"
#include "typedefs.h"
#include "util.h"

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
   * Array and function-like declarations need a separate `c_ast` pointer that
   * points to their `of_ast` or `ret_ast` (respectively) to be the "target" of
   * subsequent additions to the AST.
   */
  c_ast_t *target_ast;
};

/**
 * The kind of semantic checks to perform on an AST.
 */
enum c_check {
  C_CHECK_CAST,                         ///< Perform checks for casts.
  C_CHECK_DECL                          ///< Perform checks for declarations.
};

///////////////////////////////////////////////////////////////////////////////

/**
 * @addtogroup ast-functions-group
 * @{
 */

/**
 * Adds an array to the AST being built.
 *
 * @param ast The `c_ast` to append to.
 * @param array The array `c_ast` to append.  Its "of" type must be null.
 * @return Returns the AST to be used as the grammar production's return value.
 */
C_WARN_UNUSED_RESULT
c_ast_t* c_ast_add_array( c_ast_t *ast, c_ast_t *array );

/**
 * Adds a function-like AST to the AST being built.
 *
 * @param ast The `c_ast` to append to.
 * @param ret_ast The `c_ast` of the return-type of the function-like AST.
 * @param func The function-like AST to append.  Its "of" type must be null.
 * @return Returns the AST to be used as the grammar production's return value.
 */
C_WARN_UNUSED_RESULT
c_ast_t* c_ast_add_func( c_ast_t *ast, c_ast_t *ret_ast, c_ast_t *func );

/**
 * Checks an entire AST for semantic errors and warnings.
 *
 * @param ast The `c_ast` to check.
 * @param check The kind of checks to perform.
 * @return Returns `true` only if \a ast error-free.
 */
C_WARN_UNUSED_RESULT
bool c_ast_check( c_ast_t const *ast, c_check_t check );

/**
 * Checks whether \a ast is an AST for a builtin type.
 *
 * @param ast The `c_ast` to check.
 * @param type_id The specific type \a ast can be.
 * @return Returns `true` only if the type of \a ast is \a type_id.
 */
C_WARN_UNUSED_RESULT
bool c_ast_is_builtin( c_ast_t const *ast, c_type_id_t type_id );

/**
 * Checks whether \a ast is an AST of one of \a kind_ids or a reference or
 * rvalue reference thereto.
 *
 * @param ast The `c_ast` to check.
 * @param kind_ids The bitwise-or of the kinds(s) \a ast can be.
 * @return Returns `true` only if \a ast is one of \a kind_ids or a reference
 * or rvalue reference thereto.
 */
C_WARN_UNUSED_RESULT
bool c_ast_is_kind_any( c_ast_t const *ast, c_kind_t kind_ids );

/**
 * Checks whether \a ast is an AST for a pointer to one of \a type_ids.
 *
 * @param ast The `c_ast` to check.
 * @param type_ids The bitwise-or of type(s) to check against.
 * @return Returns `true` only if \a ast is a pointer to one of \a type_ids.
 */
C_WARN_UNUSED_RESULT
bool c_ast_is_ptr_to_type_any( c_ast_t const *ast, c_type_id_t type_ids );

/**
 * Checks whether \a ast is an AST for a reference or rvalue reference to one
 * of \a type_ids.
 *
 * @param ast The `c_ast` to check.
 * @param type_ids The bitwise-or of type(s) to check against.
 * @return Returns `true` only if \a ast is a reference or rvalue reference to
 * one of \a type_ids.
 */
C_WARN_UNUSED_RESULT
bool c_ast_is_ref_to_type_any( c_ast_t const *ast, c_type_id_t type_ids );

/**
 * Traverses \a ast attempting to find an AST node having a name.
 *
 * @param ast The `c_ast` to begin the search at.
 * @param dir The direction to search.
 * @return Returns said name or null if none.
 */
C_WARN_UNUSED_RESULT
c_sname_t* c_ast_find_name( c_ast_t const *ast, c_visit_dir_t dir );

/**
 * Traverses \a ast attempting to find an AST node having \a kind_id.
 *
 * @param ast The `c_ast` to begin at.
 * @param dir The direction to visit.
 * @param kind_id The bitwise-or of <code>\ref c_kind</code> to find.
 * @return Returns a pointer to an AST node having \a kind_id or null if none.
 */
C_WARN_UNUSED_RESULT
c_ast_t* c_ast_find_kind( c_ast_t *ast, c_visit_dir_t dir, c_kind_t kind_id );

/**
 * Traverses \a ast attempting to find an AST node having \a type_id.
 *
 * @param ast The `c_ast` to begin at.
 * @param dir The direction to visit.
 * @param type_id The bitwise-or of of <code>\ref c_type_id_t</code> to find.
 * @return Returns a pointer to an AST node having \a type_id or null if none.
 */
C_WARN_UNUSED_RESULT
c_ast_t* c_ast_find_type( c_ast_t *ast, c_visit_dir_t dir,
                          c_type_id_t type_id );

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
C_WARN_UNUSED_RESULT
c_ast_t* c_ast_patch_placeholder( c_ast_t *type_ast, c_ast_t *decl_ast );

/**
 * Takes the name, if any, away from \a ast
 * (with the intent of giving it to another `c_ast`).
 *
 * @param ast The `c_ast` (or one of its child nodes) to take from.
 * @return Returns said name or en empty name.
 */
C_WARN_UNUSED_RESULT
c_sname_t c_ast_take_name( c_ast_t *ast );

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
 * and _not_:
 * @code
 *  declare p as pointer to typedef int
 * @endcode
 *
 * @param ast The `c_ast` to check.
 * @return Returns `true` only if \a ast contains a `typedef`.
 */
C_WARN_UNUSED_RESULT
bool c_ast_take_typedef( c_ast_t *ast );

/**
 * Un-pointers \a ast, i.e., if \a ast is a <code>\ref K_POINTER</code>,
 * returns the AST of the underlying type.
 *
 * @note `typedef`s are stripped.
 * @note Even though pointers are "dereferenced," this function isn't called
 * `c_ast_dereference` to eliminate confusion with C++ references.
 *
 * @param ast The `c_ast` to un-pointer.
 * @return Returns the AST of the underlying type, \a ast if it was not a
 * pointer, or null if \a ast is null or not a pointer.
 *
 * @sa c_ast_unreference()
 * @sa c_ast_untypedef()
 */
C_WARN_UNUSED_RESULT
c_ast_t const* c_ast_unpointer( c_ast_t const *ast );

/**
 * Un-references \a ast, i.e., if \a ast is a <code>\ref K_REFERENCE</code>
 * returns the AST of the underlying type.
 *
 * @note `typedef`s are stripped.
 * @note Only <code>\ref K_REFERENCE</code> is un-referenced, not
 * <code>\ref K_RVALUE_REFERENCE</code>.
 *
 * @param ast The `c_ast` to un-reference or null.
 * @return Returns the AST of the underlying type, \a ast if it was not a
 * reference, or null only if \a ast is null.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_untypedef()
 */
C_WARN_UNUSED_RESULT
c_ast_t const* c_ast_unreference( c_ast_t const *ast );

/**
 * Un-typedefs \a ast, i.e., if \a ast is a <code>\ref K_TYPEDEF</code>,
 * returns the AST of the underlying type.
 *
 * @param ast The `c_ast` to un-typedef or null.
 * @return Returns the AST of the underlying type, \a ast if it was not a
 * `typedef`, or null only if \a ast is null.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference()
 */
C_WARN_UNUSED_RESULT
c_ast_t const* c_ast_untypedef( c_ast_t const *ast );

///////////////////////////////////////////////////////////////////////////////

/** @} */

#endif /* cdecl_c_ast_util_H */
/* vim:set et sw=2 ts=2: */
