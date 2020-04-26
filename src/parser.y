/*
**      cdecl -- C gibberish translator
**      src/parser.y
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
 * Defines helper macros, data structures, variables, functions, and the
 * grammar for C/C++ declarations.
 */

/** @cond DOXYGEN_IGNORE */

%expect 24

%{
/** @endcond */

// local
#include "cdecl.h"                      /* must go first */
#include "c_ast.h"
#include "c_ast_util.h"
#include "c_keyword.h"
#include "c_lang.h"
#include "c_operator.h"
#include "c_type.h"
#include "c_typedef.h"
#include "color.h"
#ifdef ENABLE_CDECL_DEBUG
#include "debug.h"
#endif /* ENABLE_CDECL_DEBUG */
#include "english.h"
#include "gibberish.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "print.h"
#include "slist.h"
#include "typedefs.h"
#include "util.h"

/// @cond DOXYGEN_IGNORE

// standard
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __GNUC__
// Silence these warnings for Bison-generated code.
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunreachable-code"
#endif /* __GNUC__ */

// Developer aid for tracing when Bison %destructors are called.
#if 0
#define DTRACE                    PRINT_ERR( "%d: destructor\n", __LINE__ )
#else
#define DTRACE                    NO_OP
#endif

#ifdef ENABLE_CDECL_DEBUG
#define IF_DEBUG(...)             BLOCK( if ( opt_debug ) { __VA_ARGS__ } )
#else
#define IF_DEBUG(...)             /* nothing */
#endif /* ENABLE_CDECL_DEBUG */

/// @endcond

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-group Parser
 * Helper macros, data structures, variables, functions, and the grammar for
 * C/C++ declarations.
 * @{
 */

/**
 * Calls c_ast_check(): if the check fails, calls PARSE_ABORT().
 *
 * @param AST The `c_ast` to check.
 * @param CHECK The kind of checks to perform.
 */
#define C_AST_CHECK(AST,CHECK) \
  BLOCK( if ( !c_ast_check( (AST), (CHECK) ) ) PARSE_ABORT(); )

/**
 * Calls c_type_add(): if adding the type fails, calls PARSE_ABORT().
 *
 * @param DEST_TYPE A pointer to the <code>\ref c_type_id_t</code> to add to.
 * @param NEW_TYPE The <code>\ref c_type_id_t</code> to add.
 * @param LOC The source location of \a NEW_TYPE.
 */
#define C_TYPE_ADD(DEST_TYPE,NEW_TYPE,LOC) \
  BLOCK( if ( !c_type_add( (DEST_TYPE), (NEW_TYPE), &(LOC) ) ) PARSE_ABORT(); )

/**
 * Calls elaborate_error() followed by PARSE_ABORT().
 *
 * @param ... Arguments passed to elaborate_error().
 *
 * @note
 * This must be used _only_ after an `error` token, e.g.:
 * @code
 *  | YDEFINE error
 *    {
 *      ELABORATE_ERROR( "name expected" );
 *    }
 * @endcode
 */
#define ELABORATE_ERROR(...) \
  BLOCK( elaborate_error( __VA_ARGS__ ); PARSE_ABORT(); )

/**
 * Aborts the current parse (presumably after an error message has been
 * printed).
 */
#define PARSE_ABORT()             BLOCK( parse_cleanup( true ); YYABORT; )

#define SHOW_ALL_TYPES            (~0u) ///< Show all types.
#define SHOW_PREDEFINED_TYPES     0x01u ///< Show only predefined types.
#define SHOW_USER_TYPES           0x02u ///< Show only user-defined types.

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * @defgroup parser-dump-group Debugging Macros
 * Macros that are used to dump a trace during parsing when `opt_debug` is
 * `true`.
 * @ingroup parser-group
 * @{
 */

/**
 * Dumps a comma followed by a newline the _second_ and subsequent times it's
 * called.  It's ussed to separate items being dumped.
 */
#define DUMP_COMMA \
  BLOCK( if ( true_or_set( &debug_comma ) ) PUTS_OUT( ",\n" ); )

/**
 * Dumps a `c_ast`.
 *
 * @param KEY The key name to print.
 * @param AST The `c_ast` to dump.
 */
#define DUMP_AST(KEY,AST) \
  IF_DEBUG( DUMP_COMMA; c_ast_debug( (AST), 1, (KEY), stdout ); )

/**
 * Dumps an `s_list` of `c_ast`.
 *
 * @param KEY The key name to print.
 * @param AST_LIST The `s_list` of `c_ast` to dump.
 */
#define DUMP_AST_LIST(KEY,AST_LIST) IF_DEBUG( \
  DUMP_COMMA; PUTS_OUT( "  " KEY " = " );     \
  c_ast_list_debug( &(AST_LIST), 1, stdout ); )

/**
 * Dumps an integer.
 *
 * @param KEY The key name to print.
 * @param NUM The integer to dump.
 */
#define DUMP_NUM(KEY,NUM) \
  IF_DEBUG( DUMP_COMMA; printf( "  " KEY " = %d", (int)(NUM) ); )

/**
 * Dumps a scoped name.
 *
 * @param KEY The key name to print.
 * @param SNAME The scoped name to dump.
 */
#define DUMP_SNAME(KEY,SNAME) IF_DEBUG(                   \
  DUMP_COMMA; PUTS_OUT( "  " );                           \
  kv_debug( (KEY), c_sname_full_name( SNAME ), stdout );  \
  PUTS_OUT( ", scope_type = " );                          \
  c_type_debug( c_sname_type( SNAME ), stdout ); )

/**
 * Dumps a C string.
 *
 * @param KEY The key name to print.
 * @param STR The C string to dump.
 */
#define DUMP_STR(KEY,STR) IF_DEBUG(   \
  DUMP_COMMA; PUTS_OUT( "  " );       \
  kv_debug( (KEY), (STR), stdout ); )

#ifdef ENABLE_CDECL_DEBUG
/**
 * Starts a dump block.
 *
 * @param NAME The grammar production name.
 * @param PROD The grammar production rule.
 *
 * @sa DUMP_END
 */
#define DUMP_START(NAME,PROD)                               \
  bool debug_comma = false;                                 \
  IF_DEBUG( PUTS_OUT( "\n" NAME " ::= " PROD " = {\n" ); )
#else
#define DUMP_START(NAME,PROD)     /* nothing */
#endif

/**
 * Ends a dump block.
 *
 * @sa DUMP_START
 */
#define DUMP_END()                IF_DEBUG( PUTS_OUT( "\n}\n" ); )

/**
 * Dumps the numeric value and name of a `c_type_t`.
 *
 * @param KEY The key name to print.
 * @param TYPE The type to dump.
 */
#define DUMP_TYPE(KEY,TYPE) IF_DEBUG( \
  DUMP_COMMA; PUTS_OUT( "  " KEY " = " ); c_type_debug( TYPE, stdout ); )

/** @} */

///////////////////////////////////////////////////////////////////////////////

/**
 * @addtogroup parser-group
 * @{
 */

/**
 * Print type function signature for print_type_visitor().
 */
typedef void (*print_type_t)( c_typedef_t const* );

/**
 * Information for print_type_visitor().
 */
struct print_type_info {
  print_type_t  print_fn;               ///< Print English or gibberish?
  unsigned      show_which;             ///< Predefined, user, or both?
};
typedef struct print_type_info print_type_info_t;

/**
 * Qualifier and its source location.
 */
struct c_qualifier {
  c_type_id_t type_id;                  ///< E.g., `T_CONST` or `T_VOLATILE`.
  c_loc_t     loc;                      ///< Qualifier source location.
};
typedef struct c_qualifier c_qualifier_t;

/**
 * Inherited attributes.
 */
struct in_attr {
  c_sname_t current_scope;              ///< C++ only: current scope, if any.
  slist_t   qualifier_stack;            ///< Qualifier stack.
  slist_t   type_stack;                 ///< Type stack.
};
typedef struct in_attr in_attr_t;

// extern functions
extern void           print_help( char const* );
extern void           set_option( char const*, c_loc_t const*,
                                  char const*, c_loc_t const* );

// local variables
static c_ast_depth_t  ast_depth;        ///< Parentheses nesting depth.
static slist_t        ast_gc_list;      ///< `c_ast` nodes freed after parse.
static slist_t        ast_typedef_list; ///< `c_ast` nodes for `typedef`s.
static bool           error_newlined = true;
static in_attr_t      in_attr;          ///< Inherited attributes.

////////// inline functions ///////////////////////////////////////////////////

/**
 * Creates a new `c_ast` and adds it to the garbage-collection list.
 *
 * @param kind The kind of `c_ast` to create.
 * @param loc A pointer to the token location data.
 * @return Returns a pointer to a new `c_ast`.
 */
C_WARN_UNUSED_RESULT
static inline c_ast_t* c_ast_new_gc( c_kind_t kind, c_loc_t *loc ) {
  c_ast_t *const ast = c_ast_new( kind, ast_depth, loc );
  slist_push_tail( &ast_gc_list, ast );
  return ast;
}

/**
 * Gets a printable string of the lexer's current token.
 *
 * @return Returns said string.
 */
C_WARN_UNUSED_RESULT
static inline char const* printable_token( void ) {
  return lexer_token[0] == '\n' ? "\\n" : lexer_token;
}

/**
 * Peeks at the type `c_ast` at the top of the type AST inherited attribute
 * stack.
 *
 * @return Returns said `c_ast`.
 */
C_WARN_UNUSED_RESULT
static inline c_ast_t* type_peek( void ) {
  return SLIST_HEAD( c_ast_t*, &in_attr.type_stack );
}

/**
 * Pops a type `c_ast` from the type AST inherited attribute stack.
 *
 * @return Returns said `c_ast`.
 */
C_NOWARN_UNUSED_RESULT
static inline c_ast_t* type_pop( void ) {
  return SLIST_POP_HEAD( c_ast_t*, &in_attr.type_stack );
}

/**
 * Pushes a type `c_ast` onto the type AST inherited attribute stack.
 *
 * @param ast The `c_ast` to push.
 */
static inline void type_push( c_ast_t *ast ) {
  slist_push_head( &in_attr.type_stack, ast );
}

/**
 * Peeks at the qualifier at the top of the qualifier inherited attribute
 * stack.
 *
 * @return Returns said qualifier.
 */
C_WARN_UNUSED_RESULT
static inline c_type_id_t qualifier_peek( void ) {
  return SLIST_HEAD( c_qualifier_t*, &in_attr.qualifier_stack )->type_id;
}

/**
 * Peeks at the location of the qualifier at the top of the qualifier inherited
 * attribute stack.
 *
 * @note This is a macro instead of an inline function because it should return
 * a reference (not a pointer), but C doesn't have references.
 *
 * @return Returns said qualifier location.
 */
#define qualifier_peek_loc() \
  (SLIST_HEAD( c_qualifier_t*, &in_attr.qualifier_stack )->loc)

/**
 * Pops a qualifier from the qualifier inherited attribute stack and frees it.
 */
static inline void qualifier_pop( void ) {
  FREE( slist_pop_head( &in_attr.qualifier_stack ) );
}

////////// extern functions ///////////////////////////////////////////////////

/**
 * Cleans up parser data at program termination.
 */
void parser_cleanup( void ) {
  slist_free( &ast_typedef_list, NULL, (slist_node_data_free_fn_t)&c_ast_free );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Adds a type to the global set.
 *
 * @param decl_keyword The keyword used for the declaration.
 * @param type_ast The `c_ast` of the type to add.
 * @param type_decl_loc The location of the offending type declaration.
 * @return Returns `true` either if the type was added or it's equivalent to
 * the existing type; `false` if a different type already exists having the
 * same name.
 */
C_WARN_UNUSED_RESULT
static bool add_type( char const *decl_keyword, c_ast_t const *type_ast,
                      c_loc_t const *type_decl_loc ) {
  assert( decl_keyword != NULL );
  assert( type_ast != NULL );
  assert( type_decl_loc != NULL );

  switch ( c_typedef_add( type_ast ) ) {
    case TD_ADD_ADDED:
      //
      // We have to move the AST from the ast_gc_list so it won't be garbage
      // collected at the end of the parse to a separate ast_typedef_list
      // that's freed only at program termination.
      //
      slist_push_list_tail( &ast_typedef_list, &ast_gc_list );
      break;
    case TD_ADD_DIFF:
      print_error( type_decl_loc,
        "\"%s\": \"%s\" redefinition with different type",
        c_ast_sname_full_name( type_ast ), decl_keyword
      );
      return false;
    case TD_ADD_EQUIV:
      // Do nothing.
      break;
  } // switch
  return true;
}

/**
 * Prints an additional parsing error message to standard error that continues
 * from where `yyerror()` left off.
 *
 * @param format A `printf()` style format string.
 * @param ... Arguments to print.
 */
C_PRINTF_LIKE_FUNC(1)
static void elaborate_error( char const *format, ... ) {
  assert( format != NULL );
  if ( !error_newlined ) {
    PUTS_ERR( ": " );
    if ( lexer_token[0] != '\0' )
      PRINT_ERR( "\"%s\": ", printable_token() );
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    PUTC_ERR( '\n' );
    error_newlined = true;
  }
}

/**
 * Explains a declaration.
 *
 * @param align The `alignas` specifier, if any.
 * @param align_loc The source location of \a align, if any.
 * @param type_ast The type `c_ast`.
 * @param decl_ast The declaration `c_ast`.
 * @param decl_loc The source location of \a decl_ast.
 * @return Returns `true` only upon success.
 */
C_WARN_UNUSED_RESULT
static bool explain_type_decl( c_alignas_t const *align,
                               c_loc_t const *align_loc, c_ast_t *type_ast,
                               c_ast_t *decl_ast, c_loc_t const *decl_loc ) {
  assert( type_ast != NULL );
  assert( decl_ast != NULL );
  assert( decl_loc != NULL );

  type_pop();

  bool is_typedef = c_ast_take_typedef( type_ast );

  if ( is_typedef && decl_ast->kind == K_TYPEDEF ) {
    //
    // This is for a case like:
    //
    //      explain typedef int int32_t;
    //
    // that is: explaining an existing typedef.  In order to do that, we
    // have to un-typedef it so we explain the type that it's typedef'd as.
    //
    decl_ast = CONST_CAST( c_ast_t*, c_ast_untypedef( decl_ast ) );

    //
    // However, we also have to check whether the typedef being explained
    // is not equivalent to the existing typedef.  This is for a case like:
    //
    //      explain typedef char int32_t;
    //
    if ( !c_ast_equiv( type_ast, decl_ast ) ) {
      print_error( decl_loc,
        "\"%s\": \"%s\" redefinition with different type",
        c_ast_sname_full_name( decl_ast ), L_TYPEDEF
      );
      return false;
    }
  }

  c_ast_t *const ast = c_ast_patch_placeholder( type_ast, decl_ast );
  is_typedef = is_typedef || c_ast_take_typedef( ast );

  if ( align != NULL ) {
    if ( is_typedef ) {
      //
      // We check for illegal aligned typedef here rather than in error.c
      // because the "typedef-ness" needed to be removed (above) before the
      // call to c_ast_check() below.
      //
      print_error( align_loc, "%s can not be %s", L_TYPEDEF, L_ALIGNED );
      return false;
    }
    ast->align = *align;
  }

  switch ( ast->kind ) {
    case K_CONSTRUCTOR:
    case K_USER_DEF_CONVERSION:
      ast->type_id |= type_ast->type_id;
      break;
    default:
      /* suppress warning */;
  } // switch

  if ( !c_ast_check( ast, C_CHECK_DECL ) )
    return false;

  if ( ast->kind == K_USER_DEF_CONVERSION ) {
    if ( c_ast_sname_type( ast ) == T_SCOPE )
      c_ast_sname_set_type( ast, T_CLASS );
    FPRINTF( fout, "%s ", L_DECLARE );
  }
  else {
    c_sname_t sname = c_ast_take_name( ast );
    char const *local_name = NULL;
    char const *scope_name = "";

    switch ( ast->kind ) {
      case K_OPERATOR:
        local_name = op_token_c( ast->as.oper.oper_id );
        scope_name = c_sname_full_name( &sname );
        break;
      default:
        assert( !c_sname_empty( &sname ) );
        local_name = c_sname_local_name( &sname );
        scope_name = c_sname_scope_name( &sname );
        break;
    } // switch

    assert( local_name != NULL );
    FPRINTF( fout, "%s %s ", L_DECLARE, local_name );
    if ( scope_name[0] != '\0' ) {
      c_type_id_t const sn_type = c_sname_type( &sname );
      assert( sn_type != T_NONE );
      FPRINTF( fout,
        "%s %s %s ", L_OF, c_type_name( sn_type ), scope_name
      );
    }
    FPRINTF( fout, "%s ", L_AS );
    if ( is_typedef )
      FPRINTF( fout, "%s ", L_TYPE );
    if ( ast->kind != K_OPERATOR )
      c_sname_free( &sname );
  }

  c_ast_english( ast, fout );
  FPUTC( '\n', fout );

  return true;
}

/**
 * Checks whether the `_Noreturn` token is OK in the current language.  If not,
 * prints an error message (and perhaps a hint).
 *
 * @param loc The location of the `_Noreturn` token.
 * @return Returns `true` only if `_Noreturn` is OK.
 */
C_WARN_UNUSED_RESULT
static bool _Noreturn_ok( c_loc_t const *loc ) {
  assert( loc != NULL );

  if ( c_init < C_INIT_READ_CONF || (opt_lang & LANG_C_MIN(11)) != LANG_NONE )
    return true;
  print_error( loc, "\"%s\" not supported in %s", lexer_token, C_LANG_NAME() );
  if ( opt_lang >= LANG_CPP_11 ) {
    if ( c_mode == C_ENGLISH_TO_GIBBERISH )
      print_hint( "\"%s\"", L_NORETURN );
    else
      print_hint( "[[%s]]", L_NORETURN );
  }
  return false;
}

/**
 * Cleans-up parser data after each parse.
 *
 * @param hard_reset If `true`, does a "hard" reset that currently resets the
 * EOF flag of the lexer.  This should be `true` if an error occurs and
 * `YYABORT` is called.
 */
static void parse_cleanup( bool hard_reset ) {
  lexer_reset( hard_reset );
  slist_free( &ast_gc_list, NULL, (slist_node_data_free_fn_t)&c_ast_free );
  c_sname_free( &in_attr.current_scope );
  slist_free( &in_attr.qualifier_stack, NULL, &free );
  slist_free( &in_attr.type_stack, NULL, NULL );
  MEM_ZERO( &in_attr );
}

/**
 * Gets ready to parse a command.
 */
static void parse_init( void ) {
  if ( !error_newlined ) {
    FPUTC( '\n', fout );
    error_newlined = true;
  }
}

/**
 * Prints a `c_typedef` in English.
 *
 * @param type A pointer to the `c_typedef` to print.
 */
static void print_type_as_english( c_typedef_t const *type ) {
  assert( type != NULL );

  FPRINTF( fout, "%s %s ", L_DEFINE, c_ast_sname_local_name( type->ast ) );
  if ( c_ast_sname_count( type->ast ) > 1 )
    FPRINTF( fout,
      "%s %s %s ",
      L_OF, c_ast_sname_type_name( type->ast ),
      c_ast_sname_scope_name( type->ast )
    );
  FPRINTF( fout, "%s ", L_AS );
  c_ast_english( type->ast, fout );
  FPUTC( '\n', fout );
}

/**
 * Prints a `c_typedef` as a `typedef`.
 *
 * @param type A pointer to the `c_typedef` to print.
 */
static void print_type_as_typedef( c_typedef_t const *type ) {
  assert( type != NULL );

  size_t scope_close_braces_to_print = 0;
  c_type_id_t sn_type = T_NONE;

  c_sname_t const *const sname = c_ast_find_name( type->ast, C_VISIT_DOWN );
  if ( sname != NULL && c_sname_count( sname ) > 1 ) {
    sn_type = c_sname_type( sname );
    assert( sn_type != T_NONE );
    //
    // Replace the generic "scope" with "namespace" for C++.
    //
    if ( (sn_type & T_SCOPE) != T_NONE )
      sn_type = (sn_type & ~T_SCOPE) | T_NAMESPACE;
    //
    // A type name can't be scoped in a typedef declaration, e.g.:
    //
    //      typedef int S::T::I;        // illegal
    //
    // so we have to wrap it in a scoped declaration, one of: class, namespace,
    // struct, or union.
    //
    if ( (sn_type & T_NAMESPACE) == T_NONE || opt_lang >= LANG_CPP_17 ) {
      //
      // All C++ versions support nested scope declarations, e.g.:
      //
      //      struct S::T { typedef int I; }
      //
      // However, only C++17 and later support nested namespace declarations:
      //
      //      namespace S::T { typedef int I; }
      //
      FPRINTF( fout,
        "%s %s { ", c_type_name( sn_type ), c_sname_scope_name( sname )
      );
      scope_close_braces_to_print = 1;
    }
    else {
      //
      // Namespaces in C++14 and earlier require distinct declarations:
      //
      //      namespace S { namespace T { typedef int I; } }
      //
      for ( c_scope_t const *scope = sname->head; scope != sname->tail;
            scope = scope->next ) {
        FPRINTF( fout, "%s %s { ", L_NAMESPACE, c_scope_name( scope ) );
      } // for
      scope_close_braces_to_print = c_sname_count( sname ) - 1;
    }
  }

  FPRINTF( fout, "%s ", L_TYPEDEF );
  c_ast_gibberish_declare( type->ast, G_DECL_TYPEDEF, fout );

  if ( scope_close_braces_to_print > 0 ) {
    FPUTC( ';', fout );
    while ( scope_close_braces_to_print-- > 0 )
      FPUTS( " }", fout );
  }

  if ( opt_semicolon && (sn_type & T_NAMESPACE) == T_NONE )
    FPUTC( ';', fout );
  FPUTC( '\n', fout );
}

/**
 * Prints the definition of a `typedef`.
 *
 * @param type A pointer to the `c_typedef` to print.
 * @param data Optional data passed to the visitor: in this case, the bitmask
 * of which `typedef`s to print.
 * @return Always returns `false`.
 */
C_WARN_UNUSED_RESULT
static bool print_type_visitor( c_typedef_t const *type, void *data ) {
  assert( type != NULL );

  print_type_info_t const *const pti =
    REINTERPRET_CAST( print_type_info_t const*, data );

  bool const show_it = type->user_defined ?
    (pti->show_which & SHOW_USER_TYPES) != 0 :
    (pti->show_which & SHOW_PREDEFINED_TYPES) != 0;

  if ( show_it )
    (*pti->print_fn)( type );
  return false;
}

/**
 * Pushes a qualifier onto the qualifier inherited attribute stack.
 *
 * @param qualifier The qualifier to push.
 * @param loc A pointer to the source location of the qualifier.
 */
static void qualifier_push( c_type_id_t qualifier, c_loc_t const *loc ) {
  assert( (qualifier & ~T_MASK_QUALIFIER) == T_NONE );
  assert( loc != NULL );

  c_qualifier_t *const qual = MALLOC( c_qualifier_t, 1 );
  qual->type_id = qualifier;
  qual->loc = *loc;
  slist_push_head( &in_attr.qualifier_stack, qual );
}

/**
 * Implements the cdecl `quit` command.
 *
 * @note
 * This should be marked #C_NORETURN but isn't since that would generate a
 * warning that a `break` in the Bison-generated code won't be executed.
 */
static void quit( void ) {
  exit( EX_OK );
}

/**
 * Prints a parsing error message to standard error.  This function is called
 * directly by Bison to print just `syntax error` (usually).
 *
 * @note A newline is \e not printed since the error message will be appended
 * to by `elaborate_error()`.  For example, the parts of an error message are
 * printed by the functions shown:
 *
 *      42: syntax error: "int": "into" expected
 *      |--||----------||----------------------|
 *      |   |           |
 *      |   yyerror()   elaborate_error()
 *      |
 *      print_loc()
 *
 * @param msg The error message to print.
 */
static void yyerror( char const *msg ) {
  assert( msg != NULL );

  c_loc_t loc;
  MEM_ZERO( &loc );
  lexer_loc( &loc.first_line, &loc.first_column );
  print_loc( &loc );

  SGR_START_COLOR( stderr, error );
  PUTS_ERR( msg );                      // no newline
  SGR_END_COLOR( stderr );
  error_newlined = false;

  parse_cleanup( false );
}

/** @} */

///////////////////////////////////////////////////////////////////////////////

/// @cond DOXYGEN_IGNORE

%}

%union {
  c_alignas_t         align;
  c_ast_t            *ast;
  slist_t             ast_list;   /* for function arguments */
  c_ast_pair_t        ast_pair;   /* for the AST being built */
  unsigned            bitmask;    /* multipurpose bitmask (used by show) */
  char const         *literal;    /* token literal (for new-style casts) */
  char               *name;       /* name being declared or explained */
  int                 number;     /* for array sizes */
  c_oper_id_t         oper_id;    /* overloaded operator ID */
  c_sname_t           sname;      /* name being declared or explained */
  c_type_id_t         type_id;    /* built-ins, storage classes, & qualifiers */
  c_typedef_t const  *c_typedef;  /* typedef type */
}

                    /* cdecl commands */
%token              Y_CAST
%token              Y_DECLARE
%token              Y_DEFINE
%token              Y_EXPLAIN
%token              Y_HELP
%token              Y_SET
%token              Y_SHOW
%token              Y_QUIT

                    /* english */
%token              Y_ALIGNED
%token              Y_ALL
%token              Y_ARRAY
%token              Y_AS
%token              Y_BLOCK             /* Apple: English for '^' */
%token              Y_BYTES
%token              Y_COMMANDS
%token              Y_DYNAMIC
%token              Y_ENGLISH
%token              Y_FUNCTION
%token              Y_INTO
%token              Y_LENGTH
%token              Y_MEMBER
%token              Y_NON_MEMBER
%token              Y_OF
%token              Y_POINTER
%token              Y_PREDEFINED
%token              Y_PURE
%token              Y_REFERENCE
%token              Y_REINTERPRET
%token              Y_RETURNING
%token              Y_RVALUE
%token              Y_SCOPE
%token              Y_TO
%token              Y_TYPES
%token              Y_USER
%token              Y_VARIABLE

                    /* C & C++ operators (one-character) */
%token  <oper_id>   Y_EXCLAM            /* '!' */
%token  <oper_id>   '%'
%token  <oper_id>   Y_AMPER             /* '&' */
%token  <oper_id>   '(' ')'
%token  <oper_id>   '*'
%token  <oper_id>   '+'
%token  <oper_id>   ','
%token  <oper_id>   '-'
%token  <oper_id>   '.'
%token  <oper_id>   '/'
%token              ';'
%token  <oper_id>   '<' '>'
%token  <oper_id>   '='
%token  <oper_id>   Y_QMARK_COLON "?:"
%token  <oper_id>   '[' ']'
%token  <oper_id>   Y_CIRC              /* '^' */
%token              '{'
%token  <oper_id>   Y_PIPE              /* '|' */
%token              '}'
%token  <oper_id>   Y_TILDE             /* '~' */

                    /* C & C++ operators (two-character) */
%token  <oper_id>   Y_EXCLAM_EQ   "!="
%token  <oper_id>   Y_PERCENT_EQ  "%="
%token  <oper_id>   Y_AMPER2      "&&"
%token  <oper_id>   Y_AMPER_EQ    "&="
%token  <oper_id>   Y_STAR_EQ     "*="
%token  <oper_id>   Y_PLUS2       "++"
%token  <oper_id>   Y_PLUS_EQ     "+="
%token  <oper_id>   Y_MINUS2      "--"
%token  <oper_id>   Y_MINUS_EQ    "-="
%token  <oper_id>   Y_ARROW       "->"
%token  <oper_id>   Y_SLASH_EQ    "/="
%token  <oper_id>   Y_LESS2       "<<"
%token  <oper_id>   Y_LESS2_EQ    "<<="
%token  <oper_id>   Y_LESS_EQ     "<="
%token  <oper_id>   Y_EQ2         "=="
%token  <oper_id>   Y_GREATER_EQ  ">="
%token  <oper_id>   Y_GREATER2    ">>"
%token  <oper_id>   Y_GREATER2_EQ ">>="
%token  <oper_id>   Y_CIRC_EQ     "^="
%token  <oper_id>   Y_PIPE_EQ     "|="
%token  <oper_id>   Y_PIPE2       "||"

                    /* K&R C */
%token  <type_id>   Y_AUTO_STORAGE      /* C version of "auto" */
%token              Y_BREAK
%token              Y_CASE
%token  <type_id>   Y_CHAR
%token              Y_CONTINUE
%token  <type_id>   Y_DEFAULT
%token              Y_DO
%token  <type_id>   Y_DOUBLE
%token              Y_ELSE
%token  <type_id>   Y_EXTERN
%token  <type_id>   Y_FLOAT
%token              Y_FOR
%token              Y_GOTO
%token              Y_IF
%token  <type_id>   Y_INT
%token  <type_id>   Y_LONG
%token  <type_id>   Y_REGISTER
%token              Y_RETURN
%token  <type_id>   Y_SHORT
%token  <type_id>   Y_SIZEOF
%token  <type_id>   Y_STATIC
%token  <type_id>   Y_STRUCT
%token              Y_SWITCH
%token  <type_id>   Y_TYPEDEF
%token  <type_id>   Y_UNION
%token  <type_id>   Y_UNSIGNED
%token              Y_WHILE

                    /* C89 */
%token  <type_id>   Y_CONST
%token              Y_ELLIPSIS    "..." /* for varargs */
%token  <type_id>   Y_ENUM
%token  <type_id>   Y_SIGNED
%token  <type_id>   Y_VOID
%token  <type_id>   Y_VOLATILE

                    /* C95 */
%token  <type_id>   Y_WCHAR_T

                    /* C99 */
%token  <type_id>   Y_BOOL
%token  <type_id>   Y__COMPLEX
%token  <type_id>   Y__IMAGINARY
%token  <type_id>   Y_INLINE
%token  <type_id>   Y_RESTRICT

                    /* C11 */
%token              Y__ALIGNAS
%token              Y__ALIGNOF
%token  <type_id>   Y_ATOMIC_QUAL       /* qualifier: _Atomic type */
%token  <type_id>   Y_ATOMIC_SPEC       /* specifier: _Atomic (type) */
%token              Y__GENERIC
%token  <type_id>   Y__NORETURN
%token              Y__STATIC_ASSERT

                    /* C++ */
%token  <oper_id>   Y_ARROW_STAR  "->*"
%token              Y_CATCH
%token  <type_id>   Y_CLASS
%token  <oper_id>   Y_COLON2      "::"
%token              Y_COLON2_STAR "::*"
%token  <literal>   Y_CONST_CAST
%token              Y_CONSTRUCTOR
%token  <sname>     Y_CONSTRUCTOR_SNAME
%token              Y_CONVERSION
%token  <type_id>   Y_DELETE
%token              Y_DESTRUCTOR
%token  <sname>     Y_DESTRUCTOR_SNAME
%token  <oper_id>   Y_DOT_STAR    ".*"
%token  <literal>   Y_DYNAMIC_CAST
%token  <type_id>   Y_EXPLICIT
%token              Y_EXPORT
%token  <type_id>   Y_FALSE             /* for noexcept(false) */
%token  <type_id>   Y_FRIEND
%token  <type_id>   Y_MUTABLE
%token  <type_id>   Y_NAMESPACE
%token  <type_id>   Y_NEW
%token              Y_OPERATOR
%token              Y_PRIVATE
%token              Y_PROTECTED
%token              Y_PUBLIC
%token  <literal>   Y_REINTERPRET_CAST
%token  <literal>   Y_STATIC_CAST
%token              Y_TEMPLATE
%token              Y_THIS
%token  <type_id>   Y_THROW
%token  <type_id>   Y_TRUE              /* for noexcept(true) */
%token              Y_TRY
%token              Y_TYPEID
%token              Y_TYPENAME
%token  <type_id>   Y_USING
%token  <type_id>   Y_VIRTUAL

                    /* GNU C extensions */
%token  <type_id>   Y___RESTRICT__

                    /* C++11 */
%token              Y_LBRACKET2   "[["  /* for attribute specifiers */
%token              Y_RBRACKET2   "]]"  /* for attribute specifiers */
%token              Y_ALIGNAS
%token              Y_ALIGNOF
%token  <type_id>   Y_AUTO_TYPE         /* C++11 version of "auto" */
%token  <type_id>   Y_CARRIES_DEPENDENCY
%token  <type_id>   Y_CONSTEXPR
%token              Y_DECLTYPE
%token  <type_id>   Y_FINAL
%token              Y_LITERAL
%token  <type_id>   Y_NOEXCEPT
%token              Y_NULLPTR
%token  <type_id>   Y_OVERRIDE
%token              Y_QUOTE2            /* for user-defined literals */
%token              Y_USER_DEFINED

                    /* C11 & C++11 */
%token  <type_id>   Y_CHAR16_T
%token  <type_id>   Y_CHAR32_T
%token  <type_id>   Y_THREAD_LOCAL

                    /* C++14 */
%token  <type_id>   Y_DEPRECATED

                    /* C++17 */
%token  <type_id>   Y_MAYBE_UNUSED
%token  <type_id>   Y_NORETURN
%token  <type_id>   Y_NODISCARD

                    /* C++20 */
%token  <type_id>   Y_CHAR8_T
%token              Y_CONCEPT
%token  <type_id>   Y_CONSTEVAL
%token  <oper_id>   Y_LESS_EQ_GREATER "<=>"
%token              Y_REQUIRES

                    /* miscellaneous */
%token  <type_id>   Y___BLOCK           /* Apple: block storage class */
%token              Y_END
%token              Y_ERROR
%token  <name>      Y_HYPHENATED_NAME
%token  <name>      Y_NAME
%token  <number>    Y_NUMBER
%token  <name>      Y_SET_OPTION
%token  <c_typedef> Y_TYPEDEF_NAME      /* e.g., T x */
%token  <c_typedef> Y_TYPEDEF_SNAME     /* e.g., S::T y */

                    /*
                     * Grammar rules are named according to the following
                     * conventions.  In order, if a rule:
                     *
                     *  1. Is a list, "_list" is appended.
                     *  2. Is specific to C/C++, "_c" is appended; is specific
                     *     to English, "_english" is appended.
                     *  3. Is of type:
                     *      + <ast> or <ast_pair>: "_ast" is appended.
                     *      + <c_typedef>: "_typedef" is appended.
                     *      + <name>: "_name" is appended.
                     *      + <number>: "_num" is appended.
                     *      + <sname>: "_sname" is appended.
                     *      + <type_id>: "_type" is appended.
                     *  4. Is expected, "_expected" is appended; is optional,
                     *     "_opt" is appended.
                     */
%type   <align>     alignas_specifier_english alignas_specifier_english_opt
%type   <ast_pair>  decl_english_ast
%type   <ast_list>  decl_list_english decl_list_english_opt
%type   <ast_pair>  array_decl_english_ast
%type   <number>    array_size_num_opt
%type   <type_id>   attribute_english_type
%type   <ast_pair>  block_decl_english_ast
%type   <ast_pair>  constructor_decl_english_ast
%type   <ast_pair>  destructor_decl_english_ast
%type   <ast_pair>  func_decl_english_ast
%type   <ast_pair>  oper_decl_english_ast
%type   <ast_list>  paren_decl_list_english_opt
%type   <ast_pair>  pointer_decl_english_ast
%type   <ast_pair>  qualifiable_decl_english_ast
%type   <ast_pair>  qualified_decl_english_ast
%type   <type_id>   ref_qualifier_english_type_opt
%type   <ast_pair>  reference_decl_english_ast
%type   <ast_pair>  reference_english_ast
%type   <ast_pair>  returning_english_ast_opt
%type   <type_id>   storage_class_english_type
%type   <type_id>   storage_class_list_english_type_opt
%type   <type_id>   type_attribute_english_type
%type   <ast_pair>  type_english_ast
%type   <type_id>   type_modifier_english_type
%type   <type_id>   type_modifier_list_english_type
%type   <type_id>   type_modifier_list_english_type_opt
%type   <ast_pair>  unmodified_type_english_ast
%type   <ast_pair>  user_defined_literal_decl_english_ast
%type   <ast_pair>  var_decl_english_ast

%type   <ast_pair>  cast_c_ast cast_c_ast_opt cast2_c_ast
%type   <ast_pair>  array_cast_c_ast
%type   <ast_pair>  block_cast_c_ast
%type   <ast_pair>  func_cast_c_ast
%type   <type_id>   func_equals_type_opt
%type   <ast_pair>  nested_cast_c_ast
%type   <ast_pair>  pointer_cast_c_ast
%type   <ast_pair>  pointer_to_member_cast_c_ast
%type   <ast_pair>  reference_cast_c_ast

%type   <ast_pair>  decl_c_ast decl2_c_ast
%type   <ast_pair>  array_decl_c_ast
%type   <number>    array_size_c_num
%type   <ast_pair>  block_decl_c_ast
%type   <ast_pair>  func_decl_c_ast
%type   <type_id>   func_ref_qualifier_c_type_opt
%type   <ast_pair>  nested_decl_c_ast
%type   <type_id>   noexcept_c_type_opt
%type   <ast_pair>  oper_c_ast
%type   <ast_pair>  oper_decl_c_ast
%type   <ast_pair>  pointer_decl_c_ast
%type   <ast_pair>  pointer_to_member_decl_c_ast
%type   <ast_pair>  pointer_to_member_type_c_ast
%type   <ast_pair>  pointer_type_c_ast
%type   <ast_pair>  reference_decl_c_ast
%type   <ast_pair>  reference_type_c_ast
%type   <ast_pair>  trailing_return_type_c_ast_opt
%type   <ast_pair>  typedef_type_decl_c_ast
%type   <ast_pair>  unmodified_type_c_ast
%type   <ast_pair>  user_defined_conversion_decl_c_ast
%type   <ast_pair>  user_defined_literal_c_ast
%type   <ast_pair>  user_defined_literal_decl_c_ast

%type   <ast_pair>  udc_decl_c_ast udc_decl_c_ast_opt
%type   <ast_pair>  pointer_udc_decl_c_ast
%type   <ast_pair>  pointer_to_member_udc_decl_c_ast
%type   <ast_pair>  reference_udc_decl_c_ast

%type   <ast_pair>  atomic_specifier_type_c_ast
%type   <ast_pair>  builtin_type_c_ast
%type   <ast_pair>  enum_class_struct_union_ast
%type   <ast_pair>  placeholder_c_ast
%type   <ast_pair>  type_c_ast
%type   <ast_pair>  typedef_type_c_ast

%type   <type_id>   attribute_name_c_type
%type   <type_id>   attribute_name_list_c_type attribute_name_list_c_type_opt
%type   <type_id>   attribute_specifier_list_c_type
%type   <type_id>   builtin_type
%type   <type_id>   class_struct_type class_struct_type_expected
%type   <type_id>   class_struct_union_type
%type   <type_id>   cv_qualifier_type cv_qualifier_list_c_type_opt
%type   <type_id>   enum_class_struct_union_type
%type   <type_id>   func_qualifier_c_type
%type   <type_id>   func_qualifier_list_c_type_opt
%type   <type_id>   namespace_expected
%type   <type_id>   namespace_type
%type   <type_id>   noexcept_bool_type
%type   <type_id>   storage_class_c_type
%type   <type_id>   type_modifier_c_type
%type   <type_id>   type_modifier_base_type
%type   <type_id>   type_modifier_list_c_type type_modifier_list_c_type_opt
%type   <type_id>   type_qualifier_c_type
%type   <type_id>   type_qualifier_list_c_type type_qualifier_list_c_type_opt

%type   <ast>       arg_array_size_c_ast
%type   <ast_pair>  arg_c_ast
%type   <ast_list>  arg_list_c_ast arg_list_c_ast_opt
%type   <ast_pair>  name_ast
%type   <ast_pair>  sname_c_ast
%type   <ast_pair>  typedef_name_ast
%type   <ast_pair>  using_name_ast_expected

%type   <align>     alignas_specifier_c
%type   <name>      any_name any_name_expected
%type   <sname>     any_sname_c any_sname_c_expected any_sname_c_opt
%type   <c_typedef> any_typedef
%type   <oper_id>   c_operator
%type   <literal>   help_what_opt
%type   <bitmask>   member_or_non_member_opt
%type   <name>      name_expected
%type   <literal>   new_style_cast_c new_style_cast_english
%type   <sname>     of_scope_english
%type   <sname>     of_scope_list_english of_scope_list_english_opt
%type   <type_id>   scope_english_type scope_english_type_expected
%type   <sname>     scope_sname_c_opt
%type   <sname>     sname_c sname_c_expected sname_c_opt
%type   <sname>     sname_english sname_english_expected
%type   <name>      set_option_value_opt
%type   <bitmask>   show_which_types_opt
%type   <type_id>   static_type_opt
%type   <bitmask>   typedef_opt
%type   <sname>     typedef_sname_c
%type   <type_id>   virtual_opt

/*
 * Bison %destructors.  We don't use the <identifier> syntax because older
 * versions of Bison don't support it.
 *
 * Clean-up of AST nodes is done via garbage collection using ast_gc_list.
 */

/* name */
%destructor { DTRACE; FREE( $$ ); } any_name
%destructor { DTRACE; FREE( $$ ); } any_name_expected
%destructor { DTRACE; FREE( $$ ); } name_expected
%destructor { DTRACE; FREE( $$ ); } set_option_value_opt
%destructor { DTRACE; FREE( $$ ); } Y_HYPHENATED_NAME
%destructor { DTRACE; FREE( $$ ); } Y_NAME
%destructor { DTRACE; FREE( $$ ); } Y_SET_OPTION

/* sname */
%destructor { DTRACE; c_sname_free( &$$ ); } any_sname_c
%destructor { DTRACE; c_sname_free( &$$ ); } any_sname_c_expected
%destructor { DTRACE; c_sname_free( &$$ ); } of_scope_english
%destructor { DTRACE; c_sname_free( &$$ ); } of_scope_list_english
%destructor { DTRACE; c_sname_free( &$$ ); } of_scope_list_english_opt
%destructor { DTRACE; c_sname_free( &$$ ); } scope_sname_c_opt
%destructor { DTRACE; c_sname_free( &$$ ); } sname_c
%destructor { DTRACE; c_sname_free( &$$ ); } sname_c_expected
%destructor { DTRACE; c_sname_free( &$$ ); } sname_c_opt
%destructor { DTRACE; c_sname_free( &$$ ); } sname_english
%destructor { DTRACE; c_sname_free( &$$ ); } sname_english_expected
%destructor { DTRACE; c_sname_free( &$$ ); } typedef_sname_c
%destructor { DTRACE; c_sname_free( &$$ ); } Y_CONSTRUCTOR_SNAME
%destructor { DTRACE; c_sname_free( &$$ ); } Y_DESTRUCTOR_SNAME

/*****************************************************************************/
%%

command_list
  : /* empty */
  | command_list { parse_init(); } command
    {
      //
      // We get here only after a successful parse, so a hard reset is not
      // needed.
      //
      parse_cleanup( false );
    }
  ;

command
  : cast_english semi_or_end
  | declare_english semi_or_end
  | define_english semi_or_end
  | explain_c semi_or_end
  | help_command semi_or_end
  | quit_command semi_or_end
  | scope_declaration_c
  | set_command semi_or_end
  | show_command semi_or_end
  | typedef_declaration_c semi_or_end
  | using_declaration_c semi_or_end
  | semi_or_end                         /* allows for blank lines */
  | error
    {
      if ( lexer_token[0] != '\0' )
        ELABORATE_ERROR( "unexpected token" );
      else
        ELABORATE_ERROR( "unexpected end of command" );
    }
  ;

/*****************************************************************************/
/*  cast                                                                     */
/*****************************************************************************/

cast_english
  : Y_CAST sname_english_expected as_into_to_expected decl_english_ast
    {
      DUMP_START( "cast_english",
                  "CAST sname_english_expected INTO decl_english_ast" );
      DUMP_SNAME( "sname_english_expected", &$2 );
      DUMP_AST( "decl_english_ast", $4.ast );
      DUMP_END();

      bool const ok = c_ast_check( $4.ast, C_CHECK_CAST );

      if ( ok ) {
        FPUTC( '(', fout );
        c_ast_gibberish_cast( $4.ast, fout );
        FPRINTF( fout, ")%s\n", c_sname_full_name( &$2 ) );
      }

      c_sname_free( &$2 );
      if ( !ok )
        PARSE_ABORT();
    }

  | new_style_cast_english cast_expected sname_english_expected
    as_into_to_expected decl_english_ast
    {
      DUMP_START( "cast_english",
                  "new_style_cast_english CAST sname_english_expected INTO "
                  "decl_english_ast" );
      DUMP_STR( "new_style_cast_english", $1 );
      DUMP_SNAME( "sname_english_expected", &$3 );
      DUMP_AST( "decl_english_ast", $5.ast );
      DUMP_END();

      bool ok = false;

      if ( c_init >= C_INIT_READ_CONF && opt_lang < LANG_CPP_11 ) {
        print_error( &@1, "%s not supported in %s", $1, C_LANG_NAME() );
      }
      else if ( (ok = c_ast_check( $5.ast, C_CHECK_CAST )) ) {
        FPRINTF( fout, "%s<", $1 );
        c_ast_gibberish_cast( $5.ast, fout );
        FPRINTF( fout, ">(%s)\n", c_sname_full_name( &$3 ) );
      }

      c_sname_free( &$3 );
      if ( !ok )
        PARSE_ABORT();
    }
  ;

new_style_cast_english
  : Y_CONST                       { $$ = L_CONST_CAST;        }
  | Y_DYNAMIC                     { $$ = L_DYNAMIC_CAST;      }
  | Y_REINTERPRET                 { $$ = L_REINTERPRET_CAST;  }
  | Y_STATIC                      { $$ = L_STATIC_CAST;       }
  ;

/*****************************************************************************/
/*  declare                                                                  */
/*****************************************************************************/

declare_english
  : Y_DECLARE sname_english as_expected storage_class_list_english_type_opt
    decl_english_ast alignas_specifier_english_opt
    {
      if ( $5.ast->kind == K_NAME ) {
        //
        // This checks for a case like:
        //
        //      declare x as y
        //
        // i.e., declaring a variable name as another name (unknown type).
        // This can get this far due to the nature of the C/C++ grammar.
        //
        // This check has to be done now in the parser rather than later in the
        // AST because the name of the AST node needs to be set to the variable
        // name, but the AST node is itself a name and overwriting it would
        // lose information.
        //
        assert( !c_ast_sname_empty( $5.ast ) );
        print_error( &@5,
          "\"%s\": unknown type", c_ast_sname_full_name( $5.ast )
        );
        c_sname_free( &$2 );
        PARSE_ABORT();
      }

      c_ast_sname_set_sname( $5.ast, &$2 );

      DUMP_START( "declare_english",
                  "DECLARE sname AS storage_class_list_english_type_opt "
                  "decl_english_ast alignas_specifier_english_opt" );
      DUMP_SNAME( "sname", &$2 );
      DUMP_TYPE( "storage_class_list_english_type_opt", $4 );
      switch ( $6.kind ) {
        case C_ALIGNAS_NONE:
          break;
        case C_ALIGNAS_EXPR:
          DUMP_NUM( "alignas_specifier_english.as.expr", $6.as.expr );
          break;
        case C_ALIGNAS_TYPE:
          DUMP_AST( "alignas_specifier_english.as.type_ast", $6.as.type_ast );
          break;
      } // switch

      $5.ast->loc = @2;
      C_TYPE_ADD( &$5.ast->type_id, $4, @4 );

      if ( $6.kind != C_ALIGNAS_NONE ) {
        $5.ast->align = $6;
        if ( ($5.ast->type_id & T_TYPEDEF) != T_NONE ) {
          // See comment in explain_type_decl().
          print_error( &@6, "%s can not be %s", L_TYPEDEF, L_ALIGNED );
          PARSE_ABORT();
        }
      }

      C_AST_CHECK( $5.ast, C_CHECK_DECL );

      DUMP_AST( "decl_english_ast", $5.ast );
      DUMP_END();

      c_ast_gibberish_declare( $5.ast, G_DECL_NONE, fout );
      if ( opt_semicolon )
        FPUTC( ';', fout );
      FPUTC( '\n', fout );
    }

  | Y_DECLARE c_operator of_scope_list_english_opt as_expected
    storage_class_list_english_type_opt oper_decl_english_ast
    {
      DUMP_START( "declare_english",
                  "DECLARE c_operator of_scope_list_english_opt AS "
                  "storage_class_list_english_type_opt "
                  "oper_decl_english_ast" );
      DUMP_STR( "c_operator", op_get( $2 )->name );
      DUMP_SNAME( "of_scope_list_english_opt", &$3 );
      DUMP_TYPE( "storage_class_list_english_type_opt", $5 );

      $6.ast->sname = $3;
      $6.ast->loc = @2;
      $6.ast->as.oper.oper_id = $2;
      C_TYPE_ADD( &$6.ast->type_id, $5, @5 );

      DUMP_AST( "oper_decl_english_ast", $6.ast );
      DUMP_END();

      C_AST_CHECK( $6.ast, C_CHECK_DECL );
      c_ast_gibberish_declare( $6.ast, G_DECL_NONE, fout );
      if ( opt_semicolon )
        FPUTC( ';', fout );
      FPUTC( '\n', fout );
    }

  | Y_DECLARE storage_class_list_english_type_opt Y_USER_DEFINED
    conversion_expected operator_opt of_scope_list_english_opt
    returning_expected decl_english_ast
    {
      DUMP_START( "declare_english",
                  "DECLARE type_qualifier_list_c_type_opt "
                  "USER-DEFINED CONVERSION OPERATOR "
                  "RETURNING decl_english_ast" );
      DUMP_TYPE( "storage_class_list_english_type_opt", $2 );
      DUMP_SNAME( "of_scope_list_english_opt", &$6 );
      DUMP_AST( "decl_english_ast", $8.ast );

      c_ast_t *const ast = c_ast_new_gc( K_USER_DEF_CONVERSION, &@$ );
      ast->sname = $6;
      ast->type_id = $2;
      c_ast_set_parent( $8.ast, ast );
      C_AST_CHECK( ast, C_CHECK_DECL );

      c_ast_gibberish_declare( ast, G_DECL_NONE, fout );
      if ( opt_semicolon )
        FPUTC( ';', fout );
      FPUTC( '\n', fout );
    }

  | Y_DECLARE error
    {
      if ( C_LANG_IS_CPP() )
        ELABORATE_ERROR( "name or %s expected", L_OPERATOR );
      else
        ELABORATE_ERROR( "name expected" );
    }
  ;

storage_class_list_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | storage_class_list_english_type_opt storage_class_english_type
    {
      DUMP_START( "storage_class_list_english_type_opt",
                  "storage_class_list_english_type_opt "
                  "storage_class_english_type" );
      DUMP_TYPE( "storage_class_list_english_type_opt", $1 );
      DUMP_TYPE( "storage_class_english_type", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "storage_class_list_english_type_opt", $$ );
      DUMP_END();
    }
  ;

storage_class_english_type
  : attribute_english_type
  | Y_AUTO_STORAGE
  | Y___BLOCK                           /* Apple extension */
  | Y_CONSTEVAL
  | Y_CONSTEXPR
  | Y_DEFAULT
  | Y_DELETE
  | Y_EXPLICIT
  | Y_EXTERN
  | Y_FINAL
  | Y_FRIEND
  | Y_INLINE
  | Y_MUTABLE
  | Y_NOEXCEPT
  | Y_OVERRIDE
/*| Y_REGISTER */                       /* in type_modifier_list_english_type */
  | Y_STATIC
  | Y_THREAD_LOCAL
  | Y_THROW
  | Y_TYPEDEF
  | Y_VIRTUAL
  | Y_PURE virtual_expected       { $$ = T_PURE_VIRTUAL | T_VIRTUAL; }
  ;

alignas_specifier_english_opt
  : /* empty */                   { MEM_ZERO( &$$ ); }
  | alignas_specifier_english
  ;

alignas_specifier_english
  : Y_ALIGNED as_opt Y_NUMBER bytes_opt
    {
      $$.kind = C_ALIGNAS_EXPR;
      $$.as.expr = (unsigned)$3;
    }
  | Y_ALIGNED as_opt decl_english_ast
    {
      $$.kind = C_ALIGNAS_TYPE;
      $$.as.type_ast = $3.ast;
    }
  | Y_ALIGNED as_opt error
    {
      MEM_ZERO( &$$ );
      ELABORATE_ERROR( "number or type expected" );
    }
  ;

bytes_opt
  : /* empty */
  | Y_BYTES
  ;

attribute_english_type
  : type_attribute_english_type
  | Y_NORETURN
  | Y__NORETURN
    {
      if ( !_Noreturn_ok( &@1 ) )
        PARSE_ABORT();
    }
  ;

/*****************************************************************************/
/*  define
/*****************************************************************************/

define_english
  : Y_DEFINE sname_english as_expected
    storage_class_list_english_type_opt decl_english_ast
    {
      DUMP_START( "define_english",
                  "DEFINE sname_english AS "
                  "storage_class_list_english_type_opt decl_english_ast" );
      DUMP_SNAME( "sname", &$2 );
      DUMP_TYPE( "storage_class_list_english_type_opt", $4 );
      DUMP_AST( "decl_english_ast", $5.ast );
      DUMP_END();

      if ( $5.ast->kind == K_NAME ) {   // see the comment in "declare_english"
        assert( !c_ast_sname_empty( $5.ast ) );
        print_error( &@5,
          "\"%s\": unknown type", c_ast_sname_full_name( $5.ast )
        );
        c_sname_free( &$2 );
        PARSE_ABORT();
      }

      //
      // Explicitly add T_TYPEDEF to prohibit cases like:
      //
      //      define eint as extern int
      //      define rint as register int
      //      define sint as static int
      //      ...
      //
      //  i.e., a defined type with a storage class.
      //
      bool ok = c_type_add( &$5.ast->type_id, T_TYPEDEF, &@4 ) &&
                c_type_add( &$5.ast->type_id, $4, &@4 ) &&
                c_ast_check( $5.ast, C_CHECK_DECL );

      if ( ok ) {
        // Once the semantic checks pass, remove the T_TYPEDEF.
        C_IGNORE_RV( c_ast_take_typedef( $5.ast ) );
        c_ast_sname_set_sname( $5.ast, &$2 );
        ok = add_type( L_DEFINE, $5.ast, &@5 );
      }

      if ( !ok ) {
        c_sname_free( &$2 );
        PARSE_ABORT();
      }
    }

  | Y_DEFINE error
    {
      ELABORATE_ERROR( "name expected" );
    }
  ;

/*****************************************************************************/
/*  explain                                                                  */
/*****************************************************************************/

explain_c
  : explain '(' type_c_ast { type_push( $3.ast ); } cast_c_ast_opt ')'
    sname_c_opt
    {
      type_pop();

      DUMP_START( "explain_c",
                  "EXPLAIN '(' type_c_ast cast_c_ast_opt ')' sname_c_opt" );
      DUMP_AST( "type_c_ast", $3.ast );
      DUMP_AST( "cast_c_ast_opt", $5.ast );
      DUMP_SNAME( "sname_c_opt", &$7 );

      c_ast_t *const ast = c_ast_patch_placeholder( $3.ast, $5.ast );
      bool const ok = c_ast_check( ast, C_CHECK_CAST );
      if ( ok ) {
        FPUTS( L_CAST, fout );
        if ( !c_sname_empty( &$7 ) ) {
          FPUTC( ' ', fout );
          c_sname_english( &$7, fout );
        }
        FPRINTF( fout, " %s ", L_INTO );
        c_ast_english( ast, fout );
        FPUTC( '\n', fout );
      }

      c_sname_free( &$7 );
      if ( !ok )
        PARSE_ABORT();

      DUMP_AST( "explain_c", ast );
      DUMP_END();
    }

  | explain new_style_cast_c
    lt_expected type_c_ast { type_push( $4.ast ); } cast_c_ast_opt gt_expected
    lparen_expected sname_c_expected rparen_expected
    {
      type_pop();

      DUMP_START( "explain_c",
                  "EXPLAIN new_style_cast_c '<' type_c_ast cast_c_ast_opt '>' "
                  "'(' sname ')'" );
      DUMP_STR( "new_style_cast_c", $2 );
      DUMP_AST( "type_c_ast", $4.ast );
      DUMP_AST( "cast_c_ast_opt", $6.ast );
      DUMP_SNAME( "sname", &$9 );
      DUMP_END();

      bool ok = false;
      if ( c_init >= C_INIT_READ_CONF && C_LANG_IS_C() ) {
        print_error( &@2, "%s_cast not supported in %s", $2, C_LANG_NAME() );
      }
      else {
        c_ast_t *const ast = c_ast_patch_placeholder( $4.ast, $6.ast );
        if ( (ok = c_ast_check( ast, C_CHECK_CAST )) ) {
          FPRINTF( fout, "%s %s ", $2, L_CAST );
          c_sname_english( &$9, fout );
          FPRINTF( fout, " %s ", L_INTO );
          c_ast_english( ast, fout );
          FPUTC( '\n', fout );
        }
      }

      c_sname_free( &$9 );
      if ( !ok )
        PARSE_ABORT();
    }

  | explain alignas_specifier_c type_c_ast { type_push( $3.ast ); } decl_c_ast
    {
      DUMP_START( "explain_c", "EXPLAIN ALIGNAS(...) type_c_ast decl_c_ast" );
      switch ( $2.kind ) {
        case C_ALIGNAS_NONE:
          break;
        case C_ALIGNAS_EXPR:
          DUMP_NUM( "alignas_specifier_c.as.expr", $2.as.expr );
          break;
        case C_ALIGNAS_TYPE:
          DUMP_AST( "alignas_specifier_c.as.type_ast", $2.as.type_ast );
          break;
      } // switch
      DUMP_AST( "type_c_ast", $3.ast );
      DUMP_AST( "decl_c_ast", $5.ast );
      DUMP_END();
      if ( !explain_type_decl( &$2, &@2, $3.ast, $5.ast, &@5 ) )
        PARSE_ABORT();
    }

  | explain type_c_ast { type_push( $2.ast ); } decl_c_ast
    {
      DUMP_START( "explain_c", "EXPLAIN type_c_ast decl_c_ast" );
      DUMP_AST( "type_c_ast", $2.ast );
      DUMP_AST( "decl_c_ast", $4.ast );
      DUMP_END();

      if ( !explain_type_decl( NULL, NULL, $2.ast, $4.ast, &@4 ) )
        PARSE_ABORT();
    }

    /*
     * User-defined conversion operator declaration without a storage-class-
     * like part.  This is for a case like:
     *
     *      explain operator int()
     *
     * User-defined conversion operator declarations with a storage-class-like
     * part, e.g.:
     *
     *      explain explicit operator int()
     *
     * are handled by the rule above.
     */
  | explain user_defined_conversion_decl_c_ast
    {
      DUMP_START( "explain_c", "EXPLAIN user_defined_conversion_decl_c_ast" );
      DUMP_AST( "user_defined_conversion_decl_c_ast", $2.ast );
      DUMP_END();

      C_AST_CHECK( $2.ast, C_CHECK_DECL );
      FPRINTF( fout, "%s ", L_DECLARE );
      c_ast_english( $2.ast, fout );
      FPUTC( '\n', fout );
    }

    /*
     * File-scope constructor definition, e.g.: S::S([args]).
     */
  | explain Y_CONSTRUCTOR_SNAME lparen_expected arg_list_c_ast_opt ')'
    noexcept_c_type_opt
    {
      DUMP_START( "explain_c",
                  "EXPLAIN CONSTRUCTOR_SNAME '(' arg_list_c_ast_opt ')' "
                  "noexcept_c_type_opt" );
      DUMP_SNAME( "CONSTRUCTOR_SNAME", &$2 );
      DUMP_AST_LIST( "arg_list_c_ast_opt", $4 );
      DUMP_TYPE( "noexcept_c_type_opt", $6 );
      DUMP_END();

      c_ast_t *const ast = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      ast->type_id = $6;
      ast->as.constructor.args = $4;

      bool const ok = c_ast_check( ast, C_CHECK_DECL );
      if ( ok ) {
        char const *const local_name = c_sname_local_name( &$2 );
        char const *const scope_name = c_sname_scope_name( &$2 );

        FPRINTF( fout, "%s %s ", L_DECLARE, local_name );
        if ( scope_name[0] != '\0' )
          FPRINTF( fout, "%s %s %s ", L_OF, L_CLASS, scope_name );
        FPRINTF( fout, "%s ", L_AS );
        c_ast_english( ast, fout );
        FPUTC( '\n', fout );
      }

      c_sname_free( &$2 );
      if ( !ok )
        PARSE_ABORT();
    }

    /*
     * In-class destructor declaration, e.g.: ~S().
     */
  | explain virtual_opt Y_TILDE any_name_expected
    lparen_expected rparen_expected noexcept_c_type_opt
    {
      DUMP_START( "explain_c",
                  "EXPLAIN ~ any_name_expected '(' ')'" );
      DUMP_TYPE( "virtual_opt", $2 );
      DUMP_STR( "any_name_expected", $4 );
      DUMP_TYPE( "noexcept_c_type_opt", $7 );
      DUMP_END();

      c_ast_t *const ast = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      ast->type_id = $2 | $7;

      bool const ok = c_ast_check( ast, C_CHECK_DECL );
      if ( ok ) {
        FPRINTF( fout, "%s %s %s ", L_DECLARE, $4, L_AS );
        c_ast_english( ast, fout );
        FPUTC( '\n', fout );
      }

      FREE( $4 );
      if ( !ok )
        PARSE_ABORT();
    }

    /*
     * File scope destructor definition, e.g.: S::~S().
     */
  | explain Y_DESTRUCTOR_SNAME lparen_expected rparen_expected
    noexcept_c_type_opt
    {
      DUMP_START( "explain_c",
                  "EXPLAIN DESTRUCTOR_SNAME '(' ')' noexcept_c_type_opt" );
      DUMP_SNAME( "DESTRUCTOR_SNAME", &$2 );
      DUMP_TYPE( "noexcept_c_type_opt", $5 );
      DUMP_END();

      c_ast_t *const ast = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      ast->type_id = $5;

      bool const ok = c_ast_check( ast, C_CHECK_DECL );
      if ( ok ) {
        char const *const local_name = c_sname_local_name( &$2 );
        char const *const scope_name = c_sname_scope_name( &$2 );
        FPRINTF( fout,
          "%s %s %s %s %s %s ",
          L_DECLARE, local_name, L_OF, L_CLASS, scope_name, L_AS
        );
        c_ast_english( ast, fout );
        FPUTC( '\n', fout );
      }

      c_sname_free( &$2 );
      if ( !ok )
        PARSE_ABORT();
    }

  | explain Y_USING name_expected equals_expected type_c_ast
    {
      // see the comment in "define_english" about T_TYPEDEF
      C_TYPE_ADD( &$5.ast->type_id, T_TYPEDEF, @5 );
      type_push( $5.ast );
    }
    cast_c_ast_opt
    {
      type_pop();

      DUMP_START( "explain_c",
                  "EXPLAIN USING NAME = type_c_ast cast_c_ast_opt" );
      DUMP_STR( "NAME", $3 );
      DUMP_AST( "type_c_ast", $5.ast );
      DUMP_AST( "cast_c_ast_opt", $7.ast );
      DUMP_END();

      //
      // Using declarations are supported only in C++11 and later.
      //
      // This check has to be done now in the parser rather than later in the
      // AST because using declarations are treated like typedef declarations
      // and the AST has no "memory" that such a declaration was a using
      // declaration.
      //
      bool ok = false;
      if ( c_init >= C_INIT_READ_CONF && opt_lang < LANG_CPP_11 ) {
        print_error( &@2,
          "\"%s\" not supported in %s", L_USING, C_LANG_NAME()
        );
      }
      else {
        c_ast_t *const ast = c_ast_patch_placeholder( $5.ast, $7.ast );
        if ( (ok = c_ast_check( ast, C_CHECK_DECL )) ) {
          // Once the semantic checks pass, remove the T_TYPEDEF.
          C_IGNORE_RV( c_ast_take_typedef( ast ) );
          FPRINTF( fout, "%s %s %s %s ", L_DECLARE, $3, L_AS, L_TYPE );
          c_ast_english( ast, fout );
          FPUTC( '\n', fout );
        }
      }

      FREE( $3 );
      if ( !ok )
        PARSE_ABORT();
    }

  | explain error
    {
      ELABORATE_ERROR( "cast or declaration expected" );
    }
  ;

alignas_specifier_c
  : Y_ALIGNAS lparen_expected Y_NUMBER rparen_expected
    {
      DUMP_START( "alignas_specifier_c", "ALIGNAS ( NUMBER )" );
      DUMP_NUM( "NUMBER", $3 );
      DUMP_END();

      $$.kind = C_ALIGNAS_EXPR;
      $$.as.expr = (unsigned)$3;
    }
  | Y_ALIGNAS lparen_expected type_c_ast { type_push( $3.ast ); }
    cast_c_ast_opt rparen_expected
    {
      type_pop();

      DUMP_START( "alignas_specifier_c",
                  "ALIGNAS ( type_c_ast cast_c_ast_opt )" );
      DUMP_AST( "type_c_ast", $3.ast );
      DUMP_AST( "cast_c_ast_opt", $5.ast );
      DUMP_END();

      c_ast_t *const ast = c_ast_patch_placeholder( $3.ast, $5.ast );

      $$.kind = C_ALIGNAS_TYPE;
      $$.as.type_ast = ast;
    }
  | Y_ALIGNAS lparen_expected error rparen_expected
    {
      ELABORATE_ERROR( "number or type expected" );
      $$.kind = C_ALIGNAS_NONE;
    }
  ;

explain
  : Y_EXPLAIN
    {
      //
      // Tell the lexer that we're explaining gibberish so cdecl keywords
      // (e.g., "func") are returned as ordinary names, otherwise gibberish
      // like:
      //
      //      int func(void);
      //
      // would result in a parser error.
      //
      c_mode = C_GIBBERISH_TO_ENGLISH;
    }
  ;

new_style_cast_c
  : Y_CONST_CAST                  { $$ = L_CONST;       }
  | Y_DYNAMIC_CAST                { $$ = L_DYNAMIC;     }
  | Y_REINTERPRET_CAST            { $$ = L_REINTERPRET; }
  | Y_STATIC_CAST                 { $$ = L_STATIC;      }
  ;

/*****************************************************************************/
/*  help                                                                     */
/*****************************************************************************/

help_command
  : Y_HELP help_what_opt          { print_help( $2 ); }
  ;

help_what_opt
  : /* empty */                   { $$ = L_DEFAULT;   }
  | Y_COMMANDS                    { $$ = L_COMMANDS;  }
  | Y_ENGLISH                     { $$ = L_ENGLISH;   }
  ;

/*****************************************************************************/
/*  quit                                                                     */
/*****************************************************************************/

quit_command
  : Y_QUIT                        { quit(); }
  ;

/*****************************************************************************/
/*  scope                                                                    */
/*****************************************************************************/

scope_declaration_c
  : class_struct_union_type
    {
      // see the comment in "explain"
      c_mode = C_GIBBERISH_TO_ENGLISH;
    }
    any_sname_c
    {
      if ( C_LANG_IS_CPP() ) {
        c_type_id_t const cur_type = c_sname_type( &in_attr.current_scope );
        if ( (cur_type & T_CLASS_STRUCT_UNION) != 0 ) {
          char const *const cur_name =
            c_sname_local_name( &in_attr.current_scope );
          char const *const mbr_name = SLIST_HEAD( char const*, &$3 );
          if ( strcmp( mbr_name, cur_name ) == 0 ) {
            print_error( &@3,
              "\"%s\": member has the same name as its enclosing %s",
              mbr_name, c_type_name( cur_type )
            );
            PARSE_ABORT();
          }
        }
        c_sname_set_type( &in_attr.current_scope, $1 );
        c_sname_append_sname( &in_attr.current_scope, &$3 );
      }
    }
    lbrace_expected
    scope_typedef_or_using_declaration_c_opt
    rbrace_expected
    semi_expected                       /* ';' needed for class/struct/union */
    {
      if ( C_LANG_IS_C() ) {
        print_error( &@6,
          "nested types are not supported in %s", C_LANG_NAME()
        );
        PARSE_ABORT();
      }
    }

    /*
     * Namespace needs its own rule since class/struct/union requires a
     * trailing ';' whereas namespace does not.
     */
  | namespace_type
    {
      // see the comment in "explain"
      c_mode = C_GIBBERISH_TO_ENGLISH;
    }
    sname_c
    {
      //
      // Nested namespace declarations are supported only in C++17 and later.
      // (However, we always allow them in configuration files.)
      //
      // This check has to be done now in the parser rather than later in the
      // AST because the AST has no "memory" of how a namespace was
      // constructed.
      //
      if ( c_init >= C_INIT_READ_CONF && opt_lang < LANG_CPP_17 &&
            c_sname_count( &$3 ) > 1 ) {
        print_error( &@3,
          "nested %s declarations not supported until %s",
          L_NAMESPACE, c_lang_name( LANG_CPP_17 )
        );
        c_sname_free( &$3 );
        PARSE_ABORT();
      }

      //
      // Ensure that "namespace" isn't nested within a class/struct/union.
      //
      c_type_id_t const outer_type = c_sname_type( &in_attr.current_scope );
      if ( (outer_type & T_CLASS_STRUCT_UNION) != T_NONE ) {
        print_error( &@1,
          "\"%s\" may only be nested within a %s", L_NAMESPACE, L_NAMESPACE
        );
        c_sname_free( &$3 );
        PARSE_ABORT();
      }

      c_sname_set_type( &in_attr.current_scope, $1 );
      c_sname_append_sname( &in_attr.current_scope, &$3 );
    }
    lbrace_expected
    scope_typedef_or_using_declaration_c_opt
    rbrace_expected
    semi_opt                            /* ';' optional for namespace */
  ;

scope_typedef_or_using_declaration_c_opt
  : /* empty */
  | scope_declaration_c
  | typedef_declaration_c semi_expected
  | using_declaration_c semi_expected
  ;

/*****************************************************************************/
/*  set                                                                      */
/*****************************************************************************/

set_command
  : Y_SET
    {
      set_option( NULL, NULL, NULL, NULL );
    }
  | Y_SET Y_SET_OPTION set_option_value_opt
    {
      set_option( $2, &@2, $3, &@3 );
      FREE( $2 );
      FREE( $3 );
    }
  ;

set_option_value_opt
  : /* empty */                   { $$ = NULL; }
  | '=' Y_SET_OPTION              { $$ = $2; @$ = @2; }
  | '=' error
    {
      $$ = NULL; @$ = @2;
      ELABORATE_ERROR( "option value expected" );
    }
  ;

/*****************************************************************************/
/*  show                                                                     */
/*****************************************************************************/

show_command
  : Y_SHOW any_typedef typedef_opt
    {
      if ( $3 )
        print_type_as_typedef( $2 );
      else
        print_type_as_english( $2 );
    }

  | Y_SHOW Y_NAME typedef_opt
    {
      if ( opt_lang < LANG_CPP_11 ) {
        print_error( &@2,
          "\"%s\": not defined as type via %s or %s",
          $2, L_DEFINE, L_TYPEDEF
        );
      } else {
        print_error( &@2,
          "\"%s\": not defined as type via %s, %s, or %s",
          $2, L_DEFINE, L_TYPEDEF, L_USING
        );
      }
      FREE( $2 );
      PARSE_ABORT();
    }

  | Y_SHOW show_which_types_opt typedef_opt
    {
      print_type_info_t pti;
      pti.print_fn = $3 ? &print_type_as_typedef : &print_type_as_english;
      pti.show_which = $2;
      C_IGNORE_RV( c_typedef_visit( &print_type_visitor, &pti ) );
    }

  | Y_SHOW error
    {
      ELABORATE_ERROR(
        "type name or \"%s\", \"%s\", or \"%s\" expected",
        L_ALL, L_PREDEFINED, L_USER
      );
    }
  ;

show_which_types_opt
  : /* empty */                   { $$ = SHOW_USER_TYPES; }
  | Y_ALL                         { $$ = SHOW_ALL_TYPES; }
  | Y_PREDEFINED                  { $$ = SHOW_PREDEFINED_TYPES; }
  | Y_USER                        { $$ = SHOW_USER_TYPES; }
  ;

/*****************************************************************************/
/*  typedef                                                                  */
/*****************************************************************************/

typedef_declaration_c
  : Y_TYPEDEF
    {
      // see the comment in "explain"
      c_mode = C_GIBBERISH_TO_ENGLISH;
    }
    type_c_ast
    {
      // see the comment in define_english about T_TYPEDEF
      C_TYPE_ADD( &$3.ast->type_id, T_TYPEDEF, @3 );
      type_push( $3.ast );
    }
    decl_c_ast
    {
      type_pop();

      DUMP_START( "typedef_declaration_c", "TYPEDEF type_c_ast decl_c_ast" );
      DUMP_AST( "type_c_ast", $3.ast );
      DUMP_AST( "decl_c_ast", $5.ast );

      c_ast_t *ast;
      c_sname_t temp_sname;

      if ( $5.ast->kind == K_TYPEDEF ) {
        //
        // This is for either of the following cases:
        //
        //      typedef int int32_t;
        //      typedef int32_t int_least32_t;
        //
        // that is: any type name followed by an existing typedef name.
        //
        ast = $3.ast;
        if ( c_ast_sname_empty( ast ) )
          ast->sname = c_ast_sname_dup( $5.ast->as.c_typedef->ast );
      }
      else {
        //
        // This is for either of the following cases:
        //
        //      typedef int foo;
        //      typedef int32_t foo;
        //
        // that is: any type name followed by a new name.
        //
        ast = c_ast_patch_placeholder( $3.ast, $5.ast );
        temp_sname = c_ast_take_name( $5.ast );
        c_ast_sname_set_sname( ast, &temp_sname );
      }

      C_AST_CHECK( ast, C_CHECK_DECL );
      // see the comment in define_english about T_TYPEDEF
      C_IGNORE_RV( c_ast_take_typedef( ast ) );

      if ( c_ast_sname_count( ast ) > 1 ) {
        print_error( &@5,
          "%s names can not be scoped; use: %s %s { %s ... }",
          L_TYPEDEF, L_NAMESPACE, c_ast_sname_scope_name( ast ), L_TYPEDEF
        );
        PARSE_ABORT();
      }

      temp_sname = c_sname_dup( &in_attr.current_scope );
      c_ast_sname_set_type( ast, c_sname_type( &in_attr.current_scope ) );
      c_ast_sname_prepend_sname( ast, &temp_sname );

      DUMP_AST( "typedef_declaration_c", ast );
      DUMP_END();

      if ( !add_type( L_TYPEDEF, ast, &@5 ) )
        PARSE_ABORT();
    }
  ;

/*****************************************************************************/
/*  using                                                                    */
/*****************************************************************************/

using_declaration_c
  : Y_USING
    {
      // see the comment in "explain"
      c_mode = C_GIBBERISH_TO_ENGLISH;
    }
    using_name_ast_expected equals_expected type_c_ast
    {
      // see the comment in "define_english" about T_TYPEDEF
      C_TYPE_ADD( &$5.ast->type_id, T_TYPEDEF, @5 );
      type_push( $5.ast );
    }
    cast_c_ast_opt
    {
      type_pop();

      //
      // Using declarations are supported only in C++11 and later.  (However,
      // we always allow them in configuration files.)
      //
      // This check has to be done now in the parser rather than later in the
      // AST because using declarations are treated like typedef declarations
      // and the AST has no "memory" that such a declaration was a using
      // declaration.
      //
      if ( c_init >= C_INIT_READ_CONF && opt_lang < LANG_CPP_11 ) {
        print_error( &@1,
          "\"%s\" not supported in %s", L_USING, C_LANG_NAME()
        );
        PARSE_ABORT();
      }

      DUMP_START( "using_declaration_c", "USING NAME = decl_c_ast" );
      DUMP_AST( "using_name_ast_expected", $3.ast );
      DUMP_AST( "type_c_ast", $5.ast );
      DUMP_AST( "cast_c_ast_opt", $7.ast );

      c_ast_t *const ast = c_ast_patch_placeholder( $5.ast, $7.ast );

      c_sname_t sname = $3.ast->kind == K_TYPEDEF ?
        c_ast_sname_dup( $3.ast->as.c_typedef->ast ) :
        c_ast_take_name( $3.ast );
      c_ast_sname_set_sname( ast, &sname );

      if ( c_ast_sname_count( ast ) > 1 ) {
        print_error( &@5,
          "%s names can not be scoped; use: %s %s { %s ... }",
          L_USING, L_NAMESPACE, c_ast_sname_scope_name( ast ), L_USING
        );
        PARSE_ABORT();
      }

      c_sname_t temp_sname = c_sname_dup( &in_attr.current_scope );
      c_ast_sname_set_type( ast, c_sname_type( &in_attr.current_scope ) );
      c_ast_sname_prepend_sname( ast, &temp_sname );

      DUMP_AST( "using_declaration_c", ast );
      DUMP_END();

      C_AST_CHECK( ast, C_CHECK_DECL );
      // see the comment in "define_english" about T_TYPEDEF
      C_IGNORE_RV( c_ast_take_typedef( ast ) );

      if ( !add_type( L_USING, ast, &@5 ) )
        PARSE_ABORT();
    }
  ;

using_name_ast_expected
  : name_ast
  | typedef_name_ast
  | error
    {
      ELABORATE_ERROR( "type name expected" );
    }
  ;

typedef_name_ast
  : Y_TYPEDEF_NAME
    {
      DUMP_START( "typedef_name_ast", "Y_TYPEDEF_NAME" );
      DUMP_AST( "Y_TYPEDEF_NAME", $1->ast );

      $$.ast = c_ast_new_gc( K_TYPEDEF, &@$ );
      $$.target_ast = NULL;
      $$.ast->as.c_typedef = $1;
      $$.ast->type_id = T_TYPEDEF_TYPE;

      DUMP_AST( "typedef_name_ast", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  declaration english productions                                          */
/*****************************************************************************/

decl_english_ast
  : array_decl_english_ast
  | constructor_decl_english_ast
  | destructor_decl_english_ast
  | qualified_decl_english_ast
  | user_defined_literal_decl_english_ast
  | var_decl_english_ast
  ;

array_decl_english_ast
  : Y_ARRAY static_type_opt type_qualifier_list_c_type_opt
    array_size_num_opt of_expected decl_english_ast
    {
      DUMP_START( "array_decl_english_ast",
                  "ARRAY static_type_opt type_qualifier_list_c_type_opt "
                  "array_size_num_opt OF decl_english_ast" );
      DUMP_TYPE( "static_type_opt", $2 );
      DUMP_TYPE( "type_qualifier_list_c_type_opt", $3 );
      DUMP_NUM( "array_size_num_opt", $4 );
      DUMP_AST( "decl_english_ast", $6.ast );

      $$.ast = c_ast_new_gc( K_ARRAY, &@$ );
      $$.target_ast = NULL;
      $$.ast->as.array.size = $4;
      $$.ast->as.array.type_id = $2 | $3;
      c_ast_set_parent( $6.ast, $$.ast );

      DUMP_AST( "array_decl_english_ast", $$.ast );
      DUMP_END();
    }

  | Y_VARIABLE length_opt array_expected type_qualifier_list_c_type_opt
    of_expected decl_english_ast
    {
      DUMP_START( "array_decl_english_ast",
                  "VARIABLE LENGTH ARRAY type_qualifier_list_c_type_opt "
                  "OF decl_english_ast" );
      DUMP_TYPE( "type_qualifier_list_c_type_opt", $4 );
      DUMP_AST( "decl_english_ast", $6.ast );

      $$.ast = c_ast_new_gc( K_ARRAY, &@$ );
      $$.target_ast = NULL;
      $$.ast->as.array.size = C_ARRAY_SIZE_VARIABLE;
      $$.ast->as.array.type_id = $4;
      c_ast_set_parent( $6.ast, $$.ast );

      DUMP_AST( "array_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

array_size_num_opt
  : /* empty */                   { $$ = C_ARRAY_SIZE_NONE; }
  | Y_NUMBER
  | '*'                           { $$ = C_ARRAY_SIZE_VARIABLE; }
  ;

length_opt
  : /* empty */
  | Y_LENGTH
  ;

block_decl_english_ast                  /* Apple extension */
  : Y_BLOCK paren_decl_list_english_opt returning_english_ast_opt
    {
      DUMP_START( "block_decl_english_ast",
                  "BLOCK paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TYPE( "(qualifier)", qualifier_peek() );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $2 );
      DUMP_AST( "returning_english_ast_opt", $3.ast );

      $$.ast = c_ast_new_gc( K_BLOCK, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = qualifier_peek();
      c_ast_set_parent( $3.ast, $$.ast );
      $$.ast->as.block.args = $2;

      DUMP_AST( "block_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

constructor_decl_english_ast
  : Y_CONSTRUCTOR paren_decl_list_english_opt
    {
      DUMP_START( "constructor_decl_english_ast",
                  "CONSTRUCTOR paren_decl_list_english_opt" );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $2 );

      $$.ast = c_ast_new_gc( K_CONSTRUCTOR, &@$ );
      $$.target_ast = NULL;
      $$.ast->as.func.args = $2;

      DUMP_AST( "constructor_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

destructor_decl_english_ast
  : Y_DESTRUCTOR
    {
      DUMP_START( "destructor_decl_english_ast",
                  "DESTRUCTOR" );

      $$.ast = c_ast_new_gc( K_DESTRUCTOR, &@$ );
      $$.target_ast = NULL;

      DUMP_AST( "destructor_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

func_decl_english_ast
  : ref_qualifier_english_type_opt member_or_non_member_opt
    Y_FUNCTION paren_decl_list_english_opt returning_english_ast_opt
    {
      DUMP_START( "func_decl_english_ast",
                  "ref_qualifier_english_type_opt "
                  "member_or_non_member_opt "
                  "FUNCTION paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TYPE( "(qualifier)", qualifier_peek() );
      DUMP_TYPE( "ref_qualifier_english_type_opt", $1 );
      DUMP_NUM( "member_or_non_member_opt", $2 );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $4 );
      DUMP_AST( "returning_english_ast_opt", $5.ast );

      $$.ast = c_ast_new_gc( K_FUNCTION, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = qualifier_peek() | $1;
      $$.ast->as.func.args = $4;
      $$.ast->as.func.flags = $2;
      c_ast_set_parent( $5.ast, $$.ast );

      DUMP_AST( "func_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

oper_decl_english_ast
  : type_qualifier_list_c_type_opt { qualifier_push( $1, &@1 ); }
    ref_qualifier_english_type_opt member_or_non_member_opt
    Y_OPERATOR paren_decl_list_english_opt returning_english_ast_opt
    {
      qualifier_pop();
      DUMP_START( "oper_decl_english_ast",
                  "member_or_non_member_opt "
                  "OPERATOR paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_TYPE( "ref_qualifier_english_type_opt", $3 );
      DUMP_NUM( "member_or_non_member_opt", $4 );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $6 );
      DUMP_AST( "returning_english_ast_opt", $7.ast );

      $$.ast = c_ast_new_gc( K_OPERATOR, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = $1 | $3;
      $$.ast->as.oper.args = $6;
      $$.ast->as.oper.flags = $4;
      c_ast_set_parent( $7.ast, $$.ast );

      DUMP_AST( "oper_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

paren_decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | '(' decl_list_english_opt ')'
    {
      DUMP_START( "paren_decl_list_english_opt",
                  "'(' decl_list_english_opt ')'" );
      DUMP_AST_LIST( "decl_list_english_opt", $2 );

      $$ = $2;

      DUMP_AST_LIST( "paren_decl_list_english_opt", $$ );
      DUMP_END();
    }
  ;

decl_list_english_opt
  : /* empty */                   { slist_init( &$$ ); }
  | decl_list_english
  ;

decl_list_english
  : decl_english_ast
    {
      DUMP_START( "decl_list_english", "decl_english_ast" );
      DUMP_AST( "decl_english_ast", $1.ast );

      slist_init( &$$ );
      slist_push_tail( &$$, $1.ast );

      DUMP_AST_LIST( "decl_list_english", $$ );
      DUMP_END();
    }

  | decl_list_english comma_expected decl_english_ast
    {
      DUMP_START( "decl_list_english",
                  "decl_list_english',' decl_english_ast" );
      DUMP_AST_LIST( "decl_list_english_opt", $1 );
      DUMP_AST( "decl_english_ast", $3.ast );

      $$ = $1;
      slist_push_tail( &$$, $3.ast );

      DUMP_AST_LIST( "decl_list_english", $$ );
      DUMP_END();
    }
  ;

ref_qualifier_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | Y_REFERENCE                   { $$ = T_REFERENCE; }
  | Y_RVALUE reference_expected   { $$ = T_RVALUE_REFERENCE; }
  ;

returning_english_ast_opt
  : /* empty */
    {
      DUMP_START( "returning_english_ast_opt", "<empty>" );

      $$.ast = c_ast_new_gc( K_BUILTIN, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = T_VOID;

      DUMP_AST( "returning_english_ast_opt", $$.ast );
      DUMP_END();
    }

  | Y_RETURNING decl_english_ast
    {
      DUMP_START( "returning_english_ast_opt", "RETURNING decl_english_ast" );
      DUMP_AST( "decl_english_ast", $2.ast );

      $$ = $2;

      DUMP_AST( "returning_english_ast_opt", $$.ast );
      DUMP_END();
    }

  | Y_RETURNING error
    {
      ELABORATE_ERROR( "English expected after \"%s\"", L_RETURNING );
    }
  ;

qualified_decl_english_ast
  : type_qualifier_list_c_type_opt { qualifier_push( $1, &@1 ); }
    qualifiable_decl_english_ast
    {
      qualifier_pop();
      DUMP_START( "qualified_decl_english_ast",
                  "type_qualifier_list_c_type_opt "
                  "qualifiable_decl_english_ast" );
      DUMP_TYPE( "type_qualifier_list_c_type_opt", $1 );
      DUMP_AST( "qualifiable_decl_english_ast", $3.ast );

      $$ = $3;

      DUMP_AST( "qualified_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

qualifiable_decl_english_ast
  : block_decl_english_ast
  | func_decl_english_ast
  | pointer_decl_english_ast
  | reference_decl_english_ast
  | type_english_ast
  ;

pointer_decl_english_ast
  : Y_POINTER to_expected decl_english_ast
    {
      DUMP_START( "pointer_decl_english_ast", "POINTER TO decl_english_ast" );
      DUMP_TYPE( "(qualifier)", qualifier_peek() );
      DUMP_AST( "decl_english_ast", $3.ast );

      if ( $3.ast->kind == K_NAME ) {   // see the comment in "declare_english"
        assert( !c_ast_sname_empty( $3.ast ) );
        print_error( &@3,
          "\"%s\": unknown type", c_ast_sname_full_name( $3.ast )
        );
        PARSE_ABORT();
      }

      $$.ast = c_ast_new_gc( K_POINTER, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = qualifier_peek();
      c_ast_set_parent( $3.ast, $$.ast );

      DUMP_AST( "pointer_decl_english_ast", $$.ast );
      DUMP_END();
    }

  | Y_POINTER to_expected Y_MEMBER of_expected class_struct_type_expected
    sname_english_expected decl_english_ast
    {
      DUMP_START( "pointer_to_member_decl_english",
                  "POINTER TO MEMBER OF "
                  "class_struct_type sname_english decl_english_ast" );
      DUMP_TYPE( "(qualifier)", qualifier_peek() );
      DUMP_TYPE( "class_struct_type", $5 );
      DUMP_SNAME( "sname_english_expected", &$6 );
      DUMP_AST( "decl_english_ast", $7.ast );

      $$.ast = c_ast_new_gc( K_POINTER_TO_MEMBER, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = qualifier_peek();
      $$.ast->as.ptr_mbr.class_sname = $6;
      c_ast_set_parent( $7.ast, $$.ast );
      C_TYPE_ADD( &$$.ast->type_id, $5, @5 );

      DUMP_AST( "pointer_to_member_decl_english", $$.ast );
      DUMP_END();
    }

  | Y_POINTER to_expected error
    {
      if ( C_LANG_IS_CPP() )
        ELABORATE_ERROR( "type name or \"%s\" expected", L_MEMBER );
      else
        ELABORATE_ERROR( "type name expected" );
    }
  ;

reference_decl_english_ast
  : reference_english_ast to_expected decl_english_ast
    {
      DUMP_START( "reference_decl_english_ast",
                  "reference_english_ast TO decl_english_ast" );
      DUMP_TYPE( "(qualifier)", qualifier_peek() );
      DUMP_AST( "decl_english_ast", $3.ast );

      $$ = $1;
      c_ast_set_parent( $3.ast, $$.ast );
      C_TYPE_ADD( &$$.ast->type_id, qualifier_peek(), qualifier_peek_loc() );

      DUMP_AST( "reference_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

reference_english_ast
  : Y_REFERENCE
    {
      $$.ast = c_ast_new_gc( K_REFERENCE, &@$ );
      $$.target_ast = NULL;
    }

  | Y_RVALUE reference_expected
    {
      $$.ast = c_ast_new_gc( K_RVALUE_REFERENCE, &@$ );
      $$.target_ast = NULL;
    }
  ;

user_defined_literal_decl_english_ast
  : Y_USER_DEFINED literal_expected paren_decl_list_english_opt
    returning_english_ast_opt
    {
      //
      // User-defined literals are supported only in C++11 and later.
      // (However, we always allow them in configuration files.)
      //
      // This check is better to do now in the parser rather than later in the
      // AST because it has to be done in fewer places in the code plus gives a
      // better error location.
      //
      if ( c_init >= C_INIT_READ_CONF && opt_lang < LANG_CPP_11 ) {
        print_error( &@1,
          "%s %s not supported in %s", L_USER_DEFINED, L_LITERAL, C_LANG_NAME()
        );
        PARSE_ABORT();
      }

      DUMP_START( "user_defined_literal_decl_english_ast",
                  "USER-DEFINED LITERAL paren_decl_list_english_opt "
                  "returning_english_ast_opt" );
      DUMP_AST_LIST( "paren_decl_list_english_opt", $3 );
      DUMP_AST( "returning_english_ast_opt", $4.ast );

      $$.ast = c_ast_new_gc( K_USER_DEF_LITERAL, &@$ );
      $$.target_ast = NULL;
      $$.ast->as.user_def_lit.args = $3;
      c_ast_set_parent( $4.ast, $$.ast );

      DUMP_AST( "user_defined_literal_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

var_decl_english_ast
  : sname_c Y_AS decl_english_ast
    {
      DUMP_START( "var_decl_english_ast", "NAME AS decl_english_ast" );
      DUMP_SNAME( "sname", &$1 );
      DUMP_AST( "decl_english_ast", $3.ast );

      if ( $3.ast->kind == K_NAME ) {   // see the comment in "declare_english"
        assert( !c_ast_sname_empty( $3.ast ) );
        print_error( &@3,
          "\"%s\": unknown type", c_ast_sname_full_name( $3.ast )
        );
        PARSE_ABORT();
      }

      $$ = $3;
      c_ast_sname_set_sname( $$.ast, &$1 );

      DUMP_AST( "var_decl_english_ast", $$.ast );
      DUMP_END();
    }

    /*
     * K&R C type-less variable declaration.  This doesn't need to be a scoped
     * name since C doesn't have scoped names.
     */
  | name_ast

  | "..."
    {
      DUMP_START( "var_decl_english_ast", "..." );

      $$.ast = c_ast_new_gc( K_VARIADIC, &@$ );
      $$.target_ast = NULL;

      DUMP_AST( "var_decl_english_ast", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  type english productions                                                 */
/*****************************************************************************/

type_english_ast
  : type_modifier_list_english_type_opt unmodified_type_english_ast
    {
      DUMP_START( "type_english_ast",
                  "type_modifier_list_english_type_opt "
                  "unmodified_type_english_ast" );
      DUMP_TYPE( "type_modifier_list_english_type_opt", $1 );
      DUMP_AST( "unmodified_type_english_ast", $2.ast );
      DUMP_TYPE( "(qualifier)", qualifier_peek() );

      $$ = $2;
      C_TYPE_ADD( &$$.ast->type_id, qualifier_peek(), qualifier_peek_loc() );
      C_TYPE_ADD( &$$.ast->type_id, $1, @1 );

      DUMP_AST( "type_english_ast", $$.ast );
      DUMP_END();
    }

  | type_modifier_list_english_type     /* allows implicit int in K&R C */
    {
      DUMP_START( "type_english_ast", "type_modifier_list_english_type" );
      DUMP_TYPE( "type_modifier_list_english_type", $1 );
      DUMP_TYPE( "(qualifier)", qualifier_peek() );

      // see the comment in "type_c_ast"
      c_type_id_t type_id = opt_lang < LANG_C_99 ? T_INT : T_NONE;

      C_TYPE_ADD( &type_id, qualifier_peek(), qualifier_peek_loc() );
      C_TYPE_ADD( &type_id, $1, @1 );

      $$.ast = c_ast_new_gc( K_BUILTIN, &@$ );
      $$.ast->type_id = type_id;
      $$.target_ast = NULL;

      DUMP_AST( "type_english_ast", $$.ast );
      DUMP_END();
    }
  ;

type_modifier_list_english_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | type_modifier_list_english_type
  ;

type_modifier_list_english_type
  : type_modifier_list_english_type type_modifier_english_type
    {
      DUMP_START( "type_modifier_list_english_type",
                  "type_modifier_list_english_type "
                  "type_modifier_english_type" );
      DUMP_TYPE( "type_modifier_list_english_type", $1 );
      DUMP_TYPE( "type_modifier_english_type", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "type_modifier_list_english_type_opt", $$ );
      DUMP_END();
    }

  | type_modifier_english_type
  ;

type_modifier_english_type
  : type_attribute_english_type
  | Y__COMPLEX
  | Y__IMAGINARY
  | Y_LONG
  | Y_SHORT
  | Y_SIGNED
  | Y_UNSIGNED
  /*
   * Register is here (rather than in storage_class_english_type) because it's
   * the only storage class that can be specified for function arguments.
   * Therefore, it's simpler to treat it as any other type modifier.
   */
  | Y_REGISTER
  ;

type_attribute_english_type
  : Y_CARRIES_DEPENDENCY
  | Y_DEPRECATED
  | Y_MAYBE_UNUSED
  | Y_NODISCARD
  ;

unmodified_type_english_ast
  : builtin_type_c_ast
  | enum_class_struct_union_ast
  | typedef_type_c_ast
  ;

/*****************************************************************************/
/*  declaration gibberish productions                                        */
/*****************************************************************************/

decl_c_ast
  : decl2_c_ast
  | pointer_decl_c_ast
  | pointer_to_member_decl_c_ast
  | reference_decl_c_ast
  ;

decl2_c_ast
  : array_decl_c_ast
  | block_decl_c_ast
  | func_decl_c_ast
  | nested_decl_c_ast
  | oper_decl_c_ast
  | sname_c_ast
  | typedef_type_decl_c_ast
  | user_defined_conversion_decl_c_ast
  | user_defined_literal_decl_c_ast
  ;

array_decl_c_ast
  : decl2_c_ast array_size_c_num
    {
      DUMP_START( "array_decl_c_ast", "decl2_c_ast array_size_c_num" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "decl2_c_ast", $1.ast );
      if ( $1.target_ast != NULL )
        DUMP_AST( "target_ast", $1.target_ast );
      DUMP_NUM( "array_size_c_num", $2 );

      c_ast_t *const array = c_ast_new_gc( K_ARRAY, &@$ );
      array->as.array.size = $2;
      c_ast_set_parent( c_ast_new_gc( K_PLACEHOLDER, &@1 ), array );

      if ( $1.target_ast != NULL ) {    // array-of or function/block-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, array );
      } else {
        $$.ast = c_ast_add_array( $1.ast, array );
        $$.target_ast = NULL;
      }

      DUMP_AST( "array_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

array_size_c_num
  : '[' ']'                       { $$ = C_ARRAY_SIZE_NONE; }
  | '[' Y_NUMBER ']'              { $$ = $2; }
  | '[' error ']'
    {
      ELABORATE_ERROR( "integer expected for array size" );
    }
  ;

block_decl_c_ast                        /* Apple extension */
  : /* type */ '(' Y_CIRC
    {
      //
      // A block AST has to be the type inherited attribute for decl_c_ast so
      // we have to create it here.
      //
      type_push( c_ast_new_gc( K_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_type_opt decl_c_ast ')' '(' arg_list_c_ast_opt ')'
    {
      c_ast_t *const block = type_pop();

      DUMP_START( "block_decl_c_ast",
                  "'(' '^' type_qualifier_list_c_type_opt decl_c_ast ')' "
                  "'(' arg_list_c_ast_opt ')'" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_TYPE( "type_qualifier_list_c_type_opt", $4 );
      DUMP_AST( "decl_c_ast", $5.ast );
      DUMP_AST_LIST( "arg_list_c_ast_opt", $8 );

      C_TYPE_ADD( &block->type_id, $4, @4 );
      block->as.block.args = $8;
      $$.ast = c_ast_add_func( $5.ast, type_peek(), block );
      $$.target_ast = block->as.block.ret_ast;

      DUMP_AST( "block_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

func_decl_c_ast
    /*
     * Function and C++ constructor declaration.
     *
     * This grammar rule handles both since they're so similar (and so would
     * cause grammar conflicts if they were separate rules in an LALR(1)
     * parser).
     */
  : /* type_c_ast */ decl2_c_ast '(' arg_list_c_ast_opt ')'
    func_qualifier_list_c_type_opt func_ref_qualifier_c_type_opt
    noexcept_c_type_opt trailing_return_type_c_ast_opt func_equals_type_opt
    {
      c_ast_t    *const decl2_ast = $1.ast;
      c_type_id_t const func_qualifier_type = $5;
      c_type_id_t const func_ref_qualifier_type = $6;
      c_type_id_t const noexcept_type = $7;
      c_type_id_t const pure_virtual_type = $9;
      c_ast_t    *const trailing_ret_ast = $8.ast;
      c_ast_t    *const type_ast = type_peek();

      DUMP_START( "func_decl_c_ast",
                  "decl2_c_ast '(' arg_list_c_ast_opt ')' "
                  "func_qualifier_list_c_type_opt "
                  "func_ref_qualifier_c_type_opt noexcept_c_type_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_type_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "decl2_c_ast", decl2_ast );
      DUMP_AST_LIST( "arg_list_c_ast_opt", $3 );
      DUMP_TYPE( "func_qualifier_list_c_type_opt", func_qualifier_type );
      DUMP_TYPE( "func_ref_qualifier_c_type_opt", func_ref_qualifier_type );
      DUMP_TYPE( "noexcept_c_type_opt", noexcept_type );
      DUMP_AST( "trailing_return_type_c_ast_opt", trailing_ret_ast );
      DUMP_TYPE( "func_equals_type_opt", pure_virtual_type );
      if ( $1.target_ast != NULL )
        DUMP_AST( "target_ast", $1.target_ast );

      //
      // Cdecl can't know for certain whether a "function" name is really a
      // constructor name because it:
      //
      //  1. Doesn't have the context of the surrounding class:
      //
      //          class T {
      //          public:
      //            T();                // <-- All cdecl usually has is this.
      //            // ...
      //          };
      //
      //  2. Can't check to see if the name is a typedef for a class, struct,
      //     or union (even though that would greatly help) since:
      //
      //          T(U);
      //
      //     could be either:
      //
      //      + A constructor of type T with an argument of type U; or:
      //      + A variable named U of type T (with unnecessary parentheses).
      //
      // Hence, we have to infer which of a function or a constructor was
      // likely the one meant.  If the declaration has:
      //
      //  + Any constructor-only storage-class-like type (e.g., explicit); or:
      //
      //  + Only any number of storage-class-like types that may be applied to
      //    constructors (e.g., constexpr, inline):
      //
      // then assume it's a constructor.
      //
      bool const assume_constructor =
        (type_ast->type_id & T_CONSTRUCTOR_ONLY) != T_NONE ||
          only_bits_in( type_ast->type_id, T_CONSTRUCTOR );

      c_ast_t *const func_ast =
        c_ast_new_gc( assume_constructor ? K_CONSTRUCTOR : K_FUNCTION, &@$ );
      func_ast->type_id = func_qualifier_type | func_ref_qualifier_type |
                          noexcept_type | pure_virtual_type;
      func_ast->as.func.args = $3;

      if ( assume_constructor ) {
        assert( trailing_ret_ast == NULL );
        if ( c_ast_sname_empty( func_ast ) )
          func_ast->sname = c_ast_take_name( decl2_ast );
        $$.ast = func_ast;
      }
      else {
        if ( trailing_ret_ast != NULL ) {
          $$.ast = c_ast_add_func( $1.ast, trailing_ret_ast, func_ast );
        }
        else if ( $1.target_ast != NULL ) {
          $$.ast = $1.ast;
          C_IGNORE_RV( c_ast_add_func( $1.target_ast, type_ast, func_ast ) );
        }
        else {
          $$.ast = c_ast_add_func( $1.ast, type_ast, func_ast );
        }
      }

      $$.target_ast = func_ast->as.func.ret_ast;

      DUMP_AST( "func_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

func_qualifier_list_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | func_qualifier_list_c_type_opt func_qualifier_c_type
    {
      DUMP_START( "func_qualifier_list_c_type_opt",
                  "func_qualifier_list_c_type_opt func_qualifier_c_type" );
      DUMP_TYPE( "func_qualifier_list_c_type_opt", $1 );
      DUMP_TYPE( "func_qualifier_c_type", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "func_qualifier_list_c_type_opt", $$ );
      DUMP_END();
    }
  ;

func_qualifier_c_type
  : cv_qualifier_type
  | Y_FINAL
  | Y_OVERRIDE
  /*
   * GNU C allows restricted-pointer member functions:
   * <https://gcc.gnu.org/onlinedocs/gcc/Restricted-Pointers.html>
   */
  | Y___RESTRICT__
  ;

func_ref_qualifier_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | Y_AMPER                       { $$ = T_REFERENCE; }
  | Y_AMPER2                      { $$ = T_RVALUE_REFERENCE; }
  ;

noexcept_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | Y_NOEXCEPT
  | Y_NOEXCEPT '(' noexcept_bool_type rparen_expected
    {
      $$ = $3;
    }
  | Y_THROW lparen_expected rparen_expected
  ;

noexcept_bool_type
  : Y_FALSE
  | Y_TRUE
  | error
    {
      ELABORATE_ERROR( "\"%s\" or \"%s\" expected", L_TRUE, L_FALSE );
    }
  ;

trailing_return_type_c_ast_opt
  : /* empty */                   { $$.ast = $$.target_ast = NULL; }
  | "->" type_c_ast { type_push( $2.ast ); } cast_c_ast_opt
    {
      type_pop();

      //
      // The function trailing return-type syntax is supported only in C++11
      // and later.  This check has to be done now in the parser rather than
      // later in the AST because the AST has no "memory" of where the return-
      // type came from.
      //
      if ( c_init >= C_INIT_READ_CONF && opt_lang < LANG_CPP_11 ) {
        print_error( &@1,
          "trailing return type not supported in %s", C_LANG_NAME()
        );
        PARSE_ABORT();
      }

      //
      // Ensure that a function using the C++11 trailing return type syntax
      // starts with "auto":
      //
      //      void f() -> int
      //      ^
      //      error: must be "auto"
      //
      // This check has to be done now in the parser rather than later in the
      // AST because the "auto" is just a syntactic return-type placeholder in
      // C++11 and the AST node for the placeholder is discarded and never made
      // part of the AST.
      //
      if ( type_peek()->type_id != T_AUTO_TYPE ) {
        print_error( &type_peek()->loc,
          "function with trailing return type must only specify \"%s\"", L_AUTO
        );
        PARSE_ABORT();
      }

      DUMP_START( "trailing_return_type_c_ast_opt",
                  "type_c_ast cast_c_ast_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "type_c_ast", $2.ast );
      DUMP_AST( "cast_c_ast_opt", $4.ast );

      $$ = $4.ast != NULL ? $4 : $2;

      DUMP_AST( "trailing_return_type_c_ast_opt", $$.ast );
      DUMP_END();
    }
  ;

func_equals_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | '=' Y_DEFAULT                 { $$ = $2; }
  | '=' Y_DELETE                  { $$ = $2; }
  | '=' Y_NUMBER
    {
      if ( $2 != 0 ) {
        print_error( &@2, "'0' expected" );
        PARSE_ABORT();
      }
      $$ = T_PURE_VIRTUAL;
    }
  | '=' error
    {
      ELABORATE_ERROR( "'0', \"%s\", or \"%s\" expected", L_DEFAULT, L_DELETE );
    }
  ;

nested_decl_c_ast
  : '(' placeholder_c_ast { type_push( $2.ast ); ++ast_depth; } decl_c_ast ')'
    {
      type_pop();
      --ast_depth;

      DUMP_START( "nested_decl_c_ast",
                  "'(' placeholder_c_ast decl_c_ast ')'" );
      DUMP_AST( "placeholder_c_ast", $2.ast );
      DUMP_AST( "decl_c_ast", $4.ast );

      $$ = $4;

      DUMP_AST( "nested_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

oper_decl_c_ast
  : /* type_c_ast */ oper_c_ast '(' arg_list_c_ast_opt ')'
    func_qualifier_list_c_type_opt func_ref_qualifier_c_type_opt
    noexcept_c_type_opt trailing_return_type_c_ast_opt func_equals_type_opt
    {
      DUMP_START( "oper_decl_c_ast",
                  "oper_c_ast '(' arg_list_c_ast_opt ')' "
                  "func_qualifier_list_c_type_opt "
                  "func_ref_qualifier_c_type_opt noexcept_c_type_opt "
                  "trailing_return_type_c_ast_opt "
                  "func_equals_type_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "oper_c_ast", $1.ast );
      DUMP_AST_LIST( "arg_list_c_ast_opt", $3 );
      DUMP_TYPE( "func_qualifier_list_c_type_opt", $5 );
      DUMP_TYPE( "func_ref_qualifier_c_type_opt", $6 );
      DUMP_TYPE( "noexcept_c_type_opt", $7 );
      DUMP_AST( "trailing_return_type_c_ast_opt", $8.ast );
      DUMP_TYPE( "func_equals_type_opt", $9 );

      c_ast_t *const oper = c_ast_new_gc( K_OPERATOR, &@$ );
      oper->type_id = $5 | $6 | $7 | $9;
      oper->as.oper.args = $3;
      oper->as.oper.oper_id = $1.ast->as.oper.oper_id;

      if ( $8.ast != NULL ) {
        $$.ast = c_ast_add_func( $1.ast, $8.ast, oper );
      }
      else if ( $1.target_ast != NULL ) {
        $$.ast = $1.ast;
        C_IGNORE_RV( c_ast_add_func( $1.target_ast, type_peek(), oper ) );
      }
      else {
        $$.ast = c_ast_add_func( $1.ast, type_peek(), oper );
      }
      $$.target_ast = oper->as.oper.ret_ast;

      DUMP_AST( "oper_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

oper_c_ast
  : /* type_c_ast */ scope_sname_c_opt Y_OPERATOR c_operator
    {
      DUMP_START( "oper_c_ast", "OPERATOR c_operator" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_SNAME( "scope_sname_c_opt", &$1 );
      DUMP_STR( "c_operator", op_get( $3 )->name );

      $$.ast = type_peek();
      $$.target_ast = NULL;
      $$.ast->sname = $1;
      $$.ast->as.oper.oper_id = $3;

      DUMP_AST( "oper_c_ast", $$.ast );
      DUMP_END();
    }
  ;

placeholder_c_ast
  : /* empty */
    {
      $$.ast = c_ast_new_gc( K_PLACEHOLDER, &@$ );
      $$.target_ast = NULL;
    }
  ;

pointer_decl_c_ast
  : pointer_type_c_ast { type_push( $1.ast ); } decl_c_ast
    {
      type_pop();

      DUMP_START( "pointer_decl_c_ast", "pointer_type_c_ast decl_c_ast" );
      DUMP_AST( "pointer_type_c_ast", $1.ast );
      DUMP_AST( "decl_c_ast", $3.ast );

      C_IGNORE_RV( c_ast_patch_placeholder( $1.ast, $3.ast ) );
      $$ = $3;

      DUMP_AST( "pointer_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

pointer_type_c_ast
  : /* type_c_ast */ '*' type_qualifier_list_c_type_opt
    {
      DUMP_START( "pointer_type_c_ast", "* type_qualifier_list_c_type_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_TYPE( "type_qualifier_list_c_type_opt", $2 );

      $$.ast = c_ast_new_gc( K_POINTER, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = $2;
      c_ast_set_parent( type_peek(), $$.ast );

      DUMP_AST( "pointer_type_c_ast", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_decl_c_ast
  : pointer_to_member_type_c_ast { type_push( $1.ast ); } decl_c_ast
    {
      type_pop();

      DUMP_START( "pointer_to_member_decl_c_ast",
                  "pointer_to_member_type_c_ast decl_c_ast" );
      DUMP_AST( "pointer_to_member_type_c_ast", $1.ast );
      DUMP_AST( "decl_c_ast", $3.ast );

      $$ = $3;

      DUMP_AST( "pointer_to_member_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_type_c_ast
  : /* type_c_ast */ any_sname_c Y_COLON2_STAR
    cv_qualifier_list_c_type_opt
    {
      DUMP_START( "pointer_to_member_type_c_ast",
                  "sname ::* cv_qualifier_list_c_type_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_SNAME( "sname", &$1 );
      DUMP_TYPE( "cv_qualifier_list_c_type_opt", $3 );

      $$.ast = c_ast_new_gc( K_POINTER_TO_MEMBER, &@$ );
      $$.target_ast = NULL;

      //
      // If the scoped name has a class, namespace, struct, or union scoped
      // type, adopt that type for the AST's type; otherwise default to
      // T_CLASS.
      //
      c_type_id_t sn_type = c_sname_type( &$1 );
      if ( (sn_type & (T_CLASS_STRUCT_UNION | T_NAMESPACE)) == T_NONE )
        sn_type = T_CLASS;

      $$.ast->type_id = sn_type | $3;
      $$.ast->as.ptr_mbr.class_sname = $1;
      c_ast_set_parent( type_peek(), $$.ast );

      DUMP_AST( "pointer_to_member_type_c_ast", $$.ast );
      DUMP_END();
    }
  ;

reference_decl_c_ast
  : reference_type_c_ast { type_push( $1.ast ); } decl_c_ast
    {
      type_pop();

      DUMP_START( "reference_decl_c_ast", "reference_type_c_ast decl_c_ast" );
      DUMP_AST( "reference_type_c_ast", $1.ast );
      DUMP_AST( "decl_c_ast", $3.ast );

      $$ = $3;

      DUMP_AST( "reference_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

reference_type_c_ast
  : /* type_c_ast */ Y_AMPER
    {
      DUMP_START( "reference_type_c_ast", "&" );
      DUMP_AST( "(type_c_ast)", type_peek() );

      $$.ast = c_ast_new_gc( K_REFERENCE, &@$ );
      $$.target_ast = NULL;
      c_ast_set_parent( type_peek(), $$.ast );

      DUMP_AST( "reference_type_c_ast", $$.ast );
      DUMP_END();
    }

  | /* type_c_ast */ Y_AMPER2
    {
      DUMP_START( "reference_type_c_ast", "&&" );
      DUMP_AST( "(type_c_ast)", type_peek() );

      $$.ast = c_ast_new_gc( K_RVALUE_REFERENCE, &@$ );
      $$.target_ast = NULL;
      c_ast_set_parent( type_peek(), $$.ast );

      DUMP_AST( "reference_type_c_ast", $$.ast );
      DUMP_END();
    }
  ;


user_defined_conversion_decl_c_ast
  : /* type_c_ast */ scope_sname_c_opt Y_OPERATOR
    type_c_ast { type_push( $3.ast ); } udc_decl_c_ast_opt '(' rparen_expected
    func_qualifier_list_c_type_opt noexcept_c_type_opt func_equals_type_opt
    {
      type_pop();

      DUMP_START( "user_defined_conversion_decl_c_ast",
                  "scope_sname_c_opt OPERATOR "
                  "type_c_ast udc_decl_c_ast_opt '(' ')' "
                  "func_qualifier_list_c_type_opt noexcept_c_type_opt "
                  "func_equals_type_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_SNAME( "scope_sname_c_opt", &$1 );
      DUMP_AST( "type_c_ast", $3.ast );
      DUMP_AST( "udc_decl_c_ast_opt", $5.ast );
      DUMP_TYPE( "func_qualifier_list_c_type_opt", $8 );
      DUMP_TYPE( "noexcept_c_type_opt", $9 );
      DUMP_TYPE( "func_equals_type_opt", $10 );

      $$.ast = c_ast_new_gc( K_USER_DEF_CONVERSION, &@$ );
      $$.ast->sname = $1;
      $$.ast->type_id = $8 | $9 | $10;
      $$.ast->as.user_def_conv.conv_ast = $5.ast != NULL ? $5.ast : $3.ast;
      $$.target_ast = $$.ast->as.user_def_conv.conv_ast;

      DUMP_AST( "user_defined_conversion_decl_c_ast", $$.ast );
      DUMP_END();
    }

typedef_type_decl_c_ast
  : typedef_type_c_ast
    {
      DUMP_START( "typedef_type_decl_c_ast", "typedef_type_c_ast" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "typedef_type_c_ast", $1.ast );

      if ( (type_peek()->type_id & T_TYPEDEF) != T_NONE ) {
        //
        // If we're defining a type, return the type as-is.
        //
        $$.ast = $1.ast;
      }
      else {
        //
        // Otherwise, return the type that it's typedef'd as.
        //
        $$.ast = CONST_CAST( c_ast_t*, c_ast_untypedef( $1.ast ) );
      }
      $$.target_ast = NULL;

      DUMP_AST( "typedef_type_c_ast", $$.ast );
      DUMP_END();
    }
  ;

user_defined_literal_decl_c_ast
  : /* type_c_ast */ user_defined_literal_c_ast '(' arg_list_c_ast ')'
    noexcept_c_type_opt trailing_return_type_c_ast_opt
    {
      DUMP_START( "user_defined_literal_decl_c_ast",
                  "user_defined_literal_c_ast '(' arg_list_c_ast ')' "
                  "noexcept_c_type_opt trailing_return_type_c_ast_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "oper_c_ast", $1.ast );
      DUMP_AST_LIST( "arg_list_c_ast", $3 );
      DUMP_TYPE( "noexcept_c_type_opt", $5 );
      DUMP_AST( "trailing_return_type_c_ast_opt", $6.ast );

      c_ast_t *const lit = c_ast_new_gc( K_USER_DEF_LITERAL, &@$ );
      lit->as.user_def_lit.args = $3;

      if ( $6.ast != NULL ) {
        $$.ast = c_ast_add_func( $1.ast, $6.ast, lit );
      }
      else if ( $1.target_ast != NULL ) {
        $$.ast = $1.ast;
        C_IGNORE_RV( c_ast_add_func( $1.target_ast, type_peek(), lit ) );
      }
      else {
        $$.ast = c_ast_add_func( $1.ast, type_peek(), lit );
      }
      $$.target_ast = lit->as.user_def_lit.ret_ast;

      DUMP_AST( "user_defined_literal_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

user_defined_literal_c_ast
  : /* type_c_ast */ scope_sname_c_opt Y_OPERATOR quote2_expected name_expected
    {
      DUMP_START( "user_defined_literal_c_ast", "OPERATOR \"\" NAME" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_SNAME( "scope_sname_c_opt", &$1 );
      DUMP_STR( "name", $4 );

      $$.ast = type_peek();
      $$.target_ast = NULL;
      $$.ast->sname = $1;
      c_sname_append_name( &$$.ast->sname, $4 );

      DUMP_AST( "user_defined_literal_c_ast", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  function argument gibberish productions                                  */
/*****************************************************************************/

arg_list_c_ast_opt
  : /* empty */                   { slist_init( &$$ ); }
  | arg_list_c_ast
  ;

arg_list_c_ast
  : arg_list_c_ast comma_expected arg_c_ast
    {
      DUMP_START( "arg_list_c_ast", "arg_list_c_ast ',' arg_c_ast" );
      DUMP_AST_LIST( "arg_list_c_ast", $1 );
      DUMP_AST( "arg_c_ast", $3.ast );

      $$ = $1;
      slist_push_tail( &$$, $3.ast );

      DUMP_AST_LIST( "arg_list_c_ast", $$ );
      DUMP_END();
    }

  | arg_c_ast
    {
      DUMP_START( "arg_list_c_ast", "arg_c_ast" );
      DUMP_AST( "arg_c_ast", $1.ast );

      slist_init( &$$ );
      slist_push_tail( &$$, $1.ast );

      DUMP_AST_LIST( "arg_list_c_ast", $$ );
      DUMP_END();
    }
  ;

arg_c_ast
  : type_c_ast { type_push( $1.ast ); } cast_c_ast_opt
    {
      type_pop();

      DUMP_START( "arg_c_ast", "type_c_ast cast_c_ast_opt" );
      DUMP_AST( "type_c_ast", $1.ast );
      DUMP_AST( "cast_c_ast_opt", $3.ast );

      $$ = $3.ast != NULL ? $3 : $1;
      if ( c_ast_sname_empty( $$.ast ) )
        $$.ast->sname = c_sname_dup( c_ast_find_name( $$.ast, C_VISIT_DOWN ) );

      DUMP_AST( "arg_c_ast", $$.ast );
      DUMP_END();
    }

  | name_ast                            /* K&R C type-less argument */

  | "..."
    {
      DUMP_START( "argc", "..." );

      $$.ast = c_ast_new_gc( K_VARIADIC, &@$ );
      $$.target_ast = NULL;

      DUMP_AST( "arg_c_ast", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  type gibberish productions                                               */
/*****************************************************************************/

type_c_ast
  : type_modifier_list_c_type           /* allows implicit int in K&R C */
    {
      DUMP_START( "type_c_ast", "type_modifier_list_c_type" );
      DUMP_TYPE( "type_modifier_list_c_type", $1 );

      //
      // Prior to C99, typeless declarations are implicitly int, so we set it
      // here.  In C99 and later, however, implicit int is an error, so we
      // don't set it here and c_ast_check() will catch the error later.
      //
      // Note that type modifiers, e.g., unsigned, count as a type since that
      // means unsigned int; however, neither qualifiers, e.g., const, nor
      // storage classes, e.g., register, by themselves count as a type:
      //
      //      unsigned i;   // legal in C99
      //      const    j;   // illegal in C99
      //      register k;   // illegal in C99
      //
      c_type_id_t type_id = opt_lang < LANG_C_99 ? T_INT : T_NONE;

      C_TYPE_ADD( &type_id, $1, @1 );

      $$.ast = c_ast_new_gc( K_BUILTIN, &@$ );
      $$.ast->type_id = type_id;
      $$.target_ast = NULL;

      DUMP_AST( "type_c_ast", $$.ast );
      DUMP_END();
    }

  | type_modifier_list_c_type unmodified_type_c_ast
    type_modifier_list_c_type_opt
    {
      DUMP_START( "type_c_ast",
                  "type_modifier_list_c_type unmodified_type_c_ast "
                  "type_modifier_list_c_type_opt" );
      DUMP_TYPE( "type_modifier_list_c_type", $1 );
      DUMP_AST( "unmodified_type_c_ast", $2.ast );
      DUMP_TYPE( "type_modifier_list_c_type_opt", $3 );

      $$ = $2;
      C_TYPE_ADD( &$$.ast->type_id, $1, @1 );
      C_TYPE_ADD( &$$.ast->type_id, $3, @3 );

      DUMP_AST( "type_c_ast", $$.ast );
      DUMP_END();
    }

  | unmodified_type_c_ast type_modifier_list_c_type_opt
    {
      DUMP_START( "type_c_ast",
                  "unmodified_type_c_ast type_modifier_list_c_type_opt" );
      DUMP_AST( "unmodified_type_c_ast", $1.ast );
      DUMP_TYPE( "type_modifier_list_c_type_opt", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$.ast->type_id, $2, @2 );

      DUMP_AST( "type_c_ast", $$.ast );
      DUMP_END();
    }
  ;

type_modifier_list_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | type_modifier_list_c_type
  ;

type_modifier_list_c_type
  : type_modifier_list_c_type type_modifier_c_type
    {
      DUMP_START( "type_modifier_list_c_type",
                  "type_modifier_list_c_type type_modifier_c_type" );
      DUMP_TYPE( "type_modifier_list_c_type", $1 );
      DUMP_TYPE( "type_modifier_c_type", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "type_modifier_list_c_type", $$ );
      DUMP_END();
    }

  | type_modifier_c_type
  ;

type_modifier_c_type
  : type_modifier_base_type
    {
      $$ = $1;
      //
      // This is for a case like:
      //
      //      explain typedef unsigned long size_t
      //
      // that is: explain a redefinition of a typedef'd type with the same type
      // that contains only one or more type_modifier_base_type.  The problem
      // is that, without an unmodified_type_c_ast (like int), the parser would
      // ordinarily take the typedef'd type (here, the size_t) as part of the
      // type_c_ast and then be out of tokens for the decl_c_ast -- at which
      // time it'll complain.
      //
      // Since type modifiers can't apply to a typedef'd type (e.g., "long
      // size_t" is illegal), we tell the lexer not to return either
      // Y_TYPEDEF_NAME or Y_TYPEDEF_SNAME if we encounter at least one type
      // modifier (except "register" since it's is really a storage class --
      // see the comment in type_modifier_base_type about "register").
      //
      lexer_find_typedef = $$ == T_REGISTER;
    }
  | type_qualifier_c_type
  | storage_class_c_type
  ;

type_modifier_base_type
  : Y__COMPLEX
  | Y__IMAGINARY
  | Y_LONG
  | Y_SHORT
  | Y_SIGNED
  | Y_UNSIGNED
  /*
   * Register is here (rather than in storage_class_c_type) because it's the
   * only storage class that can be specified for function arguments.
   * Therefore, it's simpler to treat it as any other type modifier.
   */
  | Y_REGISTER
  ;

unmodified_type_c_ast
  : atomic_specifier_type_c_ast
  | unmodified_type_english_ast
  ;

atomic_specifier_type_c_ast
  : Y_ATOMIC_SPEC '(' type_c_ast { type_push( $3.ast ); } cast_c_ast_opt ')'
    {
      type_pop();

      DUMP_START( "atomic_specifier_type_c_ast",
                  "ATOMIC '(' type_c_ast cast_c_ast_opt ')'" );
      DUMP_AST( "type_c_ast", $3.ast );
      DUMP_AST( "cast_c_ast_opt", $5.ast );

      $$ = $5.ast != NULL ? $5 : $3;
      C_TYPE_ADD( &$$.ast->type_id, T_ATOMIC, @1 );

      DUMP_AST( "atomic_specifier_type_c_ast", $$.ast );
      DUMP_END();
    }
  ;

builtin_type_c_ast
  : builtin_type
    {
      DUMP_START( "builtin_type_c_ast", "builtin_type" );
      DUMP_TYPE( "builtin_type", $1 );

      $$.ast = c_ast_new_gc( K_BUILTIN, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = $1;

      DUMP_AST( "builtin_type_c_ast", $$.ast );
      DUMP_END();
    }
  ;

builtin_type
  : Y_VOID
  | Y_AUTO_TYPE
  | Y_BOOL
  | Y_CHAR
  | Y_CHAR8_T
  | Y_CHAR16_T
  | Y_CHAR32_T
  | Y_WCHAR_T
  | Y_INT
  | Y_FLOAT
  | Y_DOUBLE
  ;

enum_class_struct_union_ast
  : enum_class_struct_union_type any_sname_c_expected
    {
      DUMP_START( "enum_class_struct_union_ast",
                  "enum_class_struct_union_type sname" );
      DUMP_TYPE( "enum_class_struct_union_type", $1 );
      DUMP_SNAME( "sname", &$2 );

      $$.ast = c_ast_new_gc( K_ENUM_CLASS_STRUCT_UNION, &@$ );
      $$.target_ast = NULL;
      $$.ast->type_id = $1;
      $$.ast->as.ecsu.ecsu_sname = $2;

      DUMP_AST( "enum_class_struct_union_ast", $$.ast );
      DUMP_END();
    }

  | enum_class_struct_union_type any_sname_c_opt '{'
    {
      print_error( &@3,
        "%s definitions are not supported",
        c_type_name( $1 )
      );
      PARSE_ABORT();
    }
  ;

enum_class_struct_union_type
  : Y_ENUM
  | Y_ENUM class_struct_type      { $$ = $1 | $2; }
  | class_struct_union_type
  ;

class_struct_type
  : Y_CLASS
  | Y_STRUCT
  ;

class_struct_union_type
  : class_struct_type
  | Y_UNION
  ;

type_qualifier_list_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | type_qualifier_list_c_type
  ;

type_qualifier_list_c_type
  : type_qualifier_list_c_type type_qualifier_c_type
    {
      DUMP_START( "type_qualifier_list_c_type",
                  "type_qualifier_list_c_type type_qualifier_c_type" );
      DUMP_TYPE( "type_qualifier_list_c_type", $1 );
      DUMP_TYPE( "type_qualifier_c_type", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "type_qualifier_list_c_type", $$ );
      DUMP_END();
    }

  | type_qualifier_c_type
  ;

type_qualifier_c_type
  : cv_qualifier_type
  | Y_ATOMIC_QUAL
  | Y_RESTRICT
    {
      //
      // "restrict" is supported only in GNU C.
      //
      // This check has to be done now in the parser rather than later in the
      // AST since both "restrict" and "__restrict__" map to T_RESTRICT and the
      // AST has no "memory" of which it was.
      //
      if ( C_LANG_IS_CPP() ) {
        print_error( &@1,
          "\"%s\" not supported in %s; use \"%s\" instead",
          L_RESTRICT, C_LANG_NAME(), L___RESTRICT__
        );
        PARSE_ABORT();
      }
    }
  | Y___RESTRICT__
  ;

cv_qualifier_list_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | cv_qualifier_list_c_type_opt cv_qualifier_type
    {
      DUMP_START( "cv_qualifier_list_c_type_opt",
                  "cv_qualifier_list_c_type_opt cv_qualifier_type" );
      DUMP_TYPE( "cv_qualifier_list_c_type_opt", $1 );
      DUMP_TYPE( "cv_qualifier_type", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "cv_qualifier_list_c_type_opt", $$ );
      DUMP_END();
    }
  ;

cv_qualifier_type
  : Y_CONST
  | Y_VOLATILE
  ;

storage_class_c_type
  : attribute_specifier_list_c_type
  | Y_AUTO_STORAGE
  | Y___BLOCK                           /* Apple extension */
  | Y_CONSTEVAL
  | Y_CONSTEXPR
  | Y_EXPLICIT
  | Y_EXTERN
  | Y_FINAL
  | Y_FRIEND
  | Y_INLINE
  | Y_MUTABLE
  | Y_NORETURN
  | Y__NORETURN
    {
      if ( !_Noreturn_ok( &@1 ) )
        PARSE_ABORT();
    }
  | Y_OVERRIDE
/*| Y_REGISTER */                       /* in type_modifier_base_type */
  | Y_STATIC
  | Y_TYPEDEF
  | Y_THREAD_LOCAL
  | Y_VIRTUAL
  ;

attribute_specifier_list_c_type
  : "[[" attribute_name_list_c_type_opt "]]"
    {
      DUMP_START( "attribute_specifier_list_c_type",
                  "[[ attribute_name_list_c_type_opt ]]" );
      DUMP_TYPE( "attribute_name_list_c_type_opt", $2 );
      DUMP_END();

      $$ = $2;
    }
  ;

attribute_name_list_c_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | attribute_name_list_c_type
  ;

attribute_name_list_c_type
  : attribute_name_list_c_type comma_expected attribute_name_c_type
    {
      DUMP_START( "attribute_name_list_c_type",
                  "attribute_name_list_c_type , attribute_name_c_type" );
      DUMP_TYPE( "attribute_name_list_c_type", $1 );
      DUMP_TYPE( "attribute_name_c_type", $3 );

      $$ = $1;
      C_TYPE_ADD( &$$, $3, @3 );

      DUMP_TYPE( "attribute_name_list_c_type", $$ );
      DUMP_END();
    }

  | attribute_name_c_type
  ;

attribute_name_c_type
  : name_expected
    {
      DUMP_START( "attribute_name_c_type", "Y_NAME" );
      DUMP_STR( "NAME", $1 );
      DUMP_END();

      $$ = T_NONE;

      c_keyword_t const *const a = c_attribute_find( $1 );
      if ( a == NULL ) {
        print_warning( &@1, "\"%s\": unknown attribute", $1 );
      }
      else if ( c_init >= C_INIT_READ_CONF &&
                (opt_lang & a->lang_ids) == LANG_NONE ) {
        print_warning( &@1, "\"%s\" not supported in %s", $1, C_LANG_NAME() );
      }
      else {
        $$ = a->type_id;
      }

      FREE( $1 );
    }
  ;

/*****************************************************************************/
/*  cast gibberish productions                                               */
/*****************************************************************************/

cast_c_ast_opt
  : /* empty */                   { $$.ast = $$.target_ast = NULL; }
  | cast_c_ast
  ;

cast_c_ast
  : cast2_c_ast
  | pointer_cast_c_ast
  | pointer_to_member_cast_c_ast
  | reference_cast_c_ast
  ;

cast2_c_ast
  : array_cast_c_ast
  | block_cast_c_ast
  | func_cast_c_ast
  | nested_cast_c_ast
  | sname_c_ast
/*| typedef_type_decl_c_ast */          /* you can't cast a type */
  ;

array_cast_c_ast
  : /* type */ cast_c_ast_opt arg_array_size_c_ast
    {
      DUMP_START( "array_cast_c_ast", "cast_c_ast_opt array_size_c_num" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "cast_c_ast_opt", $1.ast );
      if ( $1.target_ast != NULL )
        DUMP_AST( "target_ast", $1.target_ast );
      DUMP_AST( "arg_array_size_c_ast", $2 );

      c_ast_set_parent( c_ast_new_gc( K_PLACEHOLDER, &@1 ), $2 );

      if ( $1.target_ast != NULL ) {    // array-of or function-like-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, $2 );
      } else {
        c_ast_t *const ast = $1.ast != NULL ? $1.ast : type_peek();
        $$.ast = c_ast_add_array( ast, $2 );
        $$.target_ast = NULL;
      }

      DUMP_AST( "array_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

arg_array_size_c_ast
  : array_size_c_num
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $1;
    }
  | '[' type_qualifier_list_c_type static_type_opt Y_NUMBER ']'
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $4;
      $$->as.array.type_id = $2 | $3;
    }
  | '[' Y_STATIC type_qualifier_list_c_type_opt Y_NUMBER ']'
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = $4;
      $$->as.array.type_id = $2 | $3;
    }
  | '[' type_qualifier_list_c_type_opt '*' ']'
    {
      $$ = c_ast_new_gc( K_ARRAY, &@$ );
      $$->as.array.size = C_ARRAY_SIZE_VARIABLE;
      $$->as.array.type_id = $2;
    }
  ;

static_type_opt
  : /* empty */                   { $$ = T_NONE; }
  | Y_STATIC
  ;

block_cast_c_ast                        /* Apple extension */
  : /* type */ '(' Y_CIRC
    {
      //
      // A block AST has to be the type inherited attribute for cast_c_ast_opt
      // so we have to create it here.
      //
      type_push( c_ast_new_gc( K_BLOCK, &@$ ) );
    }
    type_qualifier_list_c_type_opt cast_c_ast_opt ')' '(' arg_list_c_ast_opt ')'
    {
      c_ast_t *const block = type_pop();

      DUMP_START( "block_cast_c_ast",
                  "'(' '^' type_qualifier_list_c_type_opt cast_c_ast_opt ')' "
                  "'(' arg_list_c_ast_opt ')'" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_TYPE( "type_qualifier_list_c_type_opt", $4 );
      DUMP_AST( "cast_c_ast_opt", $5.ast );
      DUMP_AST_LIST( "arg_list_c_ast_opt", $8 );

      C_TYPE_ADD( &block->type_id, $4, @4 );
      block->as.block.args = $8;
      $$.ast = c_ast_add_func( $5.ast, type_peek(), block );
      $$.target_ast = block->as.block.ret_ast;

      DUMP_AST( "block_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

func_cast_c_ast
  : /* type_c_ast */ cast2_c_ast '(' arg_list_c_ast_opt ')'
    func_qualifier_list_c_type_opt trailing_return_type_c_ast_opt
    {
      DUMP_START( "func_cast_c_ast",
                  "cast2_c_ast '(' arg_list_c_ast_opt ')' "
                  "func_qualifier_list_c_type_opt "
                  "trailing_return_type_c_ast_opt" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_AST( "cast2_c_ast", $1.ast );
      DUMP_AST_LIST( "arg_list_c_ast_opt", $3 );
      DUMP_TYPE( "func_qualifier_list_c_type_opt", $5 );
      DUMP_AST( "trailing_return_type_c_ast_opt", $6.ast );
      if ( $1.target_ast != NULL )
        DUMP_AST( "target_ast", $1.target_ast );

      c_ast_t *const func = c_ast_new_gc( K_FUNCTION, &@$ );
      func->type_id = $5;
      func->as.func.args = $3;

      if ( $6.ast != NULL ) {
        $$.ast = c_ast_add_func( $1.ast, $6.ast, func );
      }
      else if ( $1.target_ast != NULL ) {
        $$.ast = $1.ast;
        C_IGNORE_RV( c_ast_add_func( $1.target_ast, type_peek(), func ) );
      }
      else {
        $$.ast = c_ast_add_func( $1.ast, type_peek(), func );
      }
      $$.target_ast = func->as.func.ret_ast;

      DUMP_AST( "func_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

nested_cast_c_ast
  : '(' placeholder_c_ast
    {
      type_push( $2.ast );
      ++ast_depth;
    }
    cast_c_ast_opt ')'
    {
      type_pop();
      --ast_depth;

      DUMP_START( "nested_cast_c_ast",
                  "'(' placeholder_c_ast cast_c_ast_opt ')'" );
      DUMP_AST( "placeholder_c_ast", $2.ast );
      DUMP_AST( "cast_c_ast_opt", $4.ast );

      $$ = $4;

      DUMP_AST( "nested_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

pointer_cast_c_ast
  : pointer_type_c_ast { type_push( $1.ast ); } cast_c_ast_opt
    {
      type_pop();

      DUMP_START( "pointer_cast_c_ast", "pointer_type_c_ast cast_c_ast_opt" );
      DUMP_AST( "pointer_type_c_ast", $1.ast );
      DUMP_AST( "cast_c_ast_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "pointer_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_cast_c_ast
  : pointer_to_member_type_c_ast { type_push( $1.ast ); } cast_c_ast_opt
    {
      type_pop();

      DUMP_START( "pointer_to_member_cast_c_ast",
                  "pointer_to_member_type_c_ast cast_c_ast_opt" );
      DUMP_AST( "pointer_to_member_type_c_ast", $1.ast );
      DUMP_AST( "cast_c_ast_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "pointer_to_member_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

reference_cast_c_ast
  : reference_type_c_ast { type_push( $1.ast ); } cast_c_ast_opt
    {
      type_pop();

      DUMP_START( "reference_cast_c_ast",
                  "reference_type_c_ast cast_c_ast_opt" );
      DUMP_AST( "reference_type_c_ast", $1.ast );
      DUMP_AST( "cast_c_ast_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "reference_cast_c_ast", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  user-defined conversion gibberish productions                            */
/*                                                                           */
/*  These are a subset of cast gibberish productions, specifically without   */
/*  arrays, blocks, functions, or nested declarations, all of which are      */
/*  either illegal or ambiguous.                                             */
/*****************************************************************************/

udc_decl_c_ast_opt
  : /* empty */                   { $$.ast = $$.target_ast = NULL; }
  | udc_decl_c_ast
  ;

udc_decl_c_ast
  : pointer_udc_decl_c_ast
  | pointer_to_member_udc_decl_c_ast
  | reference_udc_decl_c_ast
  | sname_c_ast
  ;

pointer_udc_decl_c_ast
  : pointer_type_c_ast { type_push( $1.ast ); } udc_decl_c_ast_opt
    {
      type_pop();

      DUMP_START( "pointer_udc_decl_c_ast",
                  "pointer_type_c_ast udc_decl_c_ast_opt" );
      DUMP_AST( "pointer_type_c_ast", $1.ast );
      DUMP_AST( "udc_decl_c_ast_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "pointer_udc_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_udc_decl_c_ast
  : pointer_to_member_type_c_ast { type_push( $1.ast ); } udc_decl_c_ast_opt
    {
      type_pop();

      DUMP_START( "pointer_to_member_udc_decl_c_ast",
                  "pointer_to_member_type_c_ast udc_decl_c_ast_opt" );
      DUMP_AST( "pointer_to_member_type_c_ast", $1.ast );
      DUMP_AST( "udc_decl_c_ast_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "pointer_to_member_udc_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

reference_udc_decl_c_ast
  : reference_type_c_ast { type_push( $1.ast ); } udc_decl_c_ast_opt
    {
      type_pop();

      DUMP_START( "reference_udc_decl_c_ast",
                  "reference_type_c_ast udc_decl_c_ast_opt" );
      DUMP_AST( "reference_type_c_ast", $1.ast );
      DUMP_AST( "udc_decl_c_ast_opt", $3.ast );

      $$.ast = c_ast_patch_placeholder( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "reference_udc_decl_c_ast", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  name productions                                                         */
/*****************************************************************************/

any_name
  : Y_NAME
  | Y_TYPEDEF_NAME
    {
      assert( c_ast_sname_count( $1->ast ) == 1 );
      $$ = strdup( c_ast_sname_local_name( $1->ast ) );
    }
  ;

any_name_expected
  : any_name
  | error
    {
      $$ = NULL;
      ELABORATE_ERROR( "name expected" );
    }
  ;

any_sname_c
  : sname_c
  | typedef_sname_c
  ;

any_sname_c_expected
  : any_sname_c
  | error
    {
      c_sname_init( &$$ );
      ELABORATE_ERROR( "name expected" );
    }
  ;

any_sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | any_sname_c
  ;

any_typedef
  : Y_TYPEDEF_NAME
  | Y_TYPEDEF_SNAME
  ;

typedef_type_c_ast
  : any_typedef
    {
      DUMP_START( "typedef_type_c_ast", "any_typedef" );
      DUMP_AST( "any_typedef", $1->ast );

      $$.ast = c_ast_new_gc( K_TYPEDEF, &@$ );
      $$.target_ast = NULL;
      $$.ast->as.c_typedef = $1;
      $$.ast->type_id = T_TYPEDEF_TYPE;

      DUMP_AST( "typedef_type_c_ast", $$.ast );
      DUMP_END();
    }
  | /* type_c_ast */ any_typedef "::" sname_c
    {
      //
      // This is for a case like:
      //
      //      define S as struct S
      //      explain int S::x
      //
      // that is: a typedef'd type used for a scope.
      //
      DUMP_START( "typedef_type_c_ast",
                  "any_typedef :: sname_c" );
      DUMP_AST( "any_typedef", $1->ast );
      DUMP_SNAME( "sname_c", &$3 );

      $$.ast = type_peek();
      $$.target_ast = NULL;
      c_sname_t temp_name = c_ast_sname_dup( $1->ast );
      c_ast_sname_set_sname( $$.ast, &temp_name );
      c_ast_sname_append_sname( $$.ast, &$3 );

      DUMP_AST( "typedef_type_c_ast", $$.ast );
      DUMP_END();
    }
  | /* type_c_ast */ any_typedef "::" typedef_sname_c
    {
      //
      // This is for a case like:
      //
      //      define S as struct S
      //      define T as struct T
      //      explain int S::T::x
      //
      // that is: a typedef'd type used for an intermediate scope.
      //
      DUMP_START( "typedef_type_c_ast",
                  "any_typedef :: typedef_sname_c" );
      DUMP_AST( "any_typedef", $1->ast );
      DUMP_SNAME( "typedef_sname_c", &$3 );

      $$.ast = type_peek();
      $$.target_ast = NULL;
      c_sname_t temp_name = c_ast_sname_dup( $1->ast );
      c_ast_sname_set_sname( $$.ast, &temp_name );
      c_ast_sname_append_sname( $$.ast, &$3 );

      DUMP_AST( "typedef_type_c_ast", $$.ast );
      DUMP_END();
    }
  ;

name_ast
  : Y_NAME
    {
      DUMP_START( "name_ast", "NAME" );
      DUMP_STR( "NAME", $1 );

      $$.ast = c_ast_new_gc( K_NAME, &@$ );
      $$.target_ast = NULL;
      c_ast_sname_set_name( $$.ast, $1 );

      DUMP_AST( "name_ast", $$.ast );
      DUMP_END();
    }
  ;

name_expected
  : Y_NAME
  | error
    {
      $$ = NULL;
      ELABORATE_ERROR( "name expected" );
    }
  ;

scope_sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | sname_c "::"
    {
      $$ = $1;
      if ( c_sname_type( &$$ ) == T_NONE ) {
        //
        // Since we know the name in this context (followed by "::") definitely
        // refers to a scope, set the scoped name's type to T_SCOPE (if it
        // doesn't already have a scope type).
        //
        c_sname_set_type( &$$, T_SCOPE );
      }
    }
  | any_typedef "::"
    {
      //
      // This is for a case like:
      //
      //      define S as struct S
      //      explain bool S::operator!() const
      //
      // that is: a typedef'd type used for a scope.
      //
      $$ = c_ast_sname_dup( $1->ast );
    }
  ;

sname_c
  : sname_c "::" Y_NAME
    {
      // see the comment in "of_scope_list_english_opt"
      if ( c_init >= C_INIT_READ_CONF && C_LANG_IS_C() ) {
        print_error( &@2, "scoped names not supported in %s", C_LANG_NAME() );
        PARSE_ABORT();
      }
      $$ = $1;
      c_type_id_t sn_type = c_sname_type( &$1 );
      if ( sn_type == T_NONE )
        sn_type = T_SCOPE;
      c_sname_set_type( &$$, sn_type );
      c_sname_append_name( &$$, $3 );
    }
  | sname_c "::" any_typedef
    {
      //
      // This is for a case like:
      //
      //      define S::int8_t as char
      //
      // that is: the type int8_t is an existing type in no scope being defined
      // as a distinct type in a new scope.
      //
      $$ = $1;
      c_type_id_t sn_type = c_sname_type( &$$ );
      if ( sn_type == T_NONE )
        sn_type = T_SCOPE;
      c_sname_set_type( &$$, sn_type );
      c_sname_t temp = c_ast_sname_dup( $3->ast );
      c_sname_append_sname( &$$, &temp );
    }
  | Y_NAME
    {
      c_sname_init( &$$ );
      c_sname_append_name( &$$, $1 );
    }
  ;

sname_c_ast
  : /* type_c_ast */ sname_c
    {
      DUMP_START( "sname_c_ast", "sname_c" );
      DUMP_AST( "(type_c_ast)", type_peek() );
      DUMP_SNAME( "sname", &$1 );

      $$.ast = type_peek();
      $$.target_ast = NULL;
      c_ast_sname_set_sname( $$.ast, &$1 );

      DUMP_AST( "sname_c_ast", $$.ast );
      DUMP_END();
    }
  ;

sname_c_expected
  : sname_c
  | error
    {
      c_sname_init( &$$ );
      ELABORATE_ERROR( "name expected" );
    }
  ;

sname_c_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | sname_c
  ;

sname_english
  : any_sname_c of_scope_list_english_opt
    {
      $$ = $2;
      c_type_id_t sn_type = c_sname_type( &$2 );
      if ( sn_type == T_NONE )
        sn_type = c_sname_type( &$1 );
      c_sname_set_type( &$$, sn_type );
      c_sname_append_sname( &$$, &$1 );
    }
  ;

sname_english_expected
  : sname_english
  | error
    {
      c_sname_init( &$$ );
      ELABORATE_ERROR( "name expected" );
    }
  ;

typedef_sname_c
  : typedef_sname_c "::" sname_c
    {
      //
      // This is for a case like:
      //
      //      define S as struct S
      //      define S::T as struct T
      //
      $$ = $1;
      c_sname_set_type( &$$, c_sname_type( &$3 ) );
      c_sname_append_sname( &$$, &$3 );
    }
  | typedef_sname_c "::" any_typedef
    {
      //
      // This is for a case like:
      //
      //      define S as struct S
      //      define T as struct T
      //      define S::T as struct S_T
      //
      $$ = $1;
      c_sname_set_type( &$$, c_ast_sname_type( $3->ast ) );
      c_sname_t temp = c_ast_sname_dup( $3->ast );
      c_sname_append_sname( &$$, &temp );
    }
  | any_typedef                   { $$ = c_ast_sname_dup( $1->ast ); }
  ;

/*****************************************************************************/
/*  miscellaneous productions                                                */
/*****************************************************************************/

array_expected
  : Y_ARRAY
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_ARRAY );
    }
  ;

as_expected
  : Y_AS
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_AS );
    }
  ;

as_into_to_expected
  : Y_AS
  | Y_INTO
  | Y_TO
  | error
    {
      ELABORATE_ERROR(
        "\"%s\", \"%s\", or \"%s\" expected",
        L_AS, L_INTO, L_TO
      );
    }
  ;

as_opt
  : /* empty */
  | Y_AS
  ;

cast_expected
  : Y_CAST
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_CAST );
    }
  ;

class_struct_type_expected
  : class_struct_type
  | error
    {
      if ( C_LANG_IS_CPP() )
        ELABORATE_ERROR( "\"%s\" or \"%s\" expected", L_CLASS, L_STRUCT );
      else
        ELABORATE_ERROR( "\"%s\" expected", L_STRUCT );
    }
  ;

comma_expected
  : ','
  | error
    {
      ELABORATE_ERROR( "',' expected" );
    }
  ;

conversion_expected
  : Y_CONVERSION
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_CONVERSION );
    }
  ;

c_operator
  : Y_EXCLAM                      { $$ = C_OP_EXCLAM          ; }
  | Y_EXCLAM_EQ                   { $$ = C_OP_EXCLAM_EQ       ; }
  | '%'                           { $$ = C_OP_PERCENT         ; }
  | "%="                          { $$ = C_OP_PERCENT_EQ      ; }
  | Y_AMPER                       { $$ = C_OP_AMPER           ; }
  | Y_AMPER2                      { $$ = C_OP_AMPER2          ; }
  | Y_AMPER_EQ                    { $$ = C_OP_AMPER_EQ        ; }
  | '(' rparen_expected           { $$ = C_OP_PARENS          ; }
  | '*'                           { $$ = C_OP_STAR            ; }
  | "*="                          { $$ = C_OP_STAR_EQ         ; }
  | '+'                           { $$ = C_OP_PLUS            ; }
  | "++"                          { $$ = C_OP_PLUS2           ; }
  | "+="                          { $$ = C_OP_PLUS_EQ         ; }
  | ','                           { $$ = C_OP_COMMA           ; }
  | '-'                           { $$ = C_OP_MINUS           ; }
  | "--"                          { $$ = C_OP_MINUS2          ; }
  | "-="                          { $$ = C_OP_MINUS_EQ        ; }
  | "->"                          { $$ = C_OP_ARROW           ; }
  | "->*"                         { $$ = C_OP_ARROW_STAR      ; }
  | '.'                           { $$ = C_OP_DOT             ; }
  | ".*"                          { $$ = C_OP_DOT_STAR        ; }
  | '/'                           { $$ = C_OP_SLASH           ; }
  | "/="                          { $$ = C_OP_SLASH_EQ        ; }
  | "::"                          { $$ = C_OP_COLON2          ; }
  | '<'                           { $$ = C_OP_LESS            ; }
  | "<<"                          { $$ = C_OP_LESS2           ; }
  | "<<="                         { $$ = C_OP_LESS2_EQ        ; }
  | "<="                          { $$ = C_OP_LESS_EQ         ; }
  | "<=>"                         { $$ = C_OP_LESS_EQ_GREATER ; }
  | '='                           { $$ = C_OP_EQ              ; }
  | "=="                          { $$ = C_OP_EQ2             ; }
  | '>'                           { $$ = C_OP_GREATER         ; }
  | ">>"                          { $$ = C_OP_GREATER2        ; }
  | ">>="                         { $$ = C_OP_GREATER2_EQ     ; }
  | ">="                          { $$ = C_OP_GREATER_EQ      ; }
  | "?:"                          { $$ = C_OP_QMARK_COLON     ; }
  | '[' rbracket_expected         { $$ = C_OP_BRACKETS        ; }
  | Y_CIRC                        { $$ = C_OP_CIRC            ; }
  | Y_CIRC_EQ                     { $$ = C_OP_CIRC_EQ         ; }
  | Y_PIPE                        { $$ = C_OP_PIPE            ; }
  | Y_PIPE2                       { $$ = C_OP_PIPE2           ; }
  | Y_PIPE_EQ                     { $$ = C_OP_PIPE_EQ         ; }
  | Y_TILDE                       { $$ = C_OP_TILDE           ; }
  ;

equals_expected
  : '='
  | error
    {
      ELABORATE_ERROR( "'=' expected" );
    }
  ;

gt_expected
  : '>'
  | error
    {
      ELABORATE_ERROR( "'>' expected" );
    }
  ;

lbrace_expected
  : '{'
  | error
    {
      ELABORATE_ERROR( "'{' expected" );
    }
  ;

literal_expected
  : Y_LITERAL
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_LITERAL );
    }
  ;

lparen_expected
  : '('
  | error
    {
      ELABORATE_ERROR( "'(' expected" );
    }
  ;

lt_expected
  : '<'
  | error
    {
      ELABORATE_ERROR( "'<' expected" );
    }
  ;

member_or_non_member_opt
  : /* empty */                   { $$ = C_FUNC_UNSPECIFIED; }
  | Y_MEMBER                      { $$ = C_FUNC_MEMBER     ; }
  | Y_NON_MEMBER                  { $$ = C_FUNC_NON_MEMBER ; }
  ;

namespace_expected
  : Y_NAMESPACE
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_NAMESPACE );
    }
  ;

namespace_type
  : Y_NAMESPACE
  | Y_INLINE namespace_expected   { $$ = $1 | $2; }
  ;

of_expected
  : Y_OF
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_OF );
    }
  ;

of_scope_english
  : Y_OF scope_english_type_expected any_sname_c_expected
    {
      //
      // Scoped names are supported only in C++.  (However, we always allow
      // them in configuration files.)
      //
      // This check is better to do now in the parser rather than later in the
      // AST because it has to be done in fewer places in the code plus gives a
      // better error location.
      //
      if ( c_init >= C_INIT_READ_CONF && C_LANG_IS_C() ) {
        print_error( &@2, "scoped names not supported in %s", C_LANG_NAME() );
        PARSE_ABORT();
      }
      $$ = $3;
      c_sname_set_type( &$$, $2 );
    }
  ;

of_scope_list_english
  : of_scope_list_english of_scope_english
    {
      //
      // Ensure that neither "namespace" nor "scope" are nested within a
      // class/struct/union.
      //
      c_type_id_t const inner_type = c_sname_type( &$1 );
      c_type_id_t const outer_type = c_sname_type( &$2 );
      if ( (inner_type & (T_NAMESPACE | T_SCOPE)) != T_NONE &&
           (outer_type & T_CLASS_STRUCT_UNION) != T_NONE ) {
        print_error( &@2,
          "\"%s\" may only be nested within a %s or %s",
          c_type_name( inner_type ), L_NAMESPACE, L_SCOPE
        );
        PARSE_ABORT();
      }

      $$ = $2;                          // "of scope X of scope Y" means Y::X
      c_sname_set_type( &$$, inner_type );
      c_sname_append_sname( &$$, &$1 );
    }
  | of_scope_english
  ;

of_scope_list_english_opt
  : /* empty */                   { c_sname_init( &$$ ); }
  | of_scope_list_english
  ;

operator_opt
  : /* empty */
  | Y_OPERATOR
  ;

quote2_expected
  : Y_QUOTE2
  | error
    {
      ELABORATE_ERROR( "\"\" expected" );
    }
  ;

rbrace_expected
  : '}'
  | error
    {
      ELABORATE_ERROR( "'}' expected" );
    }
  ;

rbracket_expected
  : ']'
  | error
    {
      ELABORATE_ERROR( "']' expected" );
    }
  ;

reference_expected
  : Y_REFERENCE
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_REFERENCE );
    }
  ;

returning_expected
  : Y_RETURNING
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_RETURNING );
    }
  ;

rparen_expected
  : ')'
  | error
    {
      ELABORATE_ERROR( "')' expected" );
    }
  ;

scope_english_type
  : class_struct_union_type
  | namespace_type
  | Y_SCOPE                       { $$ = T_SCOPE; }
  ;

scope_english_type_expected
  : scope_english_type
  | error
    {
      ELABORATE_ERROR(
        "\"%s\", \"%s\", \"%s\", \"%s\", or \"%s\" expected",
        L_CLASS, L_NAMESPACE, L_SCOPE, L_STRUCT, L_UNION
      );
    }
  ;

semi_expected
  : ';'
  | error
    {
      ELABORATE_ERROR( "';' expected" );
    }
  ;

semi_opt
  : /* empty */
  | ';'
  ;

semi_or_end
  : ';'
  | Y_END
  ;

to_expected
  : Y_TO
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_TO );
    }
  ;

typedef_opt
  : /* empty */                   { $$ = false; }
  | Y_TYPEDEF                     { $$ = true; }
  ;

virtual_expected
  : Y_VIRTUAL
  | error
    {
      ELABORATE_ERROR( "\"%s\" expected", L_VIRTUAL );
    }
  ;

virtual_opt
  : /* empty */                   { $$ = T_NONE; }
  | Y_VIRTUAL
  ;

%%

/// @endcond

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
