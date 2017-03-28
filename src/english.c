/*
**      cdecl -- C gibberish translator
**      src/english.c
**
**      Paul J. Lucas
*/

/**
 * @file
 * Contains the code for printing an AST as English.
 */

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "ast_util.h"
#include "literals.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

////////// extern functions ///////////////////////////////////////////////////

/**
 * Prints the given AST as English.
 *
 * @param ast The AST to print.  May be null.
 * @param print_names Print names only if \c true.  (This is set to \c false
 * for function arguments.
 * @param eout The FILE to print to.
 */
static void c_ast_english_impl( c_ast_t const *ast, bool print_names,
                                FILE *eout ) {
  if ( ast == NULL )
    return;

  bool comma = false;

  switch ( ast->kind ) {
    case K_ARRAY:
      FPRINTF( eout, "%s ", L_ARRAY );
      if ( ast->as.array.size != C_ARRAY_NO_SIZE )
        FPRINTF( eout, "%d ", ast->as.array.size );
      FPRINTF( eout, "%s ", L_OF );
      c_ast_english_impl( ast->as.array.of_ast, print_names, eout );
      break;

    case K_BLOCK:                       // Apple extension
    case K_FUNCTION:
      if ( ast->type )
        FPRINTF( eout, "%s ", c_type_name( ast->type ) );
      FPUTS( c_kind_name( ast->kind ), eout );

      if ( c_ast_args( ast ) ) {
        FPUTS( " (", eout );

        for ( c_ast_t const *arg = c_ast_args( ast ); arg; arg = arg->next ) {
          if ( true_or_set( &comma ) )
            FPUTS( ", ", eout );

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
            if ( name )
              FPRINTF( eout, "%s as ", name );
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
          // here.
          //
          c_ast_english_impl( arg, false, eout );
        } // for

        FPUTC( ')', eout );
      }

      FPRINTF( eout, " %s ", L_RETURNING );
      c_ast_english_impl( ast->as.func.ret_ast, print_names, eout );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->type ), eout );
      if ( print_names && ast->name )
        FPRINTF( eout, " %s", ast->name );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( eout,
        "%s %s",
        c_type_name( ast->type ), ast->as.ecsu.ecsu_name
      );
      if ( print_names && ast->name )
        FPRINTF( eout, " %s", ast->name );
      break;

    case K_NAME:
      if ( ast->name )                  // do NOT check print_names
        FPUTS( ast->name, eout );
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_POINTER:
    case K_REFERENCE:
      if ( ast->as.ptr_ref.qualifier )
        FPRINTF( eout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
      FPRINTF( eout, "%s %s ", c_kind_name( ast->kind ), L_TO );
      c_ast_english_impl( ast->as.ptr_ref.to_ast, print_names, eout );
      break;

    case K_POINTER_TO_MEMBER: {
      if ( ast->as.ptr_mbr.qualifier )
        FPRINTF( eout, "%s ", c_type_name( ast->as.ptr_mbr.qualifier ) );
      FPRINTF( eout, "%s %s %s %s ", L_POINTER, L_TO, L_MEMBER, L_OF );
      char const *const type_name = c_type_name( ast->type );
      if ( *type_name )
        FPRINTF( eout, "%s ", type_name );
      FPRINTF( eout, "%s ", ast->as.ptr_mbr.class_name );
      c_ast_english_impl( ast->as.ptr_mbr.of_ast, print_names, eout );
      break;
    }

    case K_VARIADIC:
      FPUTS( c_kind_name( ast->kind ), eout );
      break;
  } // switch
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_english( c_ast_t const *ast, FILE *eout ) {
  c_ast_english_impl( ast, true, eout );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
