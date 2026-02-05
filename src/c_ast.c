/*
**      cdecl -- C gibberish translator
**      src/c_ast.c
**
**      Copyright (C) 2017-2026  Paul J. Lucas
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
 * Defines functions for traversing and manipulating an AST.
 */

// local
#include "pjl_config.h"                 /* IWYU pragma: keep */
#include "c_ast.h"
#include "c_sname.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>                     /* for NULL, size_t, offsetof() */
#include <stdlib.h>                     /* for free(3) */
#include <string.h>

/// @endcond

STATIC_ASSERT(
  offsetof( c_builtin_ast_t, bit_width ) ==
  offsetof( c_bit_field_ast_t, bit_width )
);

STATIC_ASSERT(
  offsetof( c_enum_ast_t, bit_width ) ==
  offsetof( c_bit_field_ast_t, bit_width )
);

STATIC_ASSERT(
  offsetof( c_typedef_ast_t, bit_width ) ==
  offsetof( c_bit_field_ast_t, bit_width )
);

STATIC_ASSERT(
  offsetof( c_apple_block_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list )
);

STATIC_ASSERT(
  offsetof( c_concept_ast_t, concept_sname ) == offsetof( c_name_ast_t, sname )
);

STATIC_ASSERT(
  offsetof( c_constructor_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list )
);

STATIC_ASSERT(
  offsetof( c_csu_ast_t, csu_sname ) == offsetof( c_name_ast_t, sname )
);

STATIC_ASSERT(
  offsetof( c_enum_ast_t, enum_sname ) == offsetof( c_name_ast_t, sname )
);

STATIC_ASSERT(
  offsetof( c_lambda_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list )
);

STATIC_ASSERT(
  offsetof( c_operator_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list )
);

STATIC_ASSERT(
  offsetof( c_ptr_mbr_ast_t, class_sname ) == offsetof( c_name_ast_t, sname )
);

STATIC_ASSERT(
  offsetof( c_udef_lit_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list )
);

STATIC_ASSERT(
  offsetof( c_operator_ast_t, member ) == offsetof( c_function_ast_t, member )
);

///////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
// local variable definitions
static size_t c_ast_count;              ///< ASTs allocated but not yet freed.
#endif /* NDEBUG */

////////// local functions ////////////////////////////////////////////////////

/**
 * @addtogroup ast-functions-group
 * @{
 */

/**
 * Cleans-up all memory associated with \a align but does _not_ free \a align
 * itself.
 *
 * @param align The \ref c_alignas to clean up.  If NULL, does nothing.
 */
static void c_alignas_cleanup( c_alignas_t *align ) {
  if ( align != NULL && align->kind == C_ALIGNAS_SNAME )
    c_sname_cleanup( &align->sname );
}

/**
 * Duplicates \a align.
 *
 * @param align The \ref c_alignas to duplicate.
 * @return Returns the duplicated alignment.
 */
NODISCARD
static c_alignas_t c_alignas_dup( c_alignas_t const *align ) {
  assert( align != NULL );

  c_alignas_t dup_align = {
    .kind = align->kind,
    .loc = align->loc
  };

  switch ( align->kind ) {
    case C_ALIGNAS_NONE:
      break;                            // LCOV_EXCL_LINE
    case C_ALIGNAS_BYTES:
      dup_align.bytes = align->bytes;
      break;
    case C_ALIGNAS_SNAME:
      dup_align.sname = c_sname_dup( &align->sname );
      break;
    case C_ALIGNAS_TYPE:
      dup_align.type_ast = align->type_ast;
      break;
  } // switch

  return dup_align;
}

/**
 * Checks whether two alignments are equal.
 *
 * @param i_align The first alignment.
 * @param j_align The second alignment.
 * @return Returns `true` only if the two alignments are equal.
 */
NODISCARD
static bool c_alignas_equal( c_alignas_t const *i_align,
                             c_alignas_t const *j_align ) {
  assert( i_align != NULL );
  assert( j_align != NULL );

  if ( i_align == j_align )
    return true;
  if ( i_align->kind != j_align->kind )
    return false;

  switch ( i_align->kind ) {
    case C_ALIGNAS_NONE:
      return true;
    case C_ALIGNAS_BYTES:
      return i_align->bytes == j_align->bytes;
    case C_ALIGNAS_SNAME:
      return c_sname_cmp( &i_align->sname, &j_align->sname ) == 0;
    case C_ALIGNAS_TYPE:
      return c_ast_equal( i_align->type_ast, j_align->type_ast );
  } // switch

  UNEXPECTED_INT_VALUE( i_align->kind );
}

#ifndef NDEBUG
/**
 * Checks \a ast for a cycle.
 *
 * @param ast The AST node to start from.
 * @return Returns `true` only if there is a cycle.
 */
NODISCARD
static bool c_ast_has_cycle( c_ast_t const *ast ) {
  assert( ast != NULL );
  for ( c_ast_t const *const start_ast = ast; ast->parent_ast != NULL; ) {
    ast = ast->parent_ast;
    if ( unlikely( ast == start_ast ) )
      return true;                      // LCOV_EXCL_LINE
  } // for
  return false;
}
#endif /* NDEBUG */

/**
 * Duplicates \a src_list.
 *
 * @param src_list The AST list to duplicate; may be NULL.
 * @param dst_list The list to append the duplicated AST nodes onto.
 * @return Returns the duplicated AST list or an empty list only if \a src_list
 * is NULL.
 *
 * @sa c_ast_dup()
 */
NODISCARD
static c_ast_list_t c_ast_list_dup( c_ast_list_t const *src_list,
                                    c_ast_list_t *dst_list ) {
  slist_t dup_list;
  slist_init( &dup_list );

  if ( src_list != NULL ) {
    FOREACH_SLIST_NODE( src_node, src_list ) {
      c_ast_t const *const src_ast = src_node->data;
      c_ast_t *const dup_ast = c_ast_dup( src_ast, dst_list );
      slist_push_back( &dup_list, dup_ast );
    } // for
  }

  return dup_list;
}

/**
 * Checks whether two AST lists are equal _except_ for AST node names.
 *
 * @param i_list The first AST list.
 * @param j_list The second AST list.
 * @return Returns `true` only if the two AST lists are equal _except_ for AST
 * node names.
 *
 * @sa c_ast_equal()
 */
NODISCARD
static inline bool c_ast_list_equal( c_ast_list_t const *i_list,
                                     c_ast_list_t const *j_list ) {
  return slist_equal(
    i_list, j_list, POINTER_CAST( slist_equal_fn_t, &c_ast_equal )
  );
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_cleanup_all( void ) {
  assert( c_ast_count == 0 );
}

c_ast_t* c_ast_dup( c_ast_t const *ast, c_ast_list_t *dst_list ) {
  assert( ast != NULL );

  c_ast_t *const dup_ast =
    c_ast_new( ast->kind, ast->depth, &ast->loc, dst_list );

  dup_ast->align = c_alignas_dup( &ast->align );
  dup_ast->dup_from_ast = ast;
  dup_ast->is_param_pack = ast->is_param_pack;
  dup_ast->sname = c_sname_dup( &ast->sname );
  dup_ast->type = ast->type;

  switch ( ast->kind ) {
    case K_ARRAY:
      dup_ast->array.kind = ast->array.kind;
      switch ( ast->array.kind ) {
        case C_ARRAY_SIZE_INT:
          dup_ast->array.size_int = ast->array.size_int;
          break;
        case C_ARRAY_SIZE_NAME:
          dup_ast->array.size_name = check_strdup( ast->array.size_name );
          break;
        case C_ARRAY_SIZE_NONE:
        case C_ARRAY_SIZE_VLA:
          // nothing to do
          break;
      } // switch
      break;

    case K_BUILTIN:
      dup_ast->builtin.BitInt.width = ast->builtin.BitInt.width;
      FALLTHROUGH;
    case K_TYPEDEF:
      dup_ast->builtin.bit_width = ast->builtin.bit_width;
      // for_ast duplicated by referrer code below
      break;

    case K_CAST:
      dup_ast->cast.kind = ast->cast.kind;
      break;

    case K_ENUM:
      dup_ast->enum_.bit_width = ast->enum_.bit_width;
      // of_ast duplicated by referrer code below
      FALLTHROUGH;
    case K_CLASS_STRUCT_UNION:
    case K_CONCEPT:
    case K_NAME:
    case K_POINTER_TO_MEMBER:
      dup_ast->name.sname = c_sname_dup( &ast->name.sname );
      break;

    case K_OPERATOR:
      dup_ast->oper.operator = ast->oper.operator;
      FALLTHROUGH;
    case K_FUNCTION:
      dup_ast->func.member = ast->func.member;
      FALLTHROUGH;
    case K_APPLE_BLOCK:
      // ret_ast duplicated by referrer code below
    case K_CONSTRUCTOR:
    case K_USER_DEFINED_LIT:
      dup_ast->func.param_ast_list =
        c_ast_list_dup( &ast->func.param_ast_list, dst_list );
      break;

    case K_LAMBDA:
      // ret_ast duplicated by referrer code below
      dup_ast->func.param_ast_list =
        c_ast_list_dup( &ast->func.param_ast_list, dst_list );
      dup_ast->lambda.capture_ast_list =
        c_ast_list_dup( &ast->lambda.capture_ast_list, dst_list );
      break;

    case K_STRUCTURED_BINDING:
      FOREACH_SLIST_NODE( sname_node, &ast->struct_bind.sname_list ) {
        c_sname_t dup_sname = c_sname_dup( sname_node->data );
        slist_push_back( &dup_ast->struct_bind.sname_list, &dup_sname );
      } // for
      break;

    case K_CAPTURE:
    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_USER_DEFINED_CONV:
      // of_ast duplicated by referrer code below
    case K_DESTRUCTOR:
    case K_PLACEHOLDER:
    case K_VARIADIC:
      // nothing to do
      break;
  } // switch

  if ( c_ast_is_referrer( ast ) ) {
    c_ast_t *const child_ast = ast->parent.of_ast;
    if ( child_ast != NULL ) {
      if ( c_ast_is_parent( ast ) ) {
        c_ast_set_parent( c_ast_dup( child_ast, dst_list ), dup_ast );
      } else {
        //
        // A non-parent referrer (e.g., K_TYPEDEF) merely refers to another
        // AST, but is _not_ the "parent" of it and the "child" (referred to)
        // AST's parent must be NULL.
        //
        dup_ast->parent.of_ast = child_ast;
        assert( child_ast->parent_ast == NULL );
      }
    }
  }

  return dup_ast;
}

bool c_ast_equal( c_ast_t const *i_ast, c_ast_t const *j_ast ) {
  if ( i_ast == j_ast )
    return true;
  if ( i_ast == NULL || j_ast == NULL )
    return false;
  if ( i_ast->kind != j_ast->kind )
    return false;
  if ( !c_alignas_equal( &i_ast->align, &j_ast->align ) )
    return false;
  if ( !c_type_equiv( &i_ast->type, &j_ast->type ) )
    return false;
  if ( i_ast->is_param_pack != j_ast->is_param_pack )
    return false;

  switch ( i_ast->kind ) {
    case K_ARRAY:;
      c_array_ast_t const *const ai_ast = &i_ast->array;
      c_array_ast_t const *const aj_ast = &j_ast->array;
      if ( ai_ast->kind != aj_ast->kind )
        return false;
      switch ( ai_ast->kind ) {
        case C_ARRAY_SIZE_INT:
          if ( ai_ast->size_int != aj_ast->size_int )
            return false;
          break;
        case C_ARRAY_SIZE_NAME:
          if ( strcmp( ai_ast->size_name, aj_ast->size_name ) != 0 )
            return false;
          break;
        case C_ARRAY_SIZE_NONE:
        case C_ARRAY_SIZE_VLA:
          break;
      } // switch
      break;

    case K_BUILTIN:
      if ( i_ast->builtin.BitInt.width != j_ast->builtin.BitInt.width )
        return false;
      FALLTHROUGH;
    case K_TYPEDEF:
      if ( i_ast->builtin.bit_width != j_ast->builtin.bit_width )
        return false;
      // for_ast compared by referrer code below
      break;

    case K_CAST:
      if ( i_ast->cast.kind != j_ast->cast.kind )
        return false;
      break;

    case K_OPERATOR:
      if ( i_ast->oper.operator != j_ast->oper.operator )
        return false;
      FALLTHROUGH;
    case K_FUNCTION:
      if ( i_ast->func.member != j_ast->func.member )
        return false;
      FALLTHROUGH;
    case K_APPLE_BLOCK:
      // ret_ast checked by referrer code below
    case K_CONSTRUCTOR:
    case K_USER_DEFINED_LIT:
      if ( !c_ast_list_equal( &i_ast->func.param_ast_list,
                              &j_ast->func.param_ast_list ) ) {
        return false;
      }
      break;

    case K_LAMBDA:
      if ( !c_ast_list_equal( &i_ast->lambda.param_ast_list,
                              &j_ast->lambda.param_ast_list ) ||
           !c_ast_list_equal( &i_ast->lambda.capture_ast_list,
                              &j_ast->lambda.capture_ast_list ) ) {
        return false;
      }
      break;

    case K_ENUM:
      // of_ast checked by referrer code below
      if ( i_ast->enum_.bit_width != j_ast->enum_.bit_width )
        return false;
      FALLTHROUGH;
    case K_CLASS_STRUCT_UNION:
    case K_CONCEPT:
    case K_NAME:
    case K_POINTER_TO_MEMBER:
      if ( c_sname_cmp( &i_ast->name.sname, &j_ast->name.sname ) != 0 )
        return false;
      break;

    case K_STRUCTURED_BINDING:
      if ( !slist_equal( &i_ast->struct_bind.sname_list,
                         &j_ast->struct_bind.sname_list,
                         POINTER_CAST( slist_equal_fn_t, &c_sname_equal ) ) ) {
        return false;
      }
      break;

    case K_CAPTURE:
    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_USER_DEFINED_CONV:
      // checked by referrer code below
    case K_DESTRUCTOR:
    case K_PLACEHOLDER:
    case K_VARIADIC:
      // nothing to do
      break;                            // LCOV_EXCL_LINE
  } // switch

  if ( !c_ast_is_referrer( i_ast ) ) {
    assert( !c_ast_is_referrer( j_ast ) );
    return true;
  }
  assert( c_ast_is_referrer( j_ast ) );

  return c_ast_equal( i_ast->parent.of_ast, j_ast->parent.of_ast );
}

void c_ast_free( c_ast_t *ast ) {
  if ( ast != NULL ) {
    assert( c_ast_count-- > 0 );        // side-effect is OK here

    c_alignas_cleanup( &ast->align );
    c_sname_cleanup( &ast->sname );
    switch ( ast->kind ) {
      case K_ARRAY:
        if ( ast->array.kind == C_ARRAY_SIZE_NAME )
          FREE( ast->array.size_name );
        break;
      case K_LAMBDA:
        // Do not pass &c_ast_free as the second argument since all ASTs are
        // free'd via the free( ast ) below. Just free the list nodes.
        slist_cleanup( &ast->lambda.capture_ast_list, /*free_fn=*/NULL );
        FALLTHROUGH;
      case K_APPLE_BLOCK:
      case K_CONSTRUCTOR:
      case K_FUNCTION:
      case K_OPERATOR:
      case K_USER_DEFINED_LIT:
        // Do not pass &c_ast_free as the second argument since all ASTs are
        // free'd via the free( ast ) below. Just free the list nodes.
        slist_cleanup( &ast->func.param_ast_list, /*free_fn=*/NULL );
        break;
      case K_CLASS_STRUCT_UNION:
      case K_CONCEPT:
      case K_ENUM:
      case K_NAME:
      case K_POINTER_TO_MEMBER:
        c_sname_cleanup( &ast->name.sname );
        break;
      case K_STRUCTURED_BINDING:
        c_sname_list_cleanup( &ast->struct_bind.sname_list );
        break;

      case K_BUILTIN:
      case K_CAPTURE:
      case K_CAST:
      case K_DESTRUCTOR:
      case K_PLACEHOLDER:
      case K_POINTER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
      case K_TYPEDEF:
      case K_USER_DEFINED_CONV:
      case K_VARIADIC:
        // nothing to do
        break;
    } // switch

    free( ast );
  }
}

void c_ast_list_set_param_of( c_ast_list_t *param_ast_list,
                              c_ast_t *func_ast ) {
  assert( param_ast_list != NULL );
  assert( func_ast != NULL );
  assert( is_1_bit_only_in_set( func_ast->kind, K_ANY_FUNCTION_LIKE ) );

  FOREACH_SLIST_NODE( param_node, param_ast_list ) {
    c_ast_t *const param_ast = param_node->data;
    assert( param_ast->param_of_ast == NULL );
    param_ast->param_of_ast = func_ast;
  } // for
}

c_ast_t* c_ast_new( c_ast_kind_t kind, unsigned depth, c_loc_t const *loc,
                    c_ast_list_t *dst_list ) {
  assert( is_1_bit( kind ) );
  assert( loc != NULL );
  assert( dst_list != NULL );

  static c_ast_id_t next_id;

  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  *ast = (c_ast_t){
    .depth = depth,
    .kind = kind,
    .loc = *loc,
    .type = T_NONE,
    .unique_id = ++next_id
  };

#ifndef NDEBUG
  ++c_ast_count;
#endif /* NDEBUG */
  slist_push_back( dst_list, ast );
  return ast;
}

void c_ast_set_parameter_pack( c_ast_t *ast ) {
  assert( ast != NULL );
  ast->is_param_pack = true;
  if ( ast->dup_from_ast != NULL ) {
    // I don't like this CONST_CAST(), but I can't think of a way around it.
    c_ast_set_parameter_pack( CONST_CAST( c_ast_t*, ast->dup_from_ast ) );
  }
}

void c_ast_set_parent( c_ast_t *child_ast, c_ast_t *parent_ast ) {
  if ( parent_ast != NULL ) {
    assert( c_ast_is_parent( parent_ast ) );
    parent_ast->parent.of_ast = child_ast;
  }
  if ( child_ast == NULL )
    return;
  child_ast->parent_ast = parent_ast;

  if ( parent_ast == NULL )
    return;
  assert( !c_ast_has_cycle( child_ast ) );

  if ( child_ast->is_param_pack &&
       (parent_ast->kind & K_ANY_FUNCTION_LIKE) == 0 ) {
    //
    // Except for function-like return type ASTs, the root AST node of a tree
    // must always be the one that is a parameter pack.  For example, given:
    //
    //      auto &...x
    //
    // the AST should be:
    //
    //      {
    //        sname: { string: "x", scopes: "none" },
    //        is_param_pack: true,
    //        kind: { value: 0x1000, string: "reference" },
    //        ...
    //        ptr_ref: {
    //          to_ast: {
    //            ...
    //            is_param_pack: false,
    //            kind: { value: 0x2, string: "built-in type" },
    //            ...
    //            type: { btid: 0x0000000000000021, string: "auto" },
    //            ...
    //          }
    //        }
    //      }
    //
    // where it's a parameter pack of a reference, not a reference to a
    // parameter pack.
    //
    child_ast->is_param_pack = false;
    parent_ast->is_param_pack = true;
  }
}

c_ast_t const* c_ast_visit( c_ast_t const *ast, c_ast_visit_dir_t dir,
                            c_ast_visit_fn_t visit_fn, user_data_t user_data ) {
  assert( visit_fn != NULL );

  switch ( dir ) {
    case C_VISIT_DOWN:
      while ( ast != NULL && !(*visit_fn)( ast, user_data ) )
        ast = c_ast_is_parent( ast ) ? ast->parent.of_ast : NULL;
      break;
    case C_VISIT_UP:
      while ( ast != NULL && !(*visit_fn)( ast, user_data ) )
        ast = ast->parent_ast;
      break;
  } // switch
  return ast;
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

extern inline bool c_ast_is_orphan( c_ast_t const* );
extern inline bool c_ast_is_parent( c_ast_t const* );
extern inline bool c_ast_is_referrer( c_ast_t const* );
extern inline c_param_t const* c_ast_params( c_ast_t const* );
extern inline c_ast_t const* c_capture_ast( c_capture_t const* );
extern inline c_ast_t const* c_param_ast( c_param_t const* );

/* vim:set et sw=2 ts=2: */
