/*
**      cdecl -- C gibberish translator
**      src/c_ast.c
**
**      Copyright (C) 2017  Paul J. Lucas, et al.
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
#include "config.h"                     /* must go first */
/// @cond DOXYGEN_IGNORE
#define CDECL_AST_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_ast.h"
#include "c_typedef.h"
#include "literals.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// system
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// local variable definitions
static unsigned   c_ast_count;          ///< ASTs allocated but not yet freed.

////////// local functions ////////////////////////////////////////////////////

#ifndef NDEBUG
/**
 * Checks \a ast for a cycle.
 *
 * @param ast The `c_ast` node to begin at.
 * @return Returns `true` only if there is a cycle.
 */
static bool c_ast_has_cycle( c_ast_t const *ast ) {
  assert( ast != NULL );
  for ( c_ast_t const *const start_ast = ast; ast->parent != NULL; ) {
    ast = ast->parent;
    if ( unlikely( ast == start_ast ) )
      return true;
  } // for
  return false;
}
#endif /* NDEBUG */

////////// extern functions ///////////////////////////////////////////////////

void c_ast_cleanup( void ) {
  if ( unlikely( c_ast_count > 0 ) )
    INTERNAL_ERR( "number of c_ast objects (%u) > 0\n", c_ast_count );
}

bool c_ast_equiv( c_ast_t const *ast_i, c_ast_t const *ast_j ) {
  if ( ast_i == ast_j )
    return true;
  if ( (ast_i != NULL && ast_j == NULL) || (ast_i == NULL && ast_j != NULL) )
    return false;

  //
  // If only one of the ASTs is a typedef, compare the other AST to the
  // typedef's AST.
  //
  if ( ast_i->kind == K_TYPEDEF && ast_j->kind != K_TYPEDEF )
    return c_ast_equiv( ast_i->as.c_typedef->ast, ast_j );
  if ( ast_i->kind != K_TYPEDEF && ast_j->kind == K_TYPEDEF )
    return c_ast_equiv( ast_i, ast_j->as.c_typedef->ast );

  if ( ast_i->kind != ast_j->kind )
    return false;
  if ( ast_i->type != ast_j->type )
    return false;

  switch ( ast_i->kind ) {
    case K_ARRAY: {
      c_array_t const *const a_i = &ast_i->as.array;
      c_array_t const *const a_j = &ast_j->as.array;
      if ( a_i->size != a_j->size )
        return false;
      if ( a_i->type != a_j->type )
        return false;
      break;
    }

    case K_BLOCK:
    case K_FUNCTION: {
      c_ast_arg_t const *arg_i = c_ast_args( ast_i );
      c_ast_arg_t const *arg_j = c_ast_args( ast_j );
      for ( ; arg_i && arg_j; arg_i = arg_i->next, arg_j = arg_j->next ) {
        if ( !c_ast_equiv( C_AST_DATA( arg_i ), C_AST_DATA( arg_j ) ) )
          return false;
      } // for
      if ( arg_i != NULL || arg_j != NULL )
        return false;
      break;
    }

    case K_ENUM_CLASS_STRUCT_UNION: {
      c_ecsu_t const *const e_i = &ast_i->as.ecsu;
      c_ecsu_t const *const e_j = &ast_j->as.ecsu;
      if ( strcmp( e_i->ecsu_name, e_j->ecsu_name ) != 0 )
        return false;
      break;
    }

    case K_NAME:
      // names don't matter
      break;

    case K_POINTER_TO_MEMBER: {
      c_ptr_mbr_t const *const p_i = &ast_i->as.ptr_mbr;
      c_ptr_mbr_t const *const p_j = &ast_j->as.ptr_mbr;
      if ( strcmp( p_i->class_name, p_j->class_name ) != 0 )
        return false;
      break;
    }

    case K_TYPEDEF: {
      c_typedef_t const *const t_i = ast_i->as.c_typedef;
      c_typedef_t const *const t_j = ast_j->as.c_typedef;
      if ( !c_ast_equiv( t_i->ast, t_j->ast ) )
        return false;
      break;
    }

    case K_NONE:
    case K_BUILTIN:
    case K_PLACEHOLDER:
    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_VARIADIC:
      // nothing to do
      break;
  } // switch

  if ( !c_ast_is_parent( ast_i ) ) {
    assert( !c_ast_is_parent( ast_j ) );
    return true;
  }
  assert( c_ast_is_parent( ast_j ) );
  return c_ast_equiv( ast_i->as.parent.of_ast, ast_j->as.parent.of_ast );
}

void c_ast_free( c_ast_t *ast ) {
  if ( ast != NULL ) {
    assert( c_ast_count > 0 );
    --c_ast_count;

    FREE( ast->name );
    switch ( ast->kind ) {
      case K_ENUM_CLASS_STRUCT_UNION:
        FREE( ast->as.ecsu.ecsu_name );
        break;
      case K_POINTER_TO_MEMBER:
        FREE( ast->as.ptr_mbr.class_name );
        break;
      case K_TYPEDEF:
        // Do not free ast->as.c_typedef here since it's global data: it will
        // be freed eventually via c_typedef_cleanup().
        break;
      default:
        /* suppress warning */;
    } // switch

    FREE( ast );
  }
}

c_ast_t* c_ast_new( c_kind_t kind, c_ast_depth_t depth, c_loc_t const *loc ) {
  assert( loc != NULL );
  static c_ast_id_t next_id;

  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  STRUCT_ZERO( ast );

  ast->depth = depth;
  ast->id = ++next_id;
  ast->kind = kind;
  ast->loc = *loc;

  ++c_ast_count;
  return ast;
}

c_ast_t* c_ast_root( c_ast_t *ast ) {
  assert( ast != NULL );
  while ( ast->parent != NULL )
    ast = ast->parent;
  return ast;
}

void c_ast_set_parent( c_ast_t *child, c_ast_t *parent ) {
  assert( child != NULL );
  assert( parent != NULL );
  assert( c_ast_is_parent( parent ) );

  child->parent = parent;
  parent->as.parent.of_ast = child;

  assert( !c_ast_has_cycle( child ) );
}

/// @cond DOXYGEN_IGNORE

c_ast_t* c_ast_visit_down( c_ast_t *ast, c_ast_visitor_t visitor, void *data ) {
  if ( ast == NULL )
    return NULL;
  if ( visitor( ast, data ) )
    return ast;
  if ( !c_ast_is_parent( ast ) )
    return NULL;
  return c_ast_visit_down( ast->as.parent.of_ast, visitor, data );
}

c_ast_t* c_ast_visit_up( c_ast_t *ast, c_ast_visitor_t visitor, void *data ) {
  if ( ast == NULL )
    return NULL;
  if ( visitor( ast, data ) )
    return ast;
  if ( ast->parent == NULL )
    return NULL;
  return c_ast_visit_up( ast->parent, visitor, data );
}

/// @endcond

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
