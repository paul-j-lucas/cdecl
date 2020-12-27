/*
**      cdecl -- C gibberish translator
**      src/c_ast_util.c
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

/**
 * @file
 * Defines functions for various cdecl-specific algorithms for construcing an
 * Abstract Syntax Tree (AST) for parsed C/C++ declarations.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "c_ast_util.h"
#include "c_ast.h"
#include "literals.h"
#include "print.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

// local functions
PJL_WARN_UNUSED_RESULT
static c_ast_t* c_ast_append_array( c_ast_t*, c_ast_t* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Adds an array to the AST being built.
 *
 * @param ast The AST to append to.
 * @param array_ast The array AST to append.  Its "of" type must be null.
 * @return Returns the AST to be used as the grammar production's return value.
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t* c_ast_add_array_impl( c_ast_t *ast, c_ast_t *array_ast ) {
  assert( array_ast != NULL );
  assert( array_ast->kind_id == K_ARRAY );

  if ( ast == NULL )
    return array_ast;

  switch ( ast->kind_id ) {
    case K_ARRAY:
      return c_ast_append_array( ast, array_ast );

    case K_POINTER:
      if ( ast->depth > array_ast->depth ) {
        PJL_IGNORE_RV(
          c_ast_add_array_impl( ast->as.ptr_ref.to_ast, array_ast )
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
        //      [ast] --> [parent]
        //      [array_ast]
        //
        // After:
        //
        //      [ast] --> [array_ast] --> [parent]
        //
        if ( c_ast_is_parent( ast->parent_ast ) )
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
 * @param array_ast The array AST to append.  Its "of" type must be null.
 * @return If \a ast is an array, returns \a ast; otherwise returns \a
 * array_ast.
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t* c_ast_append_array( c_ast_t *ast, c_ast_t *array_ast ) {
  assert( ast != NULL );
  assert( array_ast != NULL );

  switch ( ast->kind_id ) {
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
      c_ast_t *const a = c_ast_append_array( ast->as.array.of_ast, array_ast );
      c_ast_set_parent( a, ast );
      return ast;
    }

    default:
      /* suppress warning */;
  } // switch

  assert( array_ast->kind_id == K_ARRAY );
  assert( array_ast->as.array.of_ast->kind_id == K_PLACEHOLDER );
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
 * null.
 * @return Returns the AST to be used as the grammar production's return value.
 */
PJL_WARN_UNUSED_RESULT
static c_ast_t* c_ast_add_func_impl( c_ast_t *ast, c_ast_t *ret_ast,
                                     c_ast_t *func_ast ) {
  assert( ast != NULL );
  assert( func_ast != NULL );
  assert( (func_ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );

  if ( (ast->kind_id &
        (K_ARRAY | K_ANY_POINTER | K_ANY_REFERENCE)) != K_NONE ) {
    switch ( ast->as.parent.of_ast->kind_id ) {
      case K_ARRAY:
      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        PJL_IGNORE_RV(
          c_ast_add_func_impl( ast->as.ptr_ref.to_ast, ret_ast, func_ast )
        );
        return ast;

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
    rv_type.store_tid = found_ast->type.store_tid & TS_MASK_STORAGE;
    rv_type.attr_tid = found_ast->type.attr_tid;
    found_ast->type.store_tid &= c_type_id_compl( TS_MASK_STORAGE );
    found_ast->type.attr_tid = TA_NONE;
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
static bool c_ast_vistor_kind_any( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  c_kind_id_t const kind_id = c_kind_data_get( data );
  return (ast->kind_id & kind_id) != K_NONE;
}

/**
 * A visitor function to find an AST node having a particular name.
 *
 * @param ast The AST to check.
 * @param data The least number of names that the scoped name must have.
 * @return Returns `true` only if \a ast has such a scoped name.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_name( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  uintptr_t const at_least = REINTERPRET_CAST( uintptr_t, data );
  return c_ast_count_name( ast ) >= at_least;
}

/**
 * A visitor function to find an AST node having a particular type(s).
 *
 * @param ast The AST to check.
 * @param data The bitwise-or of the type(s) (cast to `void*`) \a ast can be.
 * @return Returns `true` only if the type of \a ast is one of the types.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_vistor_type_any( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  c_type_t const *const type = data;
  return c_type_intersects( &ast->type, type );
}

////////// extern functions ///////////////////////////////////////////////////

c_ast_t* c_ast_add_array( c_ast_t *ast, c_ast_t *array_ast ) {
  assert( ast != NULL );
  c_ast_t *const rv_ast = c_ast_add_array_impl( ast, array_ast );
  assert( rv_ast != NULL );
  c_type_t const taken_type = c_ast_take_storage( array_ast->as.array.of_ast );
  array_ast->type.store_tid |= taken_type.store_tid;
  array_ast->type.attr_tid |= taken_type.attr_tid;
  return rv_ast;
}

c_ast_t* c_ast_add_func( c_ast_t *ast, c_ast_t *ret_ast, c_ast_t *func_ast ) {
  assert( ast != NULL );
  c_ast_t *const rv_ast = c_ast_add_func_impl( ast, ret_ast, func_ast );
  assert( rv_ast != NULL );
  if ( c_ast_empty_name( func_ast ) )
    func_ast->sname = c_ast_take_name( ast );
  c_type_t const taken_type = c_ast_take_storage( func_ast->as.func.ret_ast );
  c_type_or_eq( &func_ast->type, &taken_type );
  return rv_ast;
}

c_ast_t* c_ast_find_kind_any( c_ast_t *ast, c_visit_dir_t dir,
                              c_kind_id_t kind_ids ) {
  void *const data = c_kind_data_new( kind_ids );
  ast = c_ast_visit( ast, dir, c_ast_vistor_kind_any, data );
  c_kind_data_free( data );
  return ast;
}

c_sname_t* c_ast_find_name( c_ast_t const *ast, c_visit_dir_t dir ) {
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );
  c_ast_t *const found_ast = c_ast_visit(
    nonconst_ast, dir, c_ast_visitor_name, REINTERPRET_CAST( void*, 1 )
  );
  return found_ast != NULL ? &found_ast->sname : NULL;
}

c_ast_t* c_ast_find_type_any( c_ast_t *ast, c_visit_dir_t dir,
                              c_type_t const *types ) {
  return c_ast_visit(
    ast, dir, c_ast_vistor_type_any, CONST_CAST( void*, types )
  );
}

bool c_ast_is_builtin( c_ast_t const *ast, c_type_id_t builtin_tid ) {
  assert( ast != NULL );
  assert( c_type_id_part_id( builtin_tid ) == TPID_BASE );

  ast = c_ast_untypedef( ast );
  if ( ast->kind_id != K_BUILTIN )
    return false;
  c_type_id_t const base_tid = c_type_id_normalize( ast->type.base_tid );
  return base_tid == builtin_tid;
}

bool c_ast_is_kind_any( c_ast_t const *ast, c_kind_id_t kind_ids ) {
  ast = c_ast_unreference( ast );
  return (ast->kind_id & kind_ids) != K_NONE;
}

bool c_ast_is_ptr_to_type( c_ast_t const *ast, c_type_t const *mask_type,
                           c_type_t const *equal_type ) {
  assert( mask_type != NULL );

  ast = c_ast_unpointer( ast );
  if ( ast == NULL )
    return false;
  c_type_t const masked_type = {
    c_type_id_normalize( ast->type.base_tid ) & mask_type->base_tid,
    ast->type.store_tid & mask_type->store_tid,
    ast->type.attr_tid & mask_type->attr_tid
  };
  return c_type_equal( &masked_type, equal_type );
}

bool c_ast_is_ptr_to_tid_any( c_ast_t const *ast, c_type_id_t tids ) {
  assert( c_type_id_part_id( tids ) == TPID_BASE );
  ast = c_ast_unpointer( ast );
  if ( ast == NULL )
    return false;
  c_type_id_t const base_tid = c_type_id_normalize( ast->type.base_tid );
  return (base_tid & tids) != TB_NONE;
}

bool c_ast_is_ref_to_tid_any( c_ast_t const *ast, c_type_id_t tids ) {
  assert( c_type_id_part_id( tids ) == TPID_BASE );
  ast = c_ast_unreference( ast );
  c_type_id_t const base_tid = c_type_id_normalize( ast->type.base_tid );
  return (base_tid & tids) != TB_NONE;
}

bool c_ast_is_typename_ok( c_ast_t const *ast ) {
  assert( ast != NULL );

  c_sname_t const *const sname = ast->kind_id == K_TYPEDEF ?
    &ast->as.c_typedef.for_ast->sname :
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

  static c_type_t const typedef_type = { TB_NONE, TS_TYPEDEF, TA_ANY };
  c_type_t type = c_ast_take_type_any( type_ast, &typedef_type );

  if ( c_type_is_tid_any( &type, TS_TYPEDEF ) &&
       decl_ast->kind_id == K_TYPEDEF ) {
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
        "\"%s\": \"%s\" redefinition with different type\n",
        c_ast_full_name( decl_ast ), L_TYPEDEF
      );
      return NULL;
    }
  }

  c_ast_t *const ast = c_ast_patch_placeholder( type_ast, decl_ast );
  c_type_t const type2 = c_ast_take_type_any( ast, &typedef_type );
  c_type_or_eq( &type, &type2 );
  c_type_or_eq( &ast->type, &type );

  if ( align != NULL && align->kind != C_ALIGNAS_NONE ) {
    ast->align = *align;
    if ( c_type_is_tid_any( &type, TS_TYPEDEF ) ) {
      //
      // We check for illegal aligned typedef here rather than in error.c
      // because the "typedef-ness" needed to be removed previously before the
      // call to c_ast_check_declaration() below.
      //
      print_error( &align->loc, "%s can not be %s\n", L_TYPEDEF, L_ALIGNED );
      return NULL;
    }
  }

  if ( ast->kind_id == K_USER_DEF_CONVERSION &&
       c_ast_local_name_type( ast )->base_tid == TB_SCOPE ) {
    //
    // User-defined conversions don't have names, but they can still have a
    // scope.  Since only classes can have them, if the scope is still
    // TB_SCOPE, change it to TB_CLASS.
    //
    c_ast_set_local_name_type( ast, &C_TYPE_LIT_B( TB_CLASS ) );
  }

  return c_ast_check_declaration( ast ) ? ast : NULL;
}

c_ast_t* c_ast_patch_placeholder( c_ast_t *type_ast, c_ast_t *decl_ast ) {
  assert( type_ast != NULL );
  if ( decl_ast == NULL )
    return type_ast;

  if ( type_ast->parent_ast == NULL ) {
    c_ast_t *const placeholder =
      c_ast_find_kind_any( decl_ast, C_VISIT_DOWN, K_PLACEHOLDER );
    if ( placeholder != NULL ) {
      if ( type_ast->depth >= decl_ast->depth ) {
        //
        // The type_ast is the final AST -- decl_ast (containing a placeholder)
        // is discarded.
        //
        if ( c_ast_empty_name( type_ast ) )
          type_ast->sname = c_ast_take_name( decl_ast );
        return type_ast;
      }
      //
      // Otherwise, excise the K_PLACEHOLDER.
      // Before:
      //
      //      [type] --> ... --> [type-root]
      //      [placeholder] --> [placeholder-parent]
      //
      // After:
      //
      //      [type] --> ... --> [type-root] --> [placeholder-parent]
      //
      c_ast_t *const type_root_ast = c_ast_root( type_ast );
      c_ast_set_parent( type_root_ast, placeholder->parent_ast );
    }
  }

  //
  // The decl_ast is the final AST -- type_ast may be discarded (if it wasn't
  // patched in), so take its name if we don't have one already.
  //
  if ( c_ast_empty_name( decl_ast ) )
    decl_ast->sname = c_ast_take_name( type_ast );
  return decl_ast;
}

c_sname_t c_ast_take_name( c_ast_t *ast ) {
  assert( ast != NULL );
  c_sname_t *const found = c_ast_find_name( ast, C_VISIT_DOWN );
  c_sname_t rv;
  if ( found == NULL ) {
    c_sname_init( &rv );
  } else {
    rv = *found;
    c_sname_init( found );
  }
  return rv;
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
  return  ast->kind_id == K_POINTER ?
          c_ast_untypedef( ast->as.ptr_ref.to_ast ) : NULL;
}

c_ast_t const* c_ast_unreference( c_ast_t const *ast ) {
  // This is a loop to implement the reference-collapsing rule.
  for (;;) {
    ast = c_ast_untypedef( ast );
    if ( ast->kind_id != K_REFERENCE )
      return ast;
    ast = ast->as.ptr_ref.to_ast;
  } // for
}

c_ast_t const* c_ast_untypedef( c_ast_t const *ast ) {
  for (;;) {
    assert( ast != NULL );
    if ( ast->kind_id != K_TYPEDEF )
      return ast;
    ast = ast->as.c_typedef.for_ast;
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
