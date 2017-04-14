/*
**      cdecl -- C gibberish translator
**      src/parser.y
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

/**
 * @file
 * Defines helper macros, data structures, variables, functions, and the
 * grammar for C/C++ declarations.
 */

%expect 7

%{
// local
#include "config.h"                     /* must come first */
#include "ast.h"
#include "ast_util.h"
#include "color.h"
#ifdef WITH_CDECL_DEBUG
#include "debug.h"
#endif /* WITH_CDECL_DEBUG */
#include "common.h"
#include "diagnostics.h"
#include "keywords.h"
#include "lang.h"
#include "lexer.h"
#include "literals.h"
#include "options.h"
#include "types.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////

#ifdef WITH_CDECL_DEBUG
#define CDEBUG(...) BLOCK( if ( opt_debug ) { __VA_ARGS__ } )
#else
#define CDEBUG(...)                     /* nothing */
#endif /* WITH_CDECL_DEBUG */

#define C_AST_CHECK(AST,CHECK) \
  BLOCK( if ( !c_ast_check( (AST), (CHECK) ) ) PARSE_ABORT(); )

#define C_AST_NEW(KIND,LOC) \
  c_ast_new( (KIND), ast_depth, (LOC) )

#define C_TYPE_ADD(DST,SRC,LOC) \
  BLOCK( if ( !c_type_add( (DST), (SRC), &(LOC) ) ) PARSE_ABORT(); )

#define DUMP_COMMA \
  CDEBUG( if ( true_or_set( &debug_comma ) ) PUTS_OUT( ",\n" ); )

#define DUMP_AST(KEY,AST) \
  CDEBUG( DUMP_COMMA; c_ast_debug( (AST), 1, (KEY), stdout ); )

#define DUMP_AST_LIST(KEY,AST_LIST) CDEBUG( \
  DUMP_COMMA; printf( "  %s = ", (KEY) );   \
  c_ast_list_debug( &(AST_LIST), 1, stdout ); )

#define DUMP_NAME(KEY,NAME) CDEBUG( \
  DUMP_COMMA; PUTS_OUT( "  " );     \
  print_kv( (KEY), (NAME), stdout ); )

#define DUMP_NUM(KEY,NUM) \
  CDEBUG( DUMP_COMMA; printf( KEY " = %d", (NUM) ); )

#ifdef WITH_CDECL_DEBUG
#define DUMP_START(NAME,PROD) \
  bool debug_comma = false;   \
  CDEBUG( PUTS_OUT( "\n" NAME " ::= " PROD " = {\n" ); )
#else
#define DUMP_START(NAME,PROD)           /* nothing */
#endif

#define DUMP_END()        CDEBUG( PUTS_OUT( "\n}\n" ); )

#define DUMP_TYPE(KEY,TYPE) CDEBUG( \
  DUMP_COMMA; PUTS_OUT( "  " );     \
  print_kv( (KEY), c_type_name( TYPE ), stdout ); )

#define PARSE_ABORT()     BLOCK( parse_cleanup( true ); YYABORT; )
#define PARSE_ERROR(...)  BLOCK( parse_error( __VA_ARGS__ ); PARSE_ABORT(); )
#define SYNTAX_ERROR()    BLOCK( yyerror( "syntax error" ); YYERROR; )

///////////////////////////////////////////////////////////////////////////////

typedef struct qualifier_link qualifier_link_t;

/**
 * A simple \c struct "derived" from \c link that additionally holds a
 * qualifier and its source location.
 */
struct qualifier_link /* : link */ {
  qualifier_link_t *next;               // must be first struct member
  c_type_t          qualifier;          // T_CONST, T_RESTRICT, or T_VOLATILE
  YYLTYPE           loc;
};

/**
 * Inherited attributes.
 */
struct in_attr {
  qualifier_link_t *qualifier_head;     // qualifier stack
  c_ast_t *type_head;                   // type stack
};
typedef struct in_attr in_attr_t;

// extern functions
extern void         print_help( void );
extern void         set_option( char const* );

// local variables
static unsigned     ast_depth;          // parentheses nesting depth
static bool         error_newlined = true;
static in_attr_t    in_attr;

// local functions
static void         qualifier_clear( void );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Gets a printable string of the lexer's current token.
 *
 * @return Returns said string.
 */
static inline char const* printable_token( void ) {
  return lexer_token[0] == '\n' ? "\\n" : lexer_token;
}

/**
 * Peeks at the type AST at the head of the type AST inherited attribute stack.
 *
 * @return Returns said AST.
 */
static inline c_ast_t* type_peek( void ) {
  return in_attr.type_head;
}

/**
 * Pops a type AST from the type AST inherited attribute stack.
 *
 * @return Returns said AST.
 */
static inline c_ast_t* type_pop( void ) {
  return LINK_POP( c_ast_t, &in_attr.type_head );
}

/**
 * Pushes a type AST onto the type AST inherited attribute stack.
 *
 * @param ast The AST to push.
 */
static inline void type_push( c_ast_t *ast ) {
  LINK_PUSH( &in_attr.type_head, ast );
}

/**
 * Peeks at the qualifier at the head of the qualifier inherited attribute
 * stack.
 *
 * @return Returns said qualifier.
 */
static inline c_type_t qualifier_peek( void ) {
  return in_attr.qualifier_head->qualifier;
}

/**
 * Peeks at the location of the qualifier at the head of the qualifier
 * inherited attribute stack.
 *
 * @return Returns said qualifier location.
 * @hideinitializer
 */
#define qualifier_peek_loc() \
  (in_attr.qualifier_head->loc)

/**
 * Pops a qualifier from the head of the qualifier inherited attribute stack
 * and frees it.
 */
static inline void qualifier_pop( void ) {
  FREE( LINK_POP( qualifier_link_t, &in_attr.qualifier_head ) );
}

////////// local functions ////////////////////////////////////////////////////

/**
 * Cleans-up parser data.
 *
 * @param hard_reset If \c true, does a "hard" reset that currently resets the
 * EOF flag of the lexer.  This should be \c true if an error occurs and
 * YYABORT is called.
 */
static void parse_cleanup( bool hard_reset ) {
  c_ast_gc();
  lexer_reset( hard_reset );
  qualifier_clear();
  memset( &in_attr, 0, sizeof in_attr );
}

/**
 * Prints an additional parsing error message to standard error that contines
 * from yyerror() left off.
 *
 * @param format A \c printf() style format string.
 */
static void parse_error( char const *format, ... ) {
  if ( !error_newlined ) {
    PUTS_ERR( ": " );
    if ( lexer_token[0] )
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
 * Clears the qualifier stack.
 */
static void qualifier_clear( void ) {
  qualifier_link_t *q;
  while ( (q = LINK_POP( qualifier_link_t, &in_attr.qualifier_head )) )
    FREE( q );
}

/**
 * Pushed a quaifier onto the front of the qualifier inherited attribute list.
 *
 * @param qualifier The qualifier to push.
 * @param loc A pointer to the source location of the qualifier.
 */
static void qualifier_push( c_type_t qualifier, YYLTYPE const *loc ) {
  assert( (qualifier & ~T_MASK_QUALIFIER) == 0 );
  assert( loc != NULL );
  qualifier_link_t *const q = MALLOC( qualifier_link_t, 1 );
  q->qualifier = qualifier;
  q->loc = *loc;
  q->next = NULL;
  LINK_PUSH( &in_attr.qualifier_head, q );
}

/**
 * Implements the cdecl "quit" command.
 */
static void quit( void ) {
  exit( EX_OK );
}

/**
 * Prints a parsing error message to standard error.  This function is called
 * directly by bison to print just "syntax error" (usually).
 *
 * @note A newline is \e not printed since the error message will be added to
 * by parse_error().  For example, the parts of an error message are printed by
 * the functions shown:
 *
 *      42: syntax error: "int": "into" expected
 *      |--||----------||----------------------|
 *      |   |           |
 *      |   yyerror()   parse_error()
 *      |
 *      print_loc()
 *
 * @param msg The error message to print.
 */
static void yyerror( char const *msg ) {
  YYLTYPE loc;
  memset( &loc, 0, sizeof loc );
  loc.first_column = lexer_column();
  print_loc( &loc );

  SGR_START_COLOR( stderr, error );
  PUTS_ERR( msg );                      // no newline
  SGR_END_COLOR( stderr );
  error_newlined = false;

  parse_cleanup( false );
}

///////////////////////////////////////////////////////////////////////////////
%}

%union {
  c_ast_list_t  ast_list; /* for function arguments */
  c_ast_pair_t  ast_pair; /* for the AST being built */
  char const   *name;     /* name being declared or explained */
  int           number;   /* for array sizes */
  c_type_t      type;     /* built-in types, storage classes, & qualifiers */
}

                    /* commands */
%token              Y_CAST
%token              Y_DECLARE
%token              Y_EXPLAIN
%token              Y_HELP
%token              Y_SET
%token              Y_QUIT

                    /* english */
%token              Y_ARRAY
%token              Y_AS
%token              Y_BLOCK             /* Apple: English for '^' */
%token              Y_FUNCTION
%token              Y_INTO
%token              Y_MEMBER
%token              Y_OF
%token              Y_POINTER
%token              Y_PURE
%token              Y_REFERENCE
%token              Y_RETURNING
%token              Y_RVALUE
%token              Y_TO

                    /* K&R C */
%token              ','
%token              '*'
%token              '[' ']'
%token              '(' ')'
%token  <type>      Y_AUTO
%token  <type>      Y_CHAR
%token  <type>      Y_DOUBLE
%token  <type>      Y_EXTERN
%token  <type>      Y_FLOAT
%token  <type>      Y_INT
%token  <type>      Y_LONG
%token  <type>      Y_REGISTER
%token  <type>      Y_SHORT
%token  <type>      Y_STATIC
%token  <type>      Y_STRUCT
%token  <type>      Y_TYPEDEF
%token  <type>      Y_UNION
%token  <type>      Y_UNSIGNED

                    /* C89 */
%token  <type>      Y_CONST
%token              Y_ELLIPSIS          "..."
%token  <type>      Y_ENUM
%token  <type>      Y_SIGNED
%token  <type>      Y_SIZE_T
%token  <type>      Y_VOID
%token  <type>      Y_VOLATILE

                    /* C99 */
%token  <type>      Y_BOOL
%token  <type>      Y_COMPLEX
%token  <type>      Y_RESTRICT
%token  <type>      Y_WCHAR_T

                    /* C11 */
%token              Y_NORETURN

                    /* C++ */
%token              '&'                 /* reference */
%token              '='                 /* used for pure virtual: = 0 */
%token  <type>      Y_CLASS
%token              Y_COLON_COLON       "::"
%token  <type>      Y_VIRTUAL

                    /* C++11 */
%token              Y_RVALUE_REFERENCE  "&&"

                    /* C11 & C++11 */
%token  <type>      Y_CHAR16_T
%token  <type>      Y_CHAR32_T
%token  <type>      Y_THREAD_LOCAL

                    /* miscellaneous */
%token              '^'                 /* Apple: block indicator */
%token  <type>      Y___BLOCK           /* Apple: block storage class */
%token  <name>      Y_CPP_LANG_NAME
%token              Y_END
%token              Y_ERROR
%token  <name>      Y_NAME
%token  <number>    Y_NUMBER

%type   <ast_pair>  decl_english
%type   <name>      declare_name_as_english
%type   <ast_list>  decl_list_english decl_list_opt_english
%type   <ast_list>  paren_decl_list_opt_english
%type   <ast_pair>  array_decl_english
%type   <number>    array_size_opt_english
%type   <ast_pair>  block_decl_english
%type   <ast_pair>  func_decl_english
%type   <name>      name_into_english
%type   <ast_pair>  pointer_decl_english
%type   <ast_pair>  pointer_to_member_decl_english
%type   <type>      pure_opt_english
%type   <ast_pair>  qualifiable_decl_english
%type   <ast_pair>  qualified_decl_english
%type   <ast_pair>  reference_decl_english
%type   <ast_pair>  reference_english
%type   <ast_pair>  returning_english
%type   <type>      storage_class_opt_english
%type   <ast_pair>  type_english
%type   <type>      type_modifier_english
%type   <type>      type_modifier_list_english
%type   <type>      type_modifier_list_opt_english
%type   <ast_pair>  unmodified_type_english
%type   <ast_pair>  var_decl_english

%type   <ast_pair>  cast_c cast2_c
%type   <ast_pair>  array_cast_c
%type   <ast_pair>  block_cast_c
%type   <ast_pair>  func_cast_c
%type   <ast_pair>  nested_cast_c
%type   <ast_pair>  pointer_cast_c
%type   <ast_pair>  pointer_to_member_cast_c
%type   <type>      pure_virtual_opt_c
%type   <ast_pair>  reference_cast_c

%type   <ast_pair>  decl_c decl2_c
%type   <ast_pair>  array_decl_c
%type   <ast_pair>  block_decl_c
%type   <ast_pair>  builtin_or_enum_class_struct_union_type_c
%type   <ast_pair>  func_decl_c
%type   <ast_pair>  name_c
%type   <ast_pair>  nested_decl_c
%type   <ast_pair>  pointer_decl_c
%type   <ast_pair>  pointer_type_c
%type   <ast_pair>  pointer_to_member_decl_c
%type   <ast_pair>  pointer_to_member_type_c
%type   <ast_pair>  reference_decl_c
%type   <ast_pair>  reference_type_c

%type   <ast_pair>  type_c
%type   <ast_pair>  placeholder_type_c
%type   <type>      builtin_type_c
%type   <type>      class_struct_type_c class_struct_type_expected_c
%type   <type>      cv_qualifier_c
%type   <type>      enum_class_struct_union_type_c
%type   <type>      storage_class_c
%type   <type>      type_modifier_c
%type   <type>      type_modifier_list_c type_modifier_list_opt_c
%type   <type>      type_qualifier_c
%type   <type>      type_qualifier_list_opt_c

%type   <ast_pair>  arg_c
%type   <ast_list>  arg_list_c arg_list_opt_c
%type   <number>    array_size_c
%type   <name>      name_expected
%type   <name>      name_opt
%type   <name>      set_option

/*****************************************************************************/
%%

command_list
  : /* empty */
  | command_list
    {
      if ( !error_newlined ) {
        FPUTC( '\n', fout );
        error_newlined = true;
      }
    }
    command
    {
      //
      // We get here only after a successful parse, so a hard reset is not
      // needed.
      //
      parse_cleanup( false );
    }
  ;

command
  : cast_english
  | declare_english
  | explain_declaration_c
  | explain_cast_c
  | help_command
  | set_command
  | quit_command
  | Y_END                               /* allows for blank lines */
  | error Y_END
    {
      if ( lexer_token[0] )
        PARSE_ERROR( "unexpected token" );
      else
        PARSE_ERROR( "unexpected end of command" );
    }
  ;

/*****************************************************************************/
/*  cast                                                                     */
/*****************************************************************************/

cast_english
  : Y_CAST name_into_english decl_english Y_END
    {
      DUMP_START( "cast_english", "CAST NAME INTO decl_english" );
      DUMP_NAME( "NAME", $2 );
      DUMP_AST( "decl_english", $3.ast );
      DUMP_END();

      C_AST_CHECK( $3.ast, CHECK_CAST );
      FPUTC( '(', fout );
      c_ast_gibberish_cast( $3.ast, fout );
      FPRINTF( fout, ")%s\n", $2 );
      FREE( $2 );
    }

  | Y_CAST decl_english Y_END
    {
      DUMP_START( "cast_english", "CAST decl_english" );
      DUMP_AST( "decl_english", $2.ast );
      DUMP_END();

      C_AST_CHECK( $2.ast, CHECK_CAST );
      FPUTC( '(', fout );
      c_ast_gibberish_cast( $2.ast, fout );
      FPUTS( ")\n", fout );
    }
  ;

name_into_english
  : Y_NAME Y_INTO
  | Y_NAME error
    {
      PARSE_ERROR( "\"%s\" expected", L_INTO );
    }
  ;

/*****************************************************************************/
/*  declare                                                                  */
/*****************************************************************************/

declare_english
  : declare_name_as_english storage_class_opt_english decl_english Y_END
    {
      C_TYPE_ADD( &$3.ast->type, $2, @2 );

      DUMP_START( "declare_english",
                  "DECLARE NAME AS storage_class_opt_english decl_english" );
      $3.ast->name = $1;
      DUMP_NAME( "NAME", $1 );
      DUMP_TYPE( "storage_class_opt_english", $2 );
      DUMP_AST( "decl_english", $3.ast );
      DUMP_END();

      C_AST_CHECK( $3.ast, CHECK_DECL );
      c_ast_gibberish_declare( $3.ast, fout );
      if ( opt_semicolon )
        FPUTC( ';', fout );
      FPUTC( '\n', fout );
    }
  ;

declare_name_as_english
  : Y_DECLARE name_expected Y_AS  { $$ = $2; }

  | Y_DECLARE Y_NAME error
    {
      PARSE_ERROR( "\"%s\" expected", L_AS );
    }
  ;

storage_class_opt_english
  : /* empty */                   { $$ = T_NONE; }
  | Y_AUTO
  | Y___BLOCK                           /* Apple extension */
  | Y_EXTERN
  | Y_REGISTER
  | Y_STATIC
  | Y_THREAD_LOCAL
  | Y_TYPEDEF
  | pure_opt_english Y_VIRTUAL    { $$ = $1 | $2; }
  ;

pure_opt_english
  : /* empty */                   { $$ = T_NONE; }
  | Y_PURE                        { $$ = T_PURE_VIRTUAL; }
  ;

/*****************************************************************************/
/*  explain                                                                  */
/*****************************************************************************/

explain_declaration_c
  : explain_c type_c { type_push( $2.ast ); } decl_c Y_END
    {
      type_pop();

      DUMP_START( "explain_declaration_c", "EXPLAIN type_c decl_c" );
      DUMP_AST( "type_c", $2.ast );
      DUMP_AST( "decl_c", $4.ast );
      DUMP_END();

      c_ast_t *const ast = c_ast_patch_none( $2.ast, $4.ast );
      C_AST_CHECK( ast, CHECK_DECL );
      char const *const name = c_ast_take_name( ast );
      assert( name != NULL );
      FPRINTF( fout, "%s %s %s ", L_DECLARE, name, L_AS );
      if ( c_ast_take_typedef( ast ) )
        FPRINTF( fout, "%s ", L_TYPE );
      c_ast_english( ast, fout );
      FPUTC( '\n', fout );
      FREE( name );
    }
  ;

explain_cast_c
  : explain_c '(' type_c { type_push( $3.ast ); } cast_c ')' name_opt Y_END
    {
      type_pop();

      DUMP_START( "explain_cast_t",
                  "EXPLAIN '(' type_c cast_c ')' name_opt" );
      DUMP_AST( "type_c", $3.ast );
      DUMP_AST( "cast_c", $5.ast );
      DUMP_NAME( "name_opt", $7 );
      DUMP_END();

      c_ast_t *const ast = c_ast_patch_none( $3.ast, $5.ast );
      C_AST_CHECK( ast, CHECK_CAST );
      FPUTS( L_CAST, fout );
      if ( $7 ) {
        FPRINTF( fout, " %s", $7 );
        FREE( $7 );
      }
      FPRINTF( fout, " %s ", L_INTO );
      c_ast_english( ast, fout );
      FPUTC( '\n', fout );
    }
  ;

explain_c
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
      explaining = true;
    }
  ;

/*****************************************************************************/
/*  help                                                                     */
/*****************************************************************************/

help_command
  : Y_HELP Y_END                  { print_help(); }
  ;

/*****************************************************************************/
/*  set                                                                      */
/*****************************************************************************/

set_command
  : Y_SET set_option Y_END        { set_option( $2 ); FREE( $2 ); }
  ;

set_option
  : name_opt
  | Y_CPP_LANG_NAME
  ;

/*****************************************************************************/
/*  quit                                                                     */
/*****************************************************************************/

quit_command
  : Y_QUIT Y_END                  { quit(); }
  ;

/*****************************************************************************/
/*  declaration english productions                                          */
/*****************************************************************************/

decl_english
  : array_decl_english
  | func_decl_english
  | qualified_decl_english
  | var_decl_english
  ;

array_decl_english
  : Y_ARRAY array_size_opt_english Y_OF decl_english
    {
      DUMP_START( "array_decl_english",
                  "ARRAY array_size_opt_english OF decl_english" );
      DUMP_NUM( "array_size_opt_english", $2 );
      DUMP_AST( "decl_english", $4.ast );

      $$.ast = C_AST_NEW( K_ARRAY, &@$ );
      $$.target_ast = NULL;
      $$.ast->as.array.size = $2;
      c_ast_set_parent( $4.ast, $$.ast );

      DUMP_AST( "array_decl_english", $$.ast );
      DUMP_END();
    }
  ;

array_size_opt_english
  : /* empty */                   { $$ = C_ARRAY_NO_SIZE; }
  | Y_NUMBER
  | error                         { PARSE_ERROR( "array size expected" ); }
  ;

block_decl_english                      /* Apple extension */
  : Y_BLOCK paren_decl_list_opt_english returning_english
    {
      DUMP_START( "block_decl_english",
                  "BLOCK paren_decl_list_opt_english returning_english" );
      DUMP_TYPE( "qualifier", qualifier_peek() );
      DUMP_AST_LIST( "paren_decl_list_opt_english", $2 );
      DUMP_AST( "returning_english", $3.ast );

      $$.ast = C_AST_NEW( K_BLOCK, &@$ );
      $$.target_ast = NULL;
      $$.ast->type = qualifier_peek();
      c_ast_set_parent( $3.ast, $$.ast );
      $$.ast->as.block.args = $2;

      DUMP_AST( "block_decl_english", $$.ast );
      DUMP_END();
    }
  ;

func_decl_english
  : Y_FUNCTION paren_decl_list_opt_english returning_english
    {
      DUMP_START( "func_decl_english",
                  "FUNCTION paren_decl_list_opt_english returning_english" );
      DUMP_AST_LIST( "decl_list_opt_english", $2 );
      DUMP_AST( "returning_english", $3.ast );

      $$.ast = C_AST_NEW( K_FUNCTION, &@$ );
      $$.target_ast = NULL;
      c_ast_set_parent( $3.ast, $$.ast );
      $$.ast->as.func.args = $2;

      DUMP_AST( "func_decl_english", $$.ast );
      DUMP_END();
    }
  ;

paren_decl_list_opt_english
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | '(' decl_list_opt_english ')'
    {
      DUMP_START( "paren_decl_list_opt_english",
                  "'(' decl_list_opt_english ')'" );
      DUMP_AST_LIST( "decl_list_opt_english", $2 );

      $$ = $2;

      DUMP_AST_LIST( "paren_decl_list_opt_english", $$ );
      DUMP_END();
    }
  ;

decl_list_opt_english
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | decl_list_english
  ;

decl_list_english
  : decl_english                  { $$.head_ast = $$.tail_ast = $1.ast; }
  | decl_list_english comma_expected decl_english
    {
      DUMP_START( "decl_list_opt_english",
                  "decl_list_opt_english ',' decl_english" );
      DUMP_AST_LIST( "decl_list_opt_english", $1 );
      DUMP_AST( "decl_english", $3.ast );

      $$ = $1;
      c_ast_list_append( &$$, $3.ast );

      DUMP_AST_LIST( "decl_list_opt_english", $$ );
      DUMP_END();
    }
  ;

returning_english
  : returning_expected decl_english
    {
      DUMP_START( "returning_english", "RETURNING decl_english" );
      DUMP_AST( "decl_english", $2.ast );

      $$ = $2;

      DUMP_AST( "returning_english", $$.ast );
      DUMP_END();
    }

  | Y_RETURNING error
    {
      PARSE_ERROR( "English expected after \"%s\"", L_RETURNING );
    }
  ;

qualified_decl_english
  : type_qualifier_list_opt_c { qualifier_push( $1, &@1 ); }
    qualifiable_decl_english
    {
      qualifier_pop();
      DUMP_START( "qualified_decl_english",
                  "type_qualifier_list_opt_c qualifiable_decl_english" );
      DUMP_TYPE( "type_qualifier_list_opt_c", $1 );
      DUMP_AST( "qualifiable_decl_english", $3.ast );

      $$ = $3;

      DUMP_AST( "qualified_decl_english", $$.ast );
      DUMP_END();
    }
  ;

qualifiable_decl_english
  : block_decl_english
  | pointer_decl_english
  | pointer_to_member_decl_english
  | reference_decl_english
  | type_english
  ;

pointer_decl_english
  : Y_POINTER to_expected decl_english
    {
      DUMP_START( "pointer_decl_english", "POINTER TO decl_english" );
      DUMP_TYPE( "qualifier", qualifier_peek() );
      DUMP_AST( "decl_english", $3.ast );

      $$.ast = C_AST_NEW( K_POINTER, &@$ );
      $$.target_ast = NULL;
      c_ast_set_parent( $3.ast, $$.ast );
      $$.ast->as.ptr_ref.qualifier = qualifier_peek();

      DUMP_AST( "pointer_decl_english", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_decl_english
  : Y_POINTER to_expected member_expected of_expected
    class_struct_type_expected_c name_expected decl_english
    {
      DUMP_START( "pointer_to_member_decl_english",
                  "POINTER TO MEMBER OF "
                  "class_struct_type_c NAME decl_english" );
      DUMP_TYPE( "qualifier", qualifier_peek() );
      DUMP_TYPE( "class_struct_type_c", $5 );
      DUMP_NAME( "NAME", $6 );
      DUMP_AST( "decl_english", $7.ast );

      $$.ast = C_AST_NEW( K_POINTER_TO_MEMBER, &@$ );
      $$.target_ast = NULL;
      $$.ast->type = $5;
      c_ast_set_parent( $7.ast, $$.ast );
      $$.ast->as.ptr_ref.qualifier = qualifier_peek();
      $$.ast->as.ptr_mbr.class_name = $6;

      DUMP_AST( "pointer_to_member_decl_english", $$.ast );
      DUMP_END();
    }
  ;

reference_decl_english
  : reference_english to_expected decl_english
    {
      DUMP_START( "reference_decl_english",
                  "reference_english TO decl_english" );
      DUMP_TYPE( "qualifier", qualifier_peek() );
      DUMP_AST( "decl_english", $3.ast );

      $$ = $1;
      c_ast_set_parent( $3.ast, $$.ast );
      $$.ast->as.ptr_ref.qualifier = qualifier_peek();

      DUMP_AST( "reference_decl_english", $$.ast );
      DUMP_END();
    }
  ;

reference_english
  : Y_REFERENCE
    {
      $$.ast = C_AST_NEW( K_REFERENCE, &@$ );
      $$.target_ast = NULL;
    }

  | Y_RVALUE reference_expected
    {
      $$.ast = C_AST_NEW( K_RVALUE_REFERENCE, &@$ );
      $$.target_ast = NULL;
    }
  ;

reference_expected
  : Y_REFERENCE
  | error
    {
      PARSE_ERROR( "\"%s\" expected", L_REFERENCE );
    }
  ;

var_decl_english
  : Y_NAME Y_AS decl_english
    {
      DUMP_START( "var_decl_english", "NAME AS decl_english" );
      DUMP_NAME( "NAME", $1 );
      DUMP_AST( "decl_english", $3.ast );

      $$ = $3;
      assert( $$.ast->name == NULL );
      $$.ast->name = $1;

      DUMP_AST( "var_decl_english", $$.ast );
      DUMP_END();
    }

  | Y_NAME                              /* K&R C type-less variable */
    {
      DUMP_START( "var_decl_english", "NAME" );
      DUMP_NAME( "NAME", $1 );

      $$.ast = C_AST_NEW( K_NAME, &@$ );
      $$.target_ast = NULL;
      $$.ast->name = $1;

      DUMP_AST( "var_decl_english", $$.ast );
      DUMP_END();
    }

  | "..."
    {
      DUMP_START( "var_decl_english", "..." );

      $$.ast = C_AST_NEW( K_VARIADIC, &@$ );
      $$.target_ast = NULL;

      DUMP_AST( "var_decl_english", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  type english productions                                                 */
/*****************************************************************************/

type_english
  : type_modifier_list_opt_english unmodified_type_english
    {
      DUMP_START( "type_english",
                  "type_modifier_list_opt_english unmodified_type_english" );
      DUMP_TYPE( "type_modifier_list_opt_english", $1 );
      DUMP_AST( "unmodified_type_english", $2.ast );
      DUMP_TYPE( "qualifier", qualifier_peek() );

      $$ = $2;
      C_TYPE_ADD( &$$.ast->type, qualifier_peek(), qualifier_peek_loc() );
      C_TYPE_ADD( &$$.ast->type, $1, @1 );

      DUMP_AST( "type_english", $$.ast );
      DUMP_END();
    }

  | type_modifier_list_english          /* allows for default int type */
    {
      DUMP_START( "type_english", "type_modifier_list_english" );
      DUMP_TYPE( "type_modifier_list_english", $1 );
      DUMP_TYPE( "qualifier", qualifier_peek() );

      $$.ast = C_AST_NEW( K_BUILTIN, &@$ );
      $$.target_ast = NULL;
      $$.ast->type = T_INT;
      C_TYPE_ADD( &$$.ast->type, qualifier_peek(), qualifier_peek_loc() );
      C_TYPE_ADD( &$$.ast->type, $1, @1 );

      DUMP_AST( "type_english", $$.ast );
      DUMP_END();
    }
  ;

type_modifier_list_opt_english
  : /* empty */                   { $$ = T_NONE; }
  | type_modifier_list_english
  ;

type_modifier_list_english
  : type_modifier_list_opt_english type_modifier_english
    {
      DUMP_START( "type_modifier_list_opt_english",
                  "type_modifier_list_opt_english type_modifier_english" );
      DUMP_TYPE( "type_modifier_list_opt_english", $1 );
      DUMP_TYPE( "type_modifier_english", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "type_modifier_list_opt_english", $$ );
      DUMP_END();
    }

  | type_modifier_english
  ;

type_modifier_english
  : Y_COMPLEX
  | Y_LONG
  | Y_SHORT
  | Y_SIGNED
  | Y_UNSIGNED
  /*
   * Register is here (rather than in storage_class_c) because it's the only
   * storage class that can be specified for function arguments.  Therefore,
   * it's simpler to treat it as any other type modifier.
   */
  | Y_REGISTER
  ;

unmodified_type_english
  : builtin_or_enum_class_struct_union_type_c
  ;

/*****************************************************************************/
/*  declaration gibberish productions                                        */
/*****************************************************************************/

decl_c
  : decl2_c
  | pointer_decl_c
  | pointer_to_member_decl_c
  | reference_decl_c
  ;

decl2_c
  : array_decl_c
  | block_decl_c
  | func_decl_c
  | name_c
  | nested_decl_c
  ;

array_decl_c
  : decl2_c array_size_c
    {
      DUMP_START( "array_decl_c", "decl2_c array_size_c" );
      DUMP_AST( "type_c", type_peek() );
      DUMP_AST( "decl2_c", $1.ast );
      if ( $1.target_ast )
        DUMP_AST( "target_ast", $1.target_ast );
      DUMP_NUM( "array_size_c", $2 );

      c_ast_t *const array = C_AST_NEW( K_ARRAY, &@$ );
      array->as.array.size = $2;
      c_ast_set_parent( C_AST_NEW( K_NONE, &@1 ), array );
      if ( $1.target_ast ) {            // array-of or function/block-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, array );
      } else {
        $$.ast = c_ast_add_array( $1.ast, array );
        $$.target_ast = NULL;
      }

      DUMP_AST( "array_decl_c", $$.ast );
      DUMP_END();
    }
  ;

array_size_c
  : '[' ']'                       { $$ = C_ARRAY_NO_SIZE; }
  | '[' Y_NUMBER ']'              { $$ = $2; }
  | '[' error ']'
    {
      PARSE_ERROR( "integer expected for array size" );
    }
  ;

block_decl_c                            /* Apple extension */
  : /* type */ '(' '^'
    {
      //
      // A block AST has to be the type inherited attribute for decl_c so we
      // have to create it here.
      //
      type_push( C_AST_NEW( K_BLOCK, &@$ ) );
    }
    type_qualifier_list_opt_c decl_c ')' '(' arg_list_opt_c ')'
    {
      c_ast_t *const block = type_pop();

      DUMP_START( "block_decl_c",
                  "'(' '^' type_qualifier_list_opt_c decl_c ')' "
                  "'(' arg_list_opt_c ')'" );
      DUMP_AST( "type_c", type_peek() );
      DUMP_TYPE( "type_qualifier_list_opt_c", $4 );
      DUMP_AST( "decl_c", $5.ast );
      DUMP_AST_LIST( "arg_list_opt_c", $8 );

      C_TYPE_ADD( &block->type, $4, @4 );
      block->as.block.args = $8;
      $$.ast = c_ast_add_func( $5.ast, type_peek(), block );
      $$.target_ast = block->as.block.ret_ast;

      DUMP_AST( "block_decl_c", $$.ast );
      DUMP_END();
    }
  ;

func_decl_c
  : /* type_c */ decl2_c '(' arg_list_opt_c ')' pure_virtual_opt_c
    {
      DUMP_START( "func_decl_c", "decl2_c '(' arg_list_opt_c ')'" );
      DUMP_AST( "type_c", type_peek() );
      DUMP_AST( "decl2_c", $1.ast );
      DUMP_AST_LIST( "arg_list_opt_c", $3 );
      DUMP_TYPE( "pure_virtual_opt_c", $5 );
      if ( $1.target_ast )
        DUMP_AST( "target_ast", $1.target_ast );

      c_ast_t *const func = C_AST_NEW( K_FUNCTION, &@$ );
      func->type = $5;
      func->as.func.args = $3;
      if ( $1.target_ast ) {
        $$.ast = $1.ast;
        (void)c_ast_add_func( $1.target_ast, type_peek(), func );
      } else {
        $$.ast = c_ast_add_func( $1.ast, type_peek(), func );
      }
      $$.target_ast = func->as.func.ret_ast;

      DUMP_AST( "func_decl_c", $$.ast );
      DUMP_END();
    }
  ;

pure_virtual_opt_c
  : /* empty */                   { $$ = T_NONE; }
  | '=' zero_expected             { $$ = T_PURE_VIRTUAL; }
  ;

name_c
  : /* type_c */ Y_NAME
    {
      DUMP_START( "name_c", "NAME" );
      DUMP_AST( "type_c", type_peek() );
      DUMP_NAME( "NAME", $1 );

      $$.ast = type_peek();
      $$.target_ast = NULL;
      assert( $$.ast->name == NULL );
      $$.ast->name = $1;

      DUMP_AST( "name_c", $$.ast );
      DUMP_END();
    }
  ;

nested_decl_c
  : '(' placeholder_type_c { type_push( $2.ast ); ++ast_depth; } decl_c ')'
    {
      type_pop();
      --ast_depth;

      DUMP_START( "nested_decl_c", "'(' placeholder_type_c decl_c ')'" );
      DUMP_AST( "placeholder_type_c", $2.ast );
      DUMP_AST( "decl_c", $4.ast );

      $$ = $4;

      DUMP_AST( "nested_decl_c", $$.ast );
      DUMP_END();
    }
  ;

placeholder_type_c
  : /* empty */
    {
      $$.ast = C_AST_NEW( K_NONE, &@$ );
      $$.target_ast = NULL;
    }
  ;

pointer_decl_c
  : pointer_type_c { type_push( $1.ast ); } decl_c
    {
      type_pop();
      DUMP_START( "pointer_decl_c", "pointer_type_c decl_c" );
      DUMP_AST( "pointer_type_c", $1.ast );
      DUMP_AST( "decl_c", $3.ast );

      (void)c_ast_patch_none( $1.ast, $3.ast );
      $$ = $3;

      DUMP_AST( "pointer_decl_c", $$.ast );
      DUMP_END();
    }
  ;

pointer_type_c
  : /* type_c */ '*' type_qualifier_list_opt_c
    {
      DUMP_START( "pointer_type_c", "* type_qualifier_list_opt_c" );
      DUMP_AST( "type_c", type_peek() );
      DUMP_TYPE( "type_qualifier_list_opt_c", $2 );

      $$.ast = C_AST_NEW( K_POINTER, &@$ );
      $$.target_ast = NULL;
      $$.ast->as.ptr_ref.qualifier = $2;
      c_ast_set_parent( type_peek(), $$.ast );

      DUMP_AST( "pointer_type_c", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_decl_c
  : pointer_to_member_type_c { type_push( $1.ast ); } decl_c
    {
      type_pop();
      DUMP_START( "pointer_to_member_decl_c",
                  "pointer_to_member_type_c decl_c" );
      DUMP_AST( "pointer_to_member_type_c", $1.ast );
      DUMP_AST( "decl_c", $3.ast );

      $$ = $3;

      DUMP_AST( "pointer_to_member_decl_c", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_type_c
  : /* type_c */ Y_NAME "::" star_expected
    {
      DUMP_START( "pointer_to_member_type_c", "NAME :: *" );
      DUMP_AST( "type_c", type_peek() );
      DUMP_NAME( "NAME", $1 );

      $$.ast = C_AST_NEW( K_POINTER_TO_MEMBER, &@$ );
      $$.target_ast = NULL;
      $$.ast->type = T_CLASS;
      $$.ast->as.ptr_mbr.class_name = $1;
      c_ast_set_parent( type_peek(), $$.ast );

      DUMP_AST( "pointer_to_member_type_c", $$.ast );
      DUMP_END();
    }
  ;

reference_decl_c
  : reference_type_c { type_push( $1.ast ); } decl_c
    {
      type_pop();
      DUMP_START( "reference_decl_c", "reference_type_c decl_c" );
      DUMP_AST( "reference_type_c", $1.ast );
      DUMP_AST( "decl_c", $3.ast );

      $$ = $3;

      DUMP_AST( "reference_decl_c", $$.ast );
      DUMP_END();
    }
  ;

reference_type_c
  : /* type_c */ '&'
    {
      DUMP_START( "reference_type_c", "&" );
      DUMP_AST( "type_c", type_peek() );

      $$.ast = C_AST_NEW( K_REFERENCE, &@$ );
      $$.target_ast = NULL;
      c_ast_set_parent( type_peek(), $$.ast );

      DUMP_AST( "reference_type_c", $$.ast );
      DUMP_END();
    }

  | /* type_c */ "&&"
    {
      DUMP_START( "reference_type_c", "&&" );
      DUMP_AST( "type_c", type_peek() );

      $$.ast = C_AST_NEW( K_RVALUE_REFERENCE, &@$ );
      $$.target_ast = NULL;
      c_ast_set_parent( type_peek(), $$.ast );

      DUMP_AST( "reference_type_c", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  function argument gibberish productions                                  */
/*****************************************************************************/

arg_list_opt_c
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | arg_list_c
  ;

arg_list_c
  : arg_list_c comma_expected arg_c
    {
      DUMP_START( "arg_list_c", "arg_list_c ',' cast_c" );
      DUMP_AST_LIST( "arg_list_c", $1 );
      DUMP_AST( "cast_c", $3.ast );

      $$ = $1;
      c_ast_list_append( &$$, $3.ast );

      DUMP_AST_LIST( "arg_list_c", $$ );
      DUMP_END();
    }

  | arg_c
    {
      DUMP_START( "arg_list_c", "arg_c" );
      DUMP_AST( "arg_c", $1.ast );

      $$.head_ast = $$.tail_ast = NULL;
      c_ast_list_append( &$$, $1.ast );

      DUMP_AST_LIST( "arg_list_c", $$ );
      DUMP_END();
    }
  ;

arg_c
  : type_c { type_push( $1.ast ); } cast_c
    {
      type_pop();
      DUMP_START( "arg_c", "type_c cast_c" );
      DUMP_AST( "type_c", $1.ast );
      DUMP_AST( "cast_c", $3.ast );

      $$ = $3.ast ? $3 : $1;
      if ( $$.ast->name == NULL )
        $$.ast->name = check_strdup( c_ast_name( $$.ast, V_DOWN ) );

      DUMP_AST( "arg_c", $$.ast );
      DUMP_END();
    }

  | Y_NAME                              /* K&R C type-less argument */
    {
      DUMP_START( "argc", "NAME" );
      DUMP_NAME( "NAME", $1 );

      $$.ast = C_AST_NEW( K_NAME, &@$ );
      $$.target_ast = NULL;
      $$.ast->name = $1;

      DUMP_AST( "arg_c", $$.ast );
      DUMP_END();
    }

  | "..."
    {
      DUMP_START( "argc", "..." );

      $$.ast = C_AST_NEW( K_VARIADIC, &@$ );
      $$.target_ast = NULL;

      DUMP_AST( "arg_c", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  type gibberish productions                                               */
/*****************************************************************************/

type_c
  : type_modifier_list_c                /* allows for default int type */
    {
      DUMP_START( "type_c", "type_modifier_list_c" );
      DUMP_TYPE( "type_modifier_list_c", $1 );

      $$.ast = C_AST_NEW( K_BUILTIN, &@$ );
      $$.target_ast = NULL;
      $$.ast->type = T_INT;
      C_TYPE_ADD( &$$.ast->type, $1, @1 );

      DUMP_AST( "type_c", $$.ast );
      DUMP_END();
    }

  | type_modifier_list_c builtin_or_enum_class_struct_union_type_c
    type_modifier_list_opt_c
    {
      DUMP_START( "type_c",
                  "type_modifier_list_c builtin_type_c "
                  "type_modifier_list_opt_c" );
      DUMP_TYPE( "type_modifier_list_c", $1 );
      DUMP_AST( "builtin_or_enum_class_struct_union_type_c", $2.ast );
      DUMP_TYPE( "type_modifier_list_opt_c", $3 );

      $$ = $2;
      C_TYPE_ADD( &$$.ast->type, $1, @1 );
      C_TYPE_ADD( &$$.ast->type, $3, @3 );

      DUMP_AST( "type_c", $$.ast );
      DUMP_END();
    }

  | builtin_or_enum_class_struct_union_type_c type_modifier_list_opt_c
    {
      DUMP_START( "type_c", "builtin_type_c type_modifier_list_opt_c" );
      DUMP_AST( "builtin_or_enum_class_struct_union_type_c", $1.ast );
      DUMP_TYPE( "type_modifier_list_opt_c", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$.ast->type, $2, @2 );

      DUMP_AST( "type_c", $$.ast );
      DUMP_END();
    }
  ;

type_modifier_list_opt_c
  : /* empty */                   { $$ = T_NONE; }
  | type_modifier_list_c
  ;

type_modifier_list_c
  : type_modifier_list_c type_modifier_c
    {
      DUMP_START( "type_modifier_list_c",
                  "type_modifier_list_c type_modifier_c" );
      DUMP_TYPE( "type_modifier_list_c", $1 );
      DUMP_TYPE( "type_modifier_c", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "type_modifier_list_c", $$ );
      DUMP_END();
    }

  | type_modifier_c
  ;

type_modifier_c
  : type_modifier_english
  | type_qualifier_c
  | storage_class_c
  ;

builtin_or_enum_class_struct_union_type_c
  : builtin_type_c
    {
      DUMP_START( "builtin_or_enum_class_struct_union_type_c",
                  "builtin_type_c" );
      DUMP_TYPE( "builtin_type_c", $1 );

      $$.ast = C_AST_NEW( K_BUILTIN, &@$ );
      $$.target_ast = NULL;
      $$.ast->type = $1;

      DUMP_AST( "builtin_or_enum_class_struct_union_type_c", $$.ast );
      DUMP_END();
    }

  | enum_class_struct_union_type_c name_expected
    {
      DUMP_START( "builtin_or_enum_class_struct_union_type_c",
                  "enum_class_struct_union_type_c NAME" );
      DUMP_TYPE( "enum_class_struct_union_type_c", $1 );
      DUMP_NAME( "NAME", $2 );

      $$.ast = C_AST_NEW( K_ENUM_CLASS_STRUCT_UNION, &@$ );
      $$.target_ast = NULL;
      $$.ast->type = $1;
      $$.ast->as.ecsu.ecsu_name = $2;

      DUMP_AST( "builtin_or_enum_class_struct_union_type_c", $$.ast );
      DUMP_END();
    }
  ;

builtin_type_c
  : Y_VOID
  | Y_BOOL
  | Y_CHAR
  | Y_CHAR16_T
  | Y_CHAR32_T
  | Y_WCHAR_T
  | Y_INT
  | Y_SIZE_T
  | Y_FLOAT
  | Y_DOUBLE
  ;

enum_class_struct_union_type_c
  : Y_ENUM
  | class_struct_type_c
  | Y_UNION
  ;

class_struct_type_c
  : Y_CLASS
  | Y_STRUCT
  ;

type_qualifier_list_opt_c
  : /* empty */                   { $$ = T_NONE; }
  | type_qualifier_list_opt_c type_qualifier_c
    {
      DUMP_START( "type_qualifier_list_opt_c",
                  "type_qualifier_list_opt_c type_qualifier_c" );
      DUMP_TYPE( "type_qualifier_list_opt_c", $1 );
      DUMP_TYPE( "type_qualifier_c", $2 );

      $$ = $1;
      C_TYPE_ADD( &$$, $2, @2 );

      DUMP_TYPE( "type_qualifier_list_opt_c", $$ );
      DUMP_END();
    }
  ;

type_qualifier_c
  : cv_qualifier_c
  | Y_RESTRICT
  ;

cv_qualifier_c
  : Y_CONST
  | Y_VOLATILE
  ;

storage_class_c
  : Y_AUTO
  | Y___BLOCK                           /* Apple extension */
  | Y_EXTERN
/*| Y_REGISTER */                       /* in type_modifier_english */
  | Y_STATIC
  | Y_TYPEDEF
  | Y_THREAD_LOCAL
  | Y_VIRTUAL
  ;

/*****************************************************************************/
/*  cast gibberish productions                                               */
/*****************************************************************************/

cast_c
  : /* empty */                   { $$.ast = $$.target_ast = NULL; }
  | cast2_c
  | pointer_cast_c
  | pointer_to_member_cast_c
  | reference_cast_c
  ;

cast2_c
  : array_cast_c
  | block_cast_c
  | func_cast_c
  | name_c
  | nested_cast_c
  ;

array_cast_c
  : cast2_c array_size_c
    {
      DUMP_START( "array_cast_c", "cast2_c array_size_c" );
      DUMP_AST( "type_c", type_peek() );
      DUMP_AST( "cast2_c", $1.ast );
      DUMP_NUM( "array_size_c", $2 );
      if ( $1.target_ast )
        DUMP_AST( "target_ast", $1.target_ast );

      c_ast_t *const array = C_AST_NEW( K_ARRAY, &@$ );
      array->as.array.size = $2;
      c_ast_set_parent( C_AST_NEW( K_NONE, &@1 ), array );
      if ( $1.target_ast ) {            // array-of or function/block-ret type
        $$.ast = $1.ast;
        $$.target_ast = c_ast_add_array( $1.target_ast, array );
      } else {
        $$.ast = c_ast_add_array( $1.ast, array );
        $$.target_ast = NULL;
      }

      DUMP_AST( "array_cast_c", $$.ast );
      DUMP_END();
    }
  ;

block_cast_c                            /* Apple extension */
  : /* type */ '(' '^'
    {
      //
      // A block AST has to be the type inherited attribute for cast_c so we
      // have to create it here.
      //
      type_push( C_AST_NEW( K_BLOCK, &@$ ) );
    }
    type_qualifier_list_opt_c cast_c ')' '(' arg_list_opt_c ')'
    {
      c_ast_t *const block = type_pop();

      DUMP_START( "block_cast_c",
                  "'(' '^' type_qualifier_list_opt_c cast_c ')' "
                  "'(' arg_list_opt_c ')'" );
      DUMP_AST( "type_c", type_peek() );
      DUMP_TYPE( "type_qualifier_list_opt_c", $4 );
      DUMP_AST( "cast_c", $5.ast );
      DUMP_AST_LIST( "arg_list_opt_c", $8 );

      C_TYPE_ADD( &block->type, $4, @4 );
      block->as.block.args = $8;
      $$.ast = c_ast_add_func( $5.ast, type_peek(), block );
      $$.target_ast = block->as.block.ret_ast;

      DUMP_AST( "block_cast_c", $$.ast );
      DUMP_END();
    }
  ;

func_cast_c
  : /* type_c */ cast2_c '(' arg_list_opt_c ')'
    {
      DUMP_START( "func_cast_c", "cast2_c '(' arg_list_opt_c ')'" );
      DUMP_AST( "type_c", type_peek() );
      DUMP_AST( "cast2_c", $1.ast );
      DUMP_AST_LIST( "arg_list_opt_c", $3 );

      c_ast_t *const func = C_AST_NEW( K_FUNCTION, &@$ );
      func->as.func.args = $3;
      $$.ast = c_ast_add_func( $1.ast, type_peek(), func );
      $$.target_ast = func->as.func.ret_ast;

      DUMP_AST( "func_cast_c", $$.ast );
      DUMP_END();
    }
  ;

nested_cast_c
  : '(' placeholder_type_c { type_push( $2.ast ); ++ast_depth; } cast_c ')'
    {
      type_pop();
      --ast_depth;

      DUMP_START( "nested_cast_c", "'(' placeholder_type_c cast_c ')'" );
      DUMP_AST( "placeholder_type_c", $2.ast );
      DUMP_AST( "cast_c", $4.ast );

      $$ = $4;

      DUMP_AST( "nested_cast_c", $$.ast );
      DUMP_END();
    }
  ;

pointer_cast_c
  : pointer_type_c { type_push( $1.ast ); } cast_c
    {
      type_pop();
      DUMP_START( "pointer_cast_c", "pointer_type_c cast_c" );
      DUMP_AST( "pointer_type_c", $1.ast );
      DUMP_AST( "cast_c", $3.ast );

      $$.ast = c_ast_patch_none( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "pointer_cast_c", $$.ast );
      DUMP_END();
    }
  ;

pointer_to_member_cast_c
  : pointer_to_member_type_c { type_push( $1.ast ); } cast_c
    {
      type_pop();
      DUMP_START( "pointer_to_member_cast_c",
                  "pointer_to_member_type_c cast_c" );
      DUMP_AST( "pointer_to_member_type_c", $1.ast );
      DUMP_AST( "cast_c", $3.ast );

      $$.ast = c_ast_patch_none( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "pointer_to_member_cast_c", $$.ast );
      DUMP_END();
    }
  ;

reference_cast_c
  : reference_type_c { type_push( $1.ast ); } cast_c
    {
      type_pop();
      DUMP_START( "reference_cast_c", "reference_type_c cast_c" );
      DUMP_AST( "reference_type_c", $1.ast );
      DUMP_AST( "cast_c", $3.ast );

      $$.ast = c_ast_patch_none( $1.ast, $3.ast );
      $$.target_ast = NULL;

      DUMP_AST( "reference_cast_c", $$.ast );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  miscellaneous productions                                                */
/*****************************************************************************/

class_struct_type_expected_c
  : class_struct_type_c
  | error
    {
      PARSE_ERROR( "\"%s\" or \"%s\" expected", L_CLASS, L_STRUCT );
    }
  ;

comma_expected
  : ','
  | error                         { PARSE_ERROR( "',' expected" ); }
  ;

returning_expected
  : Y_RETURNING
  | error
    {
      PARSE_ERROR( "\"%s\" expected", L_RETURNING );
    }
  ;

star_expected
  : '*'
  | error                         { PARSE_ERROR( "'*' expected" ); }
  ;

member_expected
  : Y_MEMBER
  | error
    {
      PARSE_ERROR( "\"%s\" expected", L_MEMBER );
    }
  ;

name_expected
  : Y_NAME
  | error
    {
      PARSE_ERROR( "name expected" );
    }
  ;

name_opt
  : /* empty */                   { $$ = NULL; }
  | Y_NAME
  ;

of_expected
  : Y_OF
  | error
    {
      PARSE_ERROR( "\"%s\" expected", L_OF );
    }
  ;

to_expected
  : Y_TO
  | error
    {
      PARSE_ERROR( "\"%s\" expected", L_TO );
    }
  ;

zero_expected
  : zero
  | error                         { PARSE_ERROR( "'0' expected" ); }
  ;

zero
  : Y_NUMBER
    {
      if ( $1 != 0 )
        SYNTAX_ERROR();
    }
  ;

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
