/*
**      cdecl -- C gibberish translator
**      src/debug.c
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

// local
#include "config.h"                     /* must go first */
#include "ast.h"
#include "debug.h"
#include "util.h"

// system
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

#define PRINT_COMMA \
  BLOCK( if ( !comma ) { FPUTS( ",\n", dout ); comma = true; } )

#define INDENT_PRINT(...) \
  BLOCK( print_indent( indent, dout ); FPRINTF( dout, __VA_ARGS__ ); )

#define INDENT_PRINT_KV(KEY,VALUE) \
  BLOCK( print_indent( indent, dout ); print_kv( (KEY), (VALUE), dout ); )

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a multiple of \a indent spaces.
 *
 * @param indent How much to indent.
 * @param out The FILE to print to.
 */
static void print_indent( unsigned indent, FILE *out ) {
  FPRINTF( out, "%*s", (int)(indent * DEBUG_INDENT), "" );
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_debug( c_ast_t const *ast, unsigned indent, char const *key0,
                 FILE *dout ) {
  if ( key0 && *key0 )
    INDENT_PRINT( "%s = {\n", key0 );
  else
    INDENT_PRINT( "{\n" );

  if ( ast != NULL ) {
    ++indent;

    INDENT_PRINT( "id = %u,\n", ast->id );
    INDENT_PRINT_KV( "kind", c_kind_name( ast->kind ) );
    FPUTS( ",\n", dout );
    INDENT_PRINT( "depth = %u,\n", ast->depth );
    INDENT_PRINT_KV( "name", ast->name );
    FPUTS( ",\n", dout );
    INDENT_PRINT(
      "parent->id = %d,\n", ast->parent ? (int)ast->parent->id : -1
    );
    INDENT_PRINT_KV( "type", c_type_name( ast->type ) );

    bool comma = false;

    switch ( ast->kind ) {
      case K_BUILTIN:
      case K_NAME:
      case K_NONE:
      case K_VARIADIC:
        // nothing to do
        break;

      case K_ARRAY:
        PRINT_COMMA;
        INDENT_PRINT( "size = %d,\n", ast->as.array.size );
        c_ast_debug( ast->as.array.of_ast, indent, "of_ast", dout );
        break;

      case K_BLOCK:                     // Apple extension
      case K_FUNCTION:
        PRINT_COMMA;
        INDENT_PRINT( "args = " );
        c_ast_list_debug( &ast->as.func.args, indent, dout );
        FPUTS( ",\n", dout );
        c_ast_debug( ast->as.func.ret_ast, indent, "ret_ast", dout );
        break;

      case K_ENUM_CLASS_STRUCT_UNION:
        PRINT_COMMA;
        INDENT_PRINT_KV( "ecsu_name", ast->as.ecsu.ecsu_name );
        break;

      case K_POINTER_TO_MEMBER:
        PRINT_COMMA;
        INDENT_PRINT_KV( "class_name", ast->as.ptr_mbr.class_name );
        FPUTC( '\n', dout );
        // no break;

      case K_POINTER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        PRINT_COMMA;
        INDENT_PRINT_KV( "qualifier", c_type_name( ast->as.ptr_ref.qualifier ) );
        FPUTS( ",\n", dout );
        c_ast_debug( ast->as.ptr_ref.to_ast, indent, "to_ast", dout );
        break;
    } // switch

    FPUTC( '\n', dout );
    --indent;
  }

  INDENT_PRINT( "}" );
}

void c_ast_list_debug( c_ast_list_t const *list, unsigned indent, FILE *dout ) {
  assert( list != NULL );
  if ( list->head_ast != NULL ) {
    FPUTS( "[\n", dout );
    bool comma = false;
    for ( c_ast_t const *arg = list->head_ast; arg; arg = arg->next ) {
      if ( true_or_set( &comma ) )
        FPUTS( ",\n", dout );
      c_ast_debug( arg, indent + 1, NULL, dout );
    } // for
    FPUTC( '\n', dout );
    INDENT_PRINT( "]" );
  } else {
    FPUTS( "[]", dout );
  }
}

void print_kv( char const *key, char const *value, FILE *dout ) {
  assert( key != NULL );
  if ( value && *value )
    FPRINTF( dout, "%s = \"%s\"", key, value );
  else
    FPRINTF( dout, "%s = null", key );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
