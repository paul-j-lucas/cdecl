/*
**      cdecl -- C gibberish translator
**      src/c_ast.c
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
 * Defines functions for traversing and manipulating an AST.
 */

// local
#include "pjl_config.h"                 /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_AST_H_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_ast.h"
#include "cdecl.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>                     /* for offsetof() */

/// @endcond

static_assert(
  offsetof( c_builtin_ast_t, bit_width ) ==
  offsetof( c_bit_field_ast_t, bit_width ),
  "offsetof bit_width in c_builtin_ast_t & c_bit_field_ast_t must equal"
);

static_assert(
  offsetof( c_enum_ast_t, bit_width ) ==
  offsetof( c_bit_field_ast_t, bit_width ),
  "offsetof bit_width in enum_ast_t & c_bit_field_ast_t must equal"
);

static_assert(
  offsetof( c_typedef_ast_t, bit_width ) ==
  offsetof( c_bit_field_ast_t, bit_width ),
  "offsetof bit_width in c_typedef_ast_t & c_bit_field_ast_t must equal"
);

static_assert(
  offsetof( c_apple_block_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list ),
  "offsetof param_ast_list in c_apple_block_ast_t & c_function_ast_t must equal"
);

static_assert(
  offsetof( c_constructor_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list ),
  "offsetof param_ast_list in c_constructor_ast_t & c_function_ast_t must equal"
);

static_assert(
  offsetof( c_lambda_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list ),
  "offsetof param_ast_list in c_lambda_ast_t & c_function_ast_t must equal"
);

static_assert(
  offsetof( c_operator_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list ),
  "offsetof param_ast_list in c_operator_ast_t & c_function_ast_t must equal"
);

static_assert(
  offsetof( c_udef_lit_ast_t, param_ast_list ) ==
  offsetof( c_function_ast_t, param_ast_list ),
  "offsetof param_ast_list in c_udef_lit_ast_t & c_function_ast_t must equal"
);

static_assert(
  offsetof( c_csu_ast_t, csu_sname ) ==
  offsetof( c_enum_ast_t, enum_sname ),
  "offsetof csu_sname != offsetof enum_sname"
);

static_assert(
  offsetof( c_enum_ast_t, enum_sname ) ==
  offsetof( c_ptr_mbr_ast_t, class_sname ),
  "offsetof enum_sname != offsetof class_sname"
);

static_assert(
  offsetof( c_operator_ast_t, member ) ==
  offsetof( c_function_ast_t, member ),
  "offsetof member in c_operator_ast_t & c_function_ast_t must equal"
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

////////// extern functions ///////////////////////////////////////////////////

void c_ast_cleanup( void ) {
  assert( c_ast_count == 0 );
}

c_ast_t* c_ast_dup( c_ast_t const *ast, c_ast_list_t *node_list ) {
  if ( ast == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  c_ast_t *const dup_ast =
    c_ast_new( ast->kind, ast->depth, &ast->loc, node_list );

  dup_ast->align = ast->align;
#ifdef ENABLE_CDECL_DEBUG
  dup_ast->dup_from_id = ast->unique_id;
#endif /* ENABLE_CDECL_DEBUG */
  dup_ast->sname = c_sname_dup( &ast->sname );
  dup_ast->type = ast->type;

  switch ( ast->kind ) {
    case K_ARRAY:
      dup_ast->array.kind = ast->array.kind;
      switch ( ast->array.kind ) {
        case C_ARRAY_INT_SIZE:
          dup_ast->array.size_int = ast->array.size_int;
          break;
        case C_ARRAY_NAMED_SIZE:
          dup_ast->array.size_name = check_strdup( ast->array.size_name );
          break;
        case C_ARRAY_EMPTY_SIZE:
        case C_ARRAY_VLA_STAR:
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
    case K_POINTER_TO_MEMBER:
      dup_ast->csu.csu_sname = c_sname_dup( &ast->csu.csu_sname );
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
    case K_UDEF_LIT:
      dup_ast->func.param_ast_list =
        c_ast_list_dup( &ast->func.param_ast_list, node_list );
      break;

    case K_LAMBDA:
      // ret_ast duplicated by referrer code below
      dup_ast->func.param_ast_list =
        c_ast_list_dup( &ast->func.param_ast_list, node_list );
      dup_ast->lambda.capture_ast_list =
        c_ast_list_dup( &ast->lambda.capture_ast_list, node_list );
      break;

    case K_CAPTURE:
    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_UDEF_CONV:
      // of_ast duplicated by referrer code below
    case K_DESTRUCTOR:
    case K_NAME:
    case K_PLACEHOLDER:
    case K_VARIADIC:
      // nothing to do
      break;
  } // switch

  if ( c_ast_is_referrer( ast ) ) {
    c_ast_t *const child_ast = ast->parent.of_ast;
    c_ast_set_parent( c_ast_dup( child_ast, node_list ), dup_ast );
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

  switch ( i_ast->kind ) {
    case K_ARRAY:
      NO_OP;
      c_array_ast_t const *const ai_ast = &i_ast->array;
      c_array_ast_t const *const aj_ast = &j_ast->array;
      if ( ai_ast->kind != aj_ast->kind )
        return false;
      switch ( ai_ast->kind ) {
        case C_ARRAY_INT_SIZE:
          if ( ai_ast->size_int != aj_ast->size_int )
            return false;
          break;
        case C_ARRAY_NAMED_SIZE:
          if ( strcmp( ai_ast->size_name, aj_ast->size_name ) != 0 )
            return false;
          break;
        case C_ARRAY_EMPTY_SIZE:
        case C_ARRAY_VLA_STAR:
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
    case K_UDEF_LIT:
      if ( !c_ast_list_equal( &i_ast->func.param_ast_list,
                              &j_ast->func.param_ast_list ) ) {
        return false;
      }
      break;

    case K_LAMBDA:
      if ( !c_ast_list_equal( &i_ast->func.param_ast_list,
                              &j_ast->func.param_ast_list ) ||
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
    case K_POINTER_TO_MEMBER:
      if ( c_sname_cmp( &i_ast->csu.csu_sname, &j_ast->csu.csu_sname ) != 0 )
        return false;
      break;

    case K_CAPTURE:
    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_UDEF_CONV:
      // checked by referrer code below
    case K_NAME:                        // names don't matter
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

    c_sname_cleanup( &ast->sname );
    switch ( ast->kind ) {
      case K_ARRAY:
        if ( ast->array.kind == C_ARRAY_NAMED_SIZE )
          FREE( ast->array.size_name );
        break;
      case K_LAMBDA:
        c_ast_list_cleanup( &ast->lambda.capture_ast_list );
        FALLTHROUGH;
      case K_APPLE_BLOCK:
      case K_CONSTRUCTOR:
      case K_FUNCTION:
      case K_OPERATOR:
      case K_UDEF_LIT:
        c_ast_list_cleanup( &ast->func.param_ast_list );
        break;
      case K_CLASS_STRUCT_UNION:
      case K_ENUM:
      case K_POINTER_TO_MEMBER:
        c_sname_cleanup( &ast->csu.csu_sname );
        break;
      case K_BUILTIN:
      case K_CAPTURE:
      case K_CAST:
      case K_DESTRUCTOR:
      case K_NAME:
      case K_PLACEHOLDER:
      case K_POINTER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
      case K_TYPEDEF:
      case K_UDEF_CONV:
      case K_VARIADIC:
        // nothing to do
        break;
    } // switch

    FREE( ast );
  }
}

void c_ast_list_cleanup( c_ast_list_t *list ) {
  // Do not pass &c_ast_free as the second argument since all ASTs are free'd
  // independently. Just free the list nodes.
  slist_cleanup( list, /*free_fn=*/NULL );
}

c_ast_list_t c_ast_list_dup( c_ast_list_t const *src_list,
                             c_ast_list_t *node_list ) {
  slist_t dup_list;
  slist_init( &dup_list );

  if ( src_list != NULL ) {
    FOREACH_SLIST_NODE( src_node, src_list ) {
      c_ast_t const *const src_ast = src_node->data;
      c_ast_t *const dup_ast = c_ast_dup( src_ast, node_list );
      slist_push_back( &dup_list, dup_ast );
    } // for
  }

  return dup_list;
}

bool c_ast_list_equal( c_ast_list_t const *i_list,
                       c_ast_list_t const *j_list ) {
  assert( i_list != NULL );
  assert( j_list != NULL );

  if ( slist_len( i_list ) != slist_len( j_list ) )
    return false;

  slist_node_t const *i_node = i_list->head;
  slist_node_t const *j_node = j_list->head;

  for ( ; i_node != NULL && j_node != NULL;
          i_node = i_node->next, j_node = j_node->next ) {
    c_ast_t const *const i_ast = i_node->data;
    c_ast_t const *const j_ast = j_node->data;
    if ( !c_ast_equal( i_ast, j_ast ) )
      return false;
  } // for

  return i_node == NULL && j_node == NULL;
}

void c_ast_list_set_param_of( c_ast_list_t *param_ast_list,
                              c_ast_t *func_ast ) {
  assert( param_ast_list != NULL );
  assert( func_ast != NULL );
  assert( (func_ast->kind & K_ANY_FUNCTION_LIKE) != 0 );

  FOREACH_SLIST_NODE( param_node, param_ast_list ) {
    c_ast_t *const param_ast = param_node->data;
    assert( param_ast->param_of_ast == NULL );
    param_ast->param_of_ast = func_ast;
  } // for
}

c_ast_t* c_ast_new( c_ast_kind_t kind, unsigned depth, c_loc_t const *loc,
                    c_ast_list_t *node_list ) {
  assert( is_1_bit( kind ) );
  assert( loc != NULL );
  assert( node_list != NULL );

  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  MEM_ZERO( ast );

  ast->depth = depth;
  ast->kind = kind;
  ast->loc = *loc;
  ast->type = T_NONE;

#ifdef ENABLE_CDECL_DEBUG
  static c_ast_id_t next_id;
  ast->unique_id = ++next_id;
#endif /* ENABLE_CDECL_DEBUG */

#ifndef NDEBUG
  ++c_ast_count;
#endif /* NDEBUG */
  slist_push_back( node_list, ast );
  return ast;
}

void c_ast_set_parent( c_ast_t *child_ast, c_ast_t *parent_ast ) {
  if ( parent_ast != NULL ) {
    assert( c_ast_is_referrer( parent_ast ) );
    parent_ast->parent.of_ast = child_ast;
  }
  if ( child_ast != NULL ) {
    child_ast->parent_ast = parent_ast;
    assert( !c_ast_has_cycle( child_ast ) );
  }
}

c_ast_t* c_ast_visit( c_ast_t *ast, c_ast_visit_dir_t dir,
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

/* vim:set et sw=2 ts=2: */
