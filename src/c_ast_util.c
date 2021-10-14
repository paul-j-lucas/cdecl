/*
**      cdecl -- C gibberish translator
**      src/c_ast_util.c
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

/**
 * @file
 * Defines functions for various cdecl-specific algorithms for construcing an
 * Abstract Syntax Tree (AST) for parsed C/C++ declarations.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_AST_UTIL_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_ast_util.h"
#include "c_typedef.h"
#include "gibberish.h"
#include "literals.h"
#include "print.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

// local functions
PJL_WARN_UNUSED_RESULT
static c_ast_t* c_ast_append_array( c_ast_t*, c_ast_t*, c_ast_t* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Adds an array to the AST being built.
 *
 * @param ast The AST to append to; may be NULL.
 * @param of_ast The AST of the of-type of the array AST.
 * @param array_ast The array AST to append.  Its "of" type must be NULL.
 * @return Returns the AST to be used as the grammar production's return value.
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t* c_ast_add_array_impl( c_ast_t *ast, c_ast_t *of_ast,
                                      c_ast_t *array_ast ) {
  assert( of_ast != NULL );
  assert( array_ast != NULL );
  assert( array_ast->kind == K_ARRAY );

  if ( ast == NULL )
    return array_ast;

  switch ( ast->kind ) {
    case K_ARRAY:
      return c_ast_append_array( ast, of_ast, array_ast );

    case K_PLACEHOLDER:
      //
      // Before:
      //
      //      [array_ast]
      //      [placeholder] --> [placeholder-parent]
      //      [of_ast]
      //
      // After:
      //
      //      [of_ast] --> [array_ast] --> [placeholder-parent]
      //
      if ( ast->parent_ast != NULL )
        c_ast_set_parent( array_ast, ast->parent_ast );
      c_ast_set_parent( of_ast, array_ast );
      return array_ast;

    case K_POINTER:
      if ( ast->depth > array_ast->depth ) {
        PJL_IGNORE_RV(
          c_ast_add_array_impl( ast->as.ptr_ref.to_ast, of_ast, array_ast )
        );
        return ast;
      }
      PJL_FALLTHROUGH;

    default:
      //
      // An AST node's "depth" says how nested within () it is and controls the
      // precedence of what is an array of what.
      //
      if ( ast->depth > array_ast->depth ) {
        //
        // Before:
        //
        //      [ast-child] --> [ast]
        //      [array_ast]
        //
        // After:
        //
        //      [ast-child] --> [array_ast] --> [ast]
        //
        if ( c_ast_is_parent( ast ) )
          c_ast_set_parent( ast->as.parent.of_ast, array_ast );
        c_ast_set_parent( array_ast, ast );
        return ast;
      }
      else {
        //
        // Before:
        //
        //      [ast] --> [ast-parent]
        //      [array_ast]
        //
        // After:
        //
        //      [ast] --> [array_ast] --> [ast-parent]
        //
        if ( ast->parent_ast != NULL )
          c_ast_set_parent( array_ast, ast->parent_ast );
        c_ast_set_parent( ast, array_ast );
        return array_ast;
      }
  } // switch
}

/**
 * If \a ast is:
 *  + Not an array, makes \a array_ast an array of \a ast.
 *  + An array, appends \a array_ast to the end of the array AST chain.
 *
 * For example, given:
 *
 *  + \a ast = <code>array 3 of array 5 of int</code>
 *  + \a array_ast = <code>array 7 of NULL</code>
 *
 * this function returns:
 *
 *  + <code>array 3 of array 5 of array 7 of int</code>
 *
 * @param ast The AST to append to.
 * @param of_ast The AST of the of-type of the array AST.
 * @param array_ast The array AST to append.  Its "of" type must be NULL.
 * @return If \a ast is an array, returns \a ast; otherwise returns \a
 * array_ast.
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t* c_ast_append_array( c_ast_t *ast, c_ast_t *of_ast,
                                    c_ast_t *array_ast ) {
  assert( ast != NULL );
  assert( of_ast != NULL );
  assert( array_ast != NULL );
  assert( array_ast->kind == K_ARRAY );
  assert( array_ast->as.array.of_ast != NULL );
  assert( array_ast->as.array.of_ast->kind == K_PLACEHOLDER );

  switch ( ast->kind ) {
    case K_POINTER:
      //
      // If there's an intervening pointer, e.g.:
      //
      //      type (*(*x)[3])[5]
      //
      // (where 'x' is a "pointer to array 3 of pointer to array 5 of int"), we
      // have to recurse "through" it if its depth < the array's depth; else
      // we'd end up with a "pointer to array 3 of array 5 of pointer to int."
      //
      if ( array_ast->depth >= ast->depth )
        break;
      PJL_FALLTHROUGH;

    case K_ARRAY: {
      //
      // On the next-to-last recursive call, this sets this array to be an
      // array of the new array; for all prior recursive calls, it's a no-op.
      //
      c_ast_t *const temp_ast =
        c_ast_append_array( ast->as.array.of_ast, of_ast, array_ast );
      c_ast_set_parent( temp_ast, ast );
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
 * Adds a function-like AST to the AST being built.
 *
 * @param ast The AST to append to.
 * @param ret_ast The AST of the return-type of the function-like AST.
 * @param func_ast The function-like AST to append.  Its "of" type must be
 * NULL.
 * @return Returns the AST to be used as the grammar production's return value.
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t* c_ast_add_func_impl( c_ast_t *ast, c_ast_t *ret_ast,
                                     c_ast_t *func_ast ) {
  assert( ast != NULL );
  assert( func_ast != NULL );
  assert( c_ast_is_kind_any( func_ast, K_ANY_FUNCTION_LIKE ) );
  assert( ret_ast != NULL );

  if ( c_ast_is_kind_any( ast, K_ARRAY | K_ANY_POINTER | K_ANY_REFERENCE ) ) {
    switch ( ast->as.parent.of_ast->kind ) {
      case K_ARRAY:
      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        if ( ast->depth > func_ast->depth ) {
          PJL_IGNORE_RV(
            c_ast_add_func_impl( ast->as.ptr_ref.to_ast, ret_ast, func_ast )
          );
          return ast;
        }
        break;

      case K_PLACEHOLDER:
        if ( ret_ast == ast )
          break;
        c_ast_set_parent( func_ast, ast );
        PJL_FALLTHROUGH;

      case K_APPLE_BLOCK:
        c_ast_set_parent( ret_ast, func_ast );
        return ast;

      default:
        /* suppress warning */;
    } // switch
  }

  c_ast_set_parent( ret_ast, func_ast );
  return func_ast;
}

/**
 * Only if \a ast is a <code>\ref K_POINTER</code>, un-pointers \a ast.
 *
 * @param ast The AST to un-pointer.
 * @param cv_stids If \a ast is a pointer, receives the `const` and `volatile`
 * (cv) qualifiers (only) of the first pointed-to type.  For a declaration like
 * <code>const&nbsp;S&nbsp;*x</code> (where `S` is a `struct`), the `const` is
 * associated with the `typedef` for the `struct` and _not_ the actual `struct`
 * the `typedef` is a `typedef` for:
 * ```
 * decl_c = {
 *   sname = "x" (none),
 *   kind = "pointer",
 *   ...
 *   type = "" (base = 0x1, store = 0x2, attr = 0x4),
 *   to_ast = {
 *     sname = "",
 *     kind = "typedef",
 *     ...
 *     type = "const" (base = 0x10000001, store = 0x20000002, attr = 0x4),
 *     for_ast = {
 *       sname = "S" (struct),
 *       kind = "enum, class, struct, or union",
 *       ...
 *       type = "struct" (base = 0x800001, store = 0x2, attr = 0x4),
 *       ecsu_sname = "S" (none)
 *     },
 *     ...
 *   }
 * }
 * ```
 * Therefore, we need to copy the cv qualifiers of the `typedef` before we
 * un-`typedef` it.
 * @return Returns the pointed-to AST or NULL if \a ast is not a pointer.
 *
 * @sa c_ast_if_unreference()
 * @sa c_ast_unpointer()
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t const* c_ast_if_unpointer( c_ast_t const *ast,
                                          c_tid_t *cv_stids ) {
  ast = c_ast_untypedef( ast );
  if ( ast->kind != K_POINTER )
    return NULL;

  ast = ast->as.ptr_ref.to_ast;
  assert( ast != NULL );
  assert( cv_stids != NULL );
  *cv_stids = ast->type.stids & TS_CV;
  //
  // Now that we've gotten the cv qualifiers of the first pointed-to type, we
  // can just call the ordinary c_ast_untypedef() to peel off any remaining
  // typedef layers.
  //
  return c_ast_untypedef( ast );
}

/**
 * Only if \a ast is a <code>\ref K_REFERENCE</code>, un-references \a ast.
 *
 * @param ast The AST to un-reference.
 * @param cv_stids If \a ast is a reference, receives the `const` and
 * `volatile` (cv) qualifiers (only) of the first referred-to type.  For a
 * declaration like <code>const&nbsp;S&nbsp;&x</code> (where `S` is a
 * `struct`), the `const` is associated with the `typedef` for the `struct` and
 * _not_ the actual `struct` the `typedef` is a `typedef` for:
 * ```
 * decl_c = {
 *   sname = "x" (none),
 *   kind = "reference",
 *   ...
 *   type = "" (base = 0x1, store = 0x2, attr = 0x4),
 *   to_ast = {
 *     sname = "",
 *     kind = "typedef",
 *     ...
 *     type = "const" (base = 0x10000001, store = 0x20000002, attr = 0x4),
 *     for_ast = {
 *       sname = "S" (struct),
 *       kind = "enum, class, struct, or union",
 *       ...
 *       type = "struct" (base = 0x800001, store = 0x2, attr = 0x4),
 *       ecsu_sname = "S" (none)
 *     },
 *     bit_width = 0
 *   }
 * }
 * ```
 * Therefore, we need to copy the cv qualifiers of the `typedef` before we
 * un-`typedef` it.
 * @return Returns the referenced AST or NULL if \a ast is not a reference.
 *
 * @sa c_ast_if_unpointer()
 * @sa c_ast_unreference()
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t const* c_ast_if_unreference( c_ast_t const *ast,
                                            c_tid_t *cv_stids ) {
  ast = c_ast_untypedef( ast );
  if ( ast->kind != K_REFERENCE )
    return NULL;

  ast = ast->as.ptr_ref.to_ast;
  assert( ast != NULL );
  assert( cv_stids != NULL );
  *cv_stids = ast->type.stids & TS_CV;
  //
  // Now that we've gotten the cv qualifiers of the first referred-to type, we
  // can just call the ordinary c_ast_unreference() to peel off any remaining
  // typedef/reference layers.
  //
  return c_ast_unreference( ast );
}

/**
 * Helper function that checks whether the type of \a ast is one of \a tids.
 *
 * @param ast The AST to check; may be NULL.
 * @param ast_cv_stids The `const`/`volatiile` qualifier(s) of the `typedef`
 * for \a ast, if any.
 * @param tids The bitwise-or of type(s) to check against.
 * @return If \a ast is not NULL and the type of \a ast is one of \a tids,
 * returns \a ast; otherwise returns NULL.
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t const* c_ast_is_tid_any_impl( c_ast_t const *ast,
                                             c_tid_t ast_cv_stids,
                                             c_tid_t tids ) {
  if ( ast != NULL ) {
    c_tid_t ast_stids = c_type_get_tid( &ast->type, tids );
    ast_stids = c_tid_normalize( ast_stids );
    if ( c_tid_tpid( tids ) == C_TPID_STORE )
      ast_stids |= ast_cv_stids;
    if ( c_tid_is_any( ast_stids, tids ) )
      return ast;
  }
  return NULL;
}

/**
 * Gets the root AST node of \a ast.
 *
 * @param ast The AST node to get the root of.
 * @return Returns said AST node.
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t* c_ast_root( c_ast_t *ast ) {
  assert( ast != NULL );
  while ( ast->parent_ast != NULL )
    ast = ast->parent_ast;
  return ast;
}

/**
 * Takes the storage (and attributes), if any, away from \a ast
 * (with the intent of giving them to another AST).
 * This is used is cases like:
 * @code
 *  explain static int f()
 * @endcode
 * that should be explained as:
 * @code
 *  declare f as static function () returning int
 * @endcode
 * and _not_:
 * @code
 *  declare f as function () returning static int
 * @endcode
 * i.e., the `static` has to be taken away from `int` and given to the function
 * because it's the function that's `static`, not the `int`.
 *
 * @param ast The AST to take from.
 * @return Returns said storage (and attributes) or <code>\ref T_NONE</code>.
 */
PJL_WARN_UNUSED_RESULT
static c_type_t c_ast_take_storage( c_ast_t *ast ) {
  assert( ast != NULL );
  c_type_t rv_type = T_NONE;
  c_ast_t *const found_ast =
    c_ast_find_kind_any( ast, C_VISIT_DOWN, K_BUILTIN | K_TYPEDEF );
  if ( found_ast != NULL ) {
    rv_type.stids = found_ast->type.stids & TS_MASK_STORAGE;
    rv_type.atids = found_ast->type.atids;
    found_ast->type.stids &= c_tid_compl( TS_MASK_STORAGE );
    found_ast->type.atids = TA_NONE;
  }
  return rv_type;
}

/**
 * A visitor function to find an AST node having a particular kind(s).
 *
 * @param ast The AST to check.
 * @param data The bitwise-or of the kind(s) (cast to `void*`) \a ast can be.
 * @return Returns `true` only if the kind of \a ast is one of the kinds.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_vistor_kind_any( c_ast_t *ast, c_ast_visitor_data_t data ) {
  assert( ast != NULL );
  c_ast_kind_t const kinds = STATIC_CAST( c_ast_kind_t, data );
  return c_ast_is_kind_any( ast, kinds );
}

/**
 * A visitor function to find an AST node having a non-empty name.
 *
 * @param ast The AST to check.
 * @param data Not used.
 * @return Returns `true` only if \a ast has such a scoped name.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_name( c_ast_t *ast, c_ast_visitor_data_t data ) {
  assert( ast != NULL );
  (void)data;
  return !c_ast_name_empty( ast );
}

/**
 * A visitor function to find an AST node having a particular type(s).
 *
 * @param ast The AST to check.
 * @param data A pointer to a type where each type part is the bitwise-or of
 * type IDs to find.
 * @return Returns `true` only if the type of \a ast is one of the types.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_vistor_type_any( c_ast_t *ast, c_ast_visitor_data_t data ) {
  assert( ast != NULL );
  c_type_t const *const type = REINTERPRET_CAST( c_type_t*, data );
  return c_type_is_any( &ast->type, type );
}

////////// extern functions ///////////////////////////////////////////////////

c_ast_t* c_ast_add_array( c_ast_t *ast, c_ast_t *of_ast, c_ast_t *array_ast ) {
  assert( ast != NULL );
  c_ast_t *const rv_ast = c_ast_add_array_impl( ast, of_ast, array_ast );
  assert( rv_ast != NULL );
  if ( c_ast_name_empty( rv_ast ) )
    rv_ast->sname = c_ast_take_name( ast );
  c_type_t const taken_type = c_ast_take_storage( array_ast->as.array.of_ast );
  c_type_or_eq( &array_ast->type, &taken_type );
  return rv_ast;
}

c_ast_t* c_ast_add_func( c_ast_t *ast, c_ast_t *ret_ast, c_ast_t *func_ast ) {
  c_ast_t *const rv_ast = c_ast_add_func_impl( ast, ret_ast, func_ast );
  assert( rv_ast != NULL );
  if ( c_ast_name_empty( rv_ast ) )
    rv_ast->sname = c_ast_take_name( ast );
  c_type_t const taken_type = c_ast_take_storage( func_ast->as.func.ret_ast );
  c_type_or_eq( &rv_ast->type, &taken_type );
  return rv_ast;
}

c_ast_t* c_ast_find_kind_any( c_ast_t *ast, c_visit_dir_t dir,
                              c_ast_kind_t kinds ) {
  assert( kinds != 0 );
  c_ast_visitor_data_t const data = STATIC_CAST( c_ast_visitor_data_t, kinds );
  return c_ast_visit( ast, dir, c_ast_vistor_kind_any, data );
}

c_sname_t* c_ast_find_name( c_ast_t const *ast, c_visit_dir_t dir ) {
  c_ast_t *const found_ast =
    c_ast_visit( CONST_CAST( c_ast_t*, ast ), dir, c_ast_visitor_name, 0 );
  return found_ast != NULL ? &found_ast->sname : NULL;
}

c_ast_t* c_ast_find_type_any( c_ast_t *ast, c_visit_dir_t dir,
                              c_type_t const *type ) {
  c_ast_visitor_data_t const data =
    REINTERPRET_CAST( c_ast_visitor_data_t, type );
  return c_ast_visit( ast, dir, c_ast_vistor_type_any, data );
}

bool c_ast_is_builtin_any( c_ast_t const *ast, c_tid_t btids ) {
  assert( ast != NULL );
  assert( c_tid_tpid( btids ) == C_TPID_BASE );

  ast = c_ast_untypedef( ast );
  if ( ast->kind != K_BUILTIN )
    return false;
  c_tid_t const ast_btids = c_tid_normalize( ast->type.btids );
  return only_bits_set( ast_btids, btids );
}

bool c_ast_is_ptr_to_kind( c_ast_t const *ast, c_ast_kind_t kind ) {
  assert( ast != NULL );
  ast = c_ast_unpointer( ast );
  return ast != NULL && ast->kind == kind;
}

bool c_ast_is_ptr_to_type_any( c_ast_t const *ast, c_type_t const *mask_type,
                               c_type_t const *type ) {
  assert( mask_type != NULL );

  c_tid_t cv_stids;
  ast = c_ast_if_unpointer( ast, &cv_stids );
  if ( ast == NULL )
    return false;
  c_type_t const masked_type = {
    c_tid_normalize( ast->type.btids ) & mask_type->btids,
    (ast->type.stids | cv_stids)       & mask_type->stids,
    ast->type.atids                    & mask_type->atids
  };
  return c_type_is_any( &masked_type, type );
}

c_ast_t const* c_ast_is_ptr_to_tid_any( c_ast_t const *ast, c_tid_t tids ) {
  c_tid_t cv_stids;
  ast = c_ast_if_unpointer( ast, &cv_stids );
  return c_ast_is_tid_any_impl( ast, cv_stids, tids );
}

bool c_ast_is_ref_to_class_sname( c_ast_t const *ast, c_sname_t const *sname ) {
  ast = c_ast_is_ref_to_tid_any( ast, TB_ANY_CLASS );
  if ( ast == NULL )
    return false;
  return  c_sname_cmp( &ast->sname, sname ) == 0 ||
          c_sname_cmp( &ast->as.ecsu.ecsu_sname, sname ) == 0;
}

c_ast_t const* c_ast_is_ref_to_tid_any( c_ast_t const *ast, c_tid_t tids ) {
  c_tid_t cv_stids;
  ast = c_ast_if_unreference( ast, &cv_stids );
  return c_ast_is_tid_any_impl( ast, cv_stids, tids );
}

c_ast_t const* c_ast_is_ref_to_type_any( c_ast_t const *ast,
                                         c_type_t const *type ) {
  c_tid_t cv_stids;
  ast = c_ast_if_unreference( ast, &cv_stids );
  if ( ast == NULL )
    return NULL;

  c_type_t const ast_cv_type = {
    c_tid_normalize( ast->type.btids ),
    ast->type.stids | cv_stids,
    ast->type.atids
  };

  return c_type_is_any( &ast_cv_type, type ) ? ast : NULL;
}

c_ast_t const* c_ast_is_tid_any( c_ast_t const *ast, c_tid_t tids ) {
  ast = c_ast_untypedef( ast );
  return c_ast_is_tid_any_impl( ast, TS_NONE, tids );
}

bool c_ast_is_typename_ok( c_ast_t const *ast ) {
  assert( ast != NULL );

  c_sname_t const *const sname = ast->kind == K_TYPEDEF ?
    &ast->as.tdef.for_ast->sname :
    &ast->sname;

  if ( c_sname_count( sname ) < 2 ) {
    print_error( &ast->loc,
      "qualified name expected after \"%s\"\n", L_TYPENAME
    );
    return false;
  }
  return true;
}

c_ast_t* c_ast_join_type_decl( bool has_typename, c_alignas_t const *align,
                               c_ast_t *type_ast, c_ast_t *decl_ast,
                               c_loc_t const *decl_loc ) {
  assert( type_ast != NULL );
  assert( decl_ast != NULL );
  assert( decl_loc != NULL );

  if ( has_typename && !c_ast_is_typename_ok( type_ast ) )
    return NULL;

  c_type_t type = c_ast_take_type_any( type_ast, &T_TS_TYPEDEF );

  if ( c_type_is_tid_any( &type, TS_TYPEDEF ) &&
       decl_ast->kind == K_TYPEDEF ) {
    //
    // This is for a case like:
    //
    //      explain typedef int int32_t;
    //
    // that is: explaining an existing typedef.  In order to do that, we have
    // to un-typedef it so we explain the type that it's typedef'd as.
    //
    decl_ast = CONST_CAST( c_ast_t*, c_ast_untypedef( decl_ast ) );

    //
    // However, we also have to check whether the typedef being explained is
    // not equivalent to the existing typedef.  This is for a case like:
    //
    //      explain typedef char int32_t;
    //
    if ( !c_ast_equiv( type_ast, decl_ast ) ) {
      print_error( decl_loc,
        "\"%s\": \"%s\" redefinition with different type; original is: ",
        c_ast_full_name( decl_ast ), L_TYPEDEF
      );
      //
      // When printing the existing type in C/C++ as part of an error message,
      // we always want to omit the trailing semicolon.
      //
      bool const orig_semicolon = opt_semicolon;
      opt_semicolon = false;

      c_typedef_t const temp_tdef = { decl_ast, LANG_ANY, false, false };
      c_typedef_gibberish( &temp_tdef, C_GIB_TYPEDEF, stderr );

      opt_semicolon = orig_semicolon;
      return NULL;
    }
  }

  c_ast_t *const ast = c_ast_patch_placeholder( type_ast, decl_ast );
  c_type_t const type2 = c_ast_take_type_any( ast, &T_TS_TYPEDEF );
  c_type_or_eq( &type, &type2 );
  c_type_or_eq( &ast->type, &type );

  if ( align != NULL && align->kind != C_ALIGNAS_NONE ) {
    ast->align = *align;
    if ( c_type_is_tid_any( &type, TS_TYPEDEF ) ) {
      //
      // We check for illegal aligned typedef here rather than in errors.c
      // because the "typedef-ness" needed to be removed previously before the
      // eventual call to c_ast_check().
      //
      print_error( &align->loc, "%s can not be %s\n", L_TYPEDEF, L_ALIGNED );
      return NULL;
    }
  }

  if ( ast->kind == K_USER_DEF_CONVERSION &&
       c_ast_local_type( ast )->btids == TB_SCOPE ) {
    //
    // User-defined conversions don't have names, but they can still have a
    // scope.  Since only classes can have them, if the scope is still
    // TB_SCOPE, change it to TB_CLASS.
    //
    c_ast_set_local_type( ast, &C_TYPE_LIT_B( TB_CLASS ) );
  }

  return ast;
}

unsigned c_ast_oper_overload( c_ast_t const *ast ) {
  assert( ast != NULL );
  assert( ast->kind == K_OPERATOR );

  //
  // If the operator is either member or non-member only, then it's that.
  //
  c_operator_t const *const op = c_oper_get( ast->as.oper.oper_id );
  unsigned const op_overload_flags = op->flags & C_OP_MASK_OVERLOAD;
  switch ( op_overload_flags ) {
    case C_OP_MEMBER:
    case C_OP_NON_MEMBER:
    case C_OP_NOT_OVERLOADABLE:
      return op_overload_flags;
  } // switch

  //
  // Otherwise, the operator can be either: see if the user specified which one
  // explicitly.
  //
  unsigned const user_overload_flags = ast->as.oper.flags & C_OP_MASK_OVERLOAD;
  switch ( user_overload_flags ) {
    case C_OP_MEMBER:
    case C_OP_NON_MEMBER:
      return user_overload_flags;
  } // switch

  //
  // The user didn't specify either member or non-member explicitly: see if it
  // has a member-only type qualifier.
  //
  if ( c_type_is_tid_any( &ast->type, TS_MEMBER_FUNC_ONLY ) )
    return C_OP_MEMBER;

  size_t const n_params = c_ast_params_count( ast );

  switch ( ast->as.oper.oper_id ) {
    case C_OP_NEW:
    case C_OP_NEW_ARRAY:
    case C_OP_DELETE:
    case C_OP_DELETE_ARRAY:
      //
      // Special case for new and delete operators: they're member operators if
      // they have a name (of a class) or declared static.
      //
      return  !c_ast_name_empty( ast ) ||
              c_type_is_tid_any( &ast->type, TS_STATIC ) ?
        C_OP_MEMBER : C_OP_NON_MEMBER;

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
        return c_ast_is_builtin_any( param_ast, TB_INT ) ?
          C_OP_MEMBER : C_OP_NON_MEMBER;
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
    return C_OP_MEMBER;
  if ( n_params == op->params_max )
    return C_OP_NON_MEMBER;

  //
  // We can't determine which one, so give up.
  //
  return C_OP_UNSPECIFIED;
}

c_ast_t* c_ast_patch_placeholder( c_ast_t *type_ast, c_ast_t *decl_ast ) {
  assert( type_ast != NULL );
  if ( decl_ast == NULL )
    return type_ast;

  if ( type_ast->parent_ast == NULL ) {
    c_ast_t *const placeholder_ast =
      c_ast_find_kind_any( decl_ast, C_VISIT_DOWN, K_PLACEHOLDER );
    if ( placeholder_ast != NULL ) {
      if ( type_ast->depth >= decl_ast->depth ) {
        //
        // The type_ast is the final AST -- decl_ast (containing a placeholder)
        // is discarded.
        //
        if ( c_ast_name_empty( type_ast ) )
          type_ast->sname = c_ast_take_name( decl_ast );
        return type_ast;
      }
      //
      // Otherwise, excise the K_PLACEHOLDER.
      // Before:
      //
      //      [type_ast] --> ... --> [type_root_ast]
      //      [placeholder] --> [placeholder-parent]
      //
      // After:
      //
      //      [type_ast] --> ... --> [type_root_ast] --> [placeholder-parent]
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

  if ( c_ast_name_empty( decl_ast ) )
    decl_ast->sname = c_ast_take_name( type_ast );

  return decl_ast;
}

c_ast_t* c_ast_pointer( c_ast_t *ast, c_ast_list_t *ast_list ) {
  assert( ast != NULL );
  c_ast_t *const ptr_ast =
    c_ast_new( K_POINTER, ast->depth, &ast->loc, ast_list );
  ptr_ast->sname = c_ast_take_name( ast );
  c_ast_set_parent( ast, ptr_ast );
  return ptr_ast;
}

c_sname_t c_ast_take_name( c_ast_t *ast ) {
  assert( ast != NULL );
  c_sname_t *const found_sname = c_ast_find_name( ast, C_VISIT_DOWN );
  c_sname_t rv_sname;
  if ( found_sname == NULL ) {
    c_sname_init( &rv_sname );
  } else {
    rv_sname = *found_sname;
    c_sname_init( found_sname );
  }
  return rv_sname;
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
  return  ast->kind == K_POINTER ?
          c_ast_untypedef( ast->as.ptr_ref.to_ast ) : NULL;
}

c_ast_t const* c_ast_unreference( c_ast_t const *ast ) {
  // This is a loop to implement the reference-collapsing rule.
  for (;;) {
    ast = c_ast_untypedef( ast );
    if ( ast->kind != K_REFERENCE )
      return ast;
    ast = ast->as.ptr_ref.to_ast;
  } // for
}

c_ast_t const* c_ast_unrvalue_reference( c_ast_t const *ast ) {
  // This is a loop to implement the reference-collapsing rule.
  for (;;) {
    ast = c_ast_untypedef( ast );
    if ( ast->kind != K_RVALUE_REFERENCE )
      return ast;
    ast = ast->as.ptr_ref.to_ast;
  } // for
}

c_ast_t const* c_ast_untypedef( c_ast_t const *ast ) {
  for (;;) {
    assert( ast != NULL );
    if ( ast->kind != K_TYPEDEF )
      return ast;
    ast = ast->as.tdef.for_ast;
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
