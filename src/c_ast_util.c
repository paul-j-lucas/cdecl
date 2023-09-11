/*
**      cdecl -- C gibberish translator
**      src/c_ast_util.c
**
**      Copyright (C) 2017-2023  Paul J. Lucas
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

/**
 * @file
 * Defines functions for various cdecl-specific algorithms for construcing an
 * Abstract Syntax Tree (AST) for parsed C/C++ declarations.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_AST_UTIL_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_ast_util.h"
#include "c_operator.h"
#include "c_typedef.h"
#include "gibberish.h"
#include "literals.h"
#include "print.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

// local functions
NODISCARD
static c_ast_t* c_ast_append_array( c_ast_t*, c_ast_t*, c_ast_t* );

/**
 * @addtogroup ast-functions-group
 * @{
 */

////////// local functions ////////////////////////////////////////////////////

/**
 * @copydoc c_ast_add_array
 * @sa c_ast_add_array()
 */
NODISCARD
static c_ast_t* c_ast_add_array_impl( c_ast_t *ast, c_ast_t *array_ast,
                                      c_ast_t *of_ast ) {
  assert( array_ast != NULL );
  assert( array_ast->kind == K_ARRAY );
  assert( array_ast->array.of_ast != NULL );
  assert( array_ast->array.of_ast->kind == K_PLACEHOLDER );
  assert( of_ast != NULL );

  if ( ast == NULL )
    return array_ast;

  switch ( ast->kind ) {
    case K_ARRAY:
      return c_ast_append_array( ast, array_ast, of_ast );

    case K_PLACEHOLDER:
      //
      // Before:
      //
      //      array_ast
      //      placeholder --> placeholder-parent
      //      of_ast
      //
      // After:
      //
      //      of_ast --> array_ast --> placeholder-parent
      //
      c_ast_set_parent( array_ast, ast->parent_ast );
      c_ast_set_parent( of_ast, array_ast );
      return array_ast;

    case K_POINTER:
      if ( ast->depth > array_ast->depth ) {
        //
        // If there's an intervening pointer, e.g.:
        //
        //      int (*(*x)[3])[5]
        //
        // (where 'x' is a "pointer to array 3 of pointer to array 5 of int"),
        // we have to recurse "through" it if its depth > the array's depth;
        // else we'd end up with a "pointer to array 5 of array 3 of pointer to
        // int."
        //
        PJL_IGNORE_RV(
          c_ast_add_array_impl( ast->ptr_ref.to_ast, array_ast, of_ast )
        );
        return ast;
      }
      FALLTHROUGH;

    default:
      //
      // An AST node's "depth" says how nested within () it is and controls the
      // precedence of what is an array of what.
      //
      if ( ast->depth > array_ast->depth ) {
        //
        // Before:
        //
        //      ast-child --> ast
        //      array_ast
        //
        // After:
        //
        //      ast-child --> array_ast --> ast
        //
        if ( c_ast_is_parent( ast ) )
          c_ast_set_parent( ast->parent.of_ast, array_ast );
        c_ast_set_parent( array_ast, ast );
        return ast;
      }
      else {
        //
        // Before:
        //
        //      ast --> ast-parent
        //      array_ast
        //
        // After:
        //
        //      ast --> array_ast --> ast-parent
        //
        c_ast_set_parent( array_ast, ast->parent_ast );
        c_ast_set_parent( ast, array_ast );
        return array_ast;
      }
  } // switch
}

/**
 * Helper function for c_ast_add_array_impl().
 * If \a ast is:
 *  + Not of \ref c_ast::kind "kind" #K_ARRAY, makes \a array_ast an array of
 *    \a ast.
 *  + Of \ref c_ast::kind "kind" #K_ARRAY, appends \a array_ast to the end of
 *    the array AST chain.
 *
 * For example, given:
 *
 *  + \a ast = `array 3 of array 5 of int`
 *  + \a array_ast = `array 7 of NULL`
 *
 * this function returns:
 *
 *  + `array 3 of array 5 of array 7 of int`
 *
 * @param ast The AST to append to.
 * @param array_ast The #K_ARRAY AST to append.  Its \a ref c_array_ast::of_ast
 * "of_ast" _must_ be of kind #K_PLACEHOLDER.
 * @param of_ast The AST to become the \a ref c_array_ast::of_ast "of_ast" of
 * \a array_ast.
 * @return If \a ast is of kind #K_ARRAY, returns \a ast; otherwise returns \a
 * array_ast.
 *
 * @sa c_ast_add_array_impl()
 */
NODISCARD
static c_ast_t* c_ast_append_array( c_ast_t *ast, c_ast_t *array_ast,
                                    c_ast_t *of_ast ) {
  assert( ast != NULL );
  assert( array_ast != NULL );
  assert( array_ast->kind == K_ARRAY );
  assert( array_ast->array.of_ast != NULL );
  assert( array_ast->array.of_ast->kind == K_PLACEHOLDER );
  assert( of_ast != NULL );

  switch ( ast->kind ) {
    case K_POINTER:
      //
      // If there's an intervening pointer, e.g.:
      //
      //      int (*(*x)[3])[5]
      //
      // (where 'x' is a "pointer to array 3 of pointer to array 5 of int"), we
      // have to recurse "through" it if its depth < the array's depth; else
      // we'd end up with a "pointer to array 3 of array 5 of pointer to int."
      //
      if ( array_ast->depth >= ast->depth )
        break;
      FALLTHROUGH;

    case K_ARRAY: {
      //
      // On the next-to-last recursive call, this sets this array to be an
      // array of the new array; for all prior recursive calls, it's a no-op.
      //
      c_ast_t *const child_ast =
        c_ast_append_array( ast->array.of_ast, array_ast, of_ast );
      c_ast_set_parent( child_ast, ast );
      return ast;
    }

    default:
      /* suppress warning */;
  } // switch

  //
  // We've reached the end of the array chain: make the new array be an array
  // of this AST node and return the array so the parent will now point to it
  // instead.
  //
  c_ast_set_parent( ast, array_ast );
  return array_ast;
}

/**
 * @copydoc c_ast_add_func()
 * @sa c_ast_add_func()
 */
NODISCARD
static c_ast_t* c_ast_add_func_impl( c_ast_t *ast, c_ast_t *func_ast,
                                     c_ast_t *ret_ast ) {
  assert( ast != NULL );
  assert( func_ast != NULL );
  assert( is_1_bit_only_in_set( func_ast->kind, K_ANY_FUNCTION_LIKE ) );
  assert( (func_ast->kind & K_ANY_FUNCTION_RETURN) == 0 || ret_ast != NULL );
  assert( (func_ast->kind & K_ANY_FUNCTION_RETURN) != 0 || ret_ast == NULL );
  assert( func_ast->func.ret_ast == NULL );

  if ( (ast->kind & (K_ARRAY | K_ANY_POINTER | K_ANY_REFERENCE)) != 0 ) {
    switch ( ast->parent.of_ast->kind ) {
      case K_ARRAY:
      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        if ( ast->depth > func_ast->depth ) {
          PJL_IGNORE_RV(
            c_ast_add_func_impl( ast->ptr_ref.to_ast, func_ast, ret_ast )
          );
          return ast;
        }
        FALLTHROUGH;

      default:
        if ( ast->kind == K_ARRAY ) {
          //
          // Before:
          //
          //      ast(K_ARRAY) --> of_ast
          //      func_ast
          //
          // After:
          //
          //      ast(K_ARRAY) --> func_ast --> of_ast
          //
          // Note that an array of function is illegal, but we still construct
          // the AST properly and let c_ast_check_array() catch the error.
          //
          c_ast_set_parent( ast->array.of_ast, func_ast );
          c_ast_set_parent( func_ast, ast );
          return ast;
        }
        break;

      case K_PLACEHOLDER:
        if ( ret_ast == ast )
          break;
        c_ast_set_parent( func_ast, ast );
        FALLTHROUGH;

      case K_APPLE_BLOCK:
        c_ast_set_parent( ret_ast, func_ast );
        return ast;
    } // switch
  }

  if ( c_ast_is_parent( func_ast ) )
    c_ast_set_parent( ret_ast, func_ast );
  return func_ast;
}

/**
 * Helper function that checks whether the type of \a ast is one of \a tids.
 *
 * @param ast The AST to check; may be NULL.
 * @param tids The bitwise-or of type(s) to check against.
 * @param qual_stids The qualifier(s) of the `typedef` for \a ast, if any.
 * @return If \a ast is not NULL and the type of \a ast is one of \a tids,
 * returns \a ast; otherwise returns NULL.
 */
NODISCARD
static c_ast_t const* c_ast_is_tid_any_qual_impl( c_ast_t const *ast,
                                                  c_tid_t tids,
                                                  c_tid_t qual_stids ) {
  if ( ast != NULL ) {
    assert( c_tid_tpid( qual_stids ) == C_TPID_STORE );
    c_tid_t ast_tids = c_type_get_tid( &ast->type, tids );
    ast_tids = c_tid_normalize( ast_tids );
    if ( c_tid_tpid( tids ) == C_TPID_STORE )
      ast_tids |= qual_stids;
    if ( c_tid_is_any( ast_tids, tids ) )
      return ast;
  }
  return NULL;
}

/**
 * Takes the storage (and attributes), if any, away from \a ast
 * (with the intent of giving them to another AST).
 *
 * @remarks This is used is cases like:
 *
 *      explain static int f()
 *
 * that should be explained as:
 *
 *      declare f as static function () returning integer
 *
 * and _not_:
 *
 *      declare f as function () returning static integer
 *
 * i.e., the `static` has to be taken away from `int` and given to the function
 * because it's the function that's `static`, not the `int`.
 *
 * @param ast The AST to take from.
 * @return Returns said storage (and attributes) or #T_NONE.
 */
NODISCARD
static c_type_t c_ast_take_storage( c_ast_t *ast ) {
  assert( ast != NULL );
  c_type_t rv_type = T_NONE;
  c_ast_t *const found_ast =
    c_ast_find_kind_any( ast, C_VISIT_DOWN, K_BUILTIN | K_TYPEDEF );
  if ( found_ast != NULL ) {
    rv_type.stids = found_ast->type.stids & TS_ANY_STORAGE;
    rv_type.atids = found_ast->type.atids;
    found_ast->type.stids &= c_tid_compl( TS_ANY_STORAGE );
    found_ast->type.atids = TA_NONE;
  }
  return rv_type;
}

/**
 * Only if \a ast is a #K_POINTER, un-pointers \a ast.
 *
 * @param ast The AST to un-pointer.
 * @param rv_qual_stids If \a ast is a pointer, receives the qualifier(s) of
 * the first pointed-to type.  For a declaration like
 * <code>const&nbsp;S&nbsp;*x</code> (where `S` is a `struct`), the `const` is
 * associated with the `typedef` for the `struct` and _not_ the actual `struct`
 * the `typedef` is a `typedef` for:
 * ```
 * decl_c: {
 *   sname: { string: "x", scopes: "none" },
 *   kind: { value: 0x400, string: "pointer" },
 *   ...
 *   ptr_ref: {
 *     to_ast: {
 *       sname: { string: "" },
 *       kind: { value: 0x20, string: "typedef" },
 *       ...
 *       type: { btid: 0x20000001, stid: 0x200000002, ..., string: "const" },
 *       tdef: {
 *         for_ast: {
 *           sname: { string: "S", scopes: "struct" },
 *           kind: { value: 0x8, string: "class, struct, or union" },
 *           ...
 *           type: { btid: 0x1000001, ..., string: "struct" },
 *           csu: {
 *             csu_sname: { string: "S", scopes: "none" }
 *           }
 *         }
 *       }
 *     }
 *   }
 * }
 * ```
 * Therefore, we need to copy the cv qualifiers of the `typedef` before we
 * un-`typedef` it.
 * @return Returns the pointed-to AST or NULL if \a ast is not a pointer.
 *
 * @sa c_ast_unpointer()
 * @sa c_ast_unreference_qual()
 */
NODISCARD
static c_ast_t const* c_ast_unpointer_qual( c_ast_t const *ast,
                                            c_tid_t *rv_qual_stids ) {
  ast = c_ast_untypedef( ast );
  if ( ast->kind != K_POINTER )
    return NULL;

  ast = ast->ptr_ref.to_ast;
  assert( ast != NULL );
  assert( rv_qual_stids != NULL );
  assert( c_tid_tpid( *rv_qual_stids ) == C_TPID_STORE );
  *rv_qual_stids = ast->type.stids & TS_ANY_QUALIFIER;
  //
  // Now that we've gotten the cv qualifiers of the first pointed-to type, we
  // can just call the ordinary c_ast_untypedef() to peel off any remaining
  // typedef layers.
  //
  return c_ast_untypedef( ast );
}

/**
 * Only if \a ast is a #K_REFERENCE, un-references \a ast.
 *
 * @param ast The AST to un-reference.
 * @param rv_qual_stids If \a ast is a reference, receives the bitwise-of of
 * the qualifier(s) of the first referred-to type.  For a declaration like
 * <code>const&nbsp;S&nbsp;&x</code> (where `S` is a `struct`), the `const` is
 * associated with the `typedef` for the `struct` and _not_ the actual `struct`
 * the `typedef` is a `typedef` for:
 * ```
 * decl_c: {
 *   sname: { string: "x", scopes: "none" },
 *   kind: { value: 0x1000, string: "reference" },
 *   ...
 *   ptr_ref: {
 *     to_ast: {
 *       sname: { string: "" },
 *       kind: { value: 0x20, string: "typedef" },
 *       ...
 *       type: { btid: 0x20000001, stid: 0x200000002, ..., string: "const" },
 *       tdef: {
 *         for_ast: {
 *           sname: { string: "S", scopes: "struct" },
 *           kind: { value: 0x8, string: "class, struct, or union" },
 *           ...
 *           type: { btid: 0x1000001, ..., string: "struct" },
 *           csu: {
 *             csu_sname: { string: "S", scopes: "none" }
 *           }
 *         }
 *       }
 *     }
 *   }
 * }
 * ```
 * Therefore, we need to copy the cv qualifiers of the `typedef` before we
 * un-`typedef` it.
 * @return Returns the referenced AST or NULL if \a ast is not a reference.
 *
 * @sa c_ast_unpointer_qual()
 * @sa c_ast_unreference()
 */
NODISCARD
static c_ast_t const* c_ast_unreference_qual( c_ast_t const *ast,
                                              c_tid_t *rv_qual_stids ) {
  ast = c_ast_untypedef( ast );
  if ( ast->kind != K_REFERENCE )
    return NULL;

  ast = ast->ptr_ref.to_ast;
  assert( ast != NULL );
  assert( rv_qual_stids != NULL );
  assert( c_tid_tpid( *rv_qual_stids ) == C_TPID_STORE );
  *rv_qual_stids = ast->type.stids & TS_ANY_QUALIFIER;
  //
  // Now that we've gotten the cv qualifiers of the first referred-to type, we
  // can just call the ordinary c_ast_unreference() to peel off any remaining
  // typedef/reference layers.
  //
  return c_ast_unreference( ast );
}

/**
 * A visitor function to find an AST node having a particular kind(s).
 *
 * @param ast The AST to check.
 * @param data The bitwise-or of the kind(s) \a ast can be.
 * @return Returns `true` only if the kind of \a ast is one of the kinds.
 */
NODISCARD
static bool c_ast_vistor_kind_any( c_ast_t *ast, user_data_t data ) {
  assert( ast != NULL );
  c_ast_kind_t const kinds = STATIC_CAST( c_ast_kind_t, data.ui32 );
  return (ast->kind & kinds) != 0;
}

/**
 * A visitor function to find an AST node having a non-empty name.
 *
 * @param ast The AST to check.
 * @param data Not used.
 * @return Returns `true` only if \a ast has such a scoped name.
 */
NODISCARD
static bool c_ast_visitor_name( c_ast_t *ast, user_data_t data ) {
  assert( ast != NULL );
  (void)data;
  return !c_sname_empty( &ast->sname );
}

/**
 * A visitor function to find an AST node having a particular type(s).
 *
 * @param ast The AST to check.
 * @param data A pointer to a type where each type part is the bitwise-or of
 * type IDs to find.
 * @return Returns `true` only if the type of \a ast is one of the types.
 */
NODISCARD
static bool c_ast_vistor_type_any( c_ast_t *ast, user_data_t data ) {
  assert( ast != NULL );
  c_type_t const *const type = data.pc;
  return c_type_is_any( &ast->type, type );
}

////////// extern functions ///////////////////////////////////////////////////

c_ast_t* c_ast_add_array( c_ast_t *ast, c_ast_t *array_ast, c_ast_t *of_ast ) {
  assert( ast != NULL );
  c_ast_t *const rv_ast = c_ast_add_array_impl( ast, array_ast, of_ast );
  assert( rv_ast != NULL );
  if ( c_sname_empty( &rv_ast->sname ) )
    rv_ast->sname = c_ast_move_sname( ast );
  c_type_t const taken_type = c_ast_take_storage( array_ast->array.of_ast );
  c_type_or_eq( &array_ast->type, &taken_type );
  return rv_ast;
}

c_ast_t* c_ast_add_func( c_ast_t *ast, c_ast_t *func_ast, c_ast_t *ret_ast ) {
  c_ast_t *const rv_ast = c_ast_add_func_impl( ast, func_ast, ret_ast );
  assert( rv_ast != NULL );
  if ( c_sname_empty( &rv_ast->sname ) )
    rv_ast->sname = c_ast_move_sname( ast );
  if ( func_ast->func.ret_ast != NULL ) {
    c_type_t const taken_type = c_ast_take_storage( func_ast->func.ret_ast );
    c_type_or_eq( &rv_ast->type, &taken_type );
  }
  return rv_ast;
}

c_ast_t* c_ast_find_kind_any( c_ast_t *ast, c_ast_visit_dir_t dir,
                              c_ast_kind_t kinds ) {
  assert( kinds != 0 );
  user_data_t const data = { .ui32 = kinds };
  return c_ast_visit( ast, dir, c_ast_vistor_kind_any, data );
}

c_sname_t* c_ast_find_name( c_ast_t const *ast, c_ast_visit_dir_t dir ) {
  user_data_t const data = { .ull = 0 };
  c_ast_t *const found_ast =
    c_ast_visit( CONST_CAST( c_ast_t*, ast ), dir, c_ast_visitor_name, data );
  return found_ast != NULL ? &found_ast->sname : NULL;
}

c_ast_t const* c_ast_find_param_named( c_ast_t const *func_ast,
                                       char const *name,
                                       c_ast_t const *stop_ast ) {
  assert( func_ast != NULL );
  assert( is_1_bit_only_in_set( func_ast->kind, K_ANY_FUNCTION_LIKE ) );
  assert( name != NULL );

  SNAME_VAR_INIT( sname, name );

  FOREACH_AST_FUNC_PARAM( param, func_ast ) {
    c_ast_t const *const param_ast = c_param_ast( param );
    if ( param_ast == stop_ast )
      break;
    c_sname_t const *const param_sname =
      c_ast_find_name( param_ast, C_VISIT_DOWN );
    if ( param_sname != NULL && c_sname_cmp( param_sname, &sname ) == 0 )
      return param_ast;
  } // for

  return NULL;
}

c_ast_t* c_ast_find_type_any( c_ast_t *ast, c_ast_visit_dir_t dir,
                              c_type_t const *type ) {
  user_data_t const data = { .pc = type };
  return c_ast_visit( ast, dir, c_ast_vistor_type_any, data );
}

bool c_ast_is_builtin_any( c_ast_t const *ast, c_tid_t btids ) {
  c_tid_check( btids, C_TPID_BASE );

  ast = c_ast_untypedef( ast );
  if ( ast->kind != K_BUILTIN )
    return false;
  c_tid_t const ast_btids = c_tid_normalize( ast->type.btids );
  return is_1n_bit_only_in_set( c_tid_no_tpid( ast_btids ), btids );
}

bool c_ast_is_integral( c_ast_t const *ast ) {
  ast = c_ast_untypedef( ast );
  switch ( ast->kind ) {
    case K_BUILTIN:
      return c_tid_is_any( ast->type.btids, TB_ANY_INTEGRAL );
    case K_ENUM:
      return true;
    default:
      return false;
  } // switch
}

bool c_ast_is_ptr_to_kind_any( c_ast_t const *ast, c_ast_kind_t kinds ) {
  ast = c_ast_unpointer( ast );
  return ast != NULL && (ast->kind & kinds) != 0;
}

bool c_ast_is_ref_to_kind_any( c_ast_t const *ast, c_ast_kind_t kinds ) {
  ast = c_ast_unreference( ast );
  return ast != NULL && (ast->kind & kinds) != 0;
}

bool c_ast_is_ptr_to_type_any( c_ast_t const *ast, c_type_t const *mask_type,
                               c_type_t const *type ) {
  assert( mask_type != NULL );

  c_tid_t qual_stids = TS_NONE;
  ast = c_ast_unpointer_qual( ast, &qual_stids );
  if ( ast == NULL )
    return false;
  c_type_t const masked_type = {
    c_tid_normalize( ast->type.btids ) & mask_type->btids,
    (ast->type.stids | qual_stids)     & mask_type->stids,
    ast->type.atids                    & mask_type->atids
  };
  return c_type_is_any( &masked_type, type );
}

c_ast_t const* c_ast_is_ptr_to_tid_any( c_ast_t const *ast, c_tid_t tids ) {
  c_tid_t qual_stids = TS_NONE;
  ast = c_ast_unpointer_qual( ast, &qual_stids );
  return c_ast_is_tid_any_qual_impl( ast, tids, qual_stids );
}

bool c_ast_is_ref_to_class_sname( c_ast_t const *ast, c_sname_t const *sname ) {
  ast = c_ast_is_ref_to_tid_any( ast, TB_ANY_CLASS );
  return ast != NULL && c_sname_cmp( &ast->csu.csu_sname, sname ) == 0;
}

c_ast_t const* c_ast_is_ref_to_tid_any( c_ast_t const *ast, c_tid_t tids ) {
  c_tid_t qual_stids = TS_NONE;
  ast = c_ast_unreference_qual( ast, &qual_stids );
  return c_ast_is_tid_any_qual_impl( ast, tids, qual_stids );
}

c_ast_t const* c_ast_is_ref_to_type_any( c_ast_t const *ast,
                                         c_type_t const *type ) {
  c_tid_t qual_stids = TS_NONE;
  ast = c_ast_unreference_qual( ast, &qual_stids );
  if ( ast == NULL )
    return NULL;

  c_type_t const ast_qual_type = {
    c_tid_normalize( ast->type.btids ),
    ast->type.stids | qual_stids,
    ast->type.atids
  };

  return c_type_is_any( &ast_qual_type, type ) ? ast : NULL;
}

c_ast_t const* c_ast_is_tid_any_qual( c_ast_t const *ast, c_tid_t tids,
                                      c_tid_t *rv_qual_stids ) {
  ast = c_ast_untypedef_qual( ast, rv_qual_stids );
  return c_ast_is_tid_any_qual_impl( ast, tids, *rv_qual_stids );
}

c_ast_t const* c_ast_leaf( c_ast_t const *ast ) {
  while ( c_ast_is_referrer( ast ) ) {
    c_ast_t const *const child_ast = ast->parent.of_ast;
    if ( child_ast == NULL )            // can be NULL for K_ENUM
      break;
    ast = child_ast;
  } // while
  return ast;
}

c_sname_t c_ast_move_sname( c_ast_t *ast ) {
  assert( ast != NULL );
  c_sname_t *const found_sname = c_ast_find_name( ast, C_VISIT_DOWN );
  c_sname_t rv_sname;
  if ( found_sname == NULL )
    c_sname_init( &rv_sname );
  else
    rv_sname = c_sname_move( found_sname );
  return rv_sname;
}

c_func_member_t c_ast_oper_overload( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );

  //
  // If the operator is either member or non-member only, then it's that.
  //
  c_operator_t const *const op = ast->oper.operator;
  switch ( op->overload ) {
    case C_OVERLOAD_NONE:
    case C_OVERLOAD_MEMBER:
    case C_OVERLOAD_NON_MEMBER:
      return STATIC_CAST( c_func_member_t, op->overload );
    case C_OVERLOAD_EITHER:
      break;
  } // switch

  //
  // Otherwise, the operator can be either: see if the user specified which one
  // explicitly.
  //
  switch ( ast->oper.member ) {
    case C_FUNC_UNSPECIFIED:
      break;
    case C_FUNC_MEMBER:
    case C_FUNC_NON_MEMBER:
      return ast->oper.member;
  } // switch

  //
  // The user didn't specify either member or non-member explicitly: see if it
  // has a member-only type qualifier.
  //
  if ( c_tid_is_any( ast->type.stids, TS_MEMBER_FUNC_ONLY ) )
    return C_FUNC_MEMBER;

  size_t const n_params = c_ast_params_count( ast );

  switch ( op->oper_id ) {
    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      //
      // Special case for new and delete operators: they're member operators if
      // they have a name (of a class) or declared static.
      //
      return  !c_sname_empty( &ast->sname ) ||
              c_tid_is_any( ast->type.stids, TS_static ) ?
        C_FUNC_MEMBER : C_FUNC_NON_MEMBER;

    case C_OP_MINUS2:
    case C_OP_PLUS2:
      //
      // Special case for ++ and -- operators: if the number of parameters is:
      //
      //  0. Member.
      //  1. If the type of the argument is int, member; else, non-member.
      //  2. Non-member.
      //
      if ( n_params == 1 ) {
        c_ast_t const *const param_ast = c_param_ast( c_ast_params( ast ) );
        return c_ast_is_builtin_any( param_ast, TB_int ) ?
          C_FUNC_MEMBER : C_FUNC_NON_MEMBER;
      }
      // The 0 and 2 cases are handled below.
      break;

    default:
      /* suppress warning */;
  } // switch

  //
  // Try to infer whether it's a member or non-member based on the number of
  // parameters given.
  //
  if ( n_params == op->params_min )
    return C_FUNC_MEMBER;
  if ( n_params == op->params_max )
    return C_FUNC_NON_MEMBER;

  //
  // We can't determine which one, so give up.
  //
  return C_FUNC_UNSPECIFIED;
}

c_ast_t* c_ast_patch_placeholder( c_ast_t *type_ast, c_ast_t *decl_ast ) {
  assert( type_ast != NULL );
  if ( decl_ast == NULL )
    return type_ast;

  if ( type_ast->parent_ast == NULL ) {
    c_ast_t *const placeholder_ast =
      c_ast_find_kind_any( decl_ast, C_VISIT_DOWN, K_PLACEHOLDER );
    if ( placeholder_ast != NULL ) {
      if ( type_ast->depth >= decl_ast->depth ||
           placeholder_ast->parent_ast == NULL ) {
        //
        // The type_ast is the final AST -- decl_ast (containing a placeholder)
        // is discarded.
        //
        if ( c_sname_empty( &type_ast->sname ) )
          type_ast->sname = c_ast_move_sname( decl_ast );
        return type_ast;
      }
      //
      // Otherwise, excise the K_PLACEHOLDER.
      // Before:
      //
      //      type_ast --> ... --> type_root_ast
      //      placeholder --> placeholder-parent
      //
      // After:
      //
      //      type_ast --> ... --> type_root_ast --> placeholder-parent
      //
      c_ast_t *const type_root_ast = c_ast_root( type_ast );
      c_ast_set_parent( type_root_ast, placeholder_ast->parent_ast );
    }
  }

  //
  // The decl_ast is the final AST -- type_ast may be discarded (if it wasn't
  // patched in), so take its storage, attributes, and name (if it doesn't have
  // one already).
  //
  c_type_t const taken_type = c_ast_take_storage( type_ast );
  decl_ast->type.stids |= taken_type.stids;
  decl_ast->type.atids |= taken_type.atids;

  if ( c_sname_empty( &decl_ast->sname ) )
    decl_ast->sname = c_ast_move_sname( type_ast );

  return decl_ast;
}

c_ast_t* c_ast_pointer( c_ast_t *ast, c_ast_list_t *ast_list ) {
  assert( ast != NULL );
  c_ast_t *const ptr_ast =
    c_ast_new( K_POINTER, ast->depth, &ast->loc, ast_list );
  ptr_ast->sname = c_ast_move_sname( ast );
  c_ast_set_parent( ast, ptr_ast );
  return ptr_ast;
}

c_ast_t* c_ast_root( c_ast_t *ast ) {
  assert( ast != NULL );
  while ( ast->parent_ast != NULL )
    ast = ast->parent_ast;
  return ast;
}

c_ast_t c_ast_sub_typedef( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_TYPEDEF );

  c_tid_t qual_stids;
  c_ast_t rv_ast = *c_ast_untypedef_qual( ast, &qual_stids );
  rv_ast.align = ast->align;
  rv_ast.loc = ast->loc;
  rv_ast.type.stids |= qual_stids;
  if ( c_ast_is_integral( &rv_ast ) )
    rv_ast.bit_field.bit_width = ast->bit_field.bit_width;
  return rv_ast;
}

c_type_t c_ast_take_type_any( c_ast_t *ast, c_type_t const *type ) {
  assert( ast != NULL );
  assert( type != NULL );
  c_ast_t *const found_ast = c_ast_find_type_any( ast, C_VISIT_DOWN, type );
  if ( found_ast == NULL )
    return T_NONE;
  c_type_t const rv_type = c_type_and( &found_ast->type, type );
  c_type_and_eq_compl( &found_ast->type, type );
  return rv_type;
}

c_ast_t const* c_ast_unpointer( c_ast_t const *ast ) {
  ast = c_ast_untypedef( ast );
  return ast->kind == K_POINTER ? c_ast_untypedef( ast->ptr_ref.to_ast ) : NULL;
}

c_ast_t const* c_ast_unreference( c_ast_t const *ast ) {
  // This is a loop to implement the reference-collapsing rule.
  for (;;) {
    ast = c_ast_untypedef( ast );
    if ( ast->kind != K_REFERENCE )
      return ast;
    ast = ast->ptr_ref.to_ast;
  } // for
}

c_ast_t const* c_ast_unrvalue_reference( c_ast_t const *ast ) {
  // This is a loop to implement the reference-collapsing rule.
  for (;;) {
    ast = c_ast_untypedef( ast );
    if ( ast->kind != K_RVALUE_REFERENCE )
      return ast;
    ast = ast->ptr_ref.to_ast;
  } // for
}

c_ast_t const* c_ast_untypedef( c_ast_t const *ast ) {
  for (;;) {
    assert( ast != NULL );
    if ( ast->kind != K_TYPEDEF )
      return ast;
    ast = ast->tdef.for_ast;
  } // for
}

c_ast_t const* c_ast_untypedef_qual( c_ast_t const *ast,
                                     c_tid_t *rv_qual_stids ) {
  assert( rv_qual_stids != NULL );
  *rv_qual_stids = TS_NONE;
  for (;;) {
    assert( ast != NULL );
    *rv_qual_stids |= ast->type.stids & TS_ANY_QUALIFIER;
    if ( ast->kind != K_TYPEDEF )
      return ast;
    ast = ast->tdef.for_ast;
  } // for
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
