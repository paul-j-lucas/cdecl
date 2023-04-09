/*
**      cdecl -- C gibberish translator
**      src/dump.c
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
 * Defines functions for dumping types for debugging.
 */

// local
#include "pjl_config.h"                 /* must go first */
#include "dump.h"
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_operator.h"
#include "c_type.h"
#include "cdecl.h"
#include "english.h"
#include "literals.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

#define DUMP_FORMAT(...) BLOCK( \
  FPUTNSP( indent * DUMP_INDENT, dout ); FPRINTF( dout, __VA_ARGS__ ); )

#define DUMP_KEY(...) BLOCK( \
  fput_sep( ",\n", comma, dout ); DUMP_FORMAT( __VA_ARGS__ ); )

#define DUMP_LOC(KEY,LOC) \
  DUMP_KEY( KEY ": " ); c_loc_dump( (LOC), dout )

#define DUMP_SNAME(KEY,SNAME) BLOCK( \
  DUMP_KEY( KEY ": " ); c_sname_dump( (SNAME), dout ); )

#define DUMP_TYPE(TYPE) BLOCK( \
  DUMP_KEY( "type: " ); c_type_dump( (TYPE), dout ); )

/// @endcond

/**
 * @addtogroup dump-group
 * @{
 */

// local functions
static void c_ast_dump_impl( c_ast_t const*, unsigned, bool*, char const*,
                             FILE* );
static void c_capture_kind_dump( c_capture_kind_t, FILE* );
static void c_loc_dump( c_loc_t const*, FILE* );

// local constants
static unsigned const DUMP_INDENT = 2;  ///< Spaces per dump indent level.

////////// local functions ////////////////////////////////////////////////////

/**
 * Dumps \a align (for debugging).
 *
 * @param align The \ref c_alignas to dump.
 * @param indent The current indent.
 * @param comma A pointer to a flag to know whether to print `,`.
 * @param dout The `FILE` to dump to.
 */
static void c_alignas_dump( c_alignas_t const *align, unsigned indent,
                            bool *comma, FILE *dout ) {
  assert( align != NULL );
  assert( dout != NULL );

  if ( align->kind == C_ALIGNAS_NONE )
    return;

  DUMP_KEY( "alignas: {\n" );

  bool const orig_comma = *comma;
  *comma = false;
  ++indent;

  switch ( align->kind ) {
    case C_ALIGNAS_NONE:
      unreachable();
    case C_ALIGNAS_EXPR:
      DUMP_KEY( "expr: %u", align->expr );
      break;
    case C_ALIGNAS_TYPE:
      c_ast_dump_impl( align->type_ast, indent, comma, "type_ast", dout );
      break;
  } // switch

  DUMP_LOC( "loc", &align->loc );

  FPUTC( '\n', dout );
  --indent;
  *comma = orig_comma;
  DUMP_FORMAT( "}" );
}

/**
 * Dumps \a ast (for debugging).
 *
 * @param ast The AST to dump.  If NULL and \a key is not NULL, dumps only \a
 * key followed by `:&nbsp;null`.
 * @param indent The current indent.
 * @param comma A pointer to a flag to know whether to print `,`.
 * @param key The key for which \a ast is the value, or NULL for none.
 * @param dout The `FILE` to dump to.
 */
void c_ast_dump_impl( c_ast_t const *ast, unsigned indent, bool *comma,
                      char const *key, FILE *dout ) {
  assert( dout != NULL );
  key = null_if_empty( key );

  if ( key != NULL )
    DUMP_KEY( "%s: ", key );

  if ( ast == NULL ) {
    FPUTS( "null", dout );
    return;
  }

  if ( key != NULL )
    FPUTS( "{\n", dout );
  else
    DUMP_FORMAT( "{\n" );

  bool const orig_comma = *comma;
  *comma = false;
  ++indent;

  DUMP_SNAME( "sname", &ast->sname );
  DUMP_KEY( "unique_id: " PRId_C_AST_ID_T, ast->unique_id );
  DUMP_KEY(
    "kind: { value: 0x%X, string: \"%s\" }",
    ast->kind, c_kind_name( ast->kind )
  );
  DUMP_KEY( "depth: %u", ast->depth );
  DUMP_KEY(
    "parent__unique_id: " PRId_C_AST_SID_T,
    ast->parent_ast != NULL ?
      STATIC_CAST( c_ast_sid_t, ast->parent_ast->unique_id ) :
      STATIC_CAST( c_ast_sid_t, -1 )
  );
  c_alignas_dump( &ast->align, indent, comma, dout );
  DUMP_LOC( "loc", &ast->loc );
  DUMP_TYPE( &ast->type );

  switch ( ast->kind ) {
    case K_DESTRUCTOR:
    case K_NAME:
    case K_PLACEHOLDER:
    case K_VARIADIC:
      // nothing to do
      break;

    case K_ARRAY:
      DUMP_KEY( "size: " );
      switch ( ast->array.size ) {
        case C_ARRAY_SIZE_NONE:
          FPUTS( "\"unspecified\"", dout );
          break;
        case C_ARRAY_SIZE_VARIABLE:
          FPUTS( "'*'", dout );
          break;
        default:
          FPRINTF( dout, PRId_C_ARRAY_SIZE_T, ast->array.size );
      } // switch
      if ( ast->array.stids != TS_NONE ) {
        c_type_t const type = C_TYPE_LIT_S( ast->array.stids );
        DUMP_TYPE( &type );
      }
      c_ast_dump_impl( ast->array.of_ast, indent, comma, "of_ast", dout );
      break;

    case K_CAPTURE:
      DUMP_KEY( "capture: " );
      c_capture_kind_dump( ast->capture.kind, dout );
      break;

    case K_CAST:
      DUMP_KEY(
        "cast_kind: { value: 0x%X, string: \"%s\" }",
        ast->cast.kind, c_cast_english( ast->cast.kind )
      );
      c_ast_dump_impl( ast->cast.to_ast, indent, comma, "to_ast", dout );
      break;

    case K_CLASS_STRUCT_UNION:
      DUMP_SNAME( "csu_sname", &ast->csu.csu_sname );
      break;

    case K_OPERATOR:
      DUMP_KEY(
        "operator: { value: %d, string: \"%s\" }",
        STATIC_CAST( int, ast->oper.operator->oper_id ),
        ast->oper.operator->literal
      );
      FALLTHROUGH;

    case K_FUNCTION:
      DUMP_KEY( "flags: { value: 0x%X, string: ", ast->func.flags );
      switch ( ast->func.flags ) {
        case C_FUNC_UNSPECIFIED:
          FPUTS( "\"unspecified\"", dout );
          break;
        case C_FUNC_MEMBER:
          FPUTS( "\"member\"", dout );
          break;
        case C_FUNC_NON_MEMBER:
          FPUTS( "\"non-member\"", dout );
          break;
        case C_OPER_OVERLOADABLE:
          FPUTS( "\"overloadable\"", dout );
          break;
     // case C_OPER_NOT_OVERLOADABLE:
     //
     //   This doesn't need to be here because these flags are what the user
     //   specified:
     //
     //       declare ! as [[non-]member] operator ...
     //
     //   not c_operator::flags, so this can never be C_OPER_NOT_OVERLOADABLE.
     //
     //   break;
        default:
          FPUTS( "'?'", dout );
          break;
      } // switch
      FPUTS( " }", dout );
      FALLTHROUGH;

    case K_APPLE_BLOCK:
    case K_CONSTRUCTOR:
    case K_USER_DEF_LITERAL:
dump_params:
      DUMP_KEY( "param_ast_list: " );
      c_ast_list_dump( &ast->func.param_ast_list, indent, dout );
      if ( ast->func.ret_ast != NULL )
        c_ast_dump_impl( ast->func.ret_ast, indent, comma, "ret_ast", dout );
      break;

    case K_ENUM:
      DUMP_SNAME( "enum_sname", &ast->enum_.enum_sname );
      if ( ast->enum_.of_ast != NULL )
        c_ast_dump_impl( ast->enum_.of_ast, indent, comma, "of_ast", dout );
      break;

    case K_LAMBDA:
      DUMP_KEY( "capture_ast_list: " );
      c_ast_list_dump( &ast->lambda.capture_ast_list, indent, dout );
      goto dump_params;

    case K_POINTER_TO_MEMBER:
      DUMP_SNAME( "class_sname", &ast->ptr_mbr.class_sname );
      FALLTHROUGH;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      c_ast_dump_impl( ast->ptr_ref.to_ast, indent, comma, "to_ast", dout );
      break;

    case K_TYPEDEF:
      c_ast_dump_impl( ast->tdef.for_ast, indent, comma, "for_ast", dout );
      FALLTHROUGH;

    case K_BUILTIN:
      DUMP_KEY( "bit_width: %u", ast->builtin.bit_width );
      if ( c_ast_is_tid_any( ast, TB_BITINT ) &&
           ast->builtin.BitInt.width > 0 ) {
        DUMP_KEY( "BitInt_width: %u", ast->builtin.BitInt.width );
      }
      break;

    case K_USER_DEF_CONVERSION:
      c_ast_dump_impl(
        ast->udef_conv.conv_ast, indent, comma, "conv_ast", dout
      );
      break;
  } // switch

  FPUTC( '\n', dout );
  --indent;
  *comma = orig_comma;
  DUMP_FORMAT( "}" );
}

/**
 * Dumps \a kind (for debugging).
 *
 * @param kind The \ref c_capture_kind to dump.
 * @param dout The `FILE` to dump to.
 */
static void c_capture_kind_dump( c_capture_kind_t kind, FILE *dout ) {
  assert( dout != NULL );
  switch ( kind ) {
    case C_CAPTURE_COPY:
      FPUTS( "'='", dout );
      break;
    case C_CAPTURE_REFERENCE:
      FPUTS( "'&'", dout );
      break;
    case C_CAPTURE_STAR_THIS:
      FPUTS( "\"*this\"", dout );
      break;
    case C_CAPTURE_THIS:
      FPUTS( "\"this\"", dout );
      break;
    case C_CAPTURE_VARIABLE:
      FPUTS( "\"variable\"", dout );
      break;
  } // switch
}

/**
 * Dumps \a loc (for debugging).
 *
 * @param loc The location to dump.
 * @param dout The `FILE` to dump to.
 */
static void c_loc_dump( c_loc_t const *loc, FILE *dout ) {
  assert( loc != NULL );
  assert( dout != NULL );
  FPRINTF( dout, "{ first_column: %d", loc->first_column );
  if ( loc->last_column != loc->first_column )
    FPRINTF( dout, ", last_column: %d", loc->last_column );
  FPUTS( " }", dout );
}

/**
 * Gets a string representation of \a tpid for printing.
 *
 * @param tpid The type part id to get the string representation of.
 * @return Returns a string representation of \a tpid.
 */
static char const* c_tpid_name( c_tpid_t tpid ) {
  switch ( tpid ) {
    case C_TPID_NONE:
      return "none";                    // LCOV_EXCL_LINE
    case C_TPID_BASE:
      return "btid";
    case C_TPID_STORE:
      return "stid";
    case C_TPID_ATTR:
      return "atid";
  } // switch

  UNEXPECTED_INT_VALUE( tpid );
}

////////// extern functions ///////////////////////////////////////////////////

void bool_dump( bool value, FILE *dout ) {
  assert( dout != NULL );
  FPUTS( value ? L_true : L_false, dout );
}

void c_ast_dump( c_ast_t const *ast, unsigned indent, char const *key,
                 FILE *dout ) {
  bool comma = false;
  c_ast_dump_impl( ast, indent, &comma, key, dout );
}

void c_ast_list_dump( c_ast_list_t const *list, unsigned indent, FILE *dout ) {
  assert( list != NULL );
  assert( dout != NULL );

  if ( slist_empty( list ) ) {
    FPUTS( "[]", dout );
  } else {
    FPUTS( "[\n", dout );
    ++indent;
    bool comma = false;
    FOREACH_SLIST_NODE( node, list ) {
      fput_sep( ",\n", &comma, dout );
      c_ast_dump( c_param_ast( node ), indent, /*key=*/NULL, dout );
    } // for
    --indent;
    FPUTC( '\n', dout );
    DUMP_FORMAT( "]" );
  }
}

void c_sname_dump( c_sname_t const *sname, FILE *dout ) {
  assert( sname != NULL );
  assert( dout != NULL );

  FPRINTF( dout, "{ string: \"%s\"", c_sname_full_name( sname ) );
  if ( !c_sname_empty( sname ) ) {
    FPUTS( ", scopes: \"", dout );
    bool colon2 = false;
    FOREACH_SNAME_SCOPE( scope, sname ) {
      fput_sep( "::", &colon2, dout );
      c_type_t const *const t = &c_scope_data( scope )->type;
      FPUTS( c_type_is_none( t ) ? "none" : c_type_name_c( t ), dout );
    } // for
    FPUTC( '"', dout );
  }
  FPUTS( " }", dout );
}

void c_sname_list_dump( slist_t const *list, FILE *dout ) {
  assert( list != NULL );
  assert( dout != NULL );

  if ( slist_empty( list ) ) {
    FPUTS( "[]", dout );
  } else {
    FPUTS( "[ ", dout );
    bool comma = false;
    FOREACH_SLIST_NODE( node, list ) {
      fput_sep( ", ", &comma, dout );
      c_sname_dump( node->data, dout );
    } // for
    FPUTS( " ]", dout );
  }
}

void c_tid_dump( c_tid_t tid, FILE *dout ) {
  assert( dout != NULL );
  FPRINTF( dout,
    "{ %s: 0x%" PRIX_C_TID_T ", string: \"%s\" }",
    c_tpid_name( c_tid_tpid( tid ) ), tid, c_tid_name_c( tid )
  );
}

void c_type_dump( c_type_t const *type, FILE *dout ) {
  assert( type != NULL );
  assert( dout != NULL );

  char const *const type_name = c_type_name_c( type );
  FPRINTF( dout,
    "{ %s: 0x%" PRIX_C_TID_T
    ", %s: 0x%" PRIX_C_TID_T
    ", %s: 0x%" PRIX_C_TID_T
    ", string: \"%s\" }",
    c_tpid_name( C_TPID_BASE  ), type->btids,
    c_tpid_name( C_TPID_STORE ), type->stids,
    c_tpid_name( C_TPID_ATTR  ), type->atids,
    type_name[0] != '\0' ? type_name : "none"
  );
}

void str_dump( char const *value, FILE *dout ) {
  assert( dout != NULL );
  if ( value == NULL ) {
    FPUTS( "null", dout );
    return;
  }
  FPUTC( '"', dout );
  for ( char const *p = value; *p != '\0'; ++p ) {
    if ( *p == '"' )
      FPUTS( "\\\"", dout );
    else
      FPUTC( *p, dout );
  } // for
  FPUTC( '"', dout );
}

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
