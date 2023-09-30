/*
**      cdecl -- C gibberish translator
**      src/english.c
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
 * Defines data structures and functions for printing in pseudo-English.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "english.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_operator.h"
#include "c_typedef.h"
#include "decl_flags.h"
#include "literals.h"
#include "slist.h"
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
  FILE           *eout;                 ///< Where to print the English.
  c_ast_t const  *func_ast;             ///< The current function AST, if any.
};
typedef struct eng_state eng_state_t;

// local functions
NODISCARD
static bool c_ast_visitor_english( c_ast_t*, user_data_t );

static void c_type_name_nobase_english( c_type_t const*, FILE* );
static void c_ast_visit_english( c_ast_t const*, eng_state_t const* );
static void eng_init( eng_state_t*, FILE* );

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
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_BIT_FIELD ) );
  assert( eout != NULL );

  if ( ast->bit_field.bit_width > 0 )
    FPRINTF( eout, " width %u bits", ast->bit_field.bit_width );
}

/**
 * Helper function for c_ast_visitor_english() that prints a function-like
 * AST's parameters, if any.
 *
 * @param ast The AST to print the parameters of.
 * @param eng The eng_state to use.
 */
static void c_ast_func_params_english( c_ast_t const *ast,
                                       eng_state_t const *eng ) {
  assert( ast != NULL );
  assert( is_1_bit_only_in_set( ast->kind, K_ANY_FUNCTION_LIKE ) );
  assert( eng != NULL );

  FPUTC( '(', eng->eout );

  eng_state_t param_eng;
  eng_init( &param_eng, eng->eout );
  param_eng.func_ast = ast;

  bool comma = false;
  FOREACH_AST_FUNC_PARAM( param, ast ) {
    fput_sep( ", ", &comma, eng->eout );

    c_ast_t const *const param_ast = c_param_ast( param );
    c_sname_t const *const sname = c_ast_find_name( param_ast, C_VISIT_DOWN );
    if ( sname != NULL ) {
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
      c_sname_english( sname, eng->eout );
      if ( param_ast->kind != K_NAME )
        FPUTS( " as ", eng->eout );
    }
    else {
      //
      // If there's no name, it's an unnamed parameter, e.g.:
      //
      //      void f(int)
      //
      // so there's no "<name> as" part.
      //
    }

    c_ast_visit_english( param_ast, &param_eng );
  } // for

  FPUTC( ')', eng->eout );
}

/**
 * Helper function for c_ast_visitor_english() that prints a lambda AST's
 * captures, if any.
 *
 * @param ast The lambda AST to print the captures of.
 * @param eout The `FILE` to emit to.
 */
static void c_ast_lambda_captures_english( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( ast->kind == K_LAMBDA );
  assert( eout != NULL );

  FPUTC( '[', eout );

  bool comma = false;
  FOREACH_AST_LAMBDA_CAPTURE( capture, ast ) {
    fput_sep( ", ", &comma, eout );

    c_ast_t const *const capture_ast = c_capture_ast( capture );
    switch ( capture_ast->capture.kind ) {
      case C_CAPTURE_COPY:
        FPUTS( "copy by default", eout );
        break;
      case C_CAPTURE_REFERENCE:
        if ( c_sname_empty( &capture_ast->sname ) ) {
          FPUTS( "reference by default", eout );
          break;
        }
        FPUTS( "reference to ", eout );
        FALLTHROUGH;
      case C_CAPTURE_VARIABLE:
        c_sname_english( &capture_ast->sname, eout );
        break;
      case C_CAPTURE_STAR_THIS:
        FPUTC( '*', eout );
        FALLTHROUGH;
      case C_CAPTURE_THIS:
        FPUTS( L_this, eout );
        break;
    } // switch
  } // for

  FPUTC( ']', eout );
}

/**
 * Prints the scoped name of \a AST in pseudo-English.
 *
 * @param ast The AST to print the name of.
 * @param eout The `FILE` to emit to.
 */
static void c_ast_name_english( c_ast_t const *ast, FILE *eout ) {
  assert( ast != NULL );
  assert( eout != NULL );

  c_sname_t const *const found_sname = c_ast_find_name( ast, C_VISIT_DOWN );
  char const *local_name = "", *scope_name = "";
  c_type_t const *scope_type = NULL;

  if ( ast->kind == K_OPERATOR ) {
    local_name = c_oper_token_c( ast->oper.operator->oper_id );
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

  assert( local_name[0] != '\0' );
  FPUTS( local_name, eout );
  if ( scope_name[0] != '\0' ) {
    assert( !c_type_is_none( scope_type ) );
    FPRINTF( eout, " of %s %s", c_type_name_english( scope_type ), scope_name );
  }
}

/**
 * Convenience function for calling c_ast_visit() to print \a ast as a
 * declaration in pseudo-English.
 *
 * @param ast The AST to print.
 * @param eng The eng_state to use.
 */
static void c_ast_visit_english( c_ast_t const *ast, eng_state_t const *eng ) {
  c_ast_visit(
    CONST_CAST( c_ast_t*, ast ), C_VISIT_DOWN, c_ast_visitor_english,
    (user_data_t){ .pc = eng }
  );
}

/**
 * Visitor function that prints \a ast as pseudo-English.
 *
 * @param ast The AST to print.
 * @param user_data A pointer to a `FILE` to emit to.
 * @return Always returns `false`.
 */
NODISCARD
static bool c_ast_visitor_english( c_ast_t *ast, user_data_t user_data ) {
  assert( ast != NULL );
  eng_state_t const *const eng = user_data.pc;
  assert( eng != NULL );

  switch ( ast->kind ) {
    case K_ARRAY:
      c_type_name_nobase_english( &ast->type, eng->eout );
      switch ( ast->array.kind ) {
        case C_ARRAY_NAMED_SIZE:
          //
          // Just because an array has named size doesn't mean it's a VLA.
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

        case C_ARRAY_VLA_STAR:
          FPUTS( "variable length ", eng->eout );
          break;

        case C_ARRAY_EMPTY_SIZE:
        case C_ARRAY_INT_SIZE:
          break;
      } // switch
      FPUTS( "array ", eng->eout );
      switch ( ast->array.kind ) {
        case C_ARRAY_INT_SIZE:
          FPRINTF( eng->eout, "%u ", ast->array.size_int );
          break;
        case C_ARRAY_NAMED_SIZE:
          FPRINTF( eng->eout, "%s ", ast->array.size_name );
          break;
        case C_ARRAY_EMPTY_SIZE:
        case C_ARRAY_VLA_STAR:
          break;
      } // switch
      FPUTS( "of ", eng->eout );
      break;

    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_DESTRUCTOR:
    case K_FUNCTION:
    case K_OPERATOR:
    case K_UDEF_LIT:
      c_type_name_nobase_english( &ast->type, eng->eout );
      switch ( ast->kind ) {
        case K_FUNCTION:
          if ( c_tid_is_any( ast->type.stids, TS_MEMBER_FUNC_ONLY ) )
            FPUTS( "member ", eng->eout );
          break;
        case K_OPERATOR:
          NO_OP;
          c_func_member_t const op_mbr = c_ast_oper_overload( ast );
          char const *const op_literal =
            op_mbr == C_FUNC_MEMBER     ? "member "     :
            op_mbr == C_FUNC_NON_MEMBER ? "non-member " :
            "";
          FPUTS( op_literal, eng->eout );
          break;
        default:
          /* suppress warning */;
      } // switch

      FPUTS( c_kind_name( ast->kind ), eng->eout );
      if ( c_ast_params_count( ast ) > 0 ) {
        FPUTC( ' ', eng->eout );
        c_ast_func_params_english( ast, eng );
      }
      if ( ast->func.ret_ast != NULL )
        FPUTS( " returning ", eng->eout );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name_english( &ast->type ), eng->eout );
      if ( c_ast_is_tid_any( ast, TB__BitInt ) )
        FPRINTF( eng->eout, " width %u bits", ast->builtin.BitInt.width );
      c_ast_bit_width_english( ast, eng->eout );
      break;

    case K_CAPTURE:
      // A K_CAPTURE can occur only in a lambda capture list, not at the top-
      // level, and captures are never visited.
      UNEXPECTED_INT_VALUE( ast->kind );

    case K_CAST:
      if ( ast->cast.kind != C_CAST_C )
        FPRINTF( eng->eout, "%s ", c_cast_english( ast->cast.kind ) );
      FPUTS( L_cast, eng->eout );
      if ( !c_sname_empty( &ast->sname ) ) {
        FPUTC( ' ', eng->eout );
        c_sname_english( &ast->sname, eng->eout );
      }
      FPUTS( " into ", eng->eout );
      break;

    case K_CLASS_STRUCT_UNION:
      FPRINTF( eng->eout, "%s ", c_type_name_english( &ast->type ) );
      c_sname_english( &ast->csu.csu_sname, eng->eout );
      break;

    case K_ENUM:
      FPRINTF( eng->eout, "%s ", c_type_name_english( &ast->type ) );
      c_sname_english( &ast->enum_.enum_sname, eng->eout );
      if ( ast->enum_.of_ast != NULL )
        FPUTS( " of type ", eng->eout );
      else
        c_ast_bit_width_english( ast, eng->eout );
      break;

    case K_LAMBDA:
      if ( !c_type_is_none( &ast->type ) )
        FPRINTF( eng->eout, "%s ", c_type_name_english( &ast->type ) );
      FPUTS( L_lambda, eng->eout );
      if ( c_ast_captures_count( ast ) > 0 ) {
        FPUTS( " capturing ", eng->eout );
        c_ast_lambda_captures_english( ast, eng->eout );
      }
      if ( c_ast_params_count( ast ) > 0 ) {
        FPUTC( ' ', eng->eout );
        c_ast_func_params_english( ast, eng );
      }
      if ( ast->lambda.ret_ast != NULL )
        FPUTS( " returning ", eng->eout );
      break;

    case K_NAME:
      // A K_NAME can occur only as an untyped K&R C function parameter.  The
      // name was printed in c_ast_func_params_english() so we don't have to do
      // anything here.
      break;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      c_type_name_nobase_english( &ast->type, eng->eout );
      FPRINTF( eng->eout, "%s to ", c_kind_name( ast->kind ) );
      break;

    case K_POINTER_TO_MEMBER:
      c_type_name_nobase_english( &ast->type, eng->eout );
      FPRINTF( eng->eout, "%s of ", c_kind_name( ast->kind ) );
      fputs_sp( c_tid_name_english( ast->type.btids ), eng->eout );
      c_sname_english( &ast->ptr_mbr.class_sname, eng->eout );
      FPUTC( ' ', eng->eout );
      break;

    case K_TYPEDEF:
      if ( !c_type_equiv( &ast->type, &C_TYPE_LIT_B( TB_typedef ) ) )
        FPRINTF( eng->eout, "%s ", c_type_name_english( &ast->type ) );
      c_sname_english( &ast->tdef.for_ast->sname, eng->eout );
      c_ast_bit_width_english( ast, eng->eout );
      break;

    case K_UDEF_CONV:
      fputs_sp( c_type_name_english( &ast->type ), eng->eout );
      FPUTS( c_kind_name( ast->kind ), eng->eout );
      if ( !c_sname_empty( &ast->sname ) ) {
        FPRINTF( eng->eout,
          " of %s ", c_type_name_english( c_sname_local_type( &ast->sname ) )
        );
        c_sname_english( &ast->sname, eng->eout );
      }
      FPUTS( " returning ", eng->eout );
      break;

    case K_VARIADIC:
      FPUTS( c_kind_name( ast->kind ), eng->eout );
      break;

    case K_PLACEHOLDER:
      unreachable();
  } // switch

  return /*stop=*/false;
}

/**
 * Helper function for c_sname_english() that prints the scopes' types and
 * names in inner-to-outer order except for the inner-most scope.  For example,
 * `S::T::x` is printed as "of scope T of scope S."
 *
 * @param scope A pointer to the outermost scope.
 * @param eout The `FILE` to emit to.
 */
static void c_scope_english( c_scope_t const *scope, FILE *eout ) {
  assert( scope != NULL );
  assert( eout != NULL );

  if ( scope->next != NULL ) {
    c_scope_english( scope->next, eout );
    c_scope_data_t const *const data = c_scope_data( scope );
    FPRINTF( eout,
      " of %s %s", c_type_name_english( &data->type ), data->name
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
static void c_type_name_nobase_english( c_type_t const *type, FILE *eout ) {
  assert( type != NULL );
  assert( eout != NULL );

  c_type_t const nobase_type = { TB_NONE, type->stids, type->atids };
  fputs_sp( c_type_name_english( &nobase_type ), eout );
}

/**
 * Initializes an eng_state.
 *
 * @param eng The eng_state to initialize.
 * @param eout The `FILE` to print to.
 */
static void eng_init( eng_state_t *eng, FILE *eout ) {
  assert( eng != NULL );
  assert( eout != NULL );

  MEM_ZERO( eng );
  eng->eout = eout;
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_english( c_ast_t const *ast, unsigned eng_flags, FILE *eout ) {
  assert( ast != NULL );
  assert( is_1n_bit_only_in_set( eng_flags, C_ENG_ANY ) );
  assert( (eng_flags & C_ENG_DECL) != 0 );
  assert( eout != NULL );

  if ( (eng_flags & C_ENG_OPT_OMIT_DECLARE) == 0 && ast->kind != K_CAST ) {
    FPUTS( "declare ", eout );
    // We can't just check to see if ast->sname is empty and print it only if
    // it isn't because operators have a name but don't use ast->sname.
    switch ( ast->kind ) {
      case K_LAMBDA:
      case K_UDEF_CONV:
        break;                          // these don't have names
      default:
        c_ast_name_english( ast, eout );
        FPUTS( " as ", eout );
    } // switch
  }

  eng_state_t eng;
  eng_init( &eng, eout );
  c_ast_visit_english( ast, &eng );

  switch ( ast->align.kind ) {
    case C_ALIGNAS_NONE:
      break;
    case C_ALIGNAS_BYTES:
      if ( ast->align.bytes > 0 )
        FPRINTF( eout, " aligned as %u bytes", ast->align.bytes );
      break;
    case C_ALIGNAS_TYPE:
      FPUTS( " aligned as ", eout );
      c_ast_visit_english( ast->align.type_ast, &eng );
      break;
  } // switch
}

void c_ast_list_english( c_ast_list_t const *ast_list, FILE *eout ) {
  assert( ast_list != NULL );

  switch ( slist_len( ast_list ) ) {
    case 1:
      NO_OP;
      c_ast_t const *const ast = slist_front( ast_list );
      c_ast_english( ast, C_ENG_DECL, eout );
      FPUTC( '\n', eout );
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
    c_ast_t const *const list_ast = ast_node->data;
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

    slist_push_back( equal_ast_list, CONST_CAST( c_ast_t*, list_ast ) );
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
    FPUTS( "declare ", eout );
    bool comma = false;
    FOREACH_SLIST_NODE( equal_node, equal_ast_list ) {
      c_ast_t const *const equal_ast = equal_node->data;
      fput_sep( ", ", &comma, eout );
      c_ast_name_english( equal_ast, eout );
    } // for

    //
    // Now print "as" followed by the type.
    //
    FPUTS( " as ", eout );
    c_ast_t const *const ast = slist_front( equal_ast_list );
    c_ast_english( ast, C_ENG_DECL | C_ENG_OPT_OMIT_DECLARE, eout );
    FPUTC( '\n', eout );
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

void c_sname_english( c_sname_t const *sname, FILE *eout ) {
  assert( sname != NULL );
  assert( eout != NULL );

  if ( !c_sname_empty( sname ) ) {
    FPUTS( c_sname_local_name( sname ), eout );
    c_scope_english( sname->head, eout );
  }
}

void c_typedef_english( c_typedef_t const *tdef, FILE *eout ) {
  assert( tdef != NULL );
  assert( tdef->ast != NULL );
  assert( eout != NULL );

  FPUTS( "define ", eout );
  c_sname_english( &tdef->ast->sname, eout );
  FPUTS( " as ", eout );

  eng_state_t eng;
  eng_init( &eng, eout );
  c_ast_visit_english( tdef->ast, &eng );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
