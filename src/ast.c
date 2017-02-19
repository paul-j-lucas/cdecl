/*
**      cdecl -- C gibberish translator
**      src/ast.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "literals.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <string.h>                     /* for memset(3) */

////////// extern functions ///////////////////////////////////////////////////

#if 0
void c_ast_check( c_ast_t *ast ) {
  assert( ast );

  switch ( ast->kind ) {
    case K_NONE:
      break;

    case K_ARRAY: {
      c_ast const *const of_ast = ast->as.array.of_ast;
      switch ( of_ast.kind ) {
        case K_BUILTIN:
          if ( of_ast.type & T_VOID )
            /* complain */;
          break;
        case K_FUNCTION:
          /* complain */;
          break;
      } // switch
      break;
    }

    case K_BLOCK:
      // TODO
      break;
    case K_BUILTIN:
      // nothing to do
      break;
    case K_FUNCTION:
      for ( c_ast_t *arg = ast->as.func.args.head; arg; arg = arg->next )
        /* TODO */;
      break;
    case K_MEMBER:
      break;
    case K_NAME:
      break;
    case K_POINTER:
    case K_REFERENCE:
      break;
    case K_STRUCT:
      // TODO
      break;
  } // switch
}
#endif

void c_ast_english( c_ast_t const *ast, FILE *fout ) {
  assert( ast );

  switch ( ast->kind ) {
    case K_NONE:
      break;

    case K_ARRAY:
      FPRINTF( fout, "%s ", L_ARRAY );
      if ( ast->as.array.size != C_ARRAY_NO_SIZE )
        FPRINTF( fout, "%d ", ast->as.array.size );
      FPRINTF( fout, "%s ", L_OF );
      c_ast_english( ast->as.member.of_ast, fout );
      break;

    case K_BLOCK:
      // TODO

    case K_BUILTIN:
      FPUTS( c_type_name( ast->as.type ), fout );
      break;

    case K_FUNCTION:
      FPRINTF( fout, "%s (", L_FUNCTION );
      for ( c_ast_t *arg = ast->as.func.args.head; arg; arg = arg->next )
        c_ast_english( arg, fout );
      FPRINTF( fout, ") %s ", L_RETURNING );
      c_ast_english( ast->as.func.ret_ast, fout );
      break;

    case K_MEMBER:
      if ( ast->as.member.qualifier )
        FPRINTF( fout, "%s ", c_type_name( ast->as.member.qualifier ) );
      FPRINTF( fout,
        "%s %s %s %s %s %s ",
        L_POINTER, L_TO, L_MEMBER, L_OF, L_CLASS, ast->as.member.class_name
      );
      c_ast_english( ast->as.member.of_ast, fout );
      break;

    case K_NAME:
      if ( ast->as.name )
        FPUTS( ast->as.name, fout );
      break;

    case K_POINTER:
    case K_REFERENCE:
      if ( ast->as.ptr_ref.qualifier )
        FPRINTF( fout, "%s ", c_type_name( ast->as.ptr_ref.qualifier ) );
      FPRINTF( fout,
        "%s %s ", (ast->kind == K_POINTER ? L_POINTER : L_REFERENCE), L_TO
      );
      c_ast_english( ast->as.ptr_ref.to_ast, fout );
      break;

    case K_STRUCT:
      // TODO
      break;
  } // switch
}

void c_ast_free( c_ast_t *ast ) {
  assert( ast );

  switch ( ast->kind ) {
    case K_NONE:
      break;
    case K_ARRAY:
      if ( ast->as.array.of_ast )
        c_ast_free( ast->as.array.of_ast );
      break;
    case K_BLOCK:
      // TODO
      break;
    case K_BUILTIN:
      // nothing to do
      break;
    case K_FUNCTION:
      for ( c_ast_t *arg = ast->as.func.args.head; arg; ) {
        c_ast_t *const next = arg->next;
        c_ast_free( arg );
        arg = next;
      }
      break;
    case K_MEMBER:
      free( (void*)ast->as.member.class_name );
      c_ast_free( ast->as.member.of_ast );
      break;
    case K_NAME:
      free( (void*)ast->as.name );
      break;
    case K_POINTER:
    case K_REFERENCE:
      c_ast_free( ast->as.ptr_ref.to_ast );
      break;
    case K_STRUCT:
      // TODO
      break;
  } // switch
}

c_ast_t* c_ast_new( c_kind_t kind ) {
  c_ast_t *const ast = MALLOC( c_ast_t, 1 );
  memset( ast, 0, sizeof( c_ast_t ) );
  ast->kind = kind;
  return ast;
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
