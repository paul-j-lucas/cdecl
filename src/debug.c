/*
**      cdecl -- C gibberish translator
**      src/debug.c
*/

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

#define PRINT_COMMA \
  BLOCK( if ( !comma ) { FPUTS( ",\n", jout ); comma = true; } )

#define PRINT_JSON(...) \
  BLOCK( print_indent( indent, jout ); FPRINTF( jout, __VA_ARGS__ ); )

#define PRINT_JSON_KV(KEY,VALUE) \
  BLOCK( print_indent( indent, jout ); json_print_kv( (KEY), (VALUE), jout ); )

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a multiple of \a indent spaces.
 *
 * @param indent How much to indent.
 * @param out The FILE to print to.
 */
static void print_indent( unsigned indent, FILE *out ) {
  FPRINTF( out, "%*s", (int)(indent * JSON_INDENT), "" );
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_json( c_ast_t const *ast, unsigned indent, char const *key0,
                 FILE *jout ) {
  if ( key0 && *key0 )
    PRINT_JSON( "\"%s\": {\n", key0 );
  else
    PRINT_JSON( "{\n" );

  if ( ast != NULL ) {
    ++indent;

    PRINT_JSON_KV( "kind", c_kind_name( ast->kind ) );
    FPUTS( ",\n", jout );
    PRINT_JSON_KV(
      "parent->kind", ast->parent ? c_kind_name( ast->parent->kind ) : NULL
    );
    FPUTS( ",\n", jout );
    PRINT_JSON_KV( "name", ast->name );
    FPUTS( ",\n", jout );
    PRINT_JSON_KV( "type", c_type_name( ast->type ) );

    bool comma = false;

    switch ( ast->kind ) {
      case K_BUILTIN:
      case K_NAME:
      case K_NONE:
        // nothing to do
        break;

      case K_ARRAY:
        PRINT_COMMA;
        PRINT_JSON( "\"size\": %d,\n", ast->as.array.size );
        c_ast_json( ast->as.array.of_ast, indent, "of_ast", jout );
        break;

      case K_BLOCK:                     // Apple extension
      case K_FUNCTION:
        PRINT_COMMA;
        PRINT_JSON( "\"args\": " );
        c_ast_list_json( &ast->as.func.args, indent, jout );
        FPUTS( ",\n", jout );
        c_ast_json( ast->as.func.ret_ast, indent, "ret_ast", jout );
        break;

      case K_ENUM_CLASS_STRUCT_UNION:
        PRINT_COMMA;
        PRINT_JSON_KV( "ecsu_name", ast->as.ecsu.ecsu_name );
        break;

      case K_POINTER_TO_MEMBER:
        PRINT_COMMA;
        PRINT_JSON_KV( "class_name", ast->as.ptr_mbr.class_name );
        FPUTC( '\n', jout );
        // no break;

      case K_POINTER:
      case K_REFERENCE:
        PRINT_COMMA;
        PRINT_JSON_KV( "qualifier", c_type_name( ast->as.ptr_ref.qualifier ) );
        FPRINTF( jout, ",\n" );
        c_ast_json( ast->as.ptr_ref.to_ast, indent, "to_ast", jout );
        break;
    } // switch

    FPUTC( '\n', jout );
    --indent;
  }

  PRINT_JSON( "}" );
}

void c_ast_list_json( c_ast_list_t const *list, unsigned indent, FILE *jout ) {
  assert( list );
  if ( list->head_ast != NULL ) {
    FPUTS( "[\n", jout );
    bool comma = false;
    for ( c_ast_t const *arg = list->head_ast; arg; arg = arg->next ) {
      if ( true_or_set( &comma ) )
        FPUTS( ",\n", jout );
      c_ast_json( arg, indent + 1, NULL, jout );
    } // for
    FPUTC( '\n', jout );
    PRINT_JSON( "]" );
  } else {
    FPUTS( "[]", jout );
  }
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
