/*
**      cdecl -- C gibberish translator
**      src/english.c
**
**      Copyright (C) 2017-2024  Paul J. Lucas
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
 * Defines data structures and functions for printing in pseudo-English.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "english.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_operator.h"
#include "c_sname.h"
#include "c_typedef.h"
#include "gibberish.h"
#include "literals.h"
#include "slist.h"
#include "types.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stddef.h>                     /* for NULL */

/// @endcond

/**
 * @addtogroup printing-english-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * State maintained by c_ast_visit_english().
 */
struct eng_state {
  FILE           *fout;                 ///< Where to print the English.
  c_ast_t const  *func_ast;             ///< The current function AST, if any.
};
typedef struct eng_state eng_state_t;

// local functions
NODISCARD
static bool c_ast_is_likely_vla( c_ast_t const*, eng_state_t const* ),
            c_ast_visitor_english( c_ast_t const*, user_data_t );

static void c_builtin_ast_english( c_ast_t const*, eng_state_t const* );
static void c_cast_ast_english( c_ast_t const*, eng_state_t const* );
static void c_concept_ast_english( c_ast_t const*, eng_state_t const* );
static void c_csu_ast_english( c_ast_t const*, eng_state_t const* );
static void c_enum_ast_english( c_ast_t const*, eng_state_t const* );
static void c_func_like_ast_english( c_ast_t const*, eng_state_t const* );
static void c_lambda_ast_english( c_ast_t const*, eng_state_t const* );
static void c_name_ast_english( c_ast_t const*, eng_state_t const* );
static void c_ptr_mbr_ast_english( c_ast_t const*, eng_state_t const* );
static void c_ptr_ref_ast_english( c_ast_t const*, eng_state_t const* );
static void c_sname_english( c_sname_t const*, FILE* );
static void c_struct_bind_ast_english( c_ast_t const*, eng_state_t const* );
static void c_type_name_nobase_english( c_type_t const*, FILE* );
static void c_typedef_ast_english( c_ast_t const*, eng_state_t const* );
static void c_udef_conv_ast_english( c_ast_t const*, eng_state_t const* );
static void eng_init( eng_state_t*, FILE* );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Convenience function for calling c_ast_visit() to print \a ast as a
 * declaration in pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static inline void c_ast_visit_english( c_ast_t const *ast,
                                        eng_state_t const *eng ) {
  c_ast_visit(
    ast, C_VISIT_DOWN, &c_ast_visitor_english, (user_data_t){ .pc = eng }
  );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a #K_ARRAY \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_array_ast_english( c_ast_t const *ast, eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_ARRAY );
  assert( eng != NULL );

  c_type_name_nobase_english( &ast->type, eng->fout );

  if ( c_ast_is_likely_vla( ast, eng ) )
    FPUTS( "variable length ", eng->fout );
  FPUTS( "array ", eng->fout );

  switch ( ast->array.kind ) {
    case C_ARRAY_SIZE_INT:
      FPRINTF( eng->fout, "%u ", ast->array.size_int );
      break;
    case C_ARRAY_SIZE_NAME:
      FPRINTF( eng->fout, "%s ", ast->array.size_name );
      break;
    case C_ARRAY_SIZE_NONE:
    case C_ARRAY_SIZE_VLA:
      break;
  } // switch
  FPUTS( "of ", eng->fout );
}

/**
 * Prints the alignment of \a ast in pseudo-English.
 *
 * @param ast The AST to print the alignment of.
 * @param eng The eng_state to use.
 */
static void c_ast_alignas_english( c_ast_t const *ast,
                                   eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( eng != NULL );

  switch ( ast->align.kind ) {
    case C_ALIGNAS_NONE:
      break;
    case C_ALIGNAS_BYTES:
      if ( ast->align.bytes > 0 )
        FPRINTF( eng->fout, " aligned as %u bytes", ast->align.bytes );
      break;
    case C_ALIGNAS_SNAME:
      FPUTS( " aligned as ", eng->fout );
      c_sname_english( &ast->align.sname, eng->fout );
      FPUTS( " bytes", eng->fout );
      break;
    case C_ALIGNAS_TYPE:
      FPUTS( " aligned as ", eng->fout );
      c_ast_visit_english( ast->align.type_ast, eng );
      break;
  } // switch
}

/**
 * Prints a bit-field width, if any, in pseudo-English.
 *
 * @param ast The AST to print the bit-field width of.
 * @param fout The `FILE` to emit to.
 */
static void c_ast_bit_width_english( c_ast_t const *ast, FILE *fout ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_BIT_FIELD ) );
  assert( fout != NULL );

  if ( ast->bit_field.bit_width > 0 )
    FPRINTF( fout, " width %u bits", ast->bit_field.bit_width );
}

/**
 * Prints a function-like AST's parameters, if any, in pseudo-English.
 *
 * @param ast The AST to print the parameters of.
 * @param eng The eng_state to use.
 */
static void c_ast_func_params_english( c_ast_t const *ast,
                                       eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_FUNCTION_LIKE ) );
  assert( eng != NULL );

  FPUTC( '(', eng->fout );

  eng_state_t param_eng;
  eng_init( &param_eng, eng->fout );
  param_eng.func_ast = ast;

  bool comma = false;
  FOREACH_AST_FUNC_PARAM( param, ast ) {
    fput_sep( ", ", &comma, eng->fout );

    c_ast_t const *const param_ast = c_param_ast( param );
    c_sname_t const *const sname = c_ast_find_name( param_ast, C_VISIT_DOWN );
    if ( sname != NULL ) {
      c_sname_english( sname, eng->fout );
      //
      // For all kinds except K_NAME, we have to print:
      //
      //      <name> as <english>
      //
      // For K_NAME in K&R C, e.g.:
      //
      //      char f(x)                 // untyped K&R C function parameter
      //
      // there's no "as <english>" part.
      //
      if ( param_ast->kind != K_NAME ||
           (!opt_permissive_types && OPT_LANG_IS( PROTOTYPES )) ) {
        FPUTS( " as ", eng->fout );
      }
    }
    else {
      //
      // If there's no name, it's an unnamed parameter, e.g.:
      //
      //      char f(int)
      //
      // so there's no "<name> as" part.
      //
    }

    c_ast_visit_english( param_ast, &param_eng );
  } // for

  FPUTC( ')', eng->fout );
}

/**
 * Gets whether \a ast is likely a VLA.
 *
 * @param ast The AST to check.
 * @param eng The eng_state to use.
 * @return Returns `true` only if \a ast is likely a VLA.
 */
NODISCARD
static bool c_ast_is_likely_vla( c_ast_t const *ast, eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_ARRAY );
  assert( eng != NULL );

  switch ( ast->array.kind ) {
    case C_ARRAY_SIZE_NAME:
      //
      // Just because an array has a named size doesn't mean it's a VLA.
      //
      if ( eng->func_ast == NULL ) {
        //
        // Named size is presumably some previously declared name, e.g.:
        //
        //      int a[n]                // not necessarily a VLA
        //      int a[const n]          // not necessarily a VLA
        //
        break;
      }
      if ( !c_ast_find_param_named( eng->func_ast, ast->array.size_name,
                                    ast ) ) {
        //
        // Named size doesn't equal any previous parameter's name, e.g.:
        //
        //      void f(int n, int a[x]) // not necessarily a VLA
        //
        break;
      }
      //
      // Named size equals a previous parameter's name, e.g.:
      //
      //      void f(int n, int a[n])  // definitely a VLA
      //
      FALLTHROUGH;

    case C_ARRAY_SIZE_VLA:
      return true;

    case C_ARRAY_SIZE_NONE:
    case C_ARRAY_SIZE_INT:
      break;
  } // switch

  return false;
}

/**
 * Prints a lambda AST's captures, if any, in pseudo-English.
 *
 * @param ast The lambda AST to print the captures of.
 * @param fout The `FILE` to emit to.
 */
static void c_ast_lambda_captures_english( c_ast_t const *ast, FILE *fout ) {
  assert( ast != NULL );
  assert( ast->kind == K_LAMBDA );
  assert( fout != NULL );

  FPUTC( '[', fout );

  bool comma = false;
  FOREACH_AST_LAMBDA_CAPTURE( capture, ast ) {
    fput_sep( ", ", &comma, fout );

    c_ast_t const *const capture_ast = c_capture_ast( capture );
    switch ( capture_ast->capture.kind ) {
      case C_CAPTURE_COPY:
        FPUTS( "copy by default", fout );
        break;
      case C_CAPTURE_REFERENCE:
        if ( c_sname_empty( &capture_ast->sname ) ) {
          FPUTS( "reference by default", fout );
          break;
        }
        FPUTS( "reference to ", fout );
        FALLTHROUGH;
      case C_CAPTURE_VARIABLE:
        c_sname_english( &capture_ast->sname, fout );
        break;
      case C_CAPTURE_STAR_THIS:
        FPUTC( '*', fout );
        FALLTHROUGH;
      case C_CAPTURE_THIS:
        FPUTS( L_this, fout );
        break;
    } // switch
  } // for

  FPUTC( ']', fout );
}

/**
 * Prints the scoped name of \a AST in pseudo-English.
 *
 * @param ast The AST to print the name of.
 * @param fout The `FILE` to emit to.
 */
static void c_ast_name_english( c_ast_t const *ast, FILE *fout ) {
  assert( ast != NULL );
  assert( fout != NULL );

  c_sname_t const *const found_sname = c_ast_find_name( ast, C_VISIT_DOWN );
  char const *local_name = "", *scope_name = "";
  c_type_t const *scope_type = NULL;

  if ( ast->kind == K_OPERATOR ) {
    local_name = c_op_token_c( ast->oper.operator->op_id );
    if ( found_sname != NULL ) {
      scope_name = c_sname_gibberish( found_sname );
      scope_type = c_sname_local_type( found_sname );
    }
  } else {
    assert( found_sname != NULL );
    assert( !c_sname_empty( found_sname ) );
    local_name = c_sname_local_name( found_sname );
    scope_name = c_sname_scope_gibberish( found_sname );
    scope_type = c_sname_scope_type( found_sname );
  }

  assert( local_name[0] != '\0' );
  FPUTS( local_name, fout );
  if ( scope_name[0] != '\0' ) {
    assert( !c_type_is_none( scope_type ) );
    FPRINTF( fout, " of %s %s", c_type_english( scope_type ), scope_name );
  }
}

/**
 * Visitor function that prints \a ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param user_data A pointer to a `FILE` to emit to.
 * @return Always returns `false`.
 */
NODISCARD
static bool c_ast_visitor_english( c_ast_t const *ast, user_data_t user_data ) {
  assert( ast != NULL );
  eng_state_t const *const eng = user_data.pc;
  assert( eng != NULL );

  switch ( ast->kind ) {
    case K_ARRAY:
      c_array_ast_english( ast, eng );
      break;

    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_UDEF_LIT:
      c_func_like_ast_english( ast, eng );
      break;

    case K_BUILTIN:
      c_builtin_ast_english( ast, eng );
      break;

    case K_CAPTURE:
      // A K_CAPTURE can occur only in a lambda capture list, not at the top-
      // level, and captures are never visited.
      UNEXPECTED_INT_VALUE( ast->kind );

    case K_CAST:
      c_cast_ast_english( ast, eng );
      break;

    case K_CLASS_STRUCT_UNION:
      c_csu_ast_english( ast, eng );
      break;

    case K_CONCEPT:
      c_concept_ast_english( ast, eng );
      break;

    case K_ENUM:
      c_enum_ast_english( ast, eng );
      break;

    case K_LAMBDA:
      c_lambda_ast_english( ast, eng );
      break;

    case K_NAME:
      c_name_ast_english( ast, eng );
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      c_ptr_ref_ast_english( ast, eng );
      break;

    case K_POINTER_TO_MEMBER:
      c_ptr_mbr_ast_english( ast, eng );
      break;

    case K_STRUCTURED_BINDING:
      c_struct_bind_ast_english( ast, eng );
      break;

    case K_TYPEDEF:
      c_typedef_ast_english( ast, eng );
      break;

    case K_UDEF_CONV:
      c_udef_conv_ast_english( ast, eng );
      break;

    case K_VARIADIC:
      FPUTS( c_kind_name( ast->kind ), eng->fout );
      break;

    case K_PLACEHOLDER:
      unreachable();
  } // switch

  return /*stop=*/false;
}

/**
 * Prints a #K_BUILTIN \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_builtin_ast_english( c_ast_t const *ast,
                                   eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_BUILTIN );
  assert( eng != NULL );

  if ( c_ast_root( ast )->is_param_pack ) {
    //
    // Special case: if the root AST is a parameter pack, print that instead of
    // this AST's type.
    //
    c_type_t type = ast->type;
    assert( type.btids == TB_auto );
    type.btids = TB_NONE;
    fputs_sp( c_type_english( &type ), eng->fout );
    FPUTS( "parameter pack", eng->fout );
  }
  else {
    FPUTS( c_type_english( &ast->type ), eng->fout );
    if ( c_ast_is_tid_any( ast, TB__BitInt ) )
      FPRINTF( eng->fout, " width %u bits", ast->builtin.BitInt.width );
    c_ast_bit_width_english( ast, eng->fout );
  }
}

/**
 * Prints a #K_CAST \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_cast_ast_english( c_ast_t const *ast, eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_CAST );
  assert( eng != NULL );

  if ( ast->cast.kind != C_CAST_C )
    FPRINTF( eng->fout, "%s ", c_cast_english( ast->cast.kind ) );
  FPUTS( L_cast, eng->fout );
  if ( !c_sname_empty( &ast->sname ) ) {
    FPUTC( ' ', eng->fout );
    c_sname_english( &ast->sname, eng->fout );
  }
  FPUTS( " into ", eng->fout );
}

/**
 * Prints a #K_CONCEPT \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_concept_ast_english( c_ast_t const *ast,
                                   eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_CONCEPT );
  assert( eng != NULL );

  fputs_sp( c_type_english( &ast->type ), eng->fout );
  FPUTS( "concept ", eng->fout );
  c_sname_english( &ast->concept.concept_sname, eng->fout );
  if ( c_ast_root( ast )->is_param_pack )
    FPUTS( " parameter pack", eng->fout );
}

/**
 * Prints a #K_CLASS_STRUCT_UNION \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_csu_ast_english( c_ast_t const *ast, eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_CLASS_STRUCT_UNION );
  assert( eng != NULL );

  FPRINTF( eng->fout, "%s ", c_type_english( &ast->type ) );
  c_sname_english( &ast->csu.csu_sname, eng->fout );
}

/**
 * Prints a #K_ENUM \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_enum_ast_english( c_ast_t const *ast, eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_ENUM );
  assert( eng != NULL );

  FPRINTF( eng->fout, "%s ", c_type_english( &ast->type ) );
  c_sname_english( &ast->enum_.enum_sname, eng->fout );
  if ( ast->enum_.of_ast != NULL )
    FPUTS( " of type ", eng->fout );
  else
    c_ast_bit_width_english( ast, eng->fout );
}

/**
 * Prints a #K_APPLE_BLOCK, #K_CONSTRUCTOR, #K_DESTRUCTOR, #K_FUNCTION,
 * #K_OPERATOR, or #K_UDEF_LIT \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_func_like_ast_english( c_ast_t const *ast,
                                     eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( (ast->kind & (K_APPLE_BLOCK | K_CONSTRUCTOR | K_DESTRUCTOR |
                        K_FUNCTION | K_OPERATOR | K_UDEF_LIT)) != 0 );
  assert( eng != NULL );

  c_type_name_nobase_english( &ast->type, eng->fout );
  switch ( ast->kind ) {
    case K_FUNCTION:
      if ( c_tid_is_any( ast->type.stids, TS_MEMBER_FUNC_ONLY ) )
        FPUTS( "member ", eng->fout );
      break;
    case K_OPERATOR:
      NO_OP;
      c_func_member_t const op_mbr = c_ast_op_overload( ast );
      char const *const op_literal =
        op_mbr == C_FUNC_MEMBER     ? "member "     :
        op_mbr == C_FUNC_NON_MEMBER ? "non-member " :
        "";
      FPUTS( op_literal, eng->fout );
      break;
    default:
      /* suppress warning */;
  } // switch

  FPUTS( c_kind_name( ast->kind ), eng->fout );
  if ( !slist_empty( &ast->func.param_ast_list ) ) {
    FPUTC( ' ', eng->fout );
    c_ast_func_params_english( ast, eng );
  }
  if ( ast->func.ret_ast != NULL )
    FPUTS( " returning ", eng->fout );
}

/**
 * Prints a #K_NAME \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_name_ast_english( c_ast_t const *ast, eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_NAME );
  assert( eng != NULL );

  if ( !opt_permissive_types && OPT_LANG_IS( PROTOTYPES ) &&
        ast->param_of_ast != NULL ) {
    //
    // A name can occur as an untyped K&R C function parameter.  In C89-C17,
    // it's implicitly int:
    //
    //      cdecl> explain char f(x)
    //      declare f as function (x as integer) returning char
    //
    FPUTS( c_tid_english( TB_int ), eng->fout );
  }
  else {
    c_sname_english( &ast->name.sname, eng->fout );
  }
}

/**
 * Prints a #K_LAMBDA \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_lambda_ast_english( c_ast_t const *ast, eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_LAMBDA );
  assert( eng != NULL );

  if ( !c_type_is_none( &ast->type ) )
    FPRINTF( eng->fout, "%s ", c_type_english( &ast->type ) );
  FPUTS( L_lambda, eng->fout );
  if ( !slist_empty( &ast->lambda.capture_ast_list ) ) {
    FPUTS( " capturing ", eng->fout );
    c_ast_lambda_captures_english( ast, eng->fout );
  }
  if ( !slist_empty( &ast->lambda.param_ast_list ) ) {
    FPUTC( ' ', eng->fout );
    c_ast_func_params_english( ast, eng );
  }
  if ( ast->lambda.ret_ast != NULL )
    FPUTS( " returning ", eng->fout );
}

/**
 * Prints a #K_POINTER_TO_MEMBER \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_ptr_mbr_ast_english( c_ast_t const *ast,
                                   eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_POINTER_TO_MEMBER );
  assert( eng != NULL );

  c_type_name_nobase_english( &ast->type, eng->fout );
  FPRINTF( eng->fout, "%s of ", c_kind_name( ast->kind ) );
  fputs_sp( c_tid_english( ast->type.btids ), eng->fout );
  c_sname_english( &ast->ptr_mbr.class_sname, eng->fout );
  FPUTC( ' ', eng->fout );
}

/**
 * Prints a #K_POINTER or #K_ANY_REFERENCE \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_ptr_ref_ast_english( c_ast_t const *ast,
                                   eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( (ast->kind & (K_POINTER | K_ANY_REFERENCE)) != 0 );
  assert( eng != NULL );

  c_type_name_nobase_english( &ast->type, eng->fout );
  FPRINTF( eng->fout, "%s to ", c_kind_name( ast->kind ) );
}

/**
 * Helper function for c_sname_english() that prints the scopes' types and
 * names in inner-to-outer order except for the inner-most scope.  For example,
 * `S::T::x` is printed as "of scope T of scope S."
 *
 * @param scope A pointer to the outermost scope.
 * @param fout The `FILE` to emit to.
 */
static void c_scope_english( c_scope_t const *scope, FILE *fout ) {
  assert( scope != NULL );
  assert( fout != NULL );

  if ( scope->next != NULL ) {
    c_scope_english( scope->next, fout );
    c_scope_data_t const *const data = c_scope_data( scope );
    FPRINTF( fout, " of %s %s", c_type_english( &data->type ), data->name );
  }
}

/**
 * Prints \a sname in pseudo-English.
 *
 * @param sname The name to print.
 * @param fout The `FILE` to print to.
 *
 * @note A newline is _not_ printed.
 *
 * @sa c_sname_gibberish()
 */
static void c_sname_english( c_sname_t const *sname, FILE *fout ) {
  assert( sname != NULL );
  assert( fout != NULL );

  if ( !c_sname_empty( sname ) ) {
    FPUTS( c_sname_local_name( sname ), fout );
    c_scope_english( sname->head, fout );
  }
}

/**
 * Prints a #K_STRUCTURED_BINDING \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_struct_bind_ast_english( c_ast_t const *ast,
                                       eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_STRUCTURED_BINDING );
  assert( eng != NULL );

  fputs_sp( c_tid_english( ast->type.stids ), eng->fout );
  if ( c_tid_is_any( ast->type.stids, TS_ANY_REFERENCE ) )
    FPUTS( "to ", eng->fout );
  FPUTS( c_kind_name( ast->kind ), eng->fout );
}

/**
 * Prints the non-base (attribute(s), storage class, qualifier(s), etc.) parts
 * of \a type, if any.
 *
 * @param type The type to perhaps print.
 * @param fout The `FILE` to emit to.
 */
static void c_type_name_nobase_english( c_type_t const *type, FILE *fout ) {
  assert( type != NULL );
  assert( fout != NULL );

  c_type_t const nobase_type = { TB_NONE, type->stids, type->atids };
  fputs_sp( c_type_english( &nobase_type ), fout );
}

/**
 * Prints a #K_TYPEDEF \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_typedef_ast_english( c_ast_t const *ast,
                                   eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_TYPEDEF );
  assert( eng != NULL );

  c_type_t type = ast->type;
  bool print_type = type.atids != TA_NONE || type.stids != TS_NONE;
  c_ast_t const *const raw_ast = c_ast_untypedef( ast );
  if ( c_tid_is_any( raw_ast->type.btids, opt_explicit_ecsu_btids ) ) {
    type.btids = raw_ast->type.btids;
    print_type = true;
  }
  if ( print_type )
    FPRINTF( eng->fout, "%s ", c_type_english( &type ) );
  c_sname_english( &ast->tdef.for_ast->sname, eng->fout );
  c_ast_bit_width_english( ast, eng->fout );
}

/**
 * Prints a #K_UDEF_CONV \ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_udef_conv_ast_english( c_ast_t const *ast,
                                     eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( ast->kind == K_UDEF_CONV );
  assert( eng != NULL );

  fputs_sp( c_type_english( &ast->type ), eng->fout );
  FPUTS( c_kind_name( ast->kind ), eng->fout );
  if ( !c_sname_empty( &ast->sname ) ) {
    FPRINTF( eng->fout,
      " of %s ", c_type_english( c_sname_local_type( &ast->sname ) )
    );
    c_sname_english( &ast->sname, eng->fout );
  }
  FPUTS( " returning ", eng->fout );
}

/**
 * Initializes an eng_state.
 *
 * @param eng The eng_state to initialize.
 * @param fout The `FILE` to print to.
 */
static void eng_init( eng_state_t *eng, FILE *fout ) {
  assert( eng != NULL );
  assert( fout != NULL );

  *eng = (eng_state_t){
    .fout = fout
  };
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_english( c_ast_t const *ast, decl_flags_t eng_flags, FILE *fout ) {
  assert( ast != NULL );
  assert( is_1n_bit_only_in_set( eng_flags, C_ENG_ANY ) );
  assert( (eng_flags & C_ENG_DECL) != 0 );
  assert( fout != NULL );

  if ( (eng_flags & C_ENG_OPT_OMIT_DECLARE) == 0 && ast->kind != K_CAST ) {
    FPUTS( "declare ", fout );

    // We can't just check to see if ast->sname is empty and print it only if
    // it isn't because operators have a name but don't use ast->sname.
    switch ( ast->kind ) {
      case K_APPLE_BLOCK:
      case K_ARRAY:
      case K_BUILTIN:
      case K_CLASS_STRUCT_UNION:
      case K_CONCEPT:
      case K_CONSTRUCTOR:
      case K_DESTRUCTOR:
      case K_ENUM:
      case K_FUNCTION:
      case K_NAME:
      case K_OPERATOR:
      case K_POINTER:
      case K_POINTER_TO_MEMBER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
      case K_TYPEDEF:
      case K_UDEF_LIT:
        c_ast_name_english( ast, fout );
        FPUTS( " as ", fout );
        break;

      case K_LAMBDA:
      case K_UDEF_CONV:
        break;                          // these don't have names

      case K_STRUCTURED_BINDING:
        FOREACH_SLIST_NODE( sname_node, &ast->struct_bind.sname_list ) {
          FPUTS( c_sname_local_name( sname_node->data ), fout );
          if ( sname_node->next != NULL )
            FPUTS( ", ", fout );
        } // for
        FPUTS( " as ", fout );
        break;

      case K_CAPTURE:
      case K_CAST:
      case K_VARIADIC:
        UNEXPECTED_INT_VALUE( ast->kind );

      case K_PLACEHOLDER:
        unreachable();
    } // switch
  }

  eng_state_t eng;
  eng_init( &eng, fout );
  c_ast_visit_english( ast, &eng );
  c_ast_alignas_english( ast, &eng );
}

void c_ast_list_english( c_ast_list_t const *ast_list, FILE *fout ) {
  assert( ast_list != NULL );

  switch ( slist_len( ast_list ) ) {
    case 1:
      NO_OP;
      c_ast_t const *const ast = slist_front( ast_list );
      c_ast_english( ast, C_ENG_DECL, fout );
      FPUTC( '\n', fout );
      FALLTHROUGH;
    case 0:
      return;
  } // switch

  slist_t declare_list;
  slist_init( &declare_list );

  //
  // We want to coalesce "like" declarations so that we print those having the
  // same base type together in the same "declare" statement.  For example,
  // given:
  //
  //      explain int i, a[2], *p, j, f(), b[2]
  //
  // we want:
  //
  //      declare i, j as integer
  //      declare a, b as array 2 of integer
  //      declare p as pointer to integer
  //      declare f as function returning integer
  //
  // To do this, first split the declarations into separate lists where each
  // list only contains ASTs that are all equal.
  //
  // However, we can coalesce only objects, functions, or operators; everything
  // else must get its own "declare" statement.
  //
  FOREACH_SLIST_NODE( ast_node, ast_list ) {
    c_ast_t *const list_ast = ast_node->data;
    slist_t *equal_ast_list = NULL;

    if ( (list_ast->kind & (K_ANY_OBJECT | K_FUNCTION | K_OPERATOR)) != 0 ) {
      FOREACH_SLIST_NODE( declare_node, &declare_list ) {
        slist_t *const maybe_equal_ast_list = declare_node->data;
        c_ast_t const *const ast = slist_front( maybe_equal_ast_list );
        if ( c_ast_equal( list_ast, ast ) ) {
          equal_ast_list = maybe_equal_ast_list;
          break;
        }
      } // for
    }

    if ( equal_ast_list == NULL ) {
      equal_ast_list = MALLOC( slist_t, 1 );
      slist_init( equal_ast_list );
      slist_push_back( &declare_list, equal_ast_list );
    }
    else {
      //
      // Even though we found an equal AST list, we have to look through it for
      // an AST having the same name.  (This can happen for "tentative
      // declarations" in C.)  If we find a match, don't add the AST to the
      // list because we want only a single "declare" statement for it:
      //
      //      cdecl> explain int x, x   // legal "tentative declaration" in C
      //      declare x as integer
      //
      bool found_equal_name = false;
      FOREACH_SLIST_NODE( equal_node, equal_ast_list ) {
        c_ast_t const *const equal_ast = equal_node->data;
        if ( c_sname_cmp( &list_ast->sname, &equal_ast->sname ) == 0 ) {
          found_equal_name = true;
          break;
        }
      } // for
      if ( found_equal_name )
        continue;
    }

    slist_push_back( equal_ast_list, list_ast );
  } // for

  //
  // Now print one "declare" statement for each list of equal declarations in
  // declare_list.
  //
  FOREACH_SLIST_NODE( declare_node, &declare_list ) {
    slist_t *const equal_ast_list = declare_node->data;
    //
    // First, print "declare" followed by the names of all the declarations
    // that have the same base type.
    //
    FPUTS( "declare ", fout );
    bool comma = false;
    FOREACH_SLIST_NODE( equal_node, equal_ast_list ) {
      c_ast_t const *const equal_ast = equal_node->data;
      fput_sep( ", ", &comma, fout );
      c_ast_name_english( equal_ast, fout );
    } // for

    //
    // Now print "as" followed by the type.
    //
    FPUTS( " as ", fout );
    c_ast_t const *const ast = slist_front( equal_ast_list );
    c_ast_english( ast, C_ENG_DECL | C_ENG_OPT_OMIT_DECLARE, fout );
    FPUTC( '\n', fout );
  } // for

  // Clean-up list and sub-lists.
  FOREACH_SLIST_NODE( declare_node, &declare_list ) {
    slist_t *const equal_ast_list = declare_node->data;
    slist_cleanup( equal_ast_list, /*free_fn=*/NULL );
  } // for
  slist_cleanup( &declare_list, &free );
}

char const* c_cast_english( c_cast_kind_t kind ) {
  switch ( kind ) {
    case C_CAST_C           : return "C";
    case C_CAST_CONST       : return L_const;
    case C_CAST_DYNAMIC     : return L_dynamic;
    case C_CAST_REINTERPRET : return L_reinterpret;
    case C_CAST_STATIC      : return L_static;
  } // switch
  UNEXPECTED_INT_VALUE( kind );
}

void c_typedef_english( c_typedef_t const *tdef, FILE *fout ) {
  assert( tdef != NULL );
  assert( tdef->ast != NULL );
  assert( fout != NULL );

  FPUTS( "define ", fout );
  c_sname_english( &tdef->ast->sname, fout );
  FPUTS( " as ", fout );

  eng_state_t eng;
  eng_init( &eng, fout );
  c_ast_visit_english( tdef->ast, &eng );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
