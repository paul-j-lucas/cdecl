/*
**      cdecl -- C gibberish translator
**      src/dump.c
**
**      Copyright (C) 2017-2021  Paul J. Lucas, et al.
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
 * Defines functions for dumping types for debugging.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "dump.h"
#include "cdecl.h"
#include "c_ast.h"
#include "c_type.h"
#include "literals.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

#define DUMP_COMMA \
  BLOCK( if ( false_set( &comma ) ) FPUTS( ",\n", dout ); )

#define DUMP_FORMAT(...) \
  BLOCK( print_indent( indent, dout ); FPRINTF( dout, __VA_ARGS__ ); )

#define DUMP_LOC(KEY,LOC) \
  DUMP_FORMAT( KEY " = %d-%d,\n", (LOC)->first_column, (LOC)->last_column )

#define DUMP_SNAME(KEY,SNAME) BLOCK(  \
  print_indent( indent, dout );       \
  FPUTS( KEY " = ", dout );           \
  c_sname_dump( (SNAME), dout ); )

#define DUMP_STR(KEY,VALUE) \
  BLOCK( print_indent( indent, dout ); kv_dump( (KEY), (VALUE), dout ); )

#define DUMP_TYPE(TYPE) BLOCK(  \
  print_indent( indent, dout ); \
  FPUTS( "type = ", dout );     \
  c_type_dump( (TYPE), dout ); )

/// @endcond

static unsigned const DUMP_INDENT = 2;  ///< Spaces per dump indent level.

////////// local functions ////////////////////////////////////////////////////

/**
 * Gets a string representation of \a tpid for printing.
 *
 * @param tpid The type part id to get the string representation of.
 * @return Returns a string representation of \a tpid.
 */
static char const* c_type_part_id_name( c_type_part_id_t tpid ) {
  switch ( tpid ) {
    case C_TPID_NONE:
      break;
    case C_TPID_BASE:
      return "base";
    case C_TPID_STORE:
      return "store";
    case C_TPID_ATTR:
      return "attr";
  } // switch
  UNEXPECTED_INT_VALUE( tpid );
}

/**
 * Prints a multiple of \a indent spaces.
 *
 * @param indent How much to indent.
 * @param out The `FILE` to print to.
 */
static void print_indent( unsigned indent, FILE *out ) {
  FPRINTF( out, "%*s", (int)(indent * DUMP_INDENT), "" );
}

////////// extern functions ///////////////////////////////////////////////////

void c_ast_dump( c_ast_t const *ast, unsigned indent, char const *key0,
                 FILE *dout ) {
  if ( key0 != NULL && *key0 != '\0' )
    DUMP_FORMAT( "%s = {\n", key0 );
  else
    DUMP_FORMAT( "{\n" );

  if ( ast != NULL ) {
    ++indent;

    DUMP_SNAME( "sname", &ast->sname );
    FPUTS( ",\n", dout );
    DUMP_FORMAT( "unique_id = %u,\n", ast->unique_id );
    DUMP_STR( "kind", c_kind_name( ast->kind_id ) );
    FPUTS( ",\n", dout );
    DUMP_FORMAT( "depth = %u,\n", ast->depth );

    DUMP_FORMAT(
      "parent->unique_id = %d,\n",
      ast->parent_ast != NULL ? (int)ast->parent_ast->unique_id : -1
    );

    if ( ast->align.kind != C_ALIGNAS_NONE ) {
      switch ( ast->align.kind ) {
        case C_ALIGNAS_NONE:
          break;
        case C_ALIGNAS_EXPR:
          DUMP_FORMAT( "alignas_expr = %u,\n", ast->align.as.expr );
          break;
        case C_ALIGNAS_TYPE:
          c_ast_dump(
            ast->align.as.type_ast, indent, "alignas_type_ast", dout
          );
          FPUTS( ",\n", dout );
          break;
      } // switch
      DUMP_LOC( "alignas_loc", &ast->align.loc );
    }

    DUMP_LOC( "loc", &ast->loc );
    DUMP_TYPE( &ast->type );

    bool comma = false;

    switch ( ast->kind_id ) {
      case K_DESTRUCTOR:
      case K_NAME:
      case K_NONE:
      case K_PLACEHOLDER:
      case K_VARIADIC:
        // nothing to do
        break;

      case K_ARRAY:
        DUMP_COMMA;
        DUMP_FORMAT( "size = " );
        switch ( ast->as.array.size ) {
          case C_ARRAY_SIZE_NONE:
            FPUTS( "unspecified", dout );
            break;
          case C_ARRAY_SIZE_VARIABLE:
            FPUTC( '*', dout );
            break;
          default:
            FPRINTF( dout, "%d", ast->as.array.size );
        } // switch
        FPUTS( ",\n", dout );
        if ( ast->as.array.store_tid != TS_NONE ) {
          c_type_t const type = C_TYPE_LIT_S( ast->as.array.store_tid );
          DUMP_TYPE( &type );
          FPUTS( ",\n", dout );
        }
        c_ast_dump( ast->as.array.of_ast, indent, "of_ast", dout );
        break;

      case K_OPERATOR:
        DUMP_COMMA;
        DUMP_FORMAT( "oper_id = %u,\n", ast->as.oper.oper_id );
        DUMP_STR(
          "operator_name", c_oper_get( ast->as.oper.oper_id )->name
        );
        FPUTS( ",\n", dout );
        PJL_FALLTHROUGH;

      case K_FUNCTION:
        DUMP_COMMA;
        DUMP_FORMAT( "flags = " );
        switch ( ast->as.func.flags & C_FUNC_MASK_MEMBER ) {
          case C_FUNC_UNSPECIFIED:
            FPUTS( "unspecified", dout );
            break;
          case C_FUNC_MEMBER:
            FPUTS( L_MEMBER, dout );
            break;
          case C_FUNC_NON_MEMBER:
            FPUTS( H_NON_MEMBER, dout );
            break;
          case C_OP_OVERLOADABLE:
            FPUTS( "overloadable", dout );
            break;
          default:
            FPUTC( '?', dout );
            break;
        } // switch
        FPRINTF( dout, " (0x%x),\n", ast->as.func.flags );
        PJL_FALLTHROUGH;

      case K_APPLE_BLOCK:
      case K_CONSTRUCTOR:
      case K_USER_DEF_LITERAL:
        DUMP_COMMA;
        DUMP_FORMAT( "param_ast_list = " );
        c_ast_list_dump( &ast->as.func.param_ast_list, indent, dout );
        if ( ast->as.func.ret_ast != NULL ) {
          FPUTS( ",\n", dout );
          c_ast_dump( ast->as.func.ret_ast, indent, "ret_ast", dout );
        }
        break;

      case K_ENUM_CLASS_STRUCT_UNION:
        DUMP_COMMA;
        DUMP_SNAME( "ecsu_sname", &ast->as.ecsu.ecsu_sname );
        if ( ast->as.ecsu.of_ast != NULL ) {
          FPUTS( ",\n", dout );
          c_ast_dump( ast->as.ecsu.of_ast, indent, "of_ast", dout );
        }
        break;

      case K_POINTER_TO_MEMBER:
        DUMP_COMMA;
        DUMP_SNAME( "class_sname", &ast->as.ptr_mbr.class_sname );
        FPUTS( ",\n", dout );
        PJL_FALLTHROUGH;

      case K_POINTER:
      case K_REFERENCE:
      case K_RVALUE_REFERENCE:
        DUMP_COMMA;
        c_ast_dump( ast->as.ptr_ref.to_ast, indent, "to_ast", dout );
        break;

      case K_TYPEDEF:
        DUMP_COMMA;
        DUMP_SNAME( "typedef_name", &ast->as.tdef.for_ast->sname );
        FPUTS( ",\n", dout );
        PJL_FALLTHROUGH;

      case K_BUILTIN:
        DUMP_COMMA;
        DUMP_FORMAT( "bit_width = %u", ast->as.builtin.bit_width );
        break;

      case K_USER_DEF_CONVERSION:
        DUMP_COMMA;
        c_ast_dump( ast->as.udef_conv.conv_ast, indent, "conv_ast", dout );
        break;
    } // switch

    FPUTC( '\n', dout );
    --indent;
  }

  DUMP_FORMAT( "}" );
}

void c_ast_list_dump( c_ast_list_t const *list, unsigned indent, FILE *dout ) {
  assert( list != NULL );
  if ( slist_empty( list ) ) {
    FPUTS( "[]", dout );
  } else {
    FPUTS( "[\n", dout );
    ++indent;
    bool comma = false;
    FOREACH_SLIST( p, list, NULL ) {
      if ( true_or_set( &comma ) )
        FPUTS( ",\n", dout );
      c_ast_dump( c_param_ast( p ), indent, NULL, dout );
    } // for
    --indent;
    FPUTC( '\n', dout );
    DUMP_FORMAT( "]" );
  }
}

void c_sname_dump( c_sname_t const *sname, FILE *dout ) {
  assert( sname != NULL );
  FPRINTF( dout, "\"%s\"", c_sname_full_name( sname ) );
  if ( !c_sname_empty( sname ) ) {
    FPUTS( " (", dout );
    bool colon2 = false;
    FOREACH_SCOPE( scope, sname, NULL ) {
      if ( true_or_set( &colon2 ) )
        FPUTS( "::", dout );
      c_type_t const *const t = &c_scope_data( scope )->type;
      FPUTS( c_type_is_none( t ) ? "none" : c_type_name_c( t ), dout );
    } // for
    FPUTC( ')', dout );
  }
}

void c_type_dump( c_type_t const *type, FILE *dout ) {
  assert( type != NULL );
  FPRINTF( dout,
    "\"%s\" "
     "(%s = 0x%" PRIX_C_TYPE_ID_T
    ", %s = 0x%" PRIX_C_TYPE_ID_T
    ", %s = 0x%" PRIX_C_TYPE_ID_T ")",
    c_type_name_c( type ),
    c_type_part_id_name( C_TPID_BASE  ), type->base_tid,
    c_type_part_id_name( C_TPID_STORE ), type->store_tid,
    c_type_part_id_name( C_TPID_ATTR  ), type->attr_tid
  );
}

void c_type_id_dump( c_type_id_t tid, FILE *dout ) {
  FPRINTF( dout,
    "\"%s\" (%s = 0x%" PRIX_C_TYPE_ID_T ")",
    c_type_id_name_c( tid ), c_type_part_id_name( c_type_id_tpid( tid ) ), tid
  );
}

void kv_dump( char const *key, char const *value, FILE *dout ) {
  assert( key != NULL );
  assert( key[0] != '\0' );
  if ( value != NULL )
    FPRINTF( dout, "%s = \"%s\"", key, value );
  else
    FPRINTF( dout, "%s = null", key );
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
