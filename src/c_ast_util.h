/*
**      cdecl -- C gibberish translator
**      src/c_ast_util.h
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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
#include "c_operator.h"
#include "c_type.h"
#include "options.h"                    /* for opt_using */
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
 * Adds a #K_ARRAY AST to the \a ast being built.
 *
 * @param ast The AST to add to.
 * @param array_ast The #K_ARRAY AST to append.  Its \ref c_array_ast::of_ast
 * "of_ast" _must_ be of kind #K_PLACEHOLDER.
 * @param of_ast The AST to become the \ref c_array_ast::of_ast "of_ast" of \a
 * array_ast.
 * @return Returns the AST to be used as the grammar production's return value.
 */
NODISCARD
c_ast_t* c_ast_add_array( c_ast_t *ast, c_ast_t *array_ast, c_ast_t *of_ast );

/**
 * Adds a #K_ANY_FUNCTION_LIKE AST to the \a ast being built.
 *
 * @param ast The AST to add to.
 * @param func_ast The #K_ANY_FUNCTION_LIKE AST to append.  Its \ref
 * c_function_ast::ret_ast "ret_ast" _must_ be NULL.
 * @param ret_ast The AST to become the \ref c_function_ast::ret_ast "ret_ast"
 * of \a func_ast.  If the \ref c_ast::kind "kind" of \a func_ast is:
 *  + #K_ANY_FUNCTION_RETURN, then \a ret_ast must _not_ be NULL; or:
 *  + Not #K_ANY_FUNCTION_RETURN, then \a ret_ast _must_ be NULL.
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
 *
 * @sa c_ast_find_type_any()
 */
NODISCARD
c_ast_t const* c_ast_find_kind_any( c_ast_t const *ast, c_ast_visit_dir_t dir,
                                    c_ast_kind_t kinds );

/// @cond DOXYGEN_IGNORE
NODISCARD C_AST_UTIL_H_INLINE
c_ast_t* nonconst_c_ast_find_kind_any( c_ast_t *ast, c_ast_visit_dir_t dir,
                                       c_ast_kind_t kinds ) {
  return CONST_CAST( c_ast_t*, c_ast_find_kind_any( ast, dir, kinds ) );
}

#define c_ast_find_kind_any(AST,DIR,KINDS) \
  NONCONST_OVERLOAD( c_ast_find_kind_any, (AST), (DIR), (KINDS) )
/// @endcond

/**
 * Traverses \a ast attempting to find an AST node having a name.
 *
 * @param ast The AST to start from; may be NULL.
 * @param dir The direction to search.
 * @return Returns said name or NULL if none.
 *
 * @sa c_ast_find_param_named()
 */
NODISCARD
c_sname_t const* c_ast_find_name( c_ast_t const *ast, c_ast_visit_dir_t dir );

/// @cond DOXYGEN_IGNORE
NODISCARD C_AST_UTIL_H_INLINE
c_sname_t* nonconst_c_ast_find_name( c_ast_t *ast, c_ast_visit_dir_t dir ) {
  return CONST_CAST( c_sname_t*, c_ast_find_name( ast, dir ) );
}

#define c_ast_find_name(AST,DIR) \
  NONCONST_OVERLOAD( c_ast_find_name, (AST), (DIR) )
/// @endcond

/**
 * Find the parameter of \a func_ast having \a name, if any.
 *
 * @param func_ast The #K_ANY_FUNCTION_LIKE AST to check.
 * @param name The name to find.
 * @param stop_ast The AST to stop at, if any.
 * @return Returns the AST of the parameter of \a func_ast having \a name or
 * NULL if none.
 *
 * @sa c_ast_find_name()
 */
NODISCARD
c_ast_t const* c_ast_find_param_named( c_ast_t const *func_ast,
                                       char const *name,
                                       c_ast_t const *stop_ast );

/**
 * Traverses \a ast attempting to find an AST node having one of \a type.
 *
 * @param ast The AST to start from; may be NULL.
 * @param dir The direction to visit.
 * @param type A type where each type ID is the bitwise-or of type IDs to find.
 * @return Returns a pointer to an AST node having one of \a type or NULL if
 * none.
 *
 * @sa c_ast_find_kind_any()
 */
NODISCARD
c_ast_t const* c_ast_find_type_any( c_ast_t const *ast, c_ast_visit_dir_t dir,
                                    c_type_t const *type );

/// @cond DOXYGEN_IGNORE
NODISCARD C_AST_UTIL_H_INLINE
c_ast_t* nonconst_c_ast_find_type_any( c_ast_t *ast, c_ast_visit_dir_t dir,
                                       c_type_t const *type ) {
  return CONST_CAST( c_ast_t*, c_ast_find_type_any( ast, dir, type ) );
}

#define c_ast_find_type_any(AST,DIR,TYPE) \
  NONCONST_OVERLOAD( c_ast_find_type_any, (AST), (DIR), (TYPE) )
/// @endcond

/**
 * Checks whether \a ast is a #K_BUILTIN having one of \a btids built-in
 * type(s); or a `typedef` thereof.
 *
 * @param ast The AST to check.
 * @param btids The built-in type(s) \a ast can be.  They must be only base
 * types (no storage classes, qualfiers, or attributes).
 * @return Returns `true` only if the type of \a ast is one of \a btids.
 *
 * @sa c_ast_is_integral()
 */
NODISCARD
bool c_ast_is_builtin_any( c_ast_t const *ast, c_tid_t btids );

/**
 * Checks whether \a ast is an integral type, that is either a #K_BUILTIN
 * integral or a #K_ENUM type; or a `typedef` thereof.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if the type of \a ast is an integral type.
 *
 * @sa c_ast_is_builtin_any()
 */
NODISCARD
bool c_ast_is_integral( c_ast_t const *ast );

/**
 * Checks whether \a type_ast is likely for a constructor (as opposed to an
 * ordinary function).
 *
 * @remarks
 * @parblock
 * **Cdecl** can't know for certain whether a "function" name is really a
 * constructor name because it:
 *
 *  1. Doesn't have the context of the surrounding class:
 *
 *          class T {
 *          public:
 *            T();                // <-- All cdecl usually has is this.
 *            // ...
 *          };
 *
 *  2. Can't check to see if the name is a `typedef` for a `class`, `struct`,
 *     or `union` (even though that would greatly help) since:
 *
 *          T(U);
 *
 *     could be either:
 *
 *      + A constructor of type `T` with a parameter of type `U`; or:
 *      + A variable named `U` of type `T` (with unnecessary parentheses).
 *
 * Hence, we have to infer which of a function or a constructor was likely the
 * one meant.
 * @endparblock
 *
 * @param type_ast The base type AST to check.
 * @return Returns `true` only if \a type_ast is likely for a constructor.
 */
NODISCARD
bool c_ast_is_likely_ctor( c_ast_t const *type_ast );

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
 * Checks whether \a ast is an AST for a #K_POINTER to another AST having \a
 * type; or a `typedef` thereof.  For example:
 *
 *  + `c_ast_is_ptr_to_type_any( ast, &T_ANY, &C_TYPE_LIT_B(TB_char) )`
 *    @par
 *    Returns `true` only if \a ast is a pointer to `char` (`char*`) _exactly_.
 *
 *  + `c_ast_is_ptr_to_type_any( ast, &T_ANY, &T_const_char )`
 *    @par
 *    Returns `true` only if \a ast is a pointer to `const char` (`char
 *    const*`) _exactly_.
 *
 *  + <code>c_ast_is_ptr_to_type_any( ast, &%C_TYPE_LIT( TB_ANY, c_tid_compl(
 *    TS_const ), TA_ANY ), &%C_TYPE_LIT_B(TB_char) )</code>
 *    @par
 *    Returns `true` only if \a ast is a pointer to `char` regardless of
 *    `const` (`char*` or `char const*`).
 *
 * <p>
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
 * Checks whether \a ast is an AST for a #K_REFERENCE to another AST that is a
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
 * Checks whether \a ast is an AST for a #K_REFERENCE to another AST that is
 * one of \a kinds; or a `typedef` thereof.
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
 * Checks whether \a ast is an AST for a #K_REFERENCE to another AST having a
 * type that contains any one of \a tids; or a `typedef` thereof.
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
 * Checks whether \a ast is an AST for a #K_REFERENCE to another AST having \a
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
 * @param rv_qual_stids Receives the qualifier(s) of \a ast bitwise-or'd with
 * the qualifier(s) \a ast is a `typedef` for (if \a ast is a #K_TYPEDEF), but
 * only if this function returns non-NULL.
 * @return If the type of \a ast has one of \a tids, returns the un-`typedef`d
 * AST of \a ast; otherwise returns NULL.
 *
 * @sa c_ast_is_ptr_to_type_any()
 * @sa c_ast_is_ref_to_type_any()
 * @sa c_ast_is_tid_any()
 */
NODISCARD
c_ast_t const* c_ast_is_tid_any_qual( c_ast_t const *ast, c_tid_t tids,
                                      c_tid_t *rv_qual_stids );

/**
 * Checks whether \a ast is untyped, that is of kind #K_NAME and have \ref
 * c_name_ast::sname either be empty or equal \ref c_ast::sname.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is untyped.
 */
NODISCARD
bool c_ast_is_untyped( c_ast_t const *ast );

/**
 * Gets the leat AST node starting at \a ast.
 *
 * @param ast The AST to start from; may be NULL.
 * @return Returns said AST or NULL if \a ast is NULL.
 *
 * @sa c_ast_root()
 */
c_ast_t const* c_ast_leaf( c_ast_t const *ast );

/**
 * Takes the name, if any, away from \a ast
 * (with the intent of giving it to another AST).
 *
 * @param ast The AST (or one of its child nodes) to take from.
 * @return Returns said name or an empty name.
 *
 * @sa c_sname_move()
 */
NODISCARD
c_sname_t c_ast_move_sname( c_ast_t *ast );

/**
 * Gets whether the operator \a ast is overloaded as a member, non-member, or
 * unspecified.
 *
 * @param ast The AST of the operator.
 * @return Returns one of #C_FUNC_MEMBER, #C_FUNC_NON_MEMBER, or
 * #C_FUNC_UNSPECIFIED.
 */
NODISCARD
c_func_member_t c_ast_op_overload( c_ast_t const *ast );

/**
 * Gets the the minimum and maximum number of parameters the operator \a ast
 * can have based on whether it's a member, non-member, or unspecified.
 *
 * @param ast The AST of the operator.
 * @param params_min Receives the minimum number of parameters for \a ast.
 * @param params_max Receives the maximum number of parameters for \a ast.
 */
void c_ast_op_params_min_max( c_ast_t const *ast, unsigned *params_min,
                              unsigned *params_max );
/**
 * "Patches" \a type_ast into \a decl_ast only if:
 *  + \a type_ast has no \ref c_ast::parent_ast "parent_ast".
 *  + The \ref c_ast::depth "depth" of \a type_ast is less than that of \a
 *    decl_ast.
 *  + \a decl_ast still contains an AST node of kind #K_PLACEHOLDER.
 *
 * @param type_ast The AST of the initial type.
 * @param decl_ast The AST of a declaration; may be NULL.
 * @return Returns the final AST.
 */
NODISCARD
c_ast_t* c_ast_patch_placeholder( c_ast_t *type_ast, c_ast_t *decl_ast );

/**
 * Creates a #K_POINTER AST to \a ast.  The name of \a ast (or one of its child
 * nodes), if any, is moved to the new pointer AST.
 *
 * @param ast The AST to create a pointer to.
 * @param ast_list The list to append the new pointer AST node onto.
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
 *
 * @sa c_ast_leaf()
 */
NODISCARD
c_ast_t const* c_ast_root( c_ast_t const *ast );

/// @cond DOXYGEN_IGNORE
NODISCARD C_AST_UTIL_H_INLINE
c_ast_t* nonconst_c_ast_root( c_ast_t *ast ) {
  return CONST_CAST( c_ast_t*, c_ast_root( ast ) );
}

#define c_ast_root(AST)           NONCONST_OVERLOAD( c_ast_root, (AST) )
/// @endcond

/**
 * Checks \a ast to see if it contains one or more of \a type: if so, removes
 * them.
 *
 * @remarks This is used in cases like:
 *
 *      explain typedef int *p
 *
 * that should be explained as:
 *
 *      declare p as type pointer to integer
 *
 * and _not_:
 *
 *      declare p as pointer to typedef integer
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
 * @sa c_ast_unreference_any()
 * @sa c_ast_untypedef()
 * @sa c_ast_untypedef_qual()
 */
NODISCARD
c_ast_t const* c_ast_unpointer( c_ast_t const *ast );

/// @cond DOXYGEN_IGNORE
NODISCARD C_AST_UTIL_H_INLINE
c_ast_t* nonconst_c_ast_unpointer( c_ast_t *ast ) {
  return CONST_CAST( c_ast_t*, c_ast_unpointer( ast ) );
}

#define c_ast_unpointer(AST)      NONCONST_OVERLOAD( c_ast_unpointer, (AST) )
/// @endcond

/**
 * Un-references \a ast, i.e., if \a ast is a #K_REFERENCE, returns the
 * referred-to AST.
 *
 * @param ast The AST to un-reference.
 * @return If \a ast is a reference, returns the un-`typdef`d referenced AST;
 * otherwise returns \a ast.
 *
 * @note Only #K_REFERENCE is un-referenced, not #K_RVALUE_REFERENCE.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference_any()
 * @sa c_ast_untypedef()
 * @sa c_ast_untypedef_qual()
 */
NODISCARD
c_ast_t const* c_ast_unreference( c_ast_t const *ast );

/**
 * Un-references \a ast, i.e., if \a ast is either a #K_REFERENCE or
 * #K_RVALUE_REFERENCE, returns the referred-to AST.
 *
 * @param ast The AST to un-reference.
 * @return If \a ast is an rvalue reference, returns the un-`typedef`d
 * referenced AST; otherwise returns \a ast.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference()
 * @sa c_ast_untypedef()
 * @sa c_ast_untypedef_qual()
 */
NODISCARD
c_ast_t const* c_ast_unreference_any( c_ast_t const *ast );

/// @cond DOXYGEN_IGNORE
NODISCARD C_AST_UTIL_H_INLINE
c_ast_t* nonconst_c_ast_unreference_any( c_ast_t *ast ) {
  return CONST_CAST( c_ast_t*, c_ast_unreference_any( ast ) );
}

#define c_ast_unreference_any(AST) \
  NONCONST_OVERLOAD( c_ast_unreference_any, (AST) )
/// @endcond

/**
 * Un-`typedef`s \a ast, i.e., if \a ast is of \ref c_ast::kind "kind"
 * #K_TYPEDEF, returns the AST's \ref c_typedef_ast::for_ast "for_ast".
 *
 * @param ast The AST to un-`typedef`.
 * @return Returns the AST the `typedef` is for or \a ast if \a ast is not of
 * kind #K_TYPEDEF.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference()
 * @sa c_ast_unreference_any()
 * @sa c_ast_untypedef_qual()
 */
NODISCARD
c_ast_t const* c_ast_untypedef( c_ast_t const *ast );

/**
 * Un-`typedef`s \a ast, i.e., if \a ast is of \ref c_ast::kind "kind"
 * #K_TYPEDEF, returns the AST's \ref c_typedef_ast::for_ast "for_ast".
 *
 * @param ast The AST to un-`typedef`.
 * @param rv_qual_stids Receives the qualifier(s) of \a ast bitwise-or'd with
 * the qualifier(s) \a ast is a `typedef` for (if \a ast is a #K_TYPEDEF).
 * @return Returns the AST the `typedef` is for or \a ast if \a ast is not of
 * kind #K_TYPEDEF.
 *
 * @sa c_ast_sub_typedef()
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference()
 * @sa c_ast_unreference_any()
 * @sa c_ast_untypedef()
 */
NODISCARD
c_ast_t const* c_ast_untypedef_qual( c_ast_t const *ast,
                                     c_tid_t *rv_qual_stids );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets whether \a ast is a parameter of a function-like AST.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is a parameter.
 */
NODISCARD C_AST_UTIL_H_INLINE
bool c_ast_is_param( c_ast_t const *ast ) {
  return ast->param_of_ast != NULL;
}

/**
 * Gets whether \a ast has the `register` storage class.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast has the `register` storage class.
 */
NODISCARD C_AST_UTIL_H_INLINE
bool c_ast_is_register( c_ast_t const *ast ) {
  return c_tid_is_any( ast->type.stids, TS_register );
}

/**
 * Checks if the type of \a ast is equivalent to `size_t`.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast is `size_t`.
 *
 * @note In **cdecl**, `size_t` is `typedef`d to be `unsigned long` in
 * c_typedef.c.
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
  c_tid_t qual_stids = TS_NONE;
  return c_ast_is_tid_any_qual( ast, tids, &qual_stids );
}

/**
 * Gets whether the membership specification of the C++ operator \a ast matches
 * the overloadability of \a op.
 *
 * @param ast The C++ operator AST to check.
 * @param op The C++ operator to check against.
 * @return Returns `true` only if either:
 *  + The user didn't specify either #C_FUNC_MEMBER or #C_FUNC_NON_MEMBER; or:
 *  + The membership specification of \a ast matches the overloadability of \a
 *    op.
 */
NODISCARD C_AST_UTIL_H_INLINE
bool c_ast_op_mbr_matches( c_ast_t const *ast, c_operator_t const *op ) {
  return  ast->oper.member == C_FUNC_UNSPECIFIED ||
          (TO_UNSIGNED_EXPR( ast->oper.member ) &
           TO_UNSIGNED_EXPR( op->overload     ) ) != 0;
}

/**
 * Checks whether the \ref c_ast::kind "kind" of \ref c_ast::parent_ast
 * "parent_ast" of \a ast (if any) is \a kind.
 *
 * @param ast The AST to check the \ref c_ast::parent_ast "parent_ast" of.
 * @param kinds The bitwise-or of kind(s) to check for.
 * @return Returns `true` only if the \ref c_ast::parent_ast "parent_ast" of \a
 * ast is one of \a kinds.
 */
NODISCARD C_AST_UTIL_H_INLINE
bool c_ast_parent_is_kind_any( c_ast_t const *ast, c_ast_kind_t kinds ) {
  return ast->parent_ast != NULL && (ast->parent_ast->kind & kinds) != 0;
}

/**
 * Checks whether \a ast should be printed as a `using` declaration.
 *
 * @param ast The AST to check.
 * @return Returns `true` only if \a ast should be printed as a `using`
 * declaration.
 *
 * @sa #LANG_using_DECLS
 * @sa opt_using
 */
NODISCARD C_AST_UTIL_H_INLINE
bool c_ast_print_as_using( c_ast_t const *ast ) {
  return  opt_using && OPT_LANG_IS( using_DECLS ) &&
          c_tid_is_any( ast->type.stids, TS_typedef );
}

/**
 * Creates a temporary AST node that is a copy of the AST node that \a ast is a
 * `typedef` for, but keeping the original's alignment, bit-field width (only
 * if it's an integral type), source location, and qualifiers bitwise-or'd in,
 * effectively substituting the `typedef`'d
 *
 * For example, given:
 *
 *      cdecl> typedef enum E T
 *      cdecl> explain T x : 4
 *
 * we get what `T` is a `typedef` for and substitute it for `T`, but keeping
 * the original bit-field width yielding:
 *
 *      cdecl> explain enum E x : 4
 *
 * so that we can check _that_ for errors.  (In this case, it's an error in C
 * since enumerations can't be bit-fields in C.)
 *
 * @param ast The AST to get what it's a `typedef` for .  It _must_ be of kind
 * #K_TYPEDEF.
 * @return Returns a new AST that is the AST that \a ast is a `typedef` for,
 * but with the original alignment, bit-field width (only if an integral type),
 * source location, and qualifiers bitwise-or'd in.
 *
 * @sa c_ast_untypedef_qual()
 */
c_ast_t c_ast_sub_typedef( c_ast_t const *ast );

///////////////////////////////////////////////////////////////////////////////

/** @} */

_GL_INLINE_HEADER_END

#endif /* cdecl_c_ast_util_H */
/* vim:set et sw=2 ts=2: */
