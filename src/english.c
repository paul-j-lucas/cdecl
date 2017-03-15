/*
**      cdecl -- C gibberish translator
**      src/english.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "literals.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

////////// extern functions ///////////////////////////////////////////////////

void c_ast_english( c_ast_t const *ast, FILE *eout ) {
  if ( ast == NULL )
    return;

  bool comma = false;

  switch ( ast->kind ) {
    case K_ARRAY:
      FPRINTF( eout, "%s ", L_ARRAY );
      if ( ast->as.array.size != C_ARRAY_NO_SIZE )
        FPRINTF( eout, "%d ", ast->as.array.size );
      FPRINTF( eout, "%s ", L_OF );
      c_ast_english( ast->as.array.of_ast, eout );
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
          c_ast_english( arg, eout );
        } // for
        FPUTC( ')', eout );
      }
      FPRINTF( eout, " %s ", L_RETURNING );
      c_ast_english( ast->as.func.ret_ast, eout );
      break;

    case K_BUILTIN:
      FPUTS( c_type_name( ast->type ), eout );
      if ( ast->name )
        FPRINTF( eout, " %s", ast->name );
      break;

    case K_ENUM_CLASS_STRUCT_UNION:
      FPRINTF( eout,
        "%s %s",
        c_type_name( ast->type ), ast->as.ecsu.ecsu_name
      );
      if ( ast->name )
        FPRINTF( eout, " %s", ast->name );
      break;

    case K_NAME:
      if ( ast->name )
        FPUTS( ast->name, eout );
      break;

    case K_NONE:
      assert( ast->kind != K_NONE );

    case K_POINTER:
    case K_REFERENCE:
      if ( ast->as.ptr_ref.qualifier )
        FPRINTF( eout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
      FPRINTF( eout, "%s %s ", c_kind_name( ast->kind ), L_TO );
      c_ast_english( ast->as.ptr_ref.to_ast, eout );
      break;

    case K_POINTER_TO_MEMBER: {
      if ( ast->as.ptr_mbr.qualifier )
        FPRINTF( eout, "%s ", c_type_name( ast->as.ptr_mbr.qualifier ) );
      FPRINTF( eout, "%s %s %s %s ", L_POINTER, L_TO, L_MEMBER, L_OF );
      char const *const type_name = c_type_name( ast->type );
      if ( *type_name )
        FPRINTF( eout, "%s ", type_name );
      FPRINTF( eout, "%s ", ast->as.ptr_mbr.class_name );
      c_ast_english( ast->as.ptr_mbr.of_ast, eout );
      break;
    }
  } // switch
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
