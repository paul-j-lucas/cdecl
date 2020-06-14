/*
**      cdecl -- C gibberish translator
**      src/debug.c
**
**      Copyright (C) 2017-2020  Paul J. Lucas, et al.
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
 * Defines functions for printing abstract syntax trees for debugging.
 */

// local
#include "cdecl.h"                      /* must go first */
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_type.h"
#include "c_typedef.h"
#include "debug.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

#define PRINT_COMMA \
  BLOCK( if ( !comma ) { FPUTS( ",\n", dout ); comma = true; } )

#define INDENT_PRINT(...) \
  BLOCK( print_indent( indent, dout ); FPRINTF( dout, __VA_ARGS__ ); )

#define INDENT_PRINT_KV(KEY,VALUE) \
  BLOCK( print_indent( indent, dout ); kv_debug( (KEY), (VALUE), dout ); )

#define INDENT_PRINT_SNAME(KEY,SNAME) \
  print_sname( indent, (KEY), (SNAME), dout )

#define INDENT_PRINT_TYPE(TYPE) BLOCK(  \
  print_indent( indent, dout );         \
  FPUTS( "type = ", dout );             \
  c_type_debug( (TYPE), dout ); )

/// @endcond

#define DEBUG_INDENT              2     /**< Spaces per debug indent level. */

////////// local functions ////////////////////////////////////////////////////

/**
 * Prints a multiple of \a indent spaces.
 *
 * @param indent How much to indent.
 * @param out The `FILE` to print to.
 */
static void print_indent( unsigned indent, FILE *out ) {
  FPRINTF( out, "%*s", (int)(indent * DEBUG_INDENT), "" );
}

/**
 * Prints a scoped name and its type.
 *
 * @param indent How much to indent.
 * @param key The key to print.
 * @param sname The scoped name to print.
 * @param out The `FILE` to print to.
 */
static void print_sname( unsigned indent, char const *key,
                         c_sname_t const *sname, FILE *out ) {
  assert( key != NULL );
  assert( sname != NULL );

  print_indent( indent, out );
  char const *const full_name = c_sname_full_name( sname );
  kv_debug( key, full_name, out );
  if ( full_name[0] != '\0' ) {
    char const *const sn_type_name = c_type_name( c_sname_type( sname ) );
    FPRINTF( out,
      ", scope_type = %s", sn_type_name[0] != '\0' ? sn_type_name : "none"
    );
  }
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_debug( c_ast_t const *ast, unsigned indent, char const *key0,
                  FILE *dout ) {
  if ( key0 != NULL && *key0 != '\0' )
    INDENT_PRINT( "%s = {\n", key0 );
  else
    INDENT_PRINT( "{\n" );

  if ( ast != NULL ) {
    ++indent;

    INDENT_PRINT_SNAME( "sname", &ast->sname );
    FPUTS( ",\n", dout );
    INDENT_PRINT( "unique_id = %u,\n", ast->unique_id );
    INDENT_PRINT_KV( "kind", c_kind_name( ast->kind_id ) );
    FPUTS( ",\n", dout );
    INDENT_PRINT( "depth = %u,\n", ast->depth );

    INDENT_PRINT(
      "parent->unique_id = %d,\n",
      ast->parent_ast != NULL ? (int)ast->parent_ast->unique_id : -1
    );

    switch ( ast->align.kind ) {
      case C_ALIGNAS_NONE:
        break;
      case C_ALIGNAS_EXPR:
        INDENT_PRINT( "alignas_expr = %u,\n", ast->align.as.expr );
        break;
      case C_ALIGNAS_TYPE:
        c_ast_debug( ast->align.as.type_ast, indent, "alignas_type_ast", dout );
        FPUTS( ",\n", dout );
        break;
    } // switch

    INDENT_PRINT(
      "loc = %d-%d,\n", ast->loc.first_column, ast->loc.last_column
    );
    INDENT_PRINT_TYPE( ast->type_id );

    bool comma = false;

    switch ( ast->kind_id ) {
      case K_BUILTIN:
      case K_DESTRUCTOR:
      case K_NAME:
      case K_NONE:
      case K_PLACEHOLDER:
      case K_VARIADIC:
        // nothing to do
        break;

      case K_ARRAY:
        PRINT_COMMA;
        INDENT_PRINT( "size = " );
        switch ( ast->as.array.size ) {
          case C_ARRAY_SIZE_NONE:
            FPRINTF( dout, "unspecified" );
            break;
          case C_ARRAY_SIZE_VARIABLE:
            FPUTC( '*', dout );
            break;
          default:
            FPRINTF( dout, "%d", ast->as.array.size );
        } // switch
        FPUTS( ",\n", dout );
        if ( ast->as.array.type_id != T_NONE ) {
          INDENT_PRINT_TYPE( ast->as.array.type_id );
          FPUTS( ",\n", dout );
        }
        c_ast_debug( ast->as.array.of_ast, indent, "of_ast", dout );
        break;

      case K_OPERATOR:
        PRINT_COMMA;
        INDENT_PRINT( "oper_id = %u,\n", ast->as.oper.oper_id );
        INDENT_PRINT_KV(
          "operator_name", c_oper_get( ast->as.oper.oper_id )->name
        );
        FPUTS( ",\n", dout );
        C_FALLTHROUGH;

      case K_FUNCTION:
        PRINT_COMMA;
        INDENT_PRINT( "flags = 0x%x,\n", ast->as.func.flags );
        C_FALLTHROUGH;

      case K_APPLE_BLOCK:
      case K_CONSTRUCTOR:
      case K_USER_DEF_LITERAL:
        PRINT_COMMA;
        INDENT_PRINT( "args = " );
        c_ast_list_debug( &ast->as.func.args, indent, dout );
        if ( ast->as.func.ret_ast != NULL ) {
          FPUTS( ",\n", dout );
          c_ast_debug( ast->as.func.ret_ast, indent, "ret_ast", dout );
        }
        break;

      case K_ENUM_CLASS_STRUCT_UNION:
        PRINT_COMMA;
        INDENT_PRINT_SNAME( "ecsu_sname", &ast->as.ecsu.ecsu_sname );
        break;

      case K_POINTER_TO_MEMBER:
        PRINT_COMMA;
        INDENT_PRINT_SNAME( "class_sname", &ast->as.ptr_mbr.class_sname );
        FPUTS( ",\n", dout );
        C_FALLTHROUGH;

      case K_POINTER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        PRINT_COMMA;
        c_ast_debug( ast->as.ptr_ref.to_ast, indent, "to_ast", dout );
        break;

      case K_TYPEDEF:
        PRINT_COMMA;
        INDENT_PRINT_SNAME( "typedef_name", &ast->as.c_typedef->ast->sname );
        break;

      case K_USER_DEF_CONVERSION:
        PRINT_COMMA;
        c_ast_debug( ast->as.user_def_conv.conv_ast, indent, "conv_ast", dout );
        break;
    } // switch

    FPUTC( '\n', dout );
    --indent;
  }

  INDENT_PRINT( "}" );
}

void c_ast_list_debug( slist_t const *list, unsigned indent, FILE *dout ) {
  assert( list != NULL );
  if ( !slist_empty( list ) ) {
    FPUTS( "[\n", dout );
    bool comma = false;
    for ( slist_node_t const *p = list->head; p != NULL; p = p->next ) {
      if ( true_or_set( &comma ) )
        FPUTS( ",\n", dout );
      c_ast_debug( c_ast_arg_ast( p ), indent + 1, NULL, dout );
    } // for
    FPUTC( '\n', dout );
    INDENT_PRINT( "]" );
  } else {
    FPUTS( "[]", dout );
  }
}

void c_type_debug( c_type_id_t type_id, FILE *dout ) {
  FPRINTF( dout,
    "\"%s\" (0x%" PRIX_C_TYPE_T ")",
    c_type_name( type_id ), type_id
  );
}

void kv_debug( char const *key, char const *value, FILE *dout ) {
  assert( key != NULL );
  if ( value != NULL && *value != '\0' )
    FPRINTF( dout, "%s = \"%s\"", key, value );
  else
    FPRINTF( dout, "%s = null", key );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
