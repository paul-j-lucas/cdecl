/*
**      cdecl -- C gibberish translator
**      src/c_ast.c
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
 * Defines functions for traversing and manipulating an AST.
 */

// local
#include "cdecl.h"                      /* must go first */
/// @cond DOXYGEN_IGNORE
#define C_AST_INLINE _GL_EXTERN_INLINE
/// @endcond
#include "c_ast.h"
#include "c_typedef.h"
#include "literals.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdbool.h>

/// @endcond

///////////////////////////////////////////////////////////////////////////////

// local variable definitions
static unsigned   c_ast_count;          ///< ASTs allocated but not yet freed.

////////// local functions ////////////////////////////////////////////////////

#ifndef NDEBUG
/**
 * Checks \a ast for a cycle.
 *
 * @param ast The AST node to begin at.
 * @return Returns `true` only if there is a cycle.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_has_cycle( c_ast_t const *ast ) {
  assert( ast != NULL );
  for ( c_ast_t const *const start_ast = ast; ast->parent_ast != NULL; ) {
    ast = ast->parent_ast;
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

bool c_ast_equiv( c_ast_t const *i_ast, c_ast_t const *j_ast ) {
  if ( i_ast == j_ast )
    return true;
  if ( (i_ast != NULL && j_ast == NULL) || (i_ast == NULL && j_ast != NULL) )
    return false;

  //
  // If only one of the ASTs is a typedef, compare the other AST to the
  // typedef's AST.
  //
  if ( i_ast->kind_id == K_TYPEDEF ) {
    if ( j_ast->kind_id != K_TYPEDEF )
      return c_ast_equiv( i_ast->as.c_typedef->ast, j_ast );
  } else {
    if ( j_ast->kind_id == K_TYPEDEF )
      return c_ast_equiv( i_ast, j_ast->as.c_typedef->ast );
  }

  if ( i_ast->kind_id != j_ast->kind_id )
    return false;
  if ( !c_type_equal( &i_ast->type, &j_ast->type ) )
    return false;

  switch ( i_ast->kind_id ) {
    case K_ARRAY: {
      c_array_ast_t const *const ai_ast = &i_ast->as.array;
      c_array_ast_t const *const aj_ast = &j_ast->as.array;
      if ( ai_ast->size != aj_ast->size )
        return false;
      if ( ai_ast->store_tid != aj_ast->store_tid )
        return false;
      break;
    }

    case K_OPERATOR:
      if ( i_ast->as.oper.oper_id != j_ast->as.oper.oper_id )
        return false;
      C_FALLTHROUGH;
    case K_FUNCTION:
      if ( i_ast->as.func.flags != j_ast->as.func.flags )
        return false;
      C_FALLTHROUGH;
    case K_APPLE_BLOCK:
      // ret_ast is checked by the parent code below
    case K_CONSTRUCTOR:
    case K_USER_DEF_LITERAL: {
      c_ast_param_t const *i_param = c_ast_params( i_ast );
      c_ast_param_t const *j_param = c_ast_params( j_ast );
      for ( ; i_param != NULL && j_param != NULL;
              i_param = i_param->next, j_param = j_param->next ) {
        if ( !c_ast_equiv( c_param_ast( i_param ), c_param_ast( j_param ) ) )
          return false;
      } // for
      if ( i_param != NULL || j_param != NULL )
        return false;
      break;
    }

    case K_ENUM_CLASS_STRUCT_UNION: {
      c_ecsu_ast_t const *const ei_ast = &i_ast->as.ecsu;
      c_ecsu_ast_t const *const ej_ast = &j_ast->as.ecsu;
      if ( c_sname_cmp( &ei_ast->ecsu_sname, &ej_ast->ecsu_sname ) != 0 )
        return false;
      break;
    }

    case K_NAME:
      // names don't matter
      break;

    case K_POINTER_TO_MEMBER: {
      c_ptr_mbr_ast_t const *const pmi_ast = &i_ast->as.ptr_mbr;
      c_ptr_mbr_ast_t const *const pmj_ast = &j_ast->as.ptr_mbr;
      if ( c_sname_cmp( &pmi_ast->class_sname, &pmj_ast->class_sname ) != 0 )
        return false;
      break;
    }

    case K_TYPEDEF: {
      c_typedef_t const *const i_td = i_ast->as.c_typedef;
      c_typedef_t const *const j_td = j_ast->as.c_typedef;
      if ( !c_ast_equiv( i_td->ast, j_td->ast ) )
        return false;
      break;
    }

    case K_NONE:
    case K_BUILTIN:
    case K_DESTRUCTOR:
    case K_PLACEHOLDER:
    case K_POINTER:                     // checked by parent code below
    case K_REFERENCE:                   // checked by parent code below
    case K_RVALUE_REFERENCE:            // checked by parent code below
    case K_USER_DEF_CONVERSION:         // checked by parent code below
    case K_VARIADIC:
      // nothing to do
      break;
  } // switch

  if ( !c_ast_is_parent( i_ast ) ) {
    assert( !c_ast_is_parent( j_ast ) );
    return true;
  }
  assert( c_ast_is_parent( j_ast ) );
  return c_ast_equiv( i_ast->as.parent.of_ast, j_ast->as.parent.of_ast );
}

void c_ast_free( c_ast_t *ast ) {
  if ( ast != NULL ) {
    assert( c_ast_count > 0 );
    --c_ast_count;

    c_sname_free( &ast->sname );
    switch ( ast->kind_id ) {
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
      case K_APPLE_BLOCK:
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

c_ast_t* c_ast_new( c_kind_id_t kind_id, c_ast_depth_t depth,
                    c_loc_t const *loc ) {
  assert( loc != NULL );
  static c_ast_id_t next_id;

  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  MEM_ZERO( ast );

  ast->depth = depth;
  ast->unique_id = ++next_id;
  ast->kind_id = kind_id;
  ast->type = T_NONE;
  ast->loc = *loc;

  switch ( kind_id ) {
    case K_ARRAY:
      ast->as.array.store_tid = TS_NONE;
      break;
    case K_APPLE_BLOCK:
    case K_BUILTIN:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_ENUM_CLASS_STRUCT_UNION:
    case K_FUNCTION:
    case K_NAME:
    case K_NONE:
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
  return ast;
}

c_ast_t* c_ast_root( c_ast_t *ast ) {
  assert( ast != NULL );
  while ( ast->parent_ast != NULL )
    ast = ast->parent_ast;
  return ast;
}

void c_ast_set_parent( c_ast_t *child_ast, c_ast_t *parent_ast ) {
  assert( child_ast != NULL );
  assert( parent_ast != NULL );
  assert( c_ast_is_parent( parent_ast ) );

  child_ast->parent_ast = parent_ast;
  parent_ast->as.parent.of_ast = child_ast;

  assert( !c_ast_has_cycle( child_ast ) );
}

void c_ast_set_name( c_ast_t *ast, char *name ) {
  assert( ast != NULL );
  assert( name != NULL );
  c_sname_free( &ast->sname );
  c_sname_append_name( &ast->sname, name );
}

void c_ast_set_sname( c_ast_t *ast, c_sname_t *sname ) {
  assert( ast != NULL );
  assert( sname != NULL );
  //
  // If the scoped name has no scope type but the AST is one of a class,
  // namespace, struct, or union type, adopt that type for the scope type.
  //
  if ( c_type_is_none( c_sname_local_type( sname ) ) &&
       c_type_is_tid_any( &ast->type, TB_ANY_SCOPE ) ) {
    c_sname_set_local_type( sname,
      &C_TYPE_LIT(
        ast->type.base_tid & TB_ANY_SCOPE,
        ast->type.store_tid & TS_INLINE,
        TA_NONE
      )
    );
  }

  c_sname_free( &ast->sname );
  c_ast_append_sname( ast, sname );
}

c_ast_t* c_ast_visit( c_ast_t *ast, c_visit_dir_t dir, c_ast_visitor_t visitor,
                      void *data ) {
  switch ( dir ) {
    case C_VISIT_DOWN:
      while ( ast != NULL && !(*visitor)( ast, data ) )
        ast = c_ast_is_parent( ast ) ? ast->as.parent.of_ast : NULL;
      break;
    case C_VISIT_UP:
      while ( ast != NULL && !(*visitor)( ast, data ) )
        ast = ast->parent_ast;
      break;
  } // switch
  return ast;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
