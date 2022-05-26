/*
**      cdecl -- C gibberish translator
**      src/c_ast.c
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

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// local variable definitions
static size_t c_ast_count;              ///< ASTs allocated but not yet freed.

////////// local functions ////////////////////////////////////////////////////

/**
 * Checks whether two alignments are equal.
 *
 * @param i_align The first alignment.
 * @param j_align The second alignment.
 * @return Returns `true` only if the two alignments are equal.
 */
PJL_WARN_UNUSED_RESULT
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
    case C_ALIGNAS_EXPR:
      return i_align->as.expr == j_align->as.expr;
    case C_ALIGNAS_TYPE:
      return c_ast_equal( i_align->as.type_ast, j_align->as.type_ast );
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
PJL_WARN_UNUSED_RESULT
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
  if ( unlikely( c_ast_count > 0 ) )
    INTERNAL_ERR( "%zu: c_ast objects > 0 in c_ast_cleanup()\n", c_ast_count );
}

c_ast_t* c_ast_dup( c_ast_t const *ast, c_ast_list_t *ast_list ) {
  if ( ast == NULL )
    return NULL;                        // LCOV_EXCL_LINE
  c_ast_t *const dup_ast =
    c_ast_new( ast->kind, ast->depth, &ast->loc, ast_list );

  dup_ast->align = ast->align;
  dup_ast->cast_kind = ast->cast_kind;
  dup_ast->sname = c_sname_dup( &ast->sname );
  dup_ast->type = ast->type;

  switch ( ast->kind ) {
    case K_ARRAY:
      dup_ast->as.array.size = ast->as.array.size;
      dup_ast->as.array.stids = ast->as.array.stids;
      break;

    case K_TYPEDEF:
      // for_ast duplicated by referrer code below
    case K_BUILTIN:
      dup_ast->as.builtin.bit_width = ast->as.builtin.bit_width;
      break;

    case K_CLASS_STRUCT_UNION:
    case K_ENUM:
    case K_POINTER_TO_MEMBER:
      dup_ast->as.csu.csu_sname = c_sname_dup( &ast->as.csu.csu_sname );
      break;

    case K_OPERATOR:
      dup_ast->as.oper.oper_id = ast->as.oper.oper_id;
      PJL_FALLTHROUGH;
    case K_FUNCTION:
      dup_ast->as.func.flags = ast->as.func.flags;
      PJL_FALLTHROUGH;
    case K_APPLE_BLOCK:
      // ret_ast duplicated by referrer code below
    case K_CONSTRUCTOR:
    case K_USER_DEF_LITERAL:
      FOREACH_AST_FUNC_PARAM( param, ast ) {
        c_ast_t const *const param_ast = c_param_ast( param );
        c_ast_t *const dup_param_ast = c_ast_dup( param_ast, ast_list );
        slist_push_back( &dup_ast->as.func.param_ast_list, dup_param_ast );
      } // for
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_USER_DEF_CONVERSION:
      // of_ast duplicated by referrer code below
    case K_DESTRUCTOR:
    case K_NAME:
    case K_PLACEHOLDER:
    case K_VARIADIC:
      // nothing to do
      break;
  } // switch

  if ( c_ast_is_referrer( ast ) ) {
    c_ast_t *const child_ast = ast->as.parent.of_ast;
    if ( child_ast != NULL )
      c_ast_set_parent( c_ast_dup( child_ast, ast_list ), dup_ast );
  }

  return dup_ast;
}

bool c_ast_equal( c_ast_t const *i_ast, c_ast_t const *j_ast ) {
  if ( i_ast == j_ast )
    return true;
  if ( (i_ast != NULL && j_ast == NULL) || (i_ast == NULL && j_ast != NULL) )
    return false;

  if ( i_ast->kind != j_ast->kind )
    return false;
  if ( i_ast->cast_kind != j_ast->cast_kind )
    return false;
  if ( !c_alignas_equal( &i_ast->align, &j_ast->align ) )
    return false;
  if ( !c_type_equal( &i_ast->type, &j_ast->type ) )
    return false;

  switch ( i_ast->kind ) {
    case K_ARRAY: {
      c_array_ast_t const *const ai_ast = &i_ast->as.array;
      c_array_ast_t const *const aj_ast = &j_ast->as.array;
      if ( ai_ast->size != aj_ast->size )
        return false;
      if ( ai_ast->stids != aj_ast->stids )
        return false;
      break;
    }

    case K_TYPEDEF:
      // for_ast compared by referrer code below
    case K_BUILTIN:
      if ( i_ast->as.builtin.bit_width != j_ast->as.builtin.bit_width )
        return false;
      break;

    case K_OPERATOR:
      if ( i_ast->as.oper.oper_id != j_ast->as.oper.oper_id )
        return false;
      PJL_FALLTHROUGH;
    case K_FUNCTION:
      if ( i_ast->as.func.flags != j_ast->as.func.flags )
        return false;
      PJL_FALLTHROUGH;
    case K_APPLE_BLOCK:
      // ret_ast checked by referrer code below
    case K_CONSTRUCTOR:
    case K_USER_DEF_LITERAL: {
      c_param_t const *i_param = c_ast_params( i_ast );
      c_param_t const *j_param = c_ast_params( j_ast );
      for ( ; i_param != NULL && j_param != NULL;
              i_param = i_param->next, j_param = j_param->next ) {
        if ( !c_ast_equal( c_param_ast( i_param ), c_param_ast( j_param ) ) )
          return false;
      } // for
      if ( i_param != NULL || j_param != NULL )
        return false;
      break;
    }

    case K_CLASS_STRUCT_UNION:
    case K_ENUM:
    case K_POINTER_TO_MEMBER: {
      c_csu_ast_t const *const csui_ast = &i_ast->as.csu;
      c_csu_ast_t const *const csuj_ast = &j_ast->as.csu;
      if ( c_sname_cmp( &csui_ast->csu_sname, &csuj_ast->csu_sname ) != 0 )
        return false;
      break;
    }

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_USER_DEF_CONVERSION:
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

  return c_ast_equal( i_ast->as.parent.of_ast, j_ast->as.parent.of_ast );
}

void c_ast_free( c_ast_t *ast ) {
  if ( ast != NULL ) {
    assert( c_ast_count > 0 );
    --c_ast_count;

    c_sname_cleanup( &ast->sname );
    switch ( ast->kind ) {
      case K_APPLE_BLOCK:
      case K_CONSTRUCTOR:
      case K_FUNCTION:
      case K_OPERATOR:
      case K_USER_DEF_LITERAL:
        c_ast_list_cleanup( &ast->as.func.param_ast_list );
        break;
      case K_CLASS_STRUCT_UNION:
      case K_ENUM:
      case K_POINTER_TO_MEMBER:
        c_sname_cleanup( &ast->as.csu.csu_sname );
        break;
      case K_ARRAY:
      case K_BUILTIN:
      case K_DESTRUCTOR:
      case K_NAME:
      case K_PLACEHOLDER:
      case K_POINTER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
      case K_TYPEDEF:
      case K_USER_DEF_CONVERSION:
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

c_ast_t* c_ast_new( c_ast_kind_t kind, unsigned depth, c_loc_t const *loc,
                    c_ast_list_t *ast_list ) {
  assert( is_1_bit( kind ) );
  assert( loc != NULL );
  assert( ast_list != NULL );
  static c_ast_id_t next_id;

  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  MEM_ZERO( ast );

  ast->depth = depth;
  ast->kind = kind;
  ast->loc = *loc;
  ast->type = T_NONE;
  ast->unique_id = ++next_id;

  switch ( kind ) {
    case K_ARRAY:
      ast->as.array.stids = TS_NONE;
      break;
    case K_APPLE_BLOCK:
    case K_BUILTIN:
    case K_CLASS_STRUCT_UNION:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_ENUM:
    case K_FUNCTION:
    case K_NAME:
    case K_OPERATOR:
    case K_PLACEHOLDER:
    case K_POINTER:
    case K_POINTER_TO_MEMBER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_TYPEDEF:
    case K_USER_DEF_CONVERSION:
    case K_USER_DEF_LITERAL:
    case K_VARIADIC:
      // nothing to do
      break;
  } // switch

  ++c_ast_count;
  slist_push_back( ast_list, ast );
  return ast;
}

void c_ast_set_parent( c_ast_t *child_ast, c_ast_t *parent_ast ) {
  assert( child_ast != NULL );
  assert( parent_ast != NULL );
  assert( c_ast_is_referrer( parent_ast ) );

  child_ast->parent_ast = parent_ast;
  parent_ast->as.parent.of_ast = child_ast;

  assert( !c_ast_has_cycle( child_ast ) );
}

c_ast_t* c_ast_visit( c_ast_t *ast, c_visit_dir_t dir,
                      c_ast_visit_fn_t visit_fn, c_ast_visit_data_t avd ) {
  assert( visit_fn != NULL );

  switch ( dir ) {
    case C_VISIT_DOWN:
      while ( ast != NULL && !(*visit_fn)( ast, avd ) )
        ast = c_ast_is_parent( ast ) ? ast->as.parent.of_ast : NULL;
      break;
    case C_VISIT_UP:
      while ( ast != NULL && !(*visit_fn)( ast, avd ) )
        ast = ast->parent_ast;
      break;
  } // switch
  return ast;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
