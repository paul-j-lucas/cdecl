/*
**      cdecl -- C gibberish translator
**      src/c_ast_util.h
**
**      Copyright (C) 2017-2022  Paul J. Lucas
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
#include "options.h"
#include "types.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <stdbool.h>

_GL_INLINE_HEADER_BEGIN
#ifndef C_AST_UTIL_H_INLINE
# define C_AST_UTIL_H_INLINE _GL_INLINE
#endif /* C_AST_UTIL_H_INLINE */

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
NODISCARD
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
NODISCARD
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
NODISCARD
c_ast_t* c_ast_find_kind_any( c_ast_t *ast, c_visit_dir_t dir,
                              c_ast_kind_t kinds );

/**
 * Traverses \a ast attempting to find an AST node having a name.
 *
 * @param ast The AST to start from; may be NULL.
 * @param dir The direction to search.
 * @return Returns said name or NULL if none.
 */
NODISCARD
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
NODISCARD
c_ast_t* c_ast_find_type_any( c_ast_t *ast, c_visit_dir_t dir,
                              c_type_t const *type );

/**
 * Checks whether \a ast is an AST is one of \a btids built-in type(s); or a
 * `typedef` thereof.
 *
 * @param ast The AST to check.
 * @param btids The built-in type(s) \a ast can be.  They must be only base
 * types (no storage classes, qualfiers, or attributes).
 * @return Returns `true` only if the type of \a ast is one of \a btids.
 */
NODISCARD
bool c_ast_is_builtin_any( c_ast_t const *ast, c_tid_t btids );

/**
 * Checks whether \a ast is an AST for a #K_POINTER to another AST that is one
 * of \a kinds; or a `typedef` thereof.
 *
 * @param ast The AST to check.
 * @param kinds The bitwise-or of kind(s) to check for.
 * @return Returns `true` only if \a ast is a #K_POINTER to one of \a kinds.
 *
 * @sa c_ast_is_ptr_to_tid_any()
 * @sa c_ast_is_ptr_to_type_any()
 * @sa c_ast_is_ref_to_kind_any()
 * @sa c_ast_is_ref_to_tid_any()
 * @sa c_ast_is_ref_to_type_any()
 */
NODISCARD
bool c_ast_is_ptr_to_kind_any( c_ast_t const *ast, c_ast_kind_t kinds );

/**
 * Checks whether \a ast is an AST for a #K_POINTER to another AST having a
 * type that contains any one of \a tids ; or a `typedef` thereof.
 *
 * @param ast The AST to check.
 * @param tids The bitwise-or of type(s) to check against.
 * @return If \a ast is a pointer and the pointed-to AST has a type that is any
 * one of \a tids, returns the pointed-to AST; otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_kind_any()
 * @sa c_ast_is_ptr_to_type_any()
 * @sa c_ast_is_ref_to_kind_any()
 * @sa c_ast_is_ref_to_tid_any()
 * @sa c_ast_is_ref_to_type_any()
 */
NODISCARD
c_ast_t const* c_ast_is_ptr_to_tid_any( c_ast_t const *ast, c_tid_t tids );

/**
 * Checks whether \a ast is an AST for a pointer to another AST having \a type;
 * or a `typedef` thereof.  For example:
 *  + `c_ast_is_ptr_to_type_any( ast, &T_ANY, &C_TYPE_LIT_B(TB_CHAR) )` returns
 *    `true` only if \a ast is pointer to `char` (`char*`) _exactly_.
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
 * @sa c_ast_is_ptr_to_kind_any()
 * @sa c_ast_is_ptr_to_tid_any()
 * @sa c_ast_is_ref_to_kind_any()
 * @sa c_ast_is_ref_to_tid_any()
 * @sa c_ast_is_ref_to_type_any()
 */
NODISCARD
bool c_ast_is_ptr_to_type_any( c_ast_t const *ast, c_type_t const *mask_type,
                               c_type_t const *type );

/**
 * Checks whether \a ast is an AST for a reference to another AST that is a
 * #TB_ANY_CLASS and has a name matching \a sname; or a `typedef` thereof.
 *
 * @param ast The AST to check.
 * @param sname The scoped name to check for.
 * @return Returns `true` only if \a ast is a reference to #TB_ANY_CLASS and
 * has a name matching \a sname.
 *
 * @sa c_ast_is_ref_to_kind_any()
 * @sa c_ast_is_ref_to_tid_any()
 * @sa c_ast_is_ref_to_type_any()
 */
NODISCARD
bool c_ast_is_ref_to_class_sname( c_ast_t const *ast, c_sname_t const *sname );

/**
 * Checks whether \a ast is an AST for a reference to another AST that is one
 * of \a kinds; or a `typedef` thereof.
 *
 * @param ast The AST to check.
 * @param kinds The bitwise-or of kind(s) to check for.
 * @return Returns `true` only if \a ast is a reference to one of \a kinds.
 *
 * @sa c_ast_is_ptr_to_kind_any()
 * @sa c_ast_is_ptr_to_tid_any()
 * @sa c_ast_is_ptr_to_type_any()
 * @sa c_ast_is_ref_to_class_sname()
 * @sa c_ast_is_ref_to_tid_any()
 * @sa c_ast_is_ref_to_type_any()
 */
NODISCARD
bool c_ast_is_ref_to_kind_any( c_ast_t const *ast, c_ast_kind_t kinds );

/**
 * Checks whether \a ast is an AST for a reference to another AST having a type
 * that contains any one of \a tids; or a `typedef` thereof.
 *
 * @param ast The AST to check.
 * @param tids The bitwise-or of type(s) to check against.
 * @return If \a ast is a reference and the referred-to AST has a type that is
 * any one of \a tids, returns the referred-to AST; otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_kind_any()
 * @sa c_ast_is_ptr_to_tid_any()
 * @sa c_ast_is_ptr_to_type_any()
 * @sa c_ast_is_ref_to_class_sname()
 * @sa c_ast_is_ref_to_kind_any()
 * @sa c_ast_is_ref_to_type_any()
 * @sa c_ast_is_tid_any()
 * @sa c_ast_is_tid_any_qual()
 */
NODISCARD
c_ast_t const* c_ast_is_ref_to_tid_any( c_ast_t const *ast, c_tid_t tids );

/**
 * Checks whether \a ast is an AST for a reference to another AST having \a
 * type; or a `typedef` thereof.
 *
 * @param ast The AST to check.
 * @param type The type to check against.  Only type IDs that are not NONE are
 * compared.
 * @return If \a ast is a reference, returns the AST \a ast is a reference to;
 * otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_kind_any()
 * @sa c_ast_is_ptr_to_tid_any()
 * @sa c_ast_is_ptr_to_type_any()
 * @sa c_ast_is_ref_to_class_sname()
 * @sa c_ast_is_ref_to_kind_any()
 * @sa c_ast_is_ref_to_tid_any()
 */
NODISCARD
c_ast_t const* c_ast_is_ref_to_type_any( c_ast_t const *ast,
                                         c_type_t const *type );

/**
 * Checks whether one of the type IDs of the type of \a ast is any one of \a
 * tids.  This function is a variant of c_ast_is_tid_any() that also returns
 * the qualifier(s) of \a ast.
 *
 * @param ast The AST to check.
 * @param tids The bitwise-or of type(s) to check against.
 * @param qual_stids Receives the qualifier(s) of \a ast bitwise-or'd with the
 * qualifier(s) \a ast is a `typedef` for (if \a ast is a #K_TYPEDEF), but only
 * if this function returns non-NULL.
 * @return If the type of \a ast has one of \a tids, returns the un-`typedef`d
 * AST of \a ast; otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_type_any()
 * @sa c_ast_is_ref_to_type_any()
 * @sa c_ast_is_tid_any()
 */
NODISCARD
c_ast_t const* c_ast_is_tid_any_qual( c_ast_t const *ast, c_tid_t tids,
                                      c_tid_t *qual_stids );

/**
 * Gets whether the operator is a member, non-member, or unspecified.
 *
 * @param ast The AST of the operator.
 * @return Returns one of \ref C_OP_MEMBER, \ref C_OP_NON_MEMBER, or \ref
 * C_OP_UNSPECIFIED.
 */
NODISCARD
unsigned c_ast_oper_overload( c_ast_t const *ast );

/**
 * "Patches" \a type_ast into \a decl_ast only if:
 *  + \a type_ast has no parent.
 *  + The depth of \a type_ast is less than that of \a decl_ast.
 *  + \a decl_ast still contains an AST node of kind #K_PLACEHOLDER.
 *
 * @param type_ast The AST of the initial type.
 * @param decl_ast The AST of a declaration; may be NULL.
 * @return Returns the final AST.
 */
NODISCARD
c_ast_t* c_ast_patch_placeholder( c_ast_t *type_ast, c_ast_t *decl_ast );

/**
 * Creates a # K_POINTER AST to \a ast.  The name of \a ast (or one of its
 * child nodes), if any, is moved to the new pointer AST.
 *
 * @param ast The AST to create a pointer to.
 * @param ast_list If not NULL, the new pointer AST is appended to the list.
 * @return Returns the new pointer AST.
 *
 * @sa c_ast_unpointer()
 */
NODISCARD
c_ast_t* c_ast_pointer( c_ast_t *ast, c_ast_list_t *ast_list );

/**
 * Gets the root AST node starting at \a ast.
 *
 * @param ast The AST node to start from.
 * @return Returns said AST node.
 */
NODISCARD
c_ast_t* c_ast_root( c_ast_t *ast );

/**
 * Takes the name, if any, away from \a ast
 * (with the intent of giving it to another AST).
 *
 * @param ast The AST (or one of its child nodes) to take from.
 * @return Returns said name or en empty name.
 */
NODISCARD
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
NODISCARD
c_type_t c_ast_take_type_any( c_ast_t *ast, c_type_t const *type );

/**
 * Un-pointers \a ast, i.e., if \a ast is a #K_POINTER, returns the pointed-to
 * AST.
 *
 * @param ast The AST to un-pointer.
 * @return If \a ast is a pointer, returns the un-`typedef`d pointed-to AST;
 * otherwise returns NULL.
 *
 * @note Even though pointers are "dereferenced," this function isn't called
 * `c_ast_dereference` to avoid confusion with C++ references.
 *
 * @sa c_ast_pointer()
 * @sa c_ast_unreference()
 * @sa c_ast_unrvalue_reference()
 * @sa c_ast_untypedef()
 * @sa c_ast_untypedef_qual()
 */
NODISCARD
c_ast_t const* c_ast_unpointer( c_ast_t const *ast );

/**
 * Un-references \a ast, i.e., if \a ast is a #K_REFERENCE returns the
 * referred-to AST.
 *
 * @param ast The AST to un-reference.
 * @return If \a ast is a reference, returns the un-`typdef`d referenced AST;
 * otherwise returns \a ast.
 *
 * @note Only #K_REFERENCE is un-referenced, not #K_RVALUE_REFERENCE.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unrvalue_reference()
 * @sa c_ast_untypedef()
 * @sa c_ast_untypedef_qual()
 */
NODISCARD
c_ast_t const* c_ast_unreference( c_ast_t const *ast );

/**
 * Un-rvalue-references \a ast, i.e., if \a ast is a #K_RVALUE_REFERENCE
 * returns the referred-to AST.
 *
 * @param ast The AST to un-reference.
 * @return If \a ast is an rvalue reference, returns the un-`typedef`d
 * referenced AST; otherwise returns \a ast.
 *
 * @note Only #K_RVALUE_REFERENCE is un-referenced, not #K_REFERENCE.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference()
 * @sa c_ast_untypedef()
 * @sa c_ast_untypedef_qual()
 */
NODISCARD
c_ast_t const* c_ast_unrvalue_reference( c_ast_t const *ast );

/**
 * Un-`typedef`s \a ast, i.e., if \a ast is a #K_TYPEDEF, returns the AST the
 * `typedef` is for.
 *
 * @param ast The AST to un-`typedef`.
 * @return Returns the AST the `typedef` is for or \a ast if \a ast is not a
 * `typedef`.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference()
 * @sa c_ast_unrvalue_reference()
 * @sa c_ast_untypedef_qual()
 */
NODISCARD
c_ast_t const* c_ast_untypedef( c_ast_t const *ast );

/**
 * Un-`typedef`s \a ast, i.e., if \a ast is a #K_TYPEDEF, returns the AST the
 * `typedef` is for.
 *
 * @param ast The AST to un-`typedef`.
 * @param qual_stids Receives the qualifier(s) of \a ast bitwise-or'd with the
 * qualifier(s) \a ast is a `typedef` for (if \a ast is a #K_TYPEDEF).
 * @return Returns the AST the `typedef` is for or \a ast if \a ast is not a
 * `typedef`.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference()
 * @sa c_ast_unrvalue_reference()
 * @sa c_ast_untypedef()
 */
NODISCARD
c_ast_t const* c_ast_untypedef_qual( c_ast_t const *ast, c_tid_t *qual_stids );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Checks if the type of \a ast is equivalent to `size_t`.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is `size_t`.
 *
 * @note In cdecl, `size_t` is `typedef`d to be `unsigned long` in c_typedef.c.
 *
 * @sa c_tid_is_size_t()
 */
NODISCARD C_AST_UTIL_H_INLINE
bool c_ast_is_size_t( c_ast_t const *ast ) {
  return c_tid_is_size_t( c_ast_untypedef( ast )->type.btids );
}

/**
 * Checks whether one of the type IDs of the type of \a ast is any one of \a
 * tids.
 *
 * @param ast The AST to check.
 * @param tids The bitwise-or of type(s) to check against.
 * @return If the type of \a ast has one of \a tids, returns the un-`typedef`d
 * AST of \a ast; otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_tid_any()
 * @sa c_ast_is_ref_to_tid_any()
 * @sa c_ast_is_tid_any_qual()
 */
NODISCARD C_AST_UTIL_H_INLINE
c_ast_t const* c_ast_is_tid_any( c_ast_t const *ast, c_tid_t tids ) {
  c_tid_t qual_stids;
  return c_ast_is_tid_any_qual( ast, tids, &qual_stids );
}

/**
 * Checks whether the parent AST of \a ast (if any) is \a kind.
 *
 * @param ast The AST to check the parent of.
 * @param kind The kind to check for.
 * @return Returns `true` only if the parent of \a ast is \a kind.
 */
NODISCARD C_AST_UTIL_H_INLINE
bool c_ast_parent_is_kind( c_ast_t const *ast, c_ast_kind_t kind ) {
  return ast->parent_ast != NULL && ast->parent_ast->kind == kind;
}

/**
 * Checks whether \a ast should be printed as a `using` declaration.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast should be printed as a `using`
 * declaration.
 */
NODISCARD C_AST_UTIL_H_INLINE
bool c_ast_print_as_using( c_ast_t const *ast ) {
  return  opt_using && OPT_LANG_IS( USING_DECLARATION ) &&
          c_tid_is_any( ast->type.stids, TS_TYPEDEF );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_ast_util_H */
/* vim:set et sw=2 ts=2: */
