/*
**      cdecl -- C gibberish translator
**      src/c_ast.c
**
**      Copyright (C) 2017-2019  Paul J. Lucas, et al.
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
#include "cdecl.h"                      /* must go first */
/// @cond DOXYGEN_IGNORE
#define CDECL_AST_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_ast.h"
#include "c_typedef.h"
#include "literals.h"
#include "options.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
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
  if ( ast_i->type_id != ast_j->type_id )
    return false;

  switch ( ast_i->kind ) {
    case K_ARRAY: {
      c_array_t const *const a_i = &ast_i->as.array;
      c_array_t const *const a_j = &ast_j->as.array;
      if ( a_i->size != a_j->size )
        return false;
      if ( a_i->type_id != a_j->type_id )
        return false;
      if ( !c_ast_equiv( a_i->of_ast, a_j->of_ast ) )
        return false;
      break;
    }

    case K_OPERATOR:
      if ( ast_i->as.oper.oper_id != ast_j->as.oper.oper_id )
        return false;
      // FALLTHROUGH
    case K_FUNCTION:
      if ( ast_i->as.func.flags != ast_j->as.func.flags )
        return false;
      // FALLTHROUGH
    case K_BLOCK:
      // ret_ast is checked by the parent code below
    case K_CONSTRUCTOR:
    case K_USER_DEF_LITERAL: {
      c_ast_arg_t const *arg_i = c_ast_args( ast_i );
      c_ast_arg_t const *arg_j = c_ast_args( ast_j );
      for ( ; arg_i && arg_j; arg_i = arg_i->next, arg_j = arg_j->next ) {
        if ( !c_ast_equiv( c_ast_arg_ast( arg_i ), c_ast_arg_ast( arg_j ) ) )
          return false;
      } // for
      if ( arg_i != NULL || arg_j != NULL )
        return false;
      break;
    }

    case K_ENUM_CLASS_STRUCT_UNION: {
      c_ecsu_t const *const e_i = &ast_i->as.ecsu;
      c_ecsu_t const *const e_j = &ast_j->as.ecsu;
      if ( c_sname_cmp( &e_i->ecsu_sname, &e_j->ecsu_sname ) != 0 )
        return false;
      break;
    }

    case K_NAME:
      // names don't matter
      break;

    case K_POINTER_TO_MEMBER: {
      c_ptr_mbr_t const *const pm_i = &ast_i->as.ptr_mbr;
      c_ptr_mbr_t const *const pm_j = &ast_j->as.ptr_mbr;
      if ( c_sname_cmp( &pm_i->class_sname, &pm_j->class_sname ) != 0 )
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
    case K_DESTRUCTOR:
    case K_PLACEHOLDER:
    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
    case K_USER_DEF_CONVERSION:         // conv_ast checked by parent code below
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

    c_sname_free( &ast->sname );
    switch ( ast->kind ) {
      case K_ENUM_CLASS_STRUCT_UNION:
        c_sname_free( &ast->as.ecsu.ecsu_sname );
        break;
      case K_POINTER_TO_MEMBER:
        c_sname_free( &ast->as.ptr_mbr.class_sname );
        break;
      case K_TYPEDEF:
        // Do not free ast->as.c_typedef here since it's global data: it will
        // be freed eventually via c_typedef_cleanup().
        break;
      case K_ARRAY:
      case K_BLOCK:
      case K_BUILTIN:
      case K_CONSTRUCTOR:
      case K_DESTRUCTOR:
      case K_FUNCTION:
      case K_NAME:
      case K_NONE:
      case K_OPERATOR:
      case K_PLACEHOLDER:
      case K_POINTER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
      case K_USER_DEF_CONVERSION:
      case K_USER_DEF_LITERAL:
      case K_VARIADIC:
        // nothing to do
        break;
    } // switch

    FREE( ast );
  }
}

c_ast_t* c_ast_new( c_kind_t kind, c_ast_depth_t depth, c_loc_t const *loc ) {
  assert( loc != NULL );
  static c_ast_id_t next_id;

  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  MEM_ZERO( ast );

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

c_sname_t c_ast_sname_dup( c_ast_t const *ast ) {
  c_sname_t rv = c_sname_dup( &ast->sname );
  //
  // If the AST's type is class, namespace, struct, or union, adopt that type
  // for the scoped name's type.
  //
  if ( (ast->type_id & (T_CLASS_STRUCT_UNION | T_NAMESPACE)) != T_NONE )
    c_sname_set_type(
      &rv, ast->type_id & (T_CLASS_STRUCT_UNION | T_INLINE | T_NAMESPACE)
    );
  return rv;
}

void c_ast_sname_set_name( c_ast_t *ast, char *name ) {
  assert( ast != NULL );
  assert( name != NULL );
  c_sname_free( &ast->sname );
  c_sname_append_name( &ast->sname, name );
}

void c_ast_sname_set_sname( c_ast_t *ast, c_sname_t *sname ) {
  assert( ast != NULL );
  assert( sname != NULL );
  c_sname_free( &ast->sname );

  //
  // If the scoped name has no scope but the AST is one of a class, namespace,
  // struct, or union type, adopt that type for the scoped type.
  //
  c_type_id_t sn_type = c_sname_type( sname );
  if ( sn_type == T_NONE &&
       (ast->type_id & (T_CLASS_STRUCT_UNION | T_NAMESPACE)) != T_NONE ) {
    sn_type = ast->type_id & (T_CLASS_STRUCT_UNION | T_INLINE | T_NAMESPACE);
  }
  c_ast_sname_set_type( ast, sn_type );

  c_sname_append_sname( &ast->sname, sname );
}

c_ast_t* c_ast_visit( c_ast_t *ast, v_direction_t dir, c_ast_visitor_t visitor,
                      void *data ) {
  if ( ast == NULL || visitor( ast, data ) )
    return ast;
  switch ( dir ) {
    case C_VISIT_DOWN:
      ast = c_ast_is_parent( ast ) ? ast->as.parent.of_ast : NULL;
      break;
    case C_VISIT_UP:
      ast = ast->parent;
      break;
  } // switch
  return c_ast_visit( ast, dir, visitor, data );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
