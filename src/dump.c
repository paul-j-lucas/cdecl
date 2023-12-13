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
#include <stddef.h>                     /* for NULL */
#include <stdint.h>
#include <stdlib.h>
#include <sysexits.h>

#define DUMP_AST(D,KEY,AST) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); c_ast_dump_impl( (AST), (D) ); )

#define DUMP_AST_LIST(D,KEY,LIST) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); c_ast_list_dump_impl( (LIST), (D) ); )

#define DUMP_FORMAT(D,...) BLOCK(                   \
  FPUTNSP( (D)->indent * DUMP_INDENT, (D)->dout );  \
  FPRINTF( (D)->dout, __VA_ARGS__ ); )

#define DUMP_KEY(D,...) BLOCK(                \
  fput_sep( ",\n", &(D)->comma, (D)->dout );  \
  DUMP_FORMAT( (D), __VA_ARGS__ ); )

#define DUMP_LOC(D,KEY,LOC) BLOCK( \
  DUMP_KEY( (D), KEY ": " ); c_loc_dump( (LOC), (D)->dout ); )

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
struct dump_state {
  FILE     *dout;                       ///< File to dump to.
  unsigned  indent;                     ///< Current indentation.
  bool      comma;                      ///< Print a comma?
};
typedef struct dump_state dump_state_t;

/**
 * JSON object state.
 */
enum json_state {
  JSON_INIT      = 0u,                  ///< Initial state.
  JSON_COMMA     = (1u << 0),           ///< Previous "print a comma?" state.
  JSON_OBJ_BEGUN = (1u << 1)            ///< Has a JSON object already begun?
};
typedef enum json_state json_state_t;

// local functions
static void c_ast_dump_impl( c_ast_t const*, dump_state_t* );
static void c_ast_list_dump_impl( c_ast_list_t const*, dump_state_t const* );
static void c_loc_dump( c_loc_t const*, FILE* );
static void dump_init( dump_state_t*, unsigned, FILE* );
NODISCARD
static json_state_t json_object_begin( json_state_t, char const*,
                                       dump_state_t* );
static void json_object_end( json_state_t, dump_state_t* );

// local constants
static unsigned const DUMP_INDENT = 2;  ///< Spaces per dump indent level.

////////// local functions ////////////////////////////////////////////////////

/**
 * Dumps \a align in [JSON5](https://json5.org) format (for debugging).
 *
 * @param align The \ref c_alignas to dump.
 * @param dump The dump_state to use.
 */
static void c_alignas_dump_impl( c_alignas_t const *align,
                                 dump_state_t *dump ) {
  assert( align != NULL );
  assert( dump != NULL );

  if ( align->kind == C_ALIGNAS_NONE )
    return;

  json_state_t const json = json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  switch ( align->kind ) {
    case C_ALIGNAS_NONE:
      unreachable();
    case C_ALIGNAS_BYTES:
      DUMP_KEY( dump, "bytes: %u", align->bytes );
      break;
    case C_ALIGNAS_TYPE:
      DUMP_AST( dump, "type_ast", align->type_ast );
      break;
  } // switch

  DUMP_LOC( dump, "loc", &align->loc );
  json_object_end( json, dump );
}

/**
 * Dumps \a ast in [JSON5](https://json5.org) format (for debugging).
 *
 * @param ast The AST to dump.  If NULL, `null` is printed instead.
 * @param dump The dump_state to use.
 */
static void c_ast_dump_impl( c_ast_t const *ast, dump_state_t *dump ) {
  assert( dump != NULL );

  if ( ast == NULL ) {
    FPUTS( "null", dump->dout );
    return;
  }

  json_state_t const ast_json =
    json_object_begin( JSON_INIT, /*key=*/NULL, dump );

  DUMP_SNAME( dump, "sname", &ast->sname );
  DUMP_KEY( dump,
    "kind: { value: 0x%X, string: \"%s\" }",
    ast->kind, c_kind_name( ast->kind )
  );
  if ( (opt_cdecl_debug & CDECL_DEBUG_OPT_AST_UNIQUE_ID) != 0 ) {
    // LCOV_EXCL_START
    DUMP_KEY( dump, "unique_id: " PRId_C_AST_ID_T, ast->unique_id );
    if ( ast->dup_from_id > 0 )
      DUMP_KEY( dump, "dup_from_id: " PRId_C_AST_ID_T, ast->dup_from_id );
    DUMP_KEY( dump,
      "parent_id: " PRId_C_AST_ID_T,
      ast->parent_ast != NULL ? ast->parent_ast->unique_id : 0
    );
    if ( ast->param_of_ast != NULL ) {
      DUMP_KEY( dump,
        "param_of_id: " PRId_C_AST_ID_T, ast->param_of_ast->unique_id
      );
    }
    // LCOV_EXCL_STOP
  }
  DUMP_KEY( dump, "depth: %u", ast->depth );
  if ( ast->align.kind != C_ALIGNAS_NONE ) {
    DUMP_KEY( dump, "align: " );
    c_alignas_dump_impl( &ast->align, dump );
  }
  DUMP_LOC( dump, "loc", &ast->loc );
  DUMP_KEY( dump, "type: " );
  c_type_dump( &ast->type, dump->dout );

  json_state_t kind_json = JSON_INIT;

  switch ( ast->kind ) {
    case K_ARRAY:
      kind_json = json_object_begin( JSON_INIT, "array", dump );
      DUMP_KEY( dump, "size: " );
      switch ( ast->array.kind ) {
        case C_ARRAY_EMPTY_SIZE:
          FPUTS( "\"unspecified\"", dump->dout );
          break;
        case C_ARRAY_INT_SIZE:
          FPRINTF( dump->dout, "%u", ast->array.size_int );
          break;
        case C_ARRAY_NAMED_SIZE:
          FPRINTF( dump->dout, "\"%s\"", ast->array.size_name );
          break;
        case C_ARRAY_VLA_STAR:
          FPUTS( "'*'", dump->dout );
          break;
      } // switch
      DUMP_AST( dump, "of_ast", ast->array.of_ast );
      json_object_end( kind_json, dump );
      break;

    case K_TYPEDEF:
      kind_json = json_object_begin( JSON_INIT, "tdef", dump );
      DUMP_AST( dump, "for_ast", ast->tdef.for_ast );
      FALLTHROUGH;

    case K_BUILTIN:
      kind_json = json_object_begin( kind_json, "builtin", dump );
      DUMP_KEY( dump, "bit_width: %u", ast->builtin.bit_width );
      if ( c_ast_is_tid_any( ast, TB__BitInt ) )
        DUMP_KEY( dump, "BitInt: { width: %u }", ast->builtin.BitInt.width );
      json_object_end( kind_json, dump );
      break;

    case K_CAPTURE:
      kind_json = json_object_begin( JSON_INIT, "capture", dump );
      DUMP_KEY( dump, "kind: " );
      switch ( ast->capture.kind ) {
        case C_CAPTURE_COPY:
          FPUTS( "'='", dump->dout );
          break;
        case C_CAPTURE_REFERENCE:
          FPUTS( "'&'", dump->dout );
          break;
        case C_CAPTURE_STAR_THIS:
          FPUTS( "\"*this\"", dump->dout );
          break;
        case C_CAPTURE_THIS:
          FPUTS( "\"this\"", dump->dout );
          break;
        case C_CAPTURE_VARIABLE:
          FPUTS( "\"variable\"", dump->dout );
          break;
      } // switch
      json_object_end( kind_json, dump );
      break;

    case K_CAST:
      kind_json = json_object_begin( JSON_INIT, "cast", dump );
      DUMP_KEY( dump,
        "kind: { value: 0x%X, string: \"%s\" }",
        ast->cast.kind, c_cast_english( ast->cast.kind )
      );
      DUMP_AST( dump, "to_ast", ast->cast.to_ast );
      json_object_end( kind_json, dump );
      break;

    case K_CLASS_STRUCT_UNION:
      kind_json = json_object_begin( JSON_INIT, "csu", dump );
      DUMP_SNAME( dump, "csu_sname", &ast->csu.csu_sname );
      json_object_end( kind_json, dump );
      break;

    case K_OPERATOR:
      kind_json = json_object_begin( JSON_INIT, "oper", dump );
      DUMP_KEY( dump,
        "op_id: { value: %d, string: \"%s\" }",
        STATIC_CAST( int, ast->oper.operator->op_id ),
        ast->oper.operator->literal
      );
      FALLTHROUGH;

    case K_FUNCTION:
      kind_json = json_object_begin( kind_json, "func", dump );
      DUMP_KEY( dump, "member: \"" );
      switch ( ast->func.member ) {
        case C_FUNC_UNSPECIFIED:
          FPUTS( "unspecified", dump->dout );
          break;
        case C_FUNC_MEMBER:
          FPUTS( "member", dump->dout );
          break;
        case C_FUNC_NON_MEMBER:
          FPUTS( "non-member", dump->dout );
          break;
      } // switch
      FPUTC( '"', dump->dout );
      FALLTHROUGH;

    case K_APPLE_BLOCK:
      kind_json = json_object_begin( kind_json, "block", dump );
      FALLTHROUGH;
    case K_CONSTRUCTOR:
      kind_json = json_object_begin( kind_json, "ctor", dump );
      FALLTHROUGH;
    case K_UDEF_LIT:
      kind_json = json_object_begin( kind_json, "udef_lit", dump );
dump_params:
      DUMP_AST_LIST( dump, "param_ast_list", &ast->func.param_ast_list );
      if ( ast->func.ret_ast != NULL )
        DUMP_AST( dump, "ret_ast", ast->func.ret_ast );
      json_object_end( kind_json, dump );
      break;

    case K_ENUM:
      kind_json = json_object_begin( JSON_INIT, "enum", dump );
      DUMP_SNAME( dump, "enum_sname", &ast->enum_.enum_sname );
      if ( ast->enum_.of_ast != NULL )
        DUMP_AST( dump, "of_ast", ast->enum_.of_ast );
      if ( ast->enum_.bit_width > 0 )
        DUMP_KEY( dump, "bit_width: %u", ast->enum_.bit_width );
      json_object_end( kind_json, dump );
      break;

    case K_LAMBDA:
      kind_json = json_object_begin( JSON_INIT, "lambda", dump );
      DUMP_AST_LIST( dump, "capture_ast_list", &ast->lambda.capture_ast_list );
      goto dump_params;

    case K_POINTER_TO_MEMBER:
      kind_json = json_object_begin( JSON_INIT, "ptr_mbr", dump );
      DUMP_SNAME( dump, "class_sname", &ast->ptr_mbr.class_sname );
      FALLTHROUGH;
    case K_POINTER:
    case K_REFERENCE:
    case K_RVALUE_REFERENCE:
      kind_json = json_object_begin( kind_json, "ptr_ref", dump );
      FALLTHROUGH;
    case K_UDEF_CONV:
      kind_json = json_object_begin( kind_json, "udef_conv", dump );
      DUMP_AST( dump, "to_ast", ast->ptr_ref.to_ast );
      json_object_end( kind_json, dump );
      break;

    case K_DESTRUCTOR:
    case K_NAME:
    case K_PLACEHOLDER:
    case K_VARIADIC:
      // nothing to do
      break;
  } // switch

  json_object_end( ast_json, dump );
}

/**
 * Dumps \a list of ASTs in [JSON5](https://json5.org) format (for debugging).
 *
 * @param list The \ref slist of ASTs to dump.
 * @param dump The dump_state to use.
 */
static void c_ast_list_dump_impl( c_ast_list_t const *list,
                                  dump_state_t const *dump ) {
  assert( list != NULL );
  assert( dump != NULL );

  if ( slist_empty( list ) ) {
    FPUTS( "[]", dump->dout );
    return;
  }
  FPUTS( "[\n", dump->dout );

  dump_state_t list_dump;
  dump_init( &list_dump, dump->indent + 1, dump->dout );

  FOREACH_SLIST_NODE( node, list ) {
    DUMP_KEY( &list_dump, "%s", "" );
    c_ast_dump_impl( c_param_ast( node ), &list_dump );
  } // for

  FPUTC( '\n', dump->dout );
  DUMP_FORMAT( dump, "]" );
}

/**
 * Dumps \a loc in [JSON5](https://json5.org) format (for debugging).
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

  return "XTID";
}

/**
 * Initializes a dump_state.
 *
 * @param dump The dump_state to initialize.
 * @param indent The current indent.
 * @param dout The `FILE` to dump to.
 */
static void dump_init( dump_state_t *dump, unsigned indent, FILE *dout ) {
  assert( dump != NULL );
  assert( dout != NULL );

  MEM_ZERO( dump );
  dump->indent = indent;
  dump->dout = dout;
}

/**
 * Dumps the beginning of a JSON object.
 *
 * @param json The \ref json_state to use.  If not equal to #JSON_INIT, does
 * nothing.  It allows json_object_begin() to be to be called even inside a
 * `switch` statement and the previous `case` (that also called
 * json_object_begin()) falls through and not begin a new JSON object when that
 * happens allowing common code in the second case to be shared with the first
 * and not be duplicated.  For example, given:
 * ```cpp
 *  case C1:
 *    json = json_object_begin( JSON_INIT, "K1", dump );
 *    // Do stuff unique to C1.
 *    FALLTHROUGH;
 *  case C2:
 *    json = json_object_begin( json, "K2", dump ); // Passing 'json' here.
 *    // Do stuff common to C1 and C2.
 *    json_object_end( json, dump );
 *    break;
 * ```
 * There are two cases:
 * 1. `case C2` is entered: a JSON object will be begun having the key `K2`.
 *    (There is nothing special about this case.)
 * 2. `case C1` is entered: a JSON object will be begun having the key `K1`.
 *    When the case falls through into `case C2`, a second JSON object will
 *    _not_ be begun: the call to the second <code>%json_object_begin()</code>
 *    will do nothing.
 * @param key The key for the JSON object; may be NULL.  If neither NULL nor
 * empty, dumps \a key followed by `: `.
 * @param dump The dump_state to use.
 * @return Returns a new \ref json_state that must eventually be passed to
 * json_object_end().
 *
 * @sa json_object_end()
 */
static json_state_t json_object_begin( json_state_t json, char const *key,
                                       dump_state_t *dump ) {
  assert( dump != NULL );

  if ( json == JSON_INIT ) {
    key = null_if_empty( key );
    if ( key != NULL )
      DUMP_KEY( dump, "%s: ", key );
    FPUTS( "{\n", dump->dout );
    json = JSON_OBJ_BEGUN;
    if ( dump->comma ) {
      json |= JSON_COMMA;
      dump->comma = false;
    }
    ++dump->indent;
  }
  return json;
}

/**
 * Dumps the end of a JSON object.
 *
 * @param json The \ref json_state returned from json_object_begin().
 * @param dump The dump_state to use.
 *
 * @sa json_object_begin()
 */
static void json_object_end( json_state_t json, dump_state_t *dump ) {
  assert( json != JSON_INIT );
  assert( dump != NULL );

  FPUTC( '\n', dump->dout );
  dump->comma = !!(json & JSON_COMMA);
  --dump->indent;
  DUMP_FORMAT( dump, "}" );
}

////////// extern functions ///////////////////////////////////////////////////

void bool_dump( bool value, FILE *dout ) {
  assert( dout != NULL );
  FPUTS( value ? L_true : L_false, dout );
}

void c_alignas_dump( c_alignas_t const *align, FILE *dout ) {
  dump_state_t dump;
  dump_init( &dump, 1, dout );
  c_alignas_dump_impl( align, &dump );
}

void c_ast_dump( c_ast_t const *ast, FILE *dout ) {
  dump_state_t dump;
  dump_init( &dump, 1, dout );
  c_ast_dump_impl( ast, &dump );
}

void c_ast_list_dump( c_ast_list_t const *list, FILE *dout ) {
  dump_state_t dump;
  dump_init( &dump, 1, dout );
  c_ast_list_dump_impl( list, &dump );
}

void c_ast_pair_dump( c_ast_pair_t const *astp, FILE *dout ) {
  assert( astp != NULL );

  dump_state_t dump;
  dump_init( &dump, 1, dout );

  json_state_t const json = json_object_begin( JSON_INIT, /*key=*/NULL, &dump );
  DUMP_AST( &dump, "ast", astp->ast );
  DUMP_AST( &dump, "target_ast", astp->target_ast );
  json_object_end( json, &dump );
}

void c_sname_dump( c_sname_t const *sname, FILE *dout ) {
  assert( sname != NULL );
  assert( dout != NULL );

  if ( c_sname_empty( sname ) ) {
    FPUTS( "null", dout );
    return;
  }

  FPRINTF( dout, "{ string: \"%s\", scopes: \"", c_sname_full_name( sname ) );

  bool colon2 = false;
  FOREACH_SNAME_SCOPE( scope, sname ) {
    fput_sep( "::", &colon2, dout );
    c_type_t const *const t = &c_scope_data( scope )->type;
    FPUTS( c_type_is_none( t ) ? "none" : c_type_name_c( t ), dout );
  } // for

  FPUTS( "\" }", dout );
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

///////////////////////////////////////////////////////////////////////////////

/** @} */

/* vim:set et sw=2 ts=2: */
