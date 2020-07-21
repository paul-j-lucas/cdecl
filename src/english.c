/*
**      cdecl -- C gibberish translator
**      src/english.c
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
 * Defines functions for printing in pseudo-English.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "english.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_operator.h"
#include "c_typedef.h"
#include "literals.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

// local functions
C_WARN_UNUSED_RESULT
static bool c_ast_visitor_english( c_ast_t*, void* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for `c_ast_visitor_english()` that prints a function-like
 * AST's arguments, if any.
 *
 * @param arg The `c_ast_arg` that is the first argument to print.
 * @param eout The `FILE` to emit to.
 */
static void c_ast_english_func_args( c_ast_arg_t const *arg, FILE *eout ) {
  assert( arg != NULL );
  assert( eout != NULL );

  FPUTC( '(', eout );

  bool comma = false;
  for ( ; arg != NULL; arg = arg->next ) {
    if ( true_or_set( &comma ) )
      FPUTS( ", ", eout );

    c_ast_t const *const arg_ast = c_ast_arg_ast( arg );
    if ( arg_ast->kind_id != K_NAME ) {
      //
      // For all kinds except K_NAME, we have to print:
      //
      //      <name> as <english>
      //
      // For K_NAME, e.g.:
      //
      //      void f(x)                 // untyped K&R C function argument
      //
      // there's no "as <english>" part.
      //
      c_sname_t const *const sname = c_ast_find_name( arg_ast, C_VISIT_DOWN );
      if ( sname != NULL ) {
        c_sname_english( sname, eout );
        FPRINTF( eout, " %s ", L_AS );
      } else {
        //
        // If there's no name, it's an unnamed argument, e.g.:
        //
        //      void f(int)
        //
        // so there's no "<name> as" part.
        //
      }
    }

    c_ast_t *const nonconst_arg = CONST_CAST( c_ast_t*, arg_ast );
    c_ast_visit( nonconst_arg, C_VISIT_DOWN, c_ast_visitor_english, eout );
  } // for

  FPUTC( ')', eout );
}

/**
 * Visitor function that prints \a ast as pseudo-English.
 *
 * @param ast The `c_ast` to print.
 * @param data A pointer to a `FILE` to emit to.
 * @return Always returns `false`.
 */
C_WARN_UNUSED_RESULT
static bool c_ast_visitor_english( c_ast_t *ast, void *data ) {
  assert( ast != NULL );
  assert( data != NULL );

  FILE *const eout = REINTERPRET_CAST( FILE*, data );

  switch ( ast->kind_id ) {
    case K_ARRAY:
      if ( ast->type_id != T_NONE )     // storage class
        FPRINTF( eout, "%s ", c_type_name( ast->type_id ) );
      if ( ast->as.array.size == C_ARRAY_SIZE_VARIABLE )
        FPRINTF( eout, "%s %s ", L_VARIABLE, L_LENGTH );
      FPRINTF( eout, "%s ", L_ARRAY );
      if ( ast->as.array.type_id != T_NONE )
        FPRINTF( eout, "%s ", c_type_name( ast->as.array.type_id ) );
      if ( ast->as.array.size >= 0 )
        FPRINTF( eout, "%d ", ast->as.array.size );
      FPRINTF( eout, "%s ", L_OF );
      break;

    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      if ( ast->type_id != T_NONE )     // storage class
        FPRINTF( eout, "%s ", c_type_name( ast->type_id ) );

      switch ( ast->kind_id ) {
        case K_FUNCTION:
          if ( (ast->type_id & T_MEMBER_FUNC_ONLY) != T_NONE )
            FPRINTF( eout, "%s ", L_MEMBER );
          break;
        case K_OPERATOR: {
          unsigned const overload_flags = c_oper_get_overload( ast );
          char const *const op_literal =
            overload_flags == C_OP_MEMBER     ? L_MEMBER      :
            overload_flags == C_OP_NON_MEMBER ? L_NON_MEMBER  :
            "";
          FPRINTF( eout, "%s%s", SP_AFTER( op_literal ) );
          break;
        }
        default:
          /* suppress warning */;
      } // switch

      FPUTS( c_kind_name( ast->kind_id ), eout );
      c_ast_arg_t const *const arg = c_ast_args( ast );
      if ( arg != NULL ) {
        FPUTC( ' ', eout );
        c_ast_english_func_args( arg, eout );
      }
      if ( ast->as.func.ret_ast != NULL )
        FPRINTF( eout, " %s ", L_RETURNING );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->type_id ), eout );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( eout, "%s ", c_type_name( ast->type_id ) );
      c_sname_english( &ast->as.ecsu.ecsu_sname, eout );
      break;

    case K_NAME:
      c_sname_english( &ast->sname, eout );
      break;

    case K_NONE:
      assert( ast->kind_id != K_NONE );
      break;
    case K_PLACEHOLDER:
      assert( ast->kind_id != K_PLACEHOLDER );
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE: {
      c_type_id_t const qual_type = (ast->type_id & T_MASK_QUALIFIER);
      if ( qual_type != T_NONE )
        FPRINTF( eout, "%s ", c_type_name( qual_type ) );
      FPRINTF( eout, "%s %s ", c_kind_name( ast->kind_id ), L_TO );
      break;
    }

    case K_POINTER_TO_MEMBER: {
      c_type_id_t const qual_type = (ast->type_id & T_MASK_QUALIFIER);
      if ( qual_type != T_NONE )
        FPRINTF( eout, "%s ", c_type_name( qual_type ) );
      FPRINTF( eout, "%s %s %s %s ", L_POINTER, L_TO, L_MEMBER, L_OF );
      char const *const name = c_type_name( ast->type_id & ~T_MASK_QUALIFIER );
      FPRINTF( eout, "%s%s", SP_AFTER( name ) );
      c_sname_english( &ast->as.ptr_mbr.class_sname, eout );
      FPUTC( ' ', eout );
      break;
    }

    case K_TYPEDEF:
      if ( ast->type_id != T_TYPEDEF_TYPE )
        FPRINTF( eout, "%s ", c_type_name( ast->type_id ) );
      c_sname_english( &ast->as.c_typedef->ast->sname, eout );
      break;

    case K_USER_DEF_CONVERSION: {
      char const *const name = c_type_name( ast->type_id );
      FPRINTF( eout, "%s%s%s", SP_AFTER( name ), c_kind_name( ast->kind_id ) );
      if ( !c_ast_sname_empty( ast ) ) {
        FPRINTF( eout, " %s %s ", L_OF, c_type_name( c_ast_sname_local_type( ast ) ) );
        c_sname_english( &ast->sname, eout );
      }
      FPRINTF( eout, " %s ", L_RETURNING );
      break;
    }

    case K_VARIADIC:
      FPUTS( c_kind_name( ast->kind_id ), eout );
      break;
  } // switch

  return false;
}

/**
 * Helper function for `c_sname_english()` that prints the scopes' types and
 * names in inner-to-outer order except for the inner-most scope.  For example,
 * `S::T::x` is printed as "of scope T of scope S."
 *
 * @param scope A pointer to the outermost scope.
 * @param eout The `FILE` to emit to.
 */
static void c_sname_english_impl( c_scope_t const *scope, FILE *eout ) {
  if ( scope->next != NULL ) {
    c_sname_english_impl( scope->next, eout );
    FPRINTF( eout,
      " %s %s %s",
      L_OF, c_type_name( c_scope_type( scope ) ), c_scope_name( scope )
    );
  }
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_english( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );

  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );
  c_ast_visit( nonconst_ast, C_VISIT_DOWN, c_ast_visitor_english, eout );

  switch ( ast->align.kind ) {
    case C_ALIGNAS_NONE:
      break;
    case C_ALIGNAS_EXPR:
      if ( ast->align.as.expr > 0 )
        FPRINTF( eout,
          " %s %s %u %s", L_ALIGNED, L_AS, ast->align.as.expr, L_BYTES
        );
      break;
    case C_ALIGNAS_TYPE:
      FPRINTF( eout, " %s %s ", L_ALIGNED, L_AS );
      c_ast_english( ast->align.as.type_ast, eout );
      break;
  } // switch
}

void c_sname_english( c_sname_t const *sname, FILE *eout ) {
  assert( sname != NULL );
  if ( !c_sname_empty( sname ) ) {
    FPUTS( c_sname_local_name( sname ), eout );
    c_sname_english_impl( sname->head, eout );
  }
}

void c_typedef_english( c_typedef_t const *type, FILE *eout ) {
  assert( type != NULL );
  FPRINTF( eout, "%s ", L_DEFINE );
  c_sname_english( &type->ast->sname, eout );
  FPRINTF( eout, " %s ", L_AS );
  c_ast_english( type->ast, eout );
  FPUTC( '\n', eout );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
