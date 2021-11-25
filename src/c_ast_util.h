/*
**      cdecl -- C gibberish translator
**      src/c_ast_util.h
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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
#include "pjl_config.h"                 /* must go first */
#include "c_ast.h"
#include "c_type.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_AST_UTIL_INLINE
# define C_AST_UTIL_INLINE _GL_INLINE
#endif /* C_AST_UTIL_INLINE */

/// @endcond

////////// extern functions ///////////////////////////////////////////////////

/**
 * @addtogroup ast-functions-group
 * @{
 */

/**
 * Adds an array to the AST being built.
 *
 * @param ast The AST to append to.
 * @param array_ast The array AST to append.  Its `of_ast` must be of kind
 * #K_PLACEHOLDER.
 * @param of_ast The AST to become the `of_ast` of \a array_ast.
 * @return Returns the AST to be used as the grammar production's return value.
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_add_array( c_ast_t *ast, c_ast_t *array_ast, c_ast_t *of_ast );

/**
 * Adds a function-like AST to the AST being built.
 *
 * @param ast The AST to append to.
 * @param func_ast The function-like AST to append.  Its `ret_ast` must be
 * NULL.
 * @param ret_ast The AST to become the `ret_ast` of \a func_ast.
 * @return Returns the AST to be used as the grammar production's return value.
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_add_func( c_ast_t *ast, c_ast_t *func_ast, c_ast_t *ret_ast );

/**
 * Traverses \a ast attempting to find an AST node having one of \a kinds.
 *
 * @param ast The AST to start from; may be NULL.
 * @param dir The direction to visit.
 * @param kinds The bitwise-or of kind(s) to find.
 * @return Returns a pointer to an AST node having one of \a kinds or NULL if
 * none.
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_find_kind_any( c_ast_t *ast, c_visit_dir_t dir,
                              c_ast_kind_t kinds );

/**
 * Traverses \a ast attempting to find an AST node having a name.
 *
 * @param ast The AST to start from; may be NULL.
 * @param dir The direction to search.
 * @return Returns said name or NULL if none.
 */
PJL_WARN_UNUSED_RESULT
c_sname_t* c_ast_find_name( c_ast_t const *ast, c_visit_dir_t dir );

/**
 * Traverses \a ast attempting to find an AST node having one of \a type.
 *
 * @param ast The AST to start from; may be NULL.
 * @param dir The direction to visit.
 * @param type A type where each type ID is the bitwise-or of type IDs to find.
 * @return Returns a pointer to an AST node having one of \a type or NULL if
 * none.
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_find_type_any( c_ast_t *ast, c_visit_dir_t dir,
                              c_type_t const *type );

/**
 * Checks whether \a ast is an AST is one of \a btids built-in type(s) or a
 * `typedef` thereof.
 *
 * @param ast The AST to check.
 * @param btids The built-in type(s) \a ast can be.  They must be only base
 * types (no storage classes, qualfiers, or attributes).
 * @return Returns `true` only if the type of \a ast is one of \a btids.
 */
PJL_WARN_UNUSED_RESULT
bool c_ast_is_builtin_any( c_ast_t const *ast, c_tid_t btids );

/**
 * Checks whether \a ast is an AST for a pointer to a \a kind AST.
 *
 * @param ast The AST to check.
 * @param kind The kind to check for.
 * @return Returns `true` only if \a ast is a pointer to \a kind.
 *
 * @sa c_ast_is_ptr_to_tid_any()
 * @sa c_ast_is_ptr_to_type_any()
 */
PJL_WARN_UNUSED_RESULT
bool c_ast_is_ptr_to_kind( c_ast_t const *ast, c_ast_kind_t kind );

/**
 * Checks whether \a ast is an AST for a pointer to another AST having a type
 * that contains any one of \a tids.
 *
 * @param ast The AST to check.
 * @param tids The bitwise-or of type(s) to check against.
 * @return If \a ast is a pointer and the pointed-to AST has a type that is any
 * one of \a tids, returns the pointed-to AST; otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_kind()
 * @sa c_ast_is_ptr_to_type_any()
 * @sa c_ast_is_ref_to_tid_any()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t const* c_ast_is_ptr_to_tid_any( c_ast_t const *ast, c_tid_t tids );

/**
 * Checks whether \a ast is an AST for a pointer to another AST having \a type.
 * For example:
 *  + `c_ast_is_ptr_to_type_any( ast, &T_ANY, &C_TYPE_LIT_B(TB_CHAR) )`
 *    returns `true` only if \a ast is pointer to `char` (`char*`) _exactly_.
 *  + `c_ast_is_ptr_to_type_any( ast, &T_ANY, &C_TYPE_LIT(TB_CHAR, TS_CONST, TA_NONE) )`
 *    returns `true` only if \a ast is a pointer to `const` `char` (`char
 *    const*`) _exactly_.
 *  + `c_ast_is_ptr_to_type_any( ast, &C_TYPE_LIT_S_ANY(c_tid_compl( TS_CONST )), &C_TYPE_LIT_B(TB_CHAR) )`
 *    returns `true` if \a ast is a pointer to `char` regardless of `const`
 *    (`char*` or `char const*`).
 *
 * @param ast The AST to check.
 * @param mask_type The type mask to apply to the type of \a ast before
 * equality comparison with \a type.
 * @param type The type to check against.  Only type IDs that are not NONE are
 * compared.
 * @return Returns `true` only if \a ast (masked with \a mask_type) is a
 * pointer to an AST having \a type.
 *
 * @sa c_ast_is_ptr_to_kind()
 * @sa c_ast_is_ptr_to_tid_any()
 */
PJL_WARN_UNUSED_RESULT
bool c_ast_is_ptr_to_type_any( c_ast_t const *ast, c_type_t const *mask_type,
                               c_type_t const *type );

/**
 * Checks whether \a ast is an AST for a reference to another AST that is a
 * #TB_ANY_CLASS and has a name matching \a sname.
 *
 * @param ast The AST to check.
 * @param sname The scoped name to check for.
 * @return Returns `true` only if \a ast is a reference to #TB_ANY_CLASS and
 * has a name matching \a sname.
 *
 * @sa c_ast_is_ref_to_tid_any()
 * @sa c_ast_is_ref_to_type_any()
 */
PJL_WARN_UNUSED_RESULT
bool c_ast_is_ref_to_class_sname( c_ast_t const *ast, c_sname_t const *sname );

/**
 * Checks whether \a ast is an AST for a reference to another AST having a type
 * that contains any one of \a tids.
 *
 * @param ast The AST to check.
 * @param tids The bitwise-or of type(s) to check against.
 * @return If \a ast is a reference and the referred-to AST has a type that is
 * any one of \a tids, returns the referred-to AST; otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_tid_any()
 * @sa c_ast_is_ref_to_class_sname();
 * @sa c_ast_is_ref_to_type_any()
 * @sa c_ast_is_tid_any()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t const* c_ast_is_ref_to_tid_any( c_ast_t const *ast, c_tid_t tids );

/**
 * Checks whether \a ast is an AST for a reference to another AST having \a
 * type.
 *
 * @param ast The AST to check.
 * @param type The type to check against.  Only type IDs that are not NONE are
 * compared.
 * @return If \a ast is a reference, returns the AST \a ast is a reference to;
 * otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_type_any()
 * @sa c_ast_is_ref_to_class_sname();
 * @sa c_ast_is_ref_to_tid_any()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t const* c_ast_is_ref_to_type_any( c_ast_t const *ast,
                                         c_type_t const *type );

/**
 * Checks whether one of the type IDs of the type of \a ast is any one of \a
 * tids.
 *
 * @param ast The AST to check.  May be NULL.
 * @param tids The bitwise-or of type(s) to check against.
 * @return If the type of \a ast has one of \a tids, returns the AST after
 * `typedef`s, if any, are stripped; otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_tid_any()
 * @sa c_ast_is_ref_to_tid_any()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t const* c_ast_is_tid_any( c_ast_t const *ast, c_tid_t tids );

/**
 * Checks whether `typename` is OK since the type's name is a qualified name.
 *
 * @param ast The AST to check.
 * @return Returns `true` only upon success.
 */
PJL_WARN_UNUSED_RESULT
bool c_ast_is_typename_ok( c_ast_t const *ast );

/**
 * Joins \a type_ast and \a decl_ast into a single AST.
 *
 * @param has_typename `true` only if the declaration includes `typename`.
 * @param align The `alignas` specifier, if any.
 * @param type_ast The type AST.
 * @param decl_ast The declaration AST.
 * @return Returns The final AST on success or NULL on error.
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_join_type_decl( bool has_typename, c_alignas_t const *align,
                               c_ast_t *type_ast, c_ast_t *decl_ast );

/**
 * Gets whether the operator is a member, non-member, or unspecified.
 *
 * @param ast The AST of the operator.
 * @return Returns one of <code>\ref C_OP_MEMBER</code>,
 * <code>\ref C_OP_NON_MEMBER</code>, or <code>\ref C_OP_UNSPECIFIED</code>.
 */
PJL_WARN_UNUSED_RESULT
unsigned c_ast_oper_overload( c_ast_t const *ast );

/**
 * "Patches" \a type_ast into \a decl_ast only if:
 *  + \a type_ast has no parent.
 *  + The depth of \a type_ast is less than that of \a decl_ast.
 *  + \a decl_ast still contains an AST node of type
 *    <code>\ref K_PLACEHOLDER</code>.
 *
 * @param type_ast The AST of the initial type.
 * @param decl_ast The AST of a declaration; may be NULL.
 * @return Returns the final AST.
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_patch_placeholder( c_ast_t *type_ast, c_ast_t *decl_ast );

/**
 * Creates a <code>\ref K_POINTER</code> AST to \a ast.  The name of \a ast (or
 * one of its child nodes), if any, is moved to the new pointer AST.
 *
 * @param ast The AST to create a pointer to.
 * @param ast_list If not NULL, the new pointer AST is appended to the list.
 * @return Returns the new pointer AST.
 *
 * @sa c_ast_unpointer()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_pointer( c_ast_t *ast, c_ast_list_t *ast_list );

/**
 * Gets the root AST node starting at \a ast.
 *
 * @param ast The AST node to start from.
 * @return Returns said AST node.
 */
PJL_WARN_UNUSED_RESULT
c_ast_t* c_ast_root( c_ast_t *ast );

/**
 * Takes the name, if any, away from \a ast
 * (with the intent of giving it to another AST).
 *
 * @param ast The AST (or one of its child nodes) to take from.
 * @return Returns said name or en empty name.
 */
PJL_WARN_UNUSED_RESULT
c_sname_t c_ast_take_name( c_ast_t *ast );

/**
 * Checks \a ast to see if it contains one or more of \a type.
 * If so, removes them.
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
 * @param ast The AST to check.
 * @param type A type where each type ID is the bitwise-or of type IDs to find.
 * @return Returns the taken type.
 */
PJL_WARN_UNUSED_RESULT
c_type_t c_ast_take_type_any( c_ast_t *ast, c_type_t const *type );

/**
 * Un-pointers \a ast, i.e., if \a ast is a <code>\ref K_POINTER</code>,
 * returns the pointed-to AST.
 *
 * @note Even though pointers are "dereferenced," this function isn't called
 * `c_ast_dereference` to eliminate confusion with C++ references.
 *
 * @param ast The AST to un-pointer.
 * @return If \a ast is a pointer, returns the pointed-to AST after `typedef`s,
 * if any, are stripped; otherwise returns NULL if \a ast is not a pointer.
 *
 * @sa c_ast_pointer()
 * @sa c_ast_unreference()
 * @sa c_ast_unrvalue_reference()
 * @sa c_ast_untypedef()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t const* c_ast_unpointer( c_ast_t const *ast );

/**
 * Un-references \a ast, i.e., if \a ast is a <code>\ref K_REFERENCE</code>
 * returns the referred-to AST.
 *
 * @note Only <code>\ref K_REFERENCE</code> is un-referenced, not
 * <code>\ref K_RVALUE_REFERENCE</code>.
 *
 * @param ast The AST to un-reference.
 * @return If \a ast is a reference, Returns the referenced AST after
 * `typedef`s, if any, are stripped; otherwise returns \a ast if \a ast is not
 * a reference.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unrvalue_reference()
 * @sa c_ast_untypedef()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t const* c_ast_unreference( c_ast_t const *ast );

/**
 * Un-rvalue-references \a ast, i.e., if \a ast is a <code>\ref
 * K_RVALUE_REFERENCE</code> returns the referred-to AST.
 *
 * @note Only <code>\ref K_RVALUE_REFERENCE</code> is un-referenced, not
 * <code>\ref K_REFERENCE</code>.
 *
 * @param ast The AST to un-reference.
 * @return If \a ast is an rvalue reference, Returns the referenced AST after
 * `typedef`s, if any, are stripped; otherwise returns \a ast if \a ast is not
 * an rvalue reference.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference()
 * @sa c_ast_untypedef()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t const* c_ast_unrvalue_reference( c_ast_t const *ast );

/**
 * Un-`typedef`s \a ast, i.e., if \a ast is a <code>\ref K_TYPEDEF</code>,
 * returns the AST the `typedef` is for.
 *
 * @param ast The AST to un-`typedef`.
 * @return Returns the AST the `typedef` is for or \a ast if \a ast is not a
 * `typedef`.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference()
 * @sa c_ast_unrvalue_reference()
 */
PJL_WARN_UNUSED_RESULT
c_ast_t const* c_ast_untypedef( c_ast_t const *ast );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks if the type of \a ast is equivalent to `size_t`.
 *
 * @note
 * In cdecl, `size_t` is `typedef`d to be `unsigned long` in `c_typedef.c`.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is `size_t`.
 *
 * @sa c_tid_is_size_t()
 */
C_AST_UTIL_INLINE PJL_WARN_UNUSED_RESULT
bool c_ast_is_size_t( c_ast_t const *ast ) {
  return c_tid_is_size_t( c_ast_untypedef( ast )->type.btids );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_ast_util_H */
/* vim:set et sw=2 ts=2: */
