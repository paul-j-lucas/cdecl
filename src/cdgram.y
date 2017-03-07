/*
**      cdecl -- C gibberish translator
**      src/cdgram.y
*/

%{
// local
#include "config.h"                     /* must come first */
#include "ast.h"
#include "color.h"
#include "common.h"
#include "keywords.h"
#include "lang.h"
#include "literals.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WITH_CDECL_DEBUG
#define CDEBUG(...) BLOCK( if ( opt_debug ) { __VA_ARGS__ } )
#else
#define CDEBUG(...)                     /* nothing */
#endif /* WITH_CDECL_DEBUG */

#define DUMP_COMMA \
  CDEBUG( if ( true_or_set( &dump_comma ) ) FPUTS( ",\n", stdout ); )

#define DUMP_AST(KEY,AST) \
  CDEBUG( DUMP_COMMA; c_ast_json( (AST), 1, (KEY), stdout ); )

#define DUMP_AST_LIST(KEY,AST_LIST) CDEBUG(           \
  DUMP_COMMA; FPRINTF( stdout, "  \"%s\": ", (KEY) ); \
  c_ast_list_json( &(AST_LIST), 1, stdout ); )

#define DUMP_NAME(KEY,NAME) CDEBUG(         \
  DUMP_COMMA; FPUTS( "  ", stdout );        \
  json_print_kv( (KEY), (NAME), stdout ); )

#define DUMP_NUM(KEY,NUM) \
  CDEBUG( DUMP_COMMA; FPRINTF( stdout, "  \"" KEY "\": %d", (NUM) ); )

#ifdef WITH_CDECL_DEBUG
#define DUMP_START(NAME,PROD)               \
  bool dump_comma = false;                  \
  CDEBUG( FPUTS( "\n\"" NAME ": " PROD "\": {\n", stdout ); )
#else
#define DUMP_START(NAME,PROD)           /* nothing */
#endif

#define DUMP_END() CDEBUG( \
  FPUTS( "\n}\n", stdout ); )

#define DUMP_TYPE(KEY,TYPE) CDEBUG(         \
  DUMP_COMMA; FPUTS( "  ", stdout );        \
  json_print_kv( (KEY), c_type_name( TYPE ), stdout ); )

#define PARSE_ERROR(...) \
  BLOCK( parse_error( __VA_ARGS__ ); YYABORT; )

#define PUSH_TYPE(AST)            c_ast_push( &in_attr.type_ast, (AST) )
#define PEEK_TYPE()               in_attr.type_ast
#define POP_TYPE()                c_ast_pop( &in_attr.type_ast )

///////////////////////////////////////////////////////////////////////////////

/**
 * Inherited attributes.
 */
struct in_attr {
  char const *name;
  c_type_t    storage;
  c_ast_t    *type_ast;
  int         y_token;
};
typedef struct in_attr in_attr_t;

// external variables
#if YYTEXT_POINTER
extern char        *yytext;
#else
extern char         yytext[];
#endif /* YYTEXT_POINTER */

// extern functions
extern void         print_caret( void );
extern void         print_help( void );
extern void         set_option( char const* );
extern int          yylex( void );

// local variables
static in_attr_t    in_attr;
static bool         newlined = true;

////////// local functions ////////////////////////////////////////////////////

/*
static void cast_english( char const *name, c_ast_t *ast ) {
  switch ( ast->kind ) {
    case K_ARRAY:
      c_error( "cast into array", "cast into pointer" );
      break;
    case K_FUNCTION:
      c_error( "cast into function", "cast into pointer to function" );
      break;
    }
  } // switch
}
*/

static void parse_error( char const *format, ... ) {
  if ( !newlined ) {
    PRINT_ERR( ": " );
    if ( *yytext )
      PRINT_ERR( "\"%s\": ", yytext );
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    FPUTC( '\n', stderr );
    newlined = true;
  }
}

/**
 * Implements the cdecl "quit" command.
 */
static void quit( void ) {
  exit( EX_OK );
}

static void yyerror( char const *msg ) {
  print_caret();
  PRINT_ERR( "%s%s", (newlined ? "" : "\n"), msg );
  newlined = false;
}

///////////////////////////////////////////////////////////////////////////////
%}

%union {
  char const   *name;
  int           number;                 /* for array sizes */
  c_type_t      type;
  c_ast_t      *ast;
  c_ast_list_t  ast_list;
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
%token              Y_REFERENCE
%token              Y_RETURNING
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
%token  <type>      Y_ENUM
%token  <type>      Y_SIGNED
%token  <type>      Y_VOID
%token  <type>      Y_VOLATILE

                    /* C99 */
%token  <type>      Y_BOOL
%token  <type>      Y_COMPLEX
%token  <type>      Y_RESTRICT
%token  <type>      Y_WCHAR_T

                    /* C11 */
%token              Y_NORETURN
%token  <type>      Y_THREAD_LOCAL

                    /* C++ */
%token              '&'
%token  <type>      Y_CLASS
%token              Y_COLON_COLON

                    /* C11 & C++11 */
%token  <type>      Y_CHAR16_T
%token  <type>      Y_CHAR32_T

                    /* miscellaneous */
%token              '^'                 /* Apple: block indicator */
%token  <type>      Y___BLOCK           /* Apple: block storage class */
%token              Y_END
%token              Y_ERROR
%token  <name>      Y_NAME
%token  <number>    Y_NUMBER

%type   <ast>       decl_english
%type   <ast_list>  decl_list_english decl_list_opt_english
%type   <ast_list>  paren_decl_list_opt_english
%type   <ast>       array_decl_english
%type   <number>    array_size_opt_english
%type   <ast>       block_decl_english
%type   <ast>       func_decl_english
%type   <ast>       pointer_decl_english
%type   <ast>       pointer_to_member_decl_english
%type   <ast>       qualifiable_decl_english
%type   <ast>       qualified_decl_english
%type   <ast>       reference_decl_english
%type   <ast>       returning_english
%type   <ast>       type_english
%type   <type>      type_modifier_english
%type   <type>      type_modifier_list_english
%type   <type>      type_modifier_list_opt_english
%type   <ast>       unmodified_type_english
%type   <ast>       var_decl_english

%type   <ast>       cast_c
%type   <ast>       array_cast_c
%type   <ast>       block_cast_c
%type   <ast>       func_cast_c
%type   <ast>       pointer_cast_c
%type   <ast>       pointer_to_member_cast_c
%type   <ast>       reference_cast_c
%type   <ast>       name_cast_c

%type   <ast>       decl_c decl2_c
%type   <ast>       array_decl_c
%type   <number>    array_size_c
%type   <ast>       block_decl_c
%type   <ast>       func_decl_c
%type   <ast>       name_decl_c
%type   <ast>       named_enum_class_struct_union_type_c
%type   <ast>       nested_decl_c
%type   <ast>       pointer_decl_c
%type   <ast>       pointer_decl_type_c
%type   <ast>       pointer_to_member_decl_c
%type   <ast>       pointer_to_member_decl_type_c
%type   <ast>       reference_decl_c
%type   <ast>       reference_decl_type_c

%type   <ast>       placeholder_type_c
%type   <ast>       type_c
%type   <type>      builtin_type_c
%type   <type>      class_struct_type_c
%type   <type>      enum_class_struct_union_type_c
%type   <type>      storage_class_c storage_class_opt_c
%type   <type>      type_modifier_c
%type   <type>      type_modifier_list_c type_modifier_list_opt_c
%type   <type>      type_qualifier_c
%type   <type>      type_qualifier_list_opt_c

%type   <ast>       arg_c
%type   <ast_list>  arg_list_c arg_list_opt_c paren_arg_list_opt_opt_c
%type   <name>      name_token_opt

/*****************************************************************************/
%%

command_list
  : /* empty */
  | command_list command_init command command_cleanup
  ;

command_init
  : /* empty */
    {
      newlined = true;
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
  | Y_END
  | error Y_END
    {
      parse_error(
        "\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", or \"%s\" expected",
        L_CAST, L_DECLARE, L_EXPLAIN, L_HELP, L_SET, L_QUIT
      );
      yyerrok;
    }
  ;

command_cleanup
  : /* empty */
    {
      c_ast_gc();
    }
  ;

/*****************************************************************************/
/*  cast                                                                     */
/*****************************************************************************/

cast_english
  : Y_CAST Y_NAME Y_INTO decl_english Y_END
    {
      DUMP_START( "cast_english", "CAST NAME INTO decl_english END" );
      DUMP_NAME( "-> NAME", $2 );
      DUMP_AST( "-> decl_english", $4 );
      DUMP_END();

      if ( c_ast_check( $4 ) ) {
        FPUTC( '(', fout );
        c_ast_gibberish_cast( $4, fout );
        FPRINTF( fout, ")%s\n", $2 );
        FREE( $2 );
      }
    }

  | Y_CAST Y_NAME error
    {
      PARSE_ERROR( "\"%s\" expected", L_INTO );
    }

  | Y_CAST decl_english Y_END
    {
      DUMP_START( "cast_english", "CAST decl_english END" );
      DUMP_AST( "-> decl_english", $2 );
      DUMP_END();

      if ( c_ast_check( $2 ) ) {
        FPUTC( '(', fout );
        c_ast_gibberish_cast( $2, fout );
        FPUTS( ")\n", fout );
      }
    }
  ;

/*****************************************************************************/
/*  declare                                                                  */
/*****************************************************************************/

declare_english
  : Y_DECLARE Y_NAME Y_AS storage_class_opt_c
    {
      in_attr.name = $2;
      in_attr.storage = $4;
    }
    decl_english Y_END
    {
      DUMP_START( "declare_english",
                  "DECLARE NAME AS storage_class_opt_c decl_english END" );
      $6->name = $2;
      DUMP_NAME( "-> NAME", $2 );
      DUMP_TYPE( "-> storage_class_opt_c", $4 );
      DUMP_AST( "-> decl_english", $6 );
      DUMP_END();

      if ( c_ast_check( $6 ) ) {
        c_ast_gibberish_declare( $6, fout );
        FPUTC( '\n', fout );
      }
    }

  | Y_DECLARE error
    {
      PARSE_ERROR( "name expected" );
    }

  | Y_DECLARE Y_NAME error
    {
      PARSE_ERROR( "\"%s\" expected", L_AS );
    }
  ;

/*****************************************************************************/
/*  explain                                                                  */
/*****************************************************************************/

explain_declaration_c
  : Y_EXPLAIN type_c { PUSH_TYPE( $2 ); } decl_c Y_END
    {
      POP_TYPE();
      DUMP_START( "explain_declaration_c", "EXPLAIN type_c decl_c END" );
      DUMP_AST( "-> type_c", $2 );
      DUMP_AST( "-> decl_c", $4 );
      DUMP_END();

      if ( c_ast_check( $4 ) ) {
        char const *const name = c_ast_take_name( $4 );
        FPRINTF( fout, "declare %s as ", name );
        if ( c_ast_take_typedef( $4 ) )
          FPUTS( "type ", fout );
        c_ast_english( $4, fout );
        FPUTC( '\n', fout );
        FREE( name );
      }
    }
  ;

explain_cast_c
  : Y_EXPLAIN '(' type_c { PUSH_TYPE( $3 ); } cast_c ')'
    name_token_opt Y_END
    {
      POP_TYPE();
      DUMP_START( "explain_cast_t",
                  "EXPLAIN '(' type_c cast_c ')' name_token_opt END" );
      DUMP_AST( "-> type_c", $3 );
      DUMP_AST( "-> cast_c", $5 );
      DUMP_NAME( "-> name_token_opt", $7 );
      DUMP_END();

      if ( $7 )
        FPRINTF( fout, "cast %s into ", $7 );
      else
        FPUTS( "cast into ", fout );
      c_ast_english( $5, fout );
      FPUTC( '\n', fout );

      FREE( $7 );
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
  : Y_SET name_token_opt Y_END    { set_option( $2 ); FREE( $2 ); }
  ;

/*****************************************************************************/
/*  quit                                                                     */
/*****************************************************************************/

quit_command
  : Y_QUIT Y_END                  { quit(); }
  ;

/*****************************************************************************/
/*  cast gibberish productions                                               */
/*****************************************************************************/

cast_c
  : /* empty */                   { $$ = NULL; }
  | array_cast_c
  | block_cast_c                        /* Apple extension */
  | func_cast_c
  | name_cast_c
  | pointer_cast_c
  | pointer_to_member_cast_c
  | reference_cast_c
  ;

array_cast_c
  : /* type_c */ cast_c array_size_c
    {
      DUMP_START( "array_cast_c", "cast_c array_cast_c" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_AST( "-> cast_c", $1 );
      DUMP_NUM( "-> array_size_c", $2 );

      c_ast_t *const array = c_ast_new( K_ARRAY );
      array->name = c_ast_take_name( $1 );
      array->as.array.size = $2;

      switch ( $1->kind ) {
        case K_POINTER:
        case K_REFERENCE:
          if ( $1->as.ptr_ref.to_ast->kind == K_NONE ) {
            array->as.array.of_ast = c_ast_clone( PEEK_TYPE() );
            $1->as.ptr_ref.to_ast = array;
            $$ = $1;
            break;
          }
          // no break;
        default:
          $$ = array;
          $$->as.array.of_ast = $1;
      } // switch

      DUMP_AST( "<- array_cast_c", $$ );
      DUMP_END();
    }
  ;

block_cast_c                            /* Apple extension */
  : /* type_c */ '(' '^' cast_c ')' '(' arg_list_opt_c ')'
    {
      DUMP_START( "block_cast_c",
                  "'(' '^' cast_c ')' '(' arg_list_opt_c ')'" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_AST( "-> cast_c", $3 );
      DUMP_AST_LIST( "-> arg_list_opt_c", $6 );

      $$ = c_ast_new( K_BLOCK );
      $$->name = c_ast_name( $3 );
      $$->as.block.args = $6;
      $$->as.block.ret_ast = c_ast_clone( PEEK_TYPE() );

      DUMP_AST( "<- block_cast_c", $$ );
      DUMP_END();
    }
  ;

func_cast_c
  : /* type_c */ '(' ')'
    {
      DUMP_START( "func_cast_c", "'(' ')'" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );

      $$ = c_ast_new( K_FUNCTION );
      $$->as.func.ret_ast = c_ast_clone( PEEK_TYPE() );

      DUMP_AST( "<- func_cast_c", $$ );
      DUMP_END();
    }

  | '(' placeholder_type_c { PUSH_TYPE( $2 ); } cast_c ')'
    paren_arg_list_opt_opt_c
    {
      POP_TYPE();
      DUMP_START( "func_cast_c", "'(' cast_c ')' '(' arg_list_opt_c ')'" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_AST( "-> placeholder_type_c", $2 );
      DUMP_AST( "-> cast_c", $4 );
      DUMP_AST_LIST( "-> arg_list_opt_c", $6 );

      c_ast_t *const func = c_ast_new( K_FUNCTION );
      func->as.func.args = $6;

      switch ( $4->kind ) {
        case K_POINTER:
          if ( $4->as.ptr_ref.to_ast->kind == K_NONE ) {
            func->as.func.ret_ast = c_ast_clone( PEEK_TYPE() );
            $4->as.ptr_ref.to_ast = func;
            $$ = $4;
            break;
          }
          // no break;
        default:
          $$ = func;
          $$->as.func.ret_ast = c_ast_clone( PEEK_TYPE() );
      } // switch

      DUMP_AST( "<- func_cast_c", $$ );
      DUMP_END();
    }
  ;

name_cast_c
  : /* type_c */ Y_NAME
    {
      DUMP_START( "name_cast_c", "NAME" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_NAME( "-> NAME", $1 );

      $$ = c_ast_clone( PEEK_TYPE() );
      assert( $$->name == NULL );
      $$->name = $1;

      DUMP_AST( "<- name_cast_c", $$ );
      DUMP_END();
    }
  ;

pointer_cast_c
  : /* type_c */ '*' cast_c
    {
      DUMP_START( "pointer_cast_c", "'*' cast_c" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_AST( "-> cast_c", $2 );

      $$ = c_ast_new( K_POINTER );
      // TODO: do something with $2
      $$->as.ptr_ref.to_ast = c_ast_clone( PEEK_TYPE() );

      DUMP_AST( "<- pointer_cast_c", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_cast_c
  : /* type_c */ Y_NAME Y_COLON_COLON expect_star cast_c
    {
      DUMP_START( "pointer_to_member_cast_c", "NAME COLON_COLON '*' cast_c" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_NAME( "-> NAME", $1 );
      DUMP_AST( "-> cast_c", $4 );

      $$ = c_ast_new( K_POINTER_TO_MEMBER );
      $$->type = T_CLASS;
      $$->as.ptr_mbr.of_ast = c_ast_clone( PEEK_TYPE() );
      $$->as.ptr_mbr.class_name = $1;
      // TODO: do something with $4

      DUMP_AST( "<- pointer_to_member_cast_c", $$ );
      DUMP_END();
    }
  ;

reference_cast_c
  : /* type_c */ '&' cast_c
    {
      DUMP_START( "reference_cast_c", "'&' cast_c" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_AST( "-> cast_c", $2 );

      $$ = c_ast_new( K_REFERENCE );
      // TODO: do something with $2
      $$->as.ptr_ref.to_ast = c_ast_clone( PEEK_TYPE() );

      DUMP_AST( "<- reference_cast_c", $$ );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  function argument gibberish productions                                  */
/*****************************************************************************/

paren_arg_list_opt_opt_c
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | '(' arg_list_opt_c ')'        { $$ = $2; }
  ;

arg_list_opt_c
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | arg_list_c
  ;

arg_list_c
  : arg_list_c expect_comma arg_c
    {
      DUMP_START( "arg_list_c", "arg_list_c ',' cast_c" );
      DUMP_AST_LIST( "-> arg_list_c", $1 );
      DUMP_AST( "-> cast_c", $3 );

      $$ = $1;
      c_ast_list_append( &$$, $3 );

      DUMP_AST_LIST( "<- arg_list_c", $$ );
      DUMP_END();
    }

  | arg_c
    {
      DUMP_START( "arg_list_c", "arg_c" );
      DUMP_AST( "-> arg_c", $1 );

      $$.head_ast = $$.tail_ast = $1;

      DUMP_AST_LIST( "<- arg_list_c", $$ );
      DUMP_END();
    }
  ;

arg_c
  : type_c { PUSH_TYPE( $1 ); } cast_c
    {
      POP_TYPE();
      DUMP_START( "arg_c", "type_c cast_c" );
      DUMP_AST( "-> type_c", $1 );
      DUMP_AST( "-> cast_c", $3 );

      $$ = $3 ? $3 : $1;
      if ( $$->name == NULL )
        $$->name = check_strdup( c_ast_name( $$ ) );

      DUMP_AST( "<- arg_c", $$ );
      DUMP_END();
    }
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
      DUMP_NUM( "-> array_size_opt_english", $2 );
      DUMP_AST( "-> decl_english", $4 );

      switch ( $4->kind ) {
        case K_ARRAY:
          c_error( "inner array of unspecified size", "array of pointer" );
          break;
        case K_BUILTIN:
          if ( $4->type & T_VOID )
            c_error( "array of void", "pointer to void" );
          break;
        case K_FUNCTION:
          c_error( "array of function", "array of pointer to function" );
          break;
        default:
          /* suppress warning */;
      } // switch

      $$ = c_ast_new( K_ARRAY );
      $$->as.array.size = $2;
      $$->as.array.of_ast = $4;

      DUMP_AST( "<- array_decl_english", $$ );
      DUMP_END();
    }
  ;

array_size_opt_english
  : /* empty */                   { $$ = C_ARRAY_NO_SIZE; }
  | Y_NUMBER
  | error                         { PARSE_ERROR( "array size expected" ); }
  ;

block_decl_english                      /* Apple extension */
  : Y_BLOCK { in_attr.y_token = Y_BLOCK; } paren_decl_list_opt_english
    returning_english
    {
      DUMP_START( "block_decl_english",
                  "BLOCK paren_decl_list_opt_english returning_english" );
      DUMP_AST_LIST( "-> paren_decl_list_opt_english", $3 );
      DUMP_AST( "-> returning_english", $4 );

      $$ = c_ast_new( K_BLOCK );
      $$->as.block.ret_ast = $4;
      $$->as.block.args = $3;

      DUMP_AST( "<- block_decl_english", $$ );
      DUMP_END();
    }
  ;

func_decl_english
  : Y_FUNCTION { in_attr.y_token = Y_FUNCTION; } paren_decl_list_opt_english
    returning_english
    {
      DUMP_START( "func_decl_english",
                  "FUNCTION paren_decl_list_opt_english returning_english" );
      DUMP_AST_LIST( "-> decl_list_opt_english", $3 );
      DUMP_AST( "-> returning_english", $4 );

      $$ = c_ast_new( K_FUNCTION );
      $$->as.func.ret_ast = $4;
      $$->as.func.args = $3;

      DUMP_AST( "<- func_decl_english", $$ );
      DUMP_END();
    }
  ;

paren_decl_list_opt_english
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | '(' decl_list_opt_english ')'
    {
      DUMP_START( "paren_decl_list_opt_english",
                  "'(' decl_list_opt_english ')'" );
      DUMP_AST_LIST( "-> decl_list_opt_english", $2 );

      $$ = $2;

      DUMP_AST_LIST( "<- paren_decl_list_opt_english", $$ );
      DUMP_END();
    }
  ;

decl_list_opt_english
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | decl_list_english
  ;

decl_list_english
  : decl_english                  { $$.head_ast = $$.tail_ast = $1; }
  | decl_list_english expect_comma decl_english
    {
      DUMP_START( "decl_list_opt_english",
                  "decl_list_opt_english ',' decl_english" );
      DUMP_AST_LIST( "-> decl_list_opt_english", $1 );
      DUMP_AST( "-> decl_english", $3 );

      $$ = $1;
      c_ast_list_append( &$$, $3 );

      DUMP_AST_LIST( "<- decl_list_opt_english", $$ );
      DUMP_END();
    }
  ;

returning_english
  : Y_RETURNING decl_english
    {
      DUMP_START( "returning_english", "RETURNING decl_english" );
      DUMP_AST( "-> decl_english", $2 );

      c_keyword_t const *keyword;
      switch ( $2->kind ) {
        case K_ARRAY:
        case K_FUNCTION:
          keyword = c_keyword_find_token( in_attr.y_token );
        default:
          keyword = NULL;
      } // switch

      if ( keyword ) {
        char error_msg[ 80 ];
        char hint_msg[ 80 ];
        char const *hint;

        switch ( $2->kind ) {
          case K_ARRAY:
            hint = "pointer";
            break;
          case K_FUNCTION:
            hint = "pointer to function";
            break;
          default:
            /* suppress warning */;
        } // switch

        snprintf( error_msg, sizeof error_msg,
          "%s returning %s",
          keyword->literal, c_kind_name( $2->kind )
        );
        snprintf( hint_msg, sizeof hint_msg,
          "%s returning %s",
          keyword->literal, hint
        );

        c_error( error_msg, hint_msg );
      }

      $$ = $2;

      DUMP_AST( "<- returning_english", $$ );
      DUMP_END();
    }

  | error { PARSE_ERROR( "\"%s\" expected", L_RETURNING ); }
  ;

qualified_decl_english
  : type_qualifier_list_opt_c qualifiable_decl_english
    {
      $$ = $2;
      c_type_add( &$$->type, $1 );
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
  : pointer_to decl_english
    {
      DUMP_START( "pointer_decl_english", "POINTER TO decl_english" );
      DUMP_AST( "-> decl_english", $2 );

      if ( $2->kind == K_ARRAY )
        c_error( "pointer to array of unspecified dimension",
                 "pointer to object" );
      $$ = c_ast_new( K_POINTER );
      $$->as.ptr_ref.to_ast = $2;

      DUMP_AST( "<- pointer_decl_english", $$ );
      DUMP_END();
    }
  ;

pointer_to
  : Y_POINTER Y_TO
  | Y_POINTER error
    {
      PARSE_ERROR( "\"%s\" expected", L_TO );
    }
  ;

pointer_to_member_decl_english
  : pointer_to Y_MEMBER Y_OF class_struct_type_c Y_NAME decl_english
    {
      DUMP_START( "pointer_to_member_decl_english",
                  "POINTER TO MEMBER OF "
                  "class_struct_type_c NAME decl_english" );
      DUMP_TYPE( "-> class_struct_type_c", $4 );
      DUMP_NAME( "-> NAME", $5 );
      DUMP_AST( "-> decl_english", $6 );

      if ( opt_lang < LANG_CPP_MIN )
        c_warning( "pointer to member of class", NULL );
#if 0
      if ( c_kind == K_ARRAY )
        c_error( "pointer to array of unspecified dimension",
                 "pointer to object" );
#endif
      $$ = c_ast_new( K_POINTER_TO_MEMBER );
      $$->type = $4;
      $$->as.ptr_mbr.class_name = $5;
      $$->as.ptr_mbr.of_ast = $6;

      DUMP_AST( "<- pointer_to_member_decl_english", $$ );
      DUMP_END();
    }

  | pointer_to error
    {
      PARSE_ERROR( "\"%s\" expected", L_MEMBER );
    }

  | pointer_to Y_MEMBER error
    {
      PARSE_ERROR( "\"%s\" expected", L_OF );
    }

  | pointer_to Y_MEMBER Y_OF error
    {
      PARSE_ERROR(
        "\"%s\", \"%s\", or \"%s\" expected", L_CLASS, L_STRUCT, L_UNION
      );
    }

  | pointer_to Y_MEMBER Y_OF class_struct_type_c error
    {
      PARSE_ERROR(
        "%s, %s, or %s name expected", L_CLASS, L_STRUCT, L_UNION
      );
    }
  ;

reference_decl_english
  : Y_REFERENCE Y_TO decl_english
    {
      DUMP_START( "reference_decl_english", "REFERENCE TO decl_english" );
      DUMP_AST( "-> decl_english", $3 );

      if ( opt_lang < LANG_CPP_MIN )
        c_warning( "reference", NULL );
      switch ( $3->kind ) {
        case K_ARRAY:
          c_error( "reference to array of unspecified dimension",
                   "reference to object" );
          break;
        case K_BUILTIN:
          if ( $3->type & T_VOID )
            c_error( "reference of void", "pointer to void" );
          break;
        default:
          ;// suppress warning
      } // switch

      $$ = c_ast_new( K_REFERENCE );
      $$->as.ptr_ref.to_ast = $3;

      DUMP_AST( "<- reference_decl_english", $$ );
      DUMP_END();
    }

  | Y_REFERENCE error
    {
      PARSE_ERROR( "\"%s\" expected", L_TO );
    }
  ;

var_decl_english
  : Y_NAME Y_AS decl_english
    {
      DUMP_START( "var_decl_english", "NAME AS decl_english" );
      DUMP_NAME( "-> NAME", $1 );
      DUMP_AST( "-> decl_english", $3 );

      $$ = $3;
      assert( $$->name == NULL );
      $$->name = $1;

      DUMP_AST( "<- var_decl_english", $$ );
      DUMP_END();
    }

  | Y_NAME
    {
      DUMP_START( "var_decl_english", "NAME" );
      DUMP_NAME( "-> NAME", $1 );

      if ( opt_lang > LANG_C_KNR )
        c_warning( "missing function prototype", NULL );

      $$ = c_ast_new( K_NAME );
      $$->name = $1;

      DUMP_AST( "<- var_decl_english", $$ );
      DUMP_END();
    }
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
  | name_decl_c
  | nested_decl_c
  ;

array_decl_c
  : /* type_c */ decl2_c array_size_c
    {
      DUMP_START( "array_decl_c", "decl2_c array_size_c" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_AST( "-> decl2_c", $1 );
      DUMP_NUM( "-> array_size_c", $2 );

      c_ast_t *const array = c_ast_new( K_ARRAY );
      array->name = c_ast_take_name( $1 );
      array->as.array.size = $2;

      switch ( $1->kind ) {
        case K_POINTER:
        case K_REFERENCE:
          if ( $1->as.ptr_ref.to_ast->kind == K_NONE ) {
            array->as.array.of_ast = c_ast_clone( PEEK_TYPE() );
            $1->as.ptr_ref.to_ast = array;
            $$ = $1;
            break;
          }
          // no break;
        default:
          $$ = array;
          $$->as.array.of_ast = $1;
      } // switch

      DUMP_AST( "<- array_decl_c", $$ );
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
  : /* type_c */ '(' '^' type_qualifier_list_opt_c decl_c ')'
    '(' arg_list_opt_c ')'
    {
      DUMP_START( "block_decl_c",
                  "'(' '^' type_qualifier_list_opt_c decl_c ')' "
                  "'(' arg_list_opt_c ')'" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_TYPE( "-> type_qualifier_list_opt_c", $3 );
      DUMP_AST( "-> decl_c", $4 );
      DUMP_AST_LIST( "-> arg_list_opt_c", $7 );

      $$ = c_ast_new( K_BLOCK );
      $$->name = check_strdup( c_ast_name( $4 ) );
      $$->as.block.args = $7;
      $$->as.block.ret_ast = c_ast_clone( PEEK_TYPE() );
      $$->type = $3;

      DUMP_AST( "<- block_decl_c", $$ );
      DUMP_END();
    }
  ;

func_decl_c
  : /* type_c */ decl2_c '(' arg_list_opt_c ')'
    {
      DUMP_START( "func_decl_c", "decl2_c '(' arg_list_opt_c ')'" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_AST( "-> decl2_c", $1 );
      DUMP_AST_LIST( "-> arg_list_opt_c", $3 );

      c_ast_t *const func = c_ast_new( K_FUNCTION );
      func->name = c_ast_take_name( $1 );
      func->as.func.args = $3;

      switch ( $1->kind ) {
        case K_POINTER:
          if ( $1->as.ptr_ref.to_ast->kind == K_NONE ) {
            func->as.func.ret_ast = c_ast_clone( PEEK_TYPE() );
            $1->as.ptr_ref.to_ast = func;
            $$ = $1;
            break;
          }
          // no break;
        default:
          $$ = func;
          $$->as.func.ret_ast = c_ast_clone( PEEK_TYPE() );
      } // switch

      $$->type = c_ast_take_storage( $$->as.func.ret_ast );

      DUMP_AST( "<- func_decl_c", $$ );
      DUMP_END();
    }
  ;

name_decl_c
  : /* type_c */ Y_NAME
    {
      DUMP_START( "name_decl_c", "NAME" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_NAME( "-> NAME", $1 );

      $$ = c_ast_clone( PEEK_TYPE() );
      assert( $$->name == NULL );
      $$->name = $1;

      DUMP_AST( "<- name_decl_c", $$ );
      DUMP_END();
    }
  ;

nested_decl_c
  : '(' placeholder_type_c { PUSH_TYPE( $2 ); } decl_c ')'
    {
      POP_TYPE();
      DUMP_START( "nested_decl_c", "'(' placeholder_type_c decl_c ')'" );
      DUMP_AST( "-> placeholder_type_c", $2 );
      DUMP_AST( "-> decl_c", $4 );

      $$ = $4;

      DUMP_AST( "<- nested_decl_c", $$ );
      DUMP_END();
    }
  ;

placeholder_type_c
  : /* empty */                   { $$ = c_ast_new( K_NONE ); }
  ;

pointer_decl_c
  : pointer_decl_type_c { PUSH_TYPE( $1 ); } decl_c
    {
      POP_TYPE();
      DUMP_START( "pointer_decl_c", "pointer_decl_type_c decl_c" );
      DUMP_AST( "-> pointer_decl_type_c", $1 );
      DUMP_AST( "-> decl_c", $3 );

      $$ = $3;

      DUMP_AST( "<- pointer_decl_c", $$ );
      DUMP_END();
    }
  ;

pointer_decl_type_c
  : /* type_c */ '*' type_qualifier_list_opt_c
    {
      DUMP_START( "pointer_decl_type_c", "'*' type_qualifier_list_opt_c" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_TYPE( "-> type_qualifier_list_opt_c", $2 );

      $$ = c_ast_new( K_POINTER );
      $$->as.ptr_ref.qualifier = $2;
      $$->as.ptr_ref.to_ast = c_ast_clone( PEEK_TYPE() );

      DUMP_AST( "<- pointer_decl_type_c", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_decl_c
  : pointer_to_member_decl_type_c { PUSH_TYPE( $1 ); } decl_c
    {
      POP_TYPE();
      DUMP_START( "pointer_to_member_decl_c",
                  "pointer_to_member_decl_type_c decl_c" );
      DUMP_AST( "-> pointer_to_member_decl_type_c", $1 );
      DUMP_AST( "-> decl_c", $3 );

      $$ = $3;

      DUMP_AST( "<- pointer_to_member_decl_c", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_decl_type_c
  : /* type_c */ Y_NAME Y_COLON_COLON expect_star
    {
      DUMP_START( "pointer_to_member_decl_type_c", "NAME COLON_COLON '*'" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_NAME( "-> NAME", $1 );

      $$ = c_ast_new( K_POINTER_TO_MEMBER );
      $$->type = T_CLASS;
      $$->as.ptr_mbr.class_name = $1;
      $$->as.ptr_mbr.of_ast = c_ast_clone( PEEK_TYPE() );

      DUMP_AST( "<- pointer_to_member_decl_type_c", $$ );
      DUMP_END();
    }
  ;

reference_decl_c
  : reference_decl_type_c { PUSH_TYPE( $1 ); } decl_c
    {
      POP_TYPE();
      DUMP_START( "reference_decl_c", "reference_decl_type_c decl_c" );
      DUMP_AST( "-> reference_decl_type_c", $1 );
      DUMP_AST( "-> decl_c", $3 );

      $$ = $3;

      DUMP_AST( "<- reference_decl_c", $$ );
      DUMP_END();
    }
  ;

reference_decl_type_c
  : /* type_c */ '&' type_qualifier_list_opt_c
    {
      DUMP_START( "reference_decl_type_c", "'&' type_qualifier_list_opt_c" );
      DUMP_AST( "^^ type_c", PEEK_TYPE() );
      DUMP_TYPE( "-> type_qualifier_list_opt_c", $2 );

      $$ = c_ast_new( K_REFERENCE );
      $$->as.ptr_ref.qualifier = $2;
      $$->as.ptr_ref.to_ast = c_ast_clone( PEEK_TYPE() );

      DUMP_AST( "<- reference_decl_type_c", $$ );
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
      DUMP_TYPE( "-> type_modifier_list_opt_english", $1 );
      DUMP_AST( "-> unmodified_type_english", $2 );

      $$ = $2;
      c_type_add( &$$->type, $1 );

      DUMP_AST( "<- type_english", $$ );
      DUMP_END();
    }

  | type_modifier_list_english          /* allows for default int type */
    {
      DUMP_START( "type_english", "type_modifier_list_english" );
      DUMP_TYPE( "-> type_modifier_list_english", $1 );

      $$ = c_ast_new( K_BUILTIN );
      $$->type = T_INT;
      c_type_add( &$$->type, $1 );

      DUMP_AST( "<- type_english", $$ );
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
      DUMP_TYPE( "-> type_modifier_list_opt_english", $1 );
      DUMP_TYPE( "-> type_modifier_english", $2 );

      $$ = $1;
      c_type_add( &$$, $1 );

      DUMP_TYPE( "<- type_modifier_list_opt_english", $$ );
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
  ;

unmodified_type_english
  : builtin_type_c
    {
      $$ = c_ast_new( K_BUILTIN );
      $$->type = $1;
    }

  | enum_class_struct_union_type_c
    {
      $$ = c_ast_new( K_ENUM_CLASS_STRUCT_UNION );
      $$->type = $1;
    }
  ;

/*****************************************************************************/
/*  type gibberish productions                                               */
/*****************************************************************************/

type_c
  : type_modifier_list_c
    {
      DUMP_START( "type_c", "type_modifier_list_c" );
      DUMP_TYPE( "-> type_modifier_list_c", $1 );

      $$ = c_ast_new( K_BUILTIN );
      $$->type = $1;
      c_type_check( $$->type );

      DUMP_AST( "<- type_c", $$ );
      DUMP_END();
    }

  | type_modifier_list_c builtin_type_c type_modifier_list_opt_c
    {
      DUMP_START( "type_c",
                  "type_modifier_list_c builtin_type_c "
                  "type_modifier_list_opt_c" );
      DUMP_TYPE( "-> type_modifier_list_c", $1 );
      DUMP_TYPE( "-> builtin_type_c", $2 );
      DUMP_TYPE( "-> type_modifier_list_opt_c", $3 );

      $$ = c_ast_new( K_BUILTIN );
      $$->type = $1;
      c_type_add( &$$->type, $2 );
      c_type_add( &$$->type, $3 );
      c_type_check( $$->type );

      DUMP_AST( "<- type_c", $$ );
      DUMP_END();
    }

  | builtin_type_c type_modifier_list_opt_c
    {
      DUMP_START( "type_c", "builtin_type_c type_modifier_list_opt_c" );
      DUMP_TYPE( "-> builtin_type_c", $1 );
      DUMP_TYPE( "-> type_modifier_list_opt_c", $2 );

      $$ = c_ast_new( K_BUILTIN );
      $$->type = $1;
      c_type_add( &$$->type, $2 );
      c_type_check( $$->type );

      DUMP_AST( "<- type_c", $$ );
      DUMP_END();
    }

  | named_enum_class_struct_union_type_c
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
      DUMP_TYPE( "-> type_modifier_list_c", $1 );
      DUMP_TYPE( "-> type_modifier_c", $2 );

      $$ = $1;
      c_type_add( &$$, $2 );

      DUMP_TYPE( "<- type_modifier_list_c", $$ );
      DUMP_END();
    }
  | type_modifier_c
  ;

type_modifier_c
  : type_modifier_english
  | type_qualifier_c
  | storage_class_c
  ;

builtin_type_c
  : Y_VOID
  | Y_BOOL
  | Y_CHAR
  | Y_CHAR16_T
  | Y_CHAR32_T
  | Y_WCHAR_T
  | Y_INT
  | Y_FLOAT
  | Y_DOUBLE
  ;

named_enum_class_struct_union_type_c
  : enum_class_struct_union_type_c Y_NAME
    {
      DUMP_START( "named_enum_class_struct_union_type_c",
                  "enum_class_struct_union_type_c NAME" );
      DUMP_TYPE( "-> enum_class_struct_union_type_c", $1 );
      DUMP_NAME( "-> NAME", $2 );

      $$ = c_ast_new( K_ENUM_CLASS_STRUCT_UNION );
      $$->type = $1;
      $$->as.ecsu.ecsu_name = $2;
      c_type_check( $$->type );

      DUMP_AST( "<- named_enum_class_struct_union_type_c", $$ );
      DUMP_END();
    }

  | enum_class_struct_union_type_c error
    {
      PARSE_ERROR(
        "%s name expected", c_kind_name( K_ENUM_CLASS_STRUCT_UNION )
      );
    }
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
      DUMP_TYPE( "-> type_qualifier_list_opt_c", $1 );
      DUMP_TYPE( "-> type_qualifier_c", $2 );

      $$ = $1;
      c_type_add( &$$, $2 );

      DUMP_TYPE( "<- type_qualifier_list_opt_c", $$ );
      DUMP_END();
    }
  ;

type_qualifier_c
  : Y_CONST
  | Y_RESTRICT
  | Y_VOLATILE
  ;

storage_class_opt_c
  : /* empty */                   { $$ = T_NONE; }
  | storage_class_c
  ;

storage_class_c
  : Y_AUTO
  | Y___BLOCK                           /* Apple extension */
  | Y_EXTERN
  | Y_REGISTER
  | Y_STATIC
  | Y_TYPEDEF
  | Y_THREAD_LOCAL
  ;

/*****************************************************************************/
/*  miscellaneous productions                                                */
/*****************************************************************************/

expect_comma
  : ','
  | error                         { PARSE_ERROR( "',' expected" ); }
  ;

expect_star
  : '*'
  | error                         { PARSE_ERROR( "'*' expected" ); }
  ;

name_token_opt
  : /* empty */                   { $$ = NULL; }
  | Y_NAME
  ;

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
