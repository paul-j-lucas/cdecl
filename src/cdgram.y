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
  CDEBUG( if ( !dump_comma ) dump_comma = true; else FPUTS( ",\n", stdout ); )

#define DUMP_AST(KEY,AST) \
  CDEBUG( DUMP_COMMA; c_ast_json( (AST), 1, (KEY), stdout ); )

#define DUMP_AST_LIST(KEY,AST_LIST) CDEBUG( \
  DUMP_COMMA; c_ast_json( (AST_LIST).head_ast, 1, (KEY ".head"), stdout ); \
  DUMP_COMMA; c_ast_json( (AST_LIST).tail_ast, 1, (KEY ".tail"), stdout ); )

#define DUMP_NAME(KEY,NAME) CDEBUG(         \
  DUMP_COMMA; FPUTS( "  ", stdout );        \
  json_print_kv( (KEY), (NAME), stdout ); )

#define DUMP_NUM(KEY,NUM) \
  CDEBUG( DUMP_COMMA; FPRINTF( stdout, "  \"" KEY "\": %d", (NUM) ); )

#ifdef WITH_CDECL_DEBUG
#define DUMP_START(NAME,PROD)               \
  bool dump_comma = false;                  \
  CDEBUG( FPUTS( "\n\"" NAME ":" PROD "\": {\n", stdout ); )
#else
#define DUMP_START(NAME,PROD)           /* nothing */
#endif

#define DUMP_END() CDEBUG( \
  FPUTS( "\n}\n", stdout ); )

#define DUMP_TYPE(KEY,TYPE) CDEBUG(         \
  DUMP_COMMA; FPUTS( "  ", stdout );        \
  json_print_kv( (KEY), c_type_name( TYPE ), stdout ); )

#define PUSH_TYPE(AST)            c_ast_push( &in_attr.type_ast, (AST) )
#define PEEK_TYPE()               in_attr.type_ast
#define POP_TYPE()                c_ast_pop( &in_attr.type_ast )

///////////////////////////////////////////////////////////////////////////////

/**
 * Inherited attributes.
 */
struct in_attr {
  c_ast_t  *type_ast;
  int       y_token;
};
typedef struct in_attr in_attr_t;

// external variables
extern char const  *yytext;

// extern functions
extern void         print_caret( void );
extern void         print_help( void );
extern void         set_option( char const* );
extern int          yylex( void );

// local variables
static in_attr_t    in_attr;
static bool         newlined = true;

// local functions
static void         illegal( char const*, char const* );

///////////////////////////////////////////////////////////////////////////////

/**
 * Do the "cast" command.
 *
 * @param name TODO
 * @param ast TODO
 */
static void cast_english( char const *name, c_ast_t *ast ) {
  assert( ast );

  (void)name;           // TODO: remove

  switch ( ast->kind ) {
    case K_ARRAY:
      illegal( "cast into array", "cast into pointer" );
      break;
    case K_FUNCTION:
      illegal( "cast into function", "cast into pointer to function" );
      break;
    default: {
/*
      size_t const lenl = strlen( left ), lenr = strlen( right );
      printf(
        "(%s%*s%s)%s\n",
        type, (int)(lenl + lenr ? lenl + 1 : 0),
        left, right, name ? name : "expression"
      );
*/
    }
  } // switch
}

/**
 * Do the "declare" command.
 *
 * @param name TODO
 * @param storage TODO
 * @param left TODO
 * @param right TODO
 * @param type TODO
 */
static void declare_english( char const *name, c_type_t storage_class,
                             c_ast_t *ast ) {
  assert( name );
  assert( ast );
  (void)storage_class;

/*
  if ( c_kind == K_VOID ) {
    illegal( "Variable of type void", "variable of type pointer to void" );
    goto done;
  }

  if ( *storage == K_REFERENCE ) {
    switch ( c_kind ) {
      case K_FUNCTION:
        illegal( "Register function", NULL );
        break;
      case K_ARRAY:
        illegal( "Register array", NULL );
        break;
      case K_ENUM_CLASS_STRUCT_UNION:
        illegal( "Register struct/class", NULL );
        break;
      default:
        goto done;
    } // switch
  }

  if ( *storage )
    printf( "%s ", storage );

  printf(
    "%s %s%s%s", type, left,
    name ? name : (c_kind == K_FUNCTION) ? "f" : "var", right
  );
  if ( opt_make_c ) {
    if ( c_kind == K_FUNCTION && (*storage != 'e') )
      printf( " { }\n" );
    else
      printf( ";\n" );
  } else {
    printf( "\n" );
  }
*/
}

/**
 * Do the "explain" (declaration) command.
 *
 * @param storage TODO
 * @param constvol1 TODO
 * @param constvol2 TODO
 * @param type TODO
 * @param decl TODO
 */
void explain_declaration( char const *storage, char const *constvol1,
                          char const *constvol2, char const *type,
                          char const *decl ) {
  assert( storage );
  assert( constvol1 );
  assert( constvol2 );
  assert( decl );
  (void)type;

#if 0
  if ( type && strcmp( type, "void" ) == 0 ) {
    if ( c_kind == K_NAME )
      illegal( "Variable of type void", "variable of type pointer to void" );
    else if ( c_kind == K_ARRAY )
      illegal( "array of type void", "array of type pointer to void" );
    else if ( c_kind == K_REFERENCE )
      illegal( "reference to type void", "pointer to void" );
  }

  if ( *storage == K_REFERENCE ) {
    switch ( c_kind ) {
      case K_FUNCTION:
        illegal( "Register function", NULL );
        break;
      case K_ARRAY:
        illegal( "Register array", NULL );
        break;
      case K_ENUM_CLASS_STRUCT_UNION:
        illegal( "Register struct/union/enum/class", NULL );
        break;
      default:
        /* suppress warning */;
    } // switch
  }

  printf( "declare %s as ", c_ident );
#endif
}

static void illegal( char const *s, char const *hint ) {
  SGR_START_COLOR( stderr, warning );
  PRINT_ERR( "warning" );
  SGR_END_COLOR( stderr );
  PRINT_ERR( ": \"%s\" illegal in %s", s, lang_name( opt_lang ) );
  if ( hint )
    PRINT_ERR( " (maybe you mean \"%s\")", hint );
  PRINT_ERR( "\n" );
}

static void parse_error( char const *format, ... ) {
  if ( !newlined ) {
    PRINT_ERR( ": " );
    if ( yytext && *yytext )
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

int yywrap( void ) {
  return 1;
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
%type   <ast>       array_decl_english
%type   <number>    array_size_opt_english
%type   <ast>       block_decl_english
%type   <ast>       func_decl_english
%type   <ast>       pointer_decl_english
%type   <ast>       pointer_to_member_decl_english
%type   <ast>       reference_decl_english
%type   <ast>       returning_english
%type   <ast>       var_decl_english

%type   <ast>       cast_c
%type   <ast_list>  cast_list_c cast_list_opt_c
%type   <ast>       array_cast_c
%type   <ast>       block_cast_c
%type   <ast>       func_cast_c
%type   <ast>       pointer_cast_c
%type   <ast>       pointer_to_member_cast_c
%type   <ast>       reference_cast_c

%type   <ast>       decl_c decl2_c
%type   <number>    array_size_c
%type   <ast>       block_decl_c
%type   <ast>       array_decl_c
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

%type   <ast>       type_c
%type   <type>      builtin_type_c
%type   <type>      class_struct_type_c
%type   <type>      enum_class_struct_union_type_c
%type   <type>      storage_class_c storage_class_opt_c
%type   <type>      type_modifier_c
%type   <type>      type_modifier_list_c type_modifier_list_opt_c
%type   <type>      type_qualifier_c
%type   <type>      type_qualifier_list_opt_c

%type   <ast>       type_english
%type   <type>      type_modifier_english
%type   <type>      type_modifier_list_opt_english
%type   <type>      any_type_english

%type   <name>      name_token_opt

/*****************************************************************************/
%%

command_list
  : /* empty */
  | command_list command_init command
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

/*****************************************************************************/
/*  cast                                                                     */
/*****************************************************************************/

cast_english
  : Y_CAST Y_NAME Y_INTO decl_english Y_END
    {
      DUMP_START( "cast_english", "CAST NAME INTO decl_english END" );
      DUMP_NAME( "in:NAME", $2 );
      DUMP_AST( "in:decl_english", $4 );
      DUMP_END();

      cast_english( $2, $4 );
      FREE( $2 );
      c_ast_free( $4 );
    }

  | Y_CAST Y_NAME error
    {
      parse_error( "\"%s\" expected", L_INTO );
    }

  | Y_CAST decl_english Y_END
    {
      DUMP_START( "cast_english", "CAST decl_english END" );
      DUMP_AST( "in:decl_english", $2 );
      DUMP_END();

      cast_english( NULL, $2 );
      c_ast_free( $2 );
    }
  ;

/*****************************************************************************/
/*  declare                                                                  */
/*****************************************************************************/

declare_english
  : Y_DECLARE Y_NAME Y_AS storage_class_opt_c decl_english Y_END
    {
      DUMP_START( "declare_english",
                  "DECLARE NAME AS storage_class_opt_c decl_english END" );
      DUMP_NAME( "in:NAME", $2 );
      DUMP_TYPE( "in:storage_class_opt_c", $4 );
      DUMP_AST( "in:decl_english", $5 );
      DUMP_END();

      declare_english( $2, $4, $5 );
      FREE( $2 );
      c_ast_free( $5 );
    }

  | Y_DECLARE error
    {
      parse_error( "name expected" );
    }

  | Y_DECLARE Y_NAME error
    {
      parse_error( "\"%s\" expected", L_AS );
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
      DUMP_AST( "in:type_c", $2 );
      DUMP_AST( "in:decl_c", $4 );
      DUMP_END();

      FPRINTF( fout, "declare %s as ", c_ast_name( $4 ) );
      c_ast_english( $4, fout );
      FPUTC( '\n', fout );

      c_ast_free( $2 );
      c_ast_free( $4 );
    }
  ;

explain_cast_c
  : Y_EXPLAIN '(' type_c { PUSH_TYPE( $3 ); } cast_c ')'
    name_token_opt Y_END
    {
      POP_TYPE();
      DUMP_START( "explain_cast_t",
                  "EXPLAIN '(' type_c cast_c ')' name_token_opt END" );
      DUMP_AST( "in:cast_c", $5 );
      DUMP_NAME( "in:name_token_opt", $7 );
      DUMP_END();

      if ( $7 )
        FPRINTF( fout, "cast %s into ", $7 );
      else
        FPUTS( "cast into ", fout );
      c_ast_english( $5, fout );
      FPUTC( '\n', fout );

      c_ast_free( $3 );
      c_ast_free( $5 );
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
  | pointer_cast_c
  | pointer_to_member_cast_c
  | reference_cast_c
  ;

cast_list_c
  : cast_list_c expect_comma cast_c
    {
      DUMP_START( "cast_list_c", "cast_lsit_c ',' cast_c" );
      DUMP_AST_LIST( "in:cast_list_c", $1 );
      DUMP_AST( "in:cast_c", $3 );

      $$ = $1;
      assert( $$.tail_ast->next == NULL );
      $$.tail_ast->next = $3;
      $$.tail_ast = $3;

      DUMP_AST_LIST( "out:cast_list_c", $$ );
      DUMP_END();
    }

  | type_c { PUSH_TYPE( $1 ); } cast_c
    {
      POP_TYPE();
      DUMP_START( "cast_list_c", "type_c cast_c" );
      DUMP_AST( "in:type_c", $1 );
      DUMP_AST( "in:cast_c", $3 );

      $$.head_ast = $$.tail_ast = $3;

      DUMP_AST_LIST( "out:cast_list_c", $$ );
      DUMP_END();

      c_ast_free( $1 );
    }

  | Y_NAME
    {
      DUMP_START( "cast_list_c", "NAME" );
      DUMP_NAME( "in:NAME", $1 );

      c_ast_t *const ast = c_ast_new( K_NAME );
      ast->name = $1;
      $$.head_ast = $$.tail_ast = ast;

      DUMP_AST_LIST( "out:cast_list_c", $$ );
      DUMP_END();
    }
  ;

cast_list_opt_c
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | cast_list_c
  ;

array_cast_c
  : cast_c array_size_c
    {
      DUMP_START( "array_cast_c", "cast_c array_cast_c" );
      DUMP_AST( "in:cast_c", $1 );
      DUMP_NUM( "in:array_size_c", $2 );

      $$ = c_ast_new( K_ARRAY );
      $$->as.array.size = $2;
      $$->as.array.of_ast = $1;

      DUMP_AST( "out:array_cast_c", $$ );
      DUMP_END();
    }
  ;

block_cast_c                            /* Apple extension */
  : '(' '^' cast_c ')' '(' cast_list_opt_c ')'
    {
      DUMP_START( "block_cast_c",
                  "'(' '^' cast_c ')' '(' cast_list_opt_c ')'" );
      DUMP_AST( "ia:type_c", PEEK_TYPE() );
      DUMP_AST( "in:cast_c", $3 );
      DUMP_AST_LIST( "in:cast_list_opt_c", $6 );

      $$ = c_ast_new( K_BLOCK );
      $$->name = c_ast_name( $3 );
      $$->as.block.args = $6;
      $$->as.block.ret_ast = PEEK_TYPE();

      DUMP_AST( "out:block_cast_c", $$ );
      DUMP_END();
    }
  ;

func_cast_c
  : '(' ')'
    {
      DUMP_START( "func_cast_c", "'(' ')'" );
      DUMP_AST( "ia:type_c", PEEK_TYPE() );

      $$ = c_ast_new( K_FUNCTION );
      $$->as.func.ret_ast = PEEK_TYPE();

      DUMP_AST( "out:func_cast_c", $$ );
      DUMP_END();
    }

  | '(' cast_c ')'
    {
      DUMP_START( "func_cast_c", "'(' cast_c ')'" );
      DUMP_AST( "in:cast_c", $2 );

      $$ = $2;

      DUMP_AST( "out:func_cast_c", $$ );
      DUMP_END();
    }

  | '(' cast_c ')' '(' cast_list_opt_c ')'
    {
      DUMP_START( "func_cast_c", "'(' cast_c ')' '(' cast_list_opt_c ')'" );
      DUMP_AST( "ia:type_c", PEEK_TYPE() );
      DUMP_AST( "in:cast_c", $2 );
      DUMP_AST_LIST( "in:cast_list_opt_c", $5 );

      $$ = c_ast_new( K_FUNCTION );
      $$->name = c_ast_name( $2 );
      $$->as.func.args = $5;
      $$->as.func.ret_ast = PEEK_TYPE();

      DUMP_AST( "out:func_cast_c", $$ );
      DUMP_END();
    }
  ;

pointer_cast_c
  : '*' cast_c
    {
      DUMP_START( "pointer_cast_c", "'*' cast_c" );
      DUMP_AST( "in:cast_c", $2 );

      $$ = c_ast_new( K_POINTER );
      $$->as.ptr_ref.to_ast = $2;

      DUMP_AST( "out:pointer_cast_c", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_cast_c
  : Y_NAME Y_COLON_COLON expect_star cast_c
    {
      DUMP_START( "pointer_to_member_cast_c", "NAME COLON_COLON '*' cast_c" );
      DUMP_NAME( "in:NAME", $1 );
      DUMP_AST( "in:cast_c", $4 );

      $$ = c_ast_new( K_POINTER_TO_MEMBER );
      $$->as.ptr_mbr.type = T_CLASS;
      $$->as.ptr_mbr.class_name = $1;
      $$->as.ptr_mbr.of_ast = $4;

      DUMP_AST( "out:pointer_to_member_cast_c", $$ );
      DUMP_END();
    }
  ;

reference_cast_c
  : '&' cast_c
    {
      DUMP_START( "reference_cast_c", "'&' cast_c" );
      DUMP_AST( "in:cast_c", $2 );

      $$ = c_ast_new( K_REFERENCE );
      $$->as.ptr_ref.to_ast = $2;

      DUMP_AST( "out:reference_cast_c", $$ );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  declaration english productions                                          */
/*****************************************************************************/

decl_english
  : array_decl_english
  | block_decl_english                  /* Apple extension */
  | func_decl_english
  | pointer_decl_english
  | pointer_to_member_decl_english
  | reference_decl_english
  | type_english
  | var_decl_english
  ;

array_decl_english
  : Y_ARRAY array_size_opt_english Y_OF decl_english
    {
      DUMP_START( "array_decl_english",
                  "ARRAY array_size_opt_english OF decl_english" );
      DUMP_NUM( "in:array_size_opt_english", $2 );
      DUMP_AST( "in:decl_english", $4 );

      switch ( $4->kind ) {
        case K_ARRAY:
          illegal( "inner array of unspecified size", "array of pointer" );
          break;
        case K_BUILTIN:
          if ( $4->as.builtin.type & T_VOID )
            illegal( "array of void", "pointer to void" );
          break;
        case K_FUNCTION:
          illegal( "array of function", "array of pointer to function" );
          break;
        default:
          /* suppress warning */;
      } // switch

      $$ = c_ast_new( K_ARRAY );
      $$->as.array.size = $2;
      $$->as.array.of_ast = $4;

      DUMP_AST( "out:array_decl_english", $$ );
      DUMP_END();
    }
  ;

array_size_opt_english
  : /* empty */                   { $$ = C_ARRAY_NO_SIZE; }
  | Y_NUMBER
  | error
    {
      parse_error( "array size expected" );
    }
  ;

block_decl_english                      /* Apple extension */
  : type_qualifier_list_opt_c
    Y_BLOCK { in_attr.y_token = Y_BLOCK; } decl_list_opt_english
    returning_english
    {
      DUMP_START( "type_qualifier_list_opt_c",
                  "BLOCK decl_list_opt_english returning_english" );
      DUMP_TYPE( "in:type_qualifier_list_opt_c", $1 );
      DUMP_AST_LIST( "in:decl_list_opt_english", $4 );
      DUMP_AST( "in:returning_english", $5 );

      $$ = c_ast_new( K_BLOCK );
      $$->as.block.ret_ast = $5;
      $$->as.block.args = $4;
      $$->as.block.type = $1;

      DUMP_AST( "out:block_decl_english", $$ );
      DUMP_END();
    }
  ;

func_decl_english
  : Y_FUNCTION { in_attr.y_token = Y_FUNCTION; } decl_list_opt_english
    returning_english
    {
      DUMP_START( "func_decl_english",
                  "FUNCTION decl_list_opt_english returning_english" );
      DUMP_AST_LIST( "in:decl_list_opt_english", $3 );
      DUMP_AST( "in:returning_english", $4 );

      $$ = c_ast_new( K_FUNCTION );
      $$->as.func.ret_ast = $4;
      $$->as.func.args = $3;

      DUMP_AST( "out:func_decl_english", $$ );
      DUMP_END();
    }
  ;

decl_list_opt_english
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | '(' decl_list_english ')'
    {
      DUMP_START( "decl_list_opt_english", "'(' decl_list_english ')'" );
      DUMP_AST_LIST( "in:decl_list_english", $2 );

      $$ = $2;

      DUMP_AST_LIST( "out:decl_list_opt_english", $$ );
      DUMP_END();
    }
  ;

decl_list_english
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | decl_list_english expect_comma decl_english
    {
      DUMP_START( "decl_list_english", "decl_list_english ',' decl_english" );
      DUMP_AST_LIST( "in:decl_list_english", $1 );
      DUMP_AST( "in:decl_english", $3 );

      //$$.head_ast = $1.head_ast;
      //$$.tail_ast = $3;
      //assert( $$.tail_ast->next == NULL );
      //$1.tail_ast->next = $3.head_ast;

      DUMP_AST_LIST( "out:decl_list_english", $$ );
      DUMP_END();
    }
  ;

returning_english
  : Y_RETURNING decl_english
    {
      DUMP_START( "returning_english", "RETURNING decl_english" );
      DUMP_AST( "in:decl_english", $2 );

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

        illegal( error_msg, hint_msg );
      }

      $$ = $2;

      DUMP_AST( "out:returning_english", $$ );
      DUMP_END();
    }
  | error { parse_error( "\"%s\" expected", L_RETURNING ); }
  ;

pointer_decl_english
  : type_qualifier_list_opt_c Y_POINTER Y_TO decl_english
    {
      DUMP_START( "pointer_decl_english",
                  "type_qualifier_list_opt_c POINTER TO decl_english" );
      DUMP_TYPE( "in:type_qualifier_list_opt_c", $1 );
      DUMP_AST( "in:decl_english", $4 );

      if ( $4->kind == K_ARRAY )
        illegal( "pointer to array of unspecified dimension",
                 "pointer to object" );
      $$ = c_ast_new( K_POINTER );
      $$->as.ptr_ref.qualifier = $1;
      $$->as.ptr_ref.to_ast = $4;

      DUMP_AST( "out:pointer_decl_english", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_decl_english
  : type_qualifier_list_opt_c Y_POINTER Y_TO Y_MEMBER Y_OF
    class_struct_type_c Y_NAME decl_english
    {
      DUMP_START( "pointer_to_member_decl_english",
                  "type_qualifier_list_opt_c POINTER TO MEMBER OF "
                  "class_struct_type_c NAME decl_english" );
      DUMP_TYPE( "in:type_qualifier_list_opt_c", $1 );
      DUMP_TYPE( "in:class_struct_type_c", $6 );
      DUMP_NAME( "in:NAME", $7 );
      DUMP_AST( "in:decl_english", $8 );

      if ( opt_lang < LANG_CPP_MIN )
        illegal( "pointer to member of class", NULL );
#if 0
      if ( c_kind == K_ARRAY )
        illegal( "pointer to array of unspecified dimension",
                 "pointer to object" );
#endif
      $$ = c_ast_new( K_POINTER_TO_MEMBER );
      $$->as.ptr_mbr.qualifier = $1;
      $$->as.ptr_mbr.type = $6;
      $$->as.ptr_mbr.class_name = $7;
      $$->as.ptr_mbr.of_ast = $8;

      DUMP_AST( "out:pointer_to_member_decl_english", $$ );
      DUMP_END();
    }
  ;

reference_decl_english
  : type_qualifier_list_opt_c Y_REFERENCE Y_TO decl_english
    {
      DUMP_START( "reference_decl_english",
                  "type_qualifier_list_opt_c REFERENCE TO decl_english" );
      DUMP_TYPE( "in:type_qualifier_list_opt_c", $1 );
      DUMP_AST( "in:decl_english", $4 );

      if ( opt_lang < LANG_CPP_MIN )
        illegal( "reference", NULL );
      switch ( $4->kind ) {
        case K_ARRAY:
          illegal( "reference to array of unspecified dimension",
                   "reference to object" );
          break;
        case K_BUILTIN:
          if ( $4->as.builtin.type & T_VOID )
            illegal( "reference of void", "pointer to void" );
          break;
        default:
          ;// suppress warning
      } // switch

      $$ = c_ast_new( K_REFERENCE );
      $$->as.ptr_ref.qualifier = $1;
      $$->as.ptr_ref.to_ast = $4;

      DUMP_AST( "out:reference_decl_english", $$ );
      DUMP_END();
    }
  ;

var_decl_english
  : Y_NAME Y_AS decl_english
    {
      DUMP_START( "var_decl_english", "NAME AS decl_english" );
      DUMP_NAME( "in:NAME", $1 );
      DUMP_AST( "in:decl_english", $3 );

      $$ = $3;
      assert( $$->name == NULL );
      $$->name = $1;

      DUMP_AST( "out:var_decl_english", $$ );
      DUMP_END();
    }

  | Y_NAME
    {
      DUMP_START( "var_decl_english", "NAME" );
      DUMP_NAME( "in:NAME", $1 );

      $$ = c_ast_new( K_NAME );
      $$->name = $1;

      DUMP_AST( "out:var_decl_english", $$ );
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
  : decl2_c array_size_c
    {
      DUMP_START( "array_decl_c", "decl2_c array_size_c" );
      DUMP_AST( "in:decl2_c", $1 );
      DUMP_NUM( "in:array_size_c", $2 );

      $$ = c_ast_new( K_ARRAY );
      $$->name = c_ast_name( $1 );
      $$->as.array.size = $2;
      $$->as.array.of_ast = c_ast_clone( $1 );

      DUMP_AST( "out:array_decl_c", $$ );
      DUMP_END();
    }
  ;

array_size_c
  : '[' ']'                       { $$ = C_ARRAY_NO_SIZE; }
  | '[' Y_NUMBER ']'              { $$ = $2; }
  | '[' error ']'
    {
      parse_error( "integer expected for array size" );
    }
  ;

block_decl_c                            /* Apple extension */
  : /* type_c */ '(' '^' type_qualifier_list_opt_c decl_c ')'
    '(' cast_list_opt_c ')'
    {
      DUMP_START( "block_decl_c",
                  "'(' '^' type_qualifier_list_opt_c decl_c ')' "
                  "'(' cast_list_opt_c ')'" );
      DUMP_AST( "ia:type_c", PEEK_TYPE() );
      DUMP_TYPE( "in:type_qualifier_list_opt_c", $3 );
      DUMP_AST( "in:decl_c", $4 );
      DUMP_AST_LIST( "in:cast_list_opt_c", $7 );

      $$ = c_ast_new( K_BLOCK );
      $$->name = c_ast_name( $4 );
      $$->as.block.args = $7;
      $$->as.block.ret_ast = PEEK_TYPE();
      $$->as.block.type = $3;

      DUMP_AST( "out:block_decl_c", $$ );
      DUMP_END();
    }
  ;

func_decl_c
  : /* type_c */ decl2_c '(' cast_list_opt_c ')'
    {
      DUMP_START( "func_decl_c", "decl2_c '(' cast_list_opt_c ')'" );
      DUMP_AST( "ia:type_c", PEEK_TYPE() );
      DUMP_AST( "in:decl2_c", $1 );
      DUMP_AST_LIST( "in:cast_list_opt_c", $3 );

      $$ = c_ast_new( K_FUNCTION );
      $$->name = c_ast_name( $1 );
      $$->as.func.args = $3;
      $$->as.func.ret_ast = PEEK_TYPE();

      DUMP_AST( "out:func_decl_c", $$ );
      DUMP_END();
    }
  ;

name_decl_c
  : /* type_c */ Y_NAME
    {
      DUMP_START( "name_decl_c", "NAME" );
      DUMP_AST( "ia:type_c", PEEK_TYPE() );
      DUMP_NAME( "in:NAME", $1 );

      $$ = c_ast_clone( PEEK_TYPE() );
      assert( $$->name == NULL );
      $$->name = $1;

      DUMP_AST( "out:name_decl_c", $$ );
      DUMP_END();
    }
  ;

nested_decl_c
  : '(' decl_c ')'
    {
      DUMP_START( "nested_decl_c", "'(' decl_c ')'" );
      DUMP_AST( "in:decl_c", $2 );

      $$ = $2;

      DUMP_AST( "out:nested_decl_c", $$ );
      DUMP_END();
    }
  ;

pointer_decl_c
  : pointer_decl_type_c { PUSH_TYPE( $1 ); } decl_c
    {
      POP_TYPE();
      DUMP_START( "pointer_decl_c", "'*' type_qualifier_list_opt_c decl_c" );
      DUMP_AST( "in:pointer_decl_type_c", $1 );
      DUMP_AST( "in:decl_c", $3 );

      c_ast_free( $1 );
      $$ = $3;

      DUMP_AST( "out:pointer_decl_c", $$ );
      DUMP_END();
    }
  ;

pointer_decl_type_c
  : /* type_c */ '*' type_qualifier_list_opt_c
    {
      DUMP_START( "pointer_decl_type_c", "'*' type_qualifier_list_opt_c" );
      DUMP_AST( "ia:type_c", PEEK_TYPE() );
      DUMP_TYPE( "in:type_qualifier_list_opt_c", $2 );

      $$ = c_ast_new( K_POINTER );
      $$->as.ptr_ref.qualifier = $2;
      $$->as.ptr_ref.to_ast = PEEK_TYPE();

      DUMP_AST( "out:pointer_decl_type_c", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_decl_c
  : pointer_to_member_decl_type_c { PUSH_TYPE( $1 ); } decl_c
    {
      POP_TYPE();
      DUMP_START( "pointer_to_member_decl_c",
                  "pointer_to_member_decl_type_c decl_c" );
      DUMP_AST( "in:pointer_to_member_decl_type_c", $1 );
      DUMP_AST( "in:decl_c", $3 );

      c_ast_free( $1 );
      $$ = $3;

      DUMP_AST( "out:pointer_to_member_decl_c", $$ );
      DUMP_END();
    }
  ;

pointer_to_member_decl_type_c
  : /* type_c */ Y_NAME Y_COLON_COLON expect_star
    {
      DUMP_START( "pointer_to_member_decl_type_c", "NAME COLON_COLON '*'" );
      DUMP_AST( "ia:type_c", PEEK_TYPE() );
      DUMP_NAME( "in:NAME", $1 );

      $$ = c_ast_new( K_POINTER_TO_MEMBER );
      $$->as.ptr_mbr.class_name = $1;
      $$->as.ptr_mbr.type = T_CLASS;
      $$->as.ptr_mbr.of_ast = PEEK_TYPE();

      DUMP_AST( "out:pointer_to_member_decl_type_c", $$ );
      DUMP_END();
    }
  ;

reference_decl_c
  : reference_decl_type_c { PUSH_TYPE( $1 ); } decl_c
    {
      POP_TYPE();
      DUMP_START( "reference_decl_c", "'&' type_qualifier_list_opt_c decl_c" );
      DUMP_AST( "in:reference_decl_type_c", $1 );
      DUMP_AST( "in:decl_c", $3 );

      c_ast_free( $1 );
      $$ = $3;

      DUMP_AST( "out:reference_decl_c", $$ );
      DUMP_END();
    }
  ;

reference_decl_type_c
  : /* type_c */ '&' type_qualifier_list_opt_c
    {
      DUMP_START( "reference_decl_type_c", "'&' type_qualifier_list_opt_c" );
      DUMP_AST( "ia:type_c", PEEK_TYPE() );
      DUMP_TYPE( "in:type_qualifier_list_opt_c", $2 );

      $$ = c_ast_new( K_REFERENCE );
      $$->as.ptr_ref.qualifier = $2;
      $$->as.ptr_ref.to_ast = PEEK_TYPE();

      DUMP_AST( "out:reference_decl_type_c", $$ );
      DUMP_END();
    }
  ;

/*****************************************************************************/
/*  type english productions                                                 */
/*****************************************************************************/

type_english
  : type_qualifier_list_opt_c type_modifier_list_opt_english any_type_english
    {
      DUMP_START( "type_english",
                  "type_qualifier_list_opt_c type_modifier_list_opt_english "
                  "any_type_english" );
      DUMP_TYPE( "in:type_qualifier_list_opt_c", $1 );
      DUMP_TYPE( "in:type_modifier_list_opt_english", $2 );
      DUMP_TYPE( "in:any_type_english", $3 );

      $$ = c_ast_new( $3 );
      $$->as.builtin.type = $3;
      c_type_add( &$$->as.builtin.type, $1 );
      c_type_add( &$$->as.builtin.type, $2 );

      DUMP_AST( "out:type_english", $$ );
      DUMP_END();
    }
  ;

type_modifier_list_opt_english
  : /* empty */                   { $$ = T_NONE; }
  | type_modifier_list_opt_english type_modifier_english
    {
      DUMP_START( "type_modifier_list_opt_english",
                  "type_modifier_list_opt_english type_modifier_english" );
      DUMP_TYPE( "in:type_modifier_list_opt_english", $1 );
      DUMP_TYPE( "in:type_modifier_english", $2 );

      $$ = $1;
      c_type_add( &$$, $1 );

      DUMP_TYPE( "out:type_modifier_list_opt_english", $$ );
      DUMP_END();
    }
  ;

type_modifier_english
  : Y_COMPLEX
  | Y_LONG
  | Y_SHORT
  | Y_SIGNED
  | Y_UNSIGNED
  ;

any_type_english
  : /* empty */                   { $$ = T_NONE; }
  | builtin_type_c
  | enum_class_struct_union_type_c
  ;

/*****************************************************************************/
/*  type gibberish productions                                               */
/*****************************************************************************/

type_c
  : type_modifier_list_c
    {
      DUMP_START( "type_c", "type_modifier_list_c" );
      DUMP_TYPE( "in:type_modifier_list_c", $1 );

      $$ = c_ast_new( K_BUILTIN );
      $$->as.builtin.type = $1;
      c_type_check( $$->as.builtin.type );

      DUMP_AST( "out:type_c", $$ );
      DUMP_END();
    }

  | type_modifier_list_c builtin_type_c type_modifier_list_opt_c
    {
      DUMP_START( "type_c",
                  "type_modifier_list_c builtin_type_c "
                  "type_modifier_list_opt_c" );
      DUMP_TYPE( "in:type_modifier_list_c", $1 );
      DUMP_TYPE( "in:builtin_type_c", $2 );
      DUMP_TYPE( "in:type_modifier_list_opt_c", $3 );

      $$ = c_ast_new( K_BUILTIN );
      $$->as.builtin.type = $1;
      c_type_add( &$$->as.builtin.type, $2 );
      c_type_add( &$$->as.builtin.type, $3 );
      c_type_check( $$->as.builtin.type );

      DUMP_AST( "out:type_c", $$ );
      DUMP_END();
    }

  | builtin_type_c type_modifier_list_opt_c
    {
      DUMP_START( "type_c", "builtin_type_c type_modifier_list_opt_c" );
      DUMP_TYPE( "in:builtin_type_c", $1 );
      DUMP_TYPE( "in:type_modifier_list_opt_c", $2 );

      $$ = c_ast_new( K_BUILTIN );
      $$->as.builtin.type = $1;
      c_type_check( $$->as.builtin.type );

      DUMP_AST( "out:type_c", $$ );
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
      DUMP_START( "type_modifier_list_c", "type_modifier_list_c type_modifier_c" );
      DUMP_TYPE( "in:type_modifier_list_c", $1 );
      DUMP_TYPE( "in:type_modifier_c", $2 );

      $$ = $1;
      c_type_add( &$$, $2 );

      DUMP_TYPE( "out:type_modifier_list_c", $$ );
      DUMP_END();
    }
  | type_modifier_c
  ;

type_modifier_c
  : Y_COMPLEX
  | Y_LONG
  | Y_SHORT
  | Y_SIGNED
  | Y_UNSIGNED
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
      DUMP_TYPE( "in:enum_class_struct_union_type_c", $1 );
      DUMP_NAME( "in:NAME", $2 );

      $$ = c_ast_new( K_ENUM_CLASS_STRUCT_UNION );
      $$->name = $2;
      $$->as.ecsu.type = $1;
      c_type_check( $$->as.ecsu.type );

      DUMP_AST( "out:named_enum_class_struct_union_type_c", $$ );
      DUMP_END();
    }
  | enum_class_struct_union_type_c error
    {
      parse_error(
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
      DUMP_TYPE( "in:type_qualifier_list_opt_c", $1 );
      DUMP_TYPE( "in:type_qualifier_c", $2 );

      $$ = $1;
      c_type_add( &$$, $2 );

      DUMP_TYPE( "out:type_qualifier_list_opt_c", $$ );
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
  | error                         { parse_error( "',' expected" ); }
  ;

expect_star
  : '*'
  | error                         { parse_error( "'*' expected" ); }
  ;

name_token_opt
  : /* empty */                   { $$ = NULL; }
  | Y_NAME
  ;

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
