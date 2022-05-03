/*
**      cdecl -- C gibberish translator
**      src/english.c
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
static bool c_ast_visitor_english( c_ast_t*, c_ast_visit_data_t );

static void c_type_print_not_base( c_type_t const*, FILE* );

////////// local functions ////////////////////////////////////////////////////

/**
 * Helper function for c_ast_visitor_english() that prints a bit-field width,
 * if any.
 *
 * @param ast The AST to print the bit-field width of.
 * @param eout The `FILE` to emit to.
 */
static void c_ast_bit_width_english( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( (ast->kind & (K_BUILTIN | K_TYPEDEF)) != 0 );
  assert( eout != NULL );

  if ( ast->as.builtin.bit_width > 0 ) {
    FPRINTF( eout, " %s %u %s", L_WIDTH, ast->as.builtin.bit_width, L_BITS );
  }
}

/**
 * Prints \a ast as a declaration in pseudo-English.
 *
 * @note A newline is _not_ printed.
 *
 * @param ast The AST to print.
 * @param eout The `FILE` to print to.
 */
static void c_ast_english( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( eout != NULL );

  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );
  c_ast_visit_data_t const avd = REINTERPRET_CAST( c_ast_visit_data_t, eout );
  c_ast_visit( nonconst_ast, C_VISIT_DOWN, c_ast_visitor_english, avd );

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

/**
 * Explains \a ast as a cast in pseudo-English.
 *
 * @param ast The AST to explain.
 * @param eout The `FILE` to print to.
 *
 * @sa c_ast_explain()
 * @sa c_ast_explain_type()
 */
static void c_ast_explain_cast( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( ast->cast_kind != C_CAST_NONE );
  assert( eout != NULL );

  if ( ast->cast_kind != C_CAST_C )
    FPRINTF( eout, "%s ", c_cast_english( ast->cast_kind ) );
  FPUTS( L_CAST, eout );
  if ( !c_sname_empty( &ast->sname ) ) {
    FPUTC( ' ', eout );
    c_sname_english( &ast->sname, eout );
  }
  FPRINTF( cdecl_fout, " %s ", L_INTO );
  c_ast_english( ast, eout );
  FPUTC( '\n', eout );
}

/**
 * Helper function for c_ast_visitor_english() that prints a function-like
 * AST's parameters, if any.
 *
 * @param ast The AST to print the parameters of.
 * @param eout The `FILE` to emit to.
 */
static void c_ast_func_params_english( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( (ast->kind & K_ANY_FUNCTION_LIKE) != 0 );
  assert( eout != NULL );

  FPUTC( '(', eout );

  bool comma = false;
  FOREACH_AST_FUNC_PARAM( param, ast ) {
    fprint_sep( eout, ", ", &comma );

    c_ast_t const *const param_ast = c_param_ast( param );
    c_sname_t const *const sname = c_ast_find_name( param_ast, C_VISIT_DOWN );
    if ( sname != NULL ) {
      //
      // If there's a name, we have to print:
      //
      //      <name> as ...
      //
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

    c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, param_ast );
    c_ast_visit_data_t const avd = REINTERPRET_CAST( c_ast_visit_data_t, eout );
    c_ast_visit( nonconst_ast, C_VISIT_DOWN, c_ast_visitor_english, avd );
  } // for

  FPUTC( ')', eout );
}

/**
 * Visitor function that prints \a ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param avd A pointer to a `FILE` to emit to.
 * @return Always returns `false`.
 */
PJL_WARN_UNUSED_RESULT
static bool c_ast_visitor_english( c_ast_t *ast, c_ast_visit_data_t avd ) {
  assert( ast != NULL );
  FILE *const eout = REINTERPRET_CAST( FILE*, avd );
  assert( eout != NULL );

  switch ( ast->kind ) {
    case K_ARRAY:
      c_type_print_not_base( &ast->type, eout );
      if ( ast->as.array.size == C_ARRAY_SIZE_VARIABLE )
        FPRINTF( eout, "%s %s ", L_VARIABLE, L_LENGTH );
      FPRINTF( eout, "%s ", L_ARRAY );
      fputs_sp( c_tid_name_english( ast->as.array.stids ), eout );
      if ( ast->as.array.size >= 0 )
        FPRINTF( eout, PRId_C_ARRAY_SIZE_T " ", ast->as.array.size );
      FPRINTF( eout, "%s ", L_OF );
      break;

    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_USER_DEF_LITERAL:
      c_type_print_not_base( &ast->type, eout );
      switch ( ast->kind ) {
        case K_FUNCTION:
          if ( c_tid_is_any( ast->type.stids, TS_MEMBER_FUNC_ONLY ) )
            FPRINTF( eout, "%s ", L_MEMBER );
          break;
        case K_OPERATOR: {
          unsigned const overload_flags = c_ast_oper_overload( ast );
          char const *const op_literal =
            overload_flags == C_OP_MEMBER     ? "member "     :
            overload_flags == C_OP_NON_MEMBER ? "non-member " :
            "";
          FPUTS( op_literal, eout );
          break;
        }
        default:
          /* suppress warning */;
      } // switch

      FPUTS( c_kind_name( ast->kind ), eout );
      if ( c_ast_params_count( ast ) > 0 ) {
        FPUTC( ' ', eout );
        c_ast_func_params_english( ast, eout );
      }
      if ( ast->as.func.ret_ast != NULL )
        FPRINTF( eout, " %s ", L_RETURNING );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name_english( &ast->type ), eout );
      c_ast_bit_width_english( ast, eout );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( eout, "%s ", c_type_name_english( &ast->type ) );
      c_sname_english( &ast->as.ecsu.ecsu_sname, eout );
      if ( ast->as.ecsu.of_ast != NULL )
        FPRINTF( eout, " %s %s ", L_OF, L_TYPE );
      break;

    case K_NAME:
      assert( opt_lang < LANG_C_2X );
      //
      // In C17 and earlier, just a name for a function parameter is implicitly
      // int:
      //
      //      cdecl> explain double f(x)
      //      declare f as function (x as int) returning double
      //
      // (The "x as" was printed in c_ast_func_params_english().)
      //
      FPUTS( L_INT, eout );
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      c_type_print_not_base( &ast->type, eout );
      FPRINTF( eout, "%s %s ", c_kind_name( ast->kind ), L_TO );
      break;

    case K_POINTER_TO_MEMBER:
      c_type_print_not_base( &ast->type, eout );
      FPRINTF( eout, "%s %s ", c_kind_name( ast->kind ), L_OF );
      fputs_sp( c_tid_name_english( ast->type.btids ), eout );
      c_sname_english( &ast->as.ptr_mbr.class_sname, eout );
      FPUTC( ' ', eout );
      break;

    case K_TYPEDEF:
      if ( !c_type_equal( &ast->type, &C_TYPE_LIT_B( TB_TYPEDEF ) ) )
        FPRINTF( eout, "%s ", c_type_name_english( &ast->type ) );
      c_sname_english( &ast->as.tdef.for_ast->sname, eout );
      c_ast_bit_width_english( ast, eout );
      break;

    case K_USER_DEF_CONVERSION:
      fputs_sp( c_type_name_english( &ast->type ), eout );
      FPUTS( c_kind_name( ast->kind ), eout );
      if ( !c_sname_empty( &ast->sname ) ) {
        FPRINTF( eout,
          " %s %s ",
          L_OF, c_type_name_english( c_sname_local_type( &ast->sname ) )
        );
        c_sname_english( &ast->sname, eout );
      }
      FPRINTF( eout, " %s ", L_RETURNING );
      break;

    case K_VARIADIC:
      FPUTS( c_kind_name( ast->kind ), eout );
      break;

    CASE_K_PLACEHOLDER;
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
 * Prints the non-base (attribute(s), storage class, qualifier(s), etc.) parts
 * of \a type, if any.
 *
 * @param type The type to perhaps print.
 * @param eout The `FILE` to emit to.
 */
static void c_type_print_not_base( c_type_t const *type, FILE *eout ) {
  assert( type != NULL );
  assert( eout != NULL );

  c_type_t const nobase_type = { TB_NONE, type->stids, type->atids };
  fputs_sp( c_type_name_english( &nobase_type ), eout );
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_explain( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( eout != NULL );

  if ( ast->cast_kind != C_CAST_NONE ) {
    c_ast_explain_cast( ast, eout );
    return;
  }

  FPRINTF( eout, "%s ", L_DECLARE );
  if ( ast->kind != K_USER_DEF_CONVERSION ) {
    //
    // Every kind but a user-defined conversion has a name.
    //
    c_sname_t const *const found_sname = c_ast_find_name( ast, C_VISIT_DOWN );
    char const *local_name, *scope_name = "";
    c_type_t const *scope_type = NULL;

    if ( ast->kind == K_OPERATOR ) {
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
}

char const* c_cast_english( c_cast_kind_t kind ) {
  switch ( kind ) {
    case C_CAST_NONE        : return "none";
    case C_CAST_C           : return "C";
    case C_CAST_CONST       : return L_CONST;
    case C_CAST_DYNAMIC     : return L_DYNAMIC;
    case C_CAST_REINTERPRET : return L_REINTERPRET;
    case C_CAST_STATIC      : return L_STATIC;
  } // switch
  UNEXPECTED_INT_VALUE( kind );
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
