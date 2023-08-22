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
#include <stdint.h>
#include <stdlib.h>
#include <sysexits.h>

#define DUMP_FORMAT(D,...) BLOCK(                   \
  FPUTNSP( (D)->indent * DUMP_INDENT, (D)->dout );  \
  FPRINTF( (D)->dout, __VA_ARGS__ ); )

#define DUMP_KEY(D,...) BLOCK(                \
  fput_sep( ",\n", &(D)->comma, (D)->dout );  \
  DUMP_FORMAT( (D), __VA_ARGS__ ); )

#define DUMP_LOC(D,KEY,LOC) \
  DUMP_KEY( (D), KEY ": " ); c_loc_dump( (LOC), (D)->dout )

#define DUMP_SNAME(D,KEY,SNAME) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); c_sname_dump( (SNAME), (D)->dout ); )

/// @endcond

/**
 * @addtogroup dump-group
 * @{
 */

///////////////////////////////////////////////////////////////////////////////

/**
 * Dump state.
 */
struct d_state {
  FILE     *dout;                       ///< File to dump to.
  unsigned  indent;                     ///< Current indentation.
  bool      comma;                      ///< Print a comma?
};
typedef struct d_state d_state_t;

/**
 * JSON object state.
 */
enum j_state {
  J_INIT      = 0,                      ///< Initial state.
  J_COMMA     = (1u << 0),              ///< Previous "print a comma?" state.
  J_OBJ_BEGUN = (1u << 1)               ///< Has a JSON object already begun?
};
typedef enum j_state j_state_t;

// local functions
static void c_ast_dump_impl( c_ast_t const*, char const*, d_state_t* );
static void c_loc_dump( c_loc_t const*, FILE* );
static void d_init( d_state_t*, unsigned, FILE* );
NODISCARD
static j_state_t json_object_begin( j_state_t, char const*, d_state_t* );
static void json_object_end( j_state_t, d_state_t* );

// local constants
static unsigned const DUMP_INDENT = 2;  ///< Spaces per dump indent level.

////////// local functions ////////////////////////////////////////////////////

/**
 * Dumps \a align (for debugging).
 *
 * @param align The \ref c_alignas to dump.
 * @param d The d_state to use.
 */
static void c_alignas_dump( c_alignas_t const *align, d_state_t *d ) {
  assert( align != NULL );
  assert( d != NULL );

  if ( align->kind == C_ALIGNAS_NONE )
    return;

  j_state_t const j = json_object_begin( J_INIT, "alignas", d );

  switch ( align->kind ) {
    case C_ALIGNAS_NONE:
      unreachable();
    case C_ALIGNAS_EXPR:
      DUMP_KEY( d, "expr: %u", align->expr );
      break;
    case C_ALIGNAS_TYPE:
      c_ast_dump_impl( align->type_ast, "type_ast", d );
      break;
  } // switch

  DUMP_LOC( d, "loc", &align->loc );
  json_object_end( j, d );
}

/**
 * Dumps \a ast (for debugging).
 *
 * @param ast The AST to dump; may be NULL.  If NULL and \a key is not NULL,
 * dumps only \a key followed by `:&nbsp;null`.
 * @param key The key for which \a ast is the value; may be NULL.
 * @param d The d_state to use.
 */
void c_ast_dump_impl( c_ast_t const *ast, char const *key, d_state_t *d ) {
  assert( d != NULL );

  key = null_if_empty( key );
  if ( key != NULL )
    DUMP_KEY( d, "%s: ", key );

  if ( ast == NULL ) {
    FPUTS( "null", d->dout );
    return;
  }

  if ( key == NULL )
    DUMP_FORMAT( d, "" );

  j_state_t const ast_j = json_object_begin( J_INIT, /*key=*/NULL, d );

  DUMP_SNAME( d, "sname", &ast->sname );
  DUMP_KEY( d, "unique_id: " PRId_C_AST_ID_T, ast->unique_id );
  DUMP_KEY( d,
    "kind: { value: 0x%X, string: \"%s\" }",
    ast->kind, c_kind_name( ast->kind )
  );
  DUMP_KEY( d, "depth: %u", ast->depth );
  DUMP_KEY( d,
    "parent__unique_id: " PRId_C_AST_ID_T,
    ast->parent_ast != NULL ? ast->parent_ast->unique_id : 0
  );
  c_alignas_dump( &ast->align, d );
  DUMP_LOC( d, "loc", &ast->loc );
  DUMP_KEY( d, "type: " );
  c_type_dump( &ast->type, d->dout );

  j_state_t kind_j = J_INIT;

  switch ( ast->kind ) {
    case K_ARRAY:
      kind_j = json_object_begin( J_INIT, "array", d );
      DUMP_KEY( d, "size: " );
      switch ( ast->array.kind ) {
        case C_ARRAY_EMPTY_SIZE:
          FPUTS( "\"unspecified\"", d->dout );
          break;
        case C_ARRAY_INT_SIZE:
          FPRINTF( d->dout, "%u", ast->array.size_int );
          break;
        case C_ARRAY_NAMED_SIZE:
          FPRINTF( d->dout, "\"%s\"", ast->array.size_name );
          break;
        case C_ARRAY_VLA_STAR:
          FPUTS( "'*'", d->dout );
          break;
      } // switch
      c_ast_dump_impl( ast->array.of_ast, "of_ast", d );
      json_object_end( kind_j, d );
      break;

    case K_TYPEDEF:
      kind_j = json_object_begin( J_INIT, "tdef", d );
      c_ast_dump_impl( ast->tdef.for_ast, "for_ast", d );
      FALLTHROUGH;

    case K_BUILTIN:
      kind_j = json_object_begin( kind_j, "builtin", d );
      DUMP_KEY( d, "bit_width: %u", ast->builtin.bit_width );
      if ( c_ast_is_tid_any( ast, TB_BITINT ) ) {
        j_state_t const BitInt_j = json_object_begin( J_INIT, "BitInt", d );
        DUMP_KEY( d, "width: %u", ast->builtin.BitInt.width );
        json_object_end( BitInt_j, d );
      }
      json_object_end( kind_j, d );
      break;

    case K_CAPTURE:
      kind_j = json_object_begin( J_INIT, "capture", d );
      DUMP_KEY( d, "kind: " );
      switch ( ast->capture.kind ) {
        case C_CAPTURE_COPY:
          FPUTS( "'='", d->dout );
          break;
        case C_CAPTURE_REFERENCE:
          FPUTS( "'&'", d->dout );
          break;
        case C_CAPTURE_STAR_THIS:
          FPUTS( "\"*this\"", d->dout );
          break;
        case C_CAPTURE_THIS:
          FPUTS( "\"this\"", d->dout );
          break;
        case C_CAPTURE_VARIABLE:
          FPUTS( "\"variable\"", d->dout );
          break;
      } // switch
      json_object_end( kind_j, d );
      break;

    case K_CAST:
      kind_j = json_object_begin( J_INIT, "cast", d );
      DUMP_KEY( d,
        "kind: { value: 0x%X, string: \"%s\" }",
        ast->cast.kind, c_cast_english( ast->cast.kind )
      );
      c_ast_dump_impl( ast->cast.to_ast, "to_ast", d );
      json_object_end( kind_j, d );
      break;

    case K_CLASS_STRUCT_UNION:
      kind_j = json_object_begin( J_INIT, "csu", d );
      DUMP_SNAME( d, "csu_sname", &ast->csu.csu_sname );
      json_object_end( kind_j, d );
      break;

    case K_OPERATOR:
      kind_j = json_object_begin( J_INIT, "oper", d );
      DUMP_KEY( d,
        "oper_id: { value: %d, string: \"%s\" }",
        STATIC_CAST( int, ast->oper.operator->oper_id ),
        ast->oper.operator->literal
      );
      FALLTHROUGH;

    case K_FUNCTION:
      kind_j = json_object_begin( kind_j, "func", d );
      DUMP_KEY( d, "flags: { value: 0x%X, string: ", ast->func.flags );
      switch ( ast->func.flags ) {
        case C_FUNC_UNSPECIFIED:
          FPUTS( "\"unspecified\"", d->dout );
          break;
        case C_FUNC_MEMBER:
          FPUTS( "\"member\"", d->dout );
          break;
        case C_FUNC_NON_MEMBER:
          FPUTS( "\"non-member\"", d->dout );
          break;
        case C_OPER_OVERLOADABLE:
          FPUTS( "\"overloadable\"", d->dout );
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
          FPUTS( "'?'", d->dout );
          break;
      } // switch
      FPUTS( " }", d->dout );
      FALLTHROUGH;

    case K_APPLE_BLOCK:
      kind_j = json_object_begin( kind_j, "block", d );
      FALLTHROUGH;
    case K_CONSTRUCTOR:
      kind_j = json_object_begin( kind_j, "ctor", d );
      FALLTHROUGH;
    case K_USER_DEF_LITERAL:
      kind_j = json_object_begin( kind_j, "udef_lit", d );
dump_params:
      DUMP_KEY( d, "param_ast_list: " );
      c_ast_list_dump( &ast->func.param_ast_list, d->indent, d->dout );
      if ( ast->func.ret_ast != NULL )
        c_ast_dump_impl( ast->func.ret_ast, "ret_ast", d );
      json_object_end( kind_j, d );
      break;

    case K_ENUM:
      kind_j = json_object_begin( J_INIT, "enum", d );
      DUMP_SNAME( d, "enum_sname", &ast->enum_.enum_sname );
      if ( ast->enum_.of_ast != NULL )
        c_ast_dump_impl( ast->enum_.of_ast, "of_ast", d );
      if ( ast->enum_.bit_width > 0 )
        DUMP_KEY( d, "bit_width: %u", ast->enum_.bit_width );
      json_object_end( kind_j, d );
      break;

    case K_LAMBDA:
      kind_j = json_object_begin( J_INIT, "lambda", d );
      DUMP_KEY( d, "capture_ast_list: " );
      c_ast_list_dump( &ast->lambda.capture_ast_list, d->indent, d->dout );
      goto dump_params;

    case K_POINTER_TO_MEMBER:
      kind_j = json_object_begin( J_INIT, "ptr_mbr", d );
      DUMP_SNAME( d, "class_sname", &ast->ptr_mbr.class_sname );
      FALLTHROUGH;

    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      kind_j = json_object_begin( kind_j, "ptr_ref", d );
      c_ast_dump_impl( ast->ptr_ref.to_ast, "to_ast", d );
      json_object_end( kind_j, d );
      break;

    case K_USER_DEF_CONVERSION:
      kind_j = json_object_begin( J_INIT, "udef_conv", d );
      c_ast_dump_impl( ast->udef_conv.conv_ast, "conv_ast", d );
      json_object_end( kind_j, d );
      break;

    case K_DESTRUCTOR:
    case K_NAME:
    case K_PLACEHOLDER:
    case K_VARIADIC:
      // nothing to do
      break;
  } // switch

  json_object_end( ast_j, d );
}

/**
 * Dumps \a list of ASTs (for debugging).
 *
 * @param list The \ref slist of ASTs to dump.
 * @param d The d_state to use.
 */
static void c_ast_list_dump_impl( c_ast_list_t const *list,
                                  d_state_t const *d ) {
  assert( list != NULL );
  assert( d != NULL );

  if ( slist_empty( list ) ) {
    FPUTS( "[]", d->dout );
  } else {
    FPUTS( "[\n", d->dout );
    bool comma = false;
    unsigned const indent = d->indent + 1;
    FOREACH_SLIST_NODE( node, list ) {
      fput_sep( ",\n", &comma, d->dout );
      c_ast_dump( c_param_ast( node ), indent, /*key=*/NULL, d->dout );
    } // for
    FPUTC( '\n', d->dout );
    DUMP_FORMAT( d, "]" );
  }
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

/**
 * Initializes a d_state.
 *
 * @param d The d_state to initialize.
 * @param indent The current indent.
 * @param dout The `FILE` to dump to.
 */
static void d_init( d_state_t *d, unsigned indent, FILE *dout ) {
  assert( d != NULL );
  assert( dout != NULL );

  MEM_ZERO( d );
  d->indent = indent;
  d->dout = dout;
}

/**
 * Dumps the beginning of a JSON object.
 *
 * @param j The \ref j_state_t to use.  If not equal to #J_INIT, does nothing.
 * It allows json_object_begin() to be to be called even inside a `switch`
 * statement and the previous `case` (that also called json_object_begin())
 * falls through and not begin a new JSON object when that happens allowing
 * common code in the second case to be shared with the first and not be
 * duplicated.  For example, given:
 * ```cpp
 *  case C1:
 *    j = json_object_begin( J_INIT, "K1", d );
 *    // Do stuff unique to C1.
 *    FALLTHROUGH;
 *  case C2:
 *    j = json_object_begin( j, "K2", d );  // Note: passing 'j' here.
 *    // Do stuff common to C1 and C2.
 *    json_object_end( j, d );
 *    break;
 * ```
 * There are two cases:
 * 1. `case C2` is entered: a JSON object will be begun having the key `K2`.
 *    (There is nothing special about this case.)
 * 2. `case C1` is entered: a JSON object will be begun having the key `K1`.
 *    When the case falls through into `case C2`, a second JSON object will
 *    _not_ be begun: the call to the second json_object_begin() will do
 *    nothing.
 * @param key The key for the JSON object; may be NULL.  If not NULL, dumps the
 * key followed by `:`.
 * @param d The d_state to use.
 * @return Returns a new \ref j_state_t that must be passed to
 * json_object_end().
 *
 * @sa json_object_end()
 */
static j_state_t json_object_begin( j_state_t j, char const *key,
                                    d_state_t *d ) {
  assert( d != NULL );

  if ( j == J_INIT ) {
    key = null_if_empty( key );
    if ( key != NULL )
      DUMP_KEY( d, "%s: ", key );
    FPUTS( "{\n", d->dout );
    j = J_OBJ_BEGUN;
    if ( d->comma ) {
      j |= J_COMMA;
      d->comma = false;
    }
    ++d->indent;
  }
  return j;
}

/**
 * Dumps the end of a JSON object.
 *
 * @param j The \ref j_state_t returned from json_object_begin().
 * @param d The d_state to use.
 *
 * @sa json_object_begin()
 */
static void json_object_end( j_state_t j, d_state_t *d ) {
  assert( j != J_INIT );
  assert( d != NULL );

  FPUTC( '\n', d->dout );
  d->comma = !!(j & J_COMMA);
  --d->indent;
  DUMP_FORMAT( d, "}" );
}

////////// extern functions ///////////////////////////////////////////////////

void bool_dump( bool value, FILE *dout ) {
  assert( dout != NULL );
  FPUTS( value ? L_true : L_false, dout );
}

void c_ast_dump( c_ast_t const *ast, unsigned indent, char const *key,
                 FILE *dout ) {
  d_state_t d;
  d_init( &d, indent, dout );
  c_ast_dump_impl( ast, key, &d );
}

void c_ast_list_dump( c_ast_list_t const *list, unsigned indent, FILE *dout ) {
  d_state_t d;
  d_init( &d, indent, dout );
  c_ast_list_dump_impl( list, &d );
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
    return;
  }
  FPUTS( "[ ", dout );
  bool comma = false;
  FOREACH_SLIST_NODE( node, list ) {
    fput_sep( ", ", &comma, dout );
    c_sname_dump( node->data, dout );
  } // for
  FPUTS( " ]", dout );
}

void c_tid_dump( c_tid_t tid, FILE *dout ) {
  assert( dout != NULL );
  FPRINTF( dout,
    "{ %s: 0x%" PRIX_C_TID_T ", string: \"%s\" }",
    c_tpid_name( c_tid_tpid( tid ) ), tid,
    c_tid_is_none( tid ) ? "none" : c_tid_name_c( tid )
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
