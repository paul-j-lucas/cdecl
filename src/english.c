/*
**      cdecl -- C gibberish translator
**      src/english.c
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
 * Defines functions for printing in pseudo-English.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "english.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_operator.h"
#include "literals.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>

/// @endcond

// local functions
PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_english( c_ast_t*, uint64_t );

static void non_type_name( c_type_t const*, FILE* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for c_ast_visitor_english() that prints a bit-field width,
 * if any.
 *
 * @param ast The AST to print the bit-field width of.
 * @param eout The `FILE` to emit to.
 */
static void c_ast_english_bit_width( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( (ast->kind_id & (K_BUILTIN | K_TYPEDEF)) != K_NONE );
  assert( eout != NULL );

  if ( ast->as.builtin.bit_width > 0 )
    FPRINTF( eout, " %s %u %s", L_WIDTH, ast->as.builtin.bit_width, L_BITS );
}

/**
 * Helper function for c_ast_visitor_english() that prints a function-like
 * AST's parameters, if any.
 *
 * @param ast The AST to pring the parameters of.
 * @param eout The `FILE` to emit to.
 */
static void c_ast_english_func_params( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( (ast->kind_id & K_ANY_FUNCTION_LIKE) != K_NONE );
  assert( eout != NULL );

  FPUTC( '(', eout );

  bool comma = false;
  FOREACH_PARAM( param, ast ) {
    if ( true_or_set( &comma ) )
      FPUTS( ", ", eout );

    c_ast_t const *const param_ast = c_param_ast( param );
    if ( param_ast->kind_id != K_NAME ) {
      //
      // For all kinds except K_NAME, we have to print:
      //
      //      <name> as <english>
      //
      // For K_NAME, e.g.:
      //
      //      void f(x)                 // untyped K&R C function parameter
      //
      // there's no "as <english>" part.
      //
      c_sname_t const *const sname = c_ast_find_name( param_ast, C_VISIT_DOWN );
      if ( sname != NULL ) {
        c_sname_english( sname, eout );
        FPRINTF( eout, " %s ", L_AS );
      } else {
        //
        // If there's no name, it's an unnamed parameter, e.g.:
        //
        //      void f(int)
        //
        // so there's no "<name> as" part.
        //
      }
    }

    c_ast_visit(
      CONST_CAST( c_ast_t*, param_ast ), C_VISIT_DOWN,
      c_ast_visitor_english, REINTERPRET_CAST( uint64_t, eout )
    );
  } // for

  FPUTC( ')', eout );
}

/**
 * Visitor function that prints \a ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param data A pointer to a `FILE` to emit to.
 * @return Always returns `false`.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_english( c_ast_t *ast, uint64_t data ) {
  assert( ast != NULL );
  FILE *const eout = REINTERPRET_CAST( FILE*, data );
  assert( eout != NULL );

  switch ( ast->kind_id ) {
    case K_ARRAY:
      non_type_name( &ast->type, eout );
      if ( ast->as.array.size == C_ARRAY_SIZE_VARIABLE )
        FPRINTF( eout, "%s %s ", L_VARIABLE, L_LENGTH );
      FPRINTF( eout, "%s ", L_ARRAY );
      if ( ast->as.array.stid != TS_NONE )
        FPRINTF( eout, "%s ", c_tid_name_eng( ast->as.array.stid ) );
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
      non_type_name( &ast->type, eout );
      switch ( ast->kind_id ) {
        case K_FUNCTION:
          if ( c_type_is_tid_any( &ast->type, TS_MEMBER_FUNC_ONLY ) )
            FPRINTF( eout, "%s ", L_MEMBER );
          break;
        case K_OPERATOR: {
          unsigned const overload_flags = c_ast_oper_overload( ast );
          char const *const op_literal =
            overload_flags == C_OP_MEMBER     ? L_MEMBER      :
            overload_flags == C_OP_NON_MEMBER ? H_NON_MEMBER  :
            "";
          FPRINTF( eout, "%s%s", SP_AFTER( op_literal ) );
          break;
        }
        default:
          /* suppress warning */;
      } // switch

      FPUTS( c_kind_name( ast->kind_id ), eout );
      if ( c_ast_params_count( ast ) > 0 ) {
        FPUTC( ' ', eout );
        c_ast_english_func_params( ast, eout );
      }
      if ( ast->as.func.ret_ast != NULL )
        FPRINTF( eout, " %s ", L_RETURNING );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name_english( &ast->type ), eout );
      c_ast_english_bit_width( ast, eout );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( eout, "%s ", c_type_name_english( &ast->type ) );
      c_sname_english( &ast->as.ecsu.ecsu_sname, eout );
      if ( ast->as.ecsu.of_ast != NULL )
        FPRINTF( eout, " %s %s ", L_OF, L_TYPE );
      break;

    case K_NAME:
      c_sname_english( &ast->sname, eout );
      break;

    case K_NONE:                        // should not occur in completed AST
      assert( ast->kind_id != K_NONE );
      break;
    case K_PLACEHOLDER:                 // should not occur in completed AST
      assert( ast->kind_id != K_PLACEHOLDER );
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      non_type_name( &ast->type, eout );
      FPRINTF( eout, "%s %s ", c_kind_name( ast->kind_id ), L_TO );
      break;

    case K_POINTER_TO_MEMBER: {
      non_type_name( &ast->type, eout );
      FPRINTF( eout, "%s %s %s %s ", L_POINTER, L_TO, L_MEMBER, L_OF );
      char const *const name = c_tid_name_eng( ast->type.btid );
      FPRINTF( eout, "%s%s", SP_AFTER( name ) );
      c_sname_english( &ast->as.ptr_mbr.class_sname, eout );
      FPUTC( ' ', eout );
      break;
    }

    case K_TYPEDEF:
      if ( !c_type_equal( &ast->type, &C_TYPE_LIT_B( TB_TYPEDEF ) ) )
        FPRINTF( eout, "%s ", c_type_name_english( &ast->type ) );
      c_sname_english( &ast->as.tdef.for_ast->sname, eout );
      c_ast_english_bit_width( ast, eout );
      break;

    case K_USER_DEF_CONVERSION: {
      char const *const name = c_type_name_english( &ast->type );
      FPRINTF( eout, "%s%s%s", SP_AFTER( name ), c_kind_name( ast->kind_id ) );
      if ( !c_ast_empty_name( ast ) ) {
        FPRINTF( eout,
          " %s %s ", L_OF, c_type_name_english( c_ast_local_type( ast ) )
        );
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
 * Helper function for c_sname_english() that prints the scopes' types and
 * names in inner-to-outer order except for the inner-most scope.  For example,
 * `S::T::x` is printed as "of scope T of scope S."
 *
 * @param scope A pointer to the outermost scope.
 * @param eout The `FILE` to emit to.
 */
static void c_sname_english_impl( c_scope_t const *scope, FILE *eout ) {
  assert( scope != NULL );
  assert( eout != NULL );

  if ( scope->next != NULL ) {
    c_sname_english_impl( scope->next, eout );
    c_scope_data_t const *const data = c_scope_data( scope );
    FPRINTF( eout,
      " %s %s %s", L_OF, c_type_name_english( &data->type ), data->name
    );
  }
}

/**
 * Prints the non-type (attribute(s), storage class, qualifier(s), etc.) parts
 * of \a type, if any.
 *
 * @param type The type to perhaps print.
 * @param eout The `FILE` to emit to.
 */
static void non_type_name( c_type_t const *type, FILE *eout ) {
  assert( type != NULL );
  assert( eout != NULL );

  c_type_t const temp_type = { TB_NONE, type->stid, type->atid };
  if ( !c_type_is_none( &temp_type ) )
    FPRINTF( eout, "%s ", c_type_name_english( &temp_type ) );
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_english( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( eout != NULL );

  c_ast_visit(
    CONST_CAST( c_ast_t*, ast ), C_VISIT_DOWN,
    c_ast_visitor_english, REINTERPRET_CAST( uint64_t, eout )
  );

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

void c_ast_explain_declaration( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( eout != NULL );

  FPRINTF( eout, "%s ", L_DECLARE );
  if ( ast->kind_id != K_USER_DEF_CONVERSION ) {
    //
    // Every kind but a user-defined conversion has a name.
    //
    c_sname_t const *const found_sname = c_ast_find_name( ast, C_VISIT_DOWN );
    char const *local_name, *scope_name = "";
    c_type_t const *scope_type = NULL;

    if ( ast->kind_id == K_OPERATOR ) {
      local_name = c_oper_token_c( ast->as.oper.oper_id );
      if ( found_sname != NULL ) {
        scope_name = c_sname_full_name( found_sname );
        scope_type = c_sname_local_type( found_sname );
      }
    } else {
      assert( found_sname != NULL );
      assert( !c_sname_empty( found_sname ) );
      local_name = c_sname_local_name( found_sname );
      scope_name = c_sname_scope_name( found_sname );
      scope_type = c_sname_scope_type( found_sname );
    }

    assert( local_name != NULL );
    FPRINTF( eout, "%s ", local_name );
    if ( scope_name[0] != '\0' ) {
      assert( !c_type_is_none( scope_type ) );
      FPRINTF( eout,
        "%s %s %s ", L_OF, c_type_name_english( scope_type ), scope_name
      );
    }
    FPRINTF( eout, "%s ", L_AS );
  }

  c_ast_english( ast, eout );
  FPUTC( '\n', eout );
}

void c_ast_explain_type( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( eout != NULL );

  FPRINTF( eout, "%s ", L_DEFINE );
  c_sname_english( &ast->sname, eout );
  FPRINTF( eout, " %s ", L_AS );
  c_ast_english( ast, eout );
  FPUTC( '\n', eout );
}

void c_sname_english( c_sname_t const *sname, FILE *eout ) {
  assert( sname != NULL );
  assert( eout != NULL );

  if ( !c_sname_empty( sname ) ) {
    FPUTS( c_sname_local_name( sname ), eout );
    c_sname_english_impl( sname->head, eout );
  }
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
