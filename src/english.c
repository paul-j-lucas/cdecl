/*
**      cdecl -- C gibberish translator
**      src/english.c
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
 * Defines functions for printing an AST in pseudo-English.
 */

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "ast_util.h"
#include "literals.h"
#include "typedefs.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

///////////////////////////////////////////////////////////////////////////////

/**
 * Parameters used by c_ast_visitor_english().
 */
struct e_param {
  bool  print_names;                    // print names only if true
  FILE *eout;                           // where to write the pseudo-English
};
typedef struct e_param e_param_t;

////////// local functions ////////////////////////////////////////////////////

/**
 * Visitor function that prints an AST as pseudo-English.
 *
 * @param ast The AST to print.
 * @param data A pointer to an \c e_param \c struct.
 * @return Always returns \c false.
 */
static bool c_ast_visitor_english( c_ast_t *ast, void *data ) {
  e_param_t const *const param = REINTERPRET_CAST( e_param_t*, data );
  bool comma = false;

  switch ( ast->kind ) {
    case K_ARRAY:
      if ( ast->type != T_NONE )        // storage class
        FPRINTF( param->eout, "%s ", c_type_name( ast->type ) );
      if ( ast->as.array.size == C_ARRAY_SIZE_VARIABLE )
        FPRINTF( param->eout, "%s %s ", L_VARIABLE, L_LENGTH );
      FPRINTF( param->eout, "%s ", L_ARRAY );
      if ( ast->as.array.type != T_NONE )
        FPRINTF( param->eout, "%s ", c_type_name( ast->as.array.type ) );
      if ( ast->as.array.size >= 0 )
        FPRINTF( param->eout, "%d ", ast->as.array.size );
      FPRINTF( param->eout, "%s ", L_OF );
      break;

    case K_BLOCK:                       // Apple extension
    case K_FUNCTION:
      if ( ast->type != T_NONE )        // storage class
        FPRINTF( param->eout, "%s ", c_type_name( ast->type ) );
      FPUTS( c_kind_name( ast->kind ), param->eout );

      if ( c_ast_args( ast ) ) {        // print function arguments
        FPUTS( " (", param->eout );

        e_param_t arg_param;
        arg_param.print_names = false;
        arg_param.eout = param->eout;

        for ( c_ast_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
          if ( true_or_set( &comma ) )
            FPUTS( ", ", param->eout );

          if ( arg->kind != K_NAME ) {
            //
            // For all kinds except K_NAME, we have to print:
            //
            //      <name> as <english>
            //
            // For K_NAME, e.g.:
            //
            //      void f(x)           // untyped K&R C function argument
            //
            // there's no "as" part, so just let the K_NAME case below print
            // the name itself.
            //
            char const *const name = c_ast_name( arg, V_DOWN );
            if ( name != NULL )
              FPRINTF( param->eout, "%s %s ", name, L_AS );
            else {
              //
              // If there's no name, it's an unnamed argument, e.g.:
              //
              //    void f(int)
              //
              // so there's no "<name> as" part.
              //
            }
          }

          //
          // For all kinds except K_NAME, we'e already printed the name above,
          // so we have to suppress printing the name again; hence, pass false
          // for print_names here.
          //
          c_ast_t *const nonconst_arg = CONST_CAST( c_ast_t*, arg );
          c_ast_visit(
            nonconst_arg, V_DOWN, c_ast_visitor_english, &arg_param
          );
        } // for

        FPUTC( ')', param->eout );
      }

      FPRINTF( param->eout, " %s ", L_RETURNING );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->type ), param->eout );
      if ( param->print_names && ast->name != NULL )
        FPRINTF( param->eout, " %s", ast->name );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( param->eout,
        "%s %s",
        c_type_name( ast->type ), ast->as.ecsu.ecsu_name
      );
      if ( param->print_names && ast->name != NULL )
        FPRINTF( param->eout, " %s", ast->name );
      break;

    case K_NAME:
      if ( ast->name != NULL )          // do NOT check print_names
        FPUTS( ast->name, param->eout );
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );
    case K_PLACEHOLDER:
      assert( ast->kind != K_PLACEHOLDER );

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE: {
      c_type_t const qualifier = (ast->type & T_MASK_QUALIFIER);
      if ( qualifier != T_NONE )
        FPRINTF( param->eout, "%s ", c_type_name( qualifier ) );
      FPRINTF( param->eout, "%s %s ", c_kind_name( ast->kind ), L_TO );
      break;
    }

    case K_POINTER_TO_MEMBER: {
      c_type_t const qualifier = (ast->type & T_MASK_QUALIFIER);
      if ( qualifier != T_NONE )
        FPRINTF( param->eout, "%s ", c_type_name( qualifier ) );
      FPRINTF( param->eout, "%s %s %s %s ", L_POINTER, L_TO, L_MEMBER, L_OF );
      char const *const name = c_type_name( ast->type & ~T_MASK_QUALIFIER );
      if ( *name != '\0' )
        FPRINTF( param->eout, "%s ", name );
      FPRINTF( param->eout, "%s ", ast->as.ptr_mbr.class_name );
      break;
    }

    case K_TYPEDEF:
      FPUTS( ast->as.c_typedef->type_name, param->eout );
      break;

    case K_VARIADIC:
      FPUTS( c_kind_name( ast->kind ), param->eout );
      break;
  } // switch

  return false;
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_english( c_ast_t const *ast, FILE *eout ) {
  c_ast_t *const nonconst_ast = CONST_CAST( c_ast_t*, ast );
  e_param_t param;
  param.print_names = true;
  param.eout = eout;
  c_ast_visit( nonconst_ast, V_DOWN, c_ast_visitor_english, &param );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
