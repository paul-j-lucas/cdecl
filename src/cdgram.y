/*
**      cdecl -- C gibberish translator
**      src/cdgram.y
*/

%{
// local
#include "config.h"                     /* must cone first */
#include "ast.h"
#include "keywords.h"
#include "lang.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WITH_CDECL_DEBUG
#define CDEBUG(...) \
  BLOCK( if ( opt_debug ) __VA_ARGS__; )
#else
#define CDEBUG(...)                     /* nothing */
#endif /* WITH_CDECL_DEBUG */

///////////////////////////////////////////////////////////////////////////////

/**
 * Inherited attribute.
 */
union in_attr {
  int y_token;
};
typedef union in_attr in_attr_t;

// external variables
extern char const  *prompt;
extern char         prompt_buf[];
extern char const  *yytext;

// extern functions
extern void         print_help( void );
extern int          yylex( void );

// local variables
static in_attr_t    in_attr;
static bool         newlined = true;

// local functions
static void         illegal( lang_t, char const*, char const* );
static void         unsupp( char const*, char const* );

///////////////////////////////////////////////////////////////////////////////

#if 0
/**
 * TODO
 *
 * @param type TODO
 */
static void c_type_add( c_type_t type ) {
  if ( type == T_LONG && (_type & T_LONG) ) {
    //
    // TODO
    //
    type = T_LONG_LONG;
  }

  if ( !(c_type & type) ) {
    c_type |= type;
  } else {
    PRINT_ERR(
      "error: \"%s\" can not be combined with previous declaration\n",
      c_type_name( type )
    );
  }
}
#endif

/**
 * Do the "cast" command.
 *
 * @param name TODO
 * @param ast TODO
 */
static void do_cast( char const *name, c_ast_t *ast ) {
  assert( ast );

  switch ( ast->kind ) {
    case K_ARRAY:
      unsupp( "cast into array", "cast into pointer" );
      break;
    case K_FUNCTION:
      unsupp( "cast into function", "cast into pointer to function" );
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

  FREE( name );
  c_ast_free( ast );
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
static void do_declare( char const *name, c_ast_t *ast ) {
  assert( ast );

/*
  if ( c_kind == K_VOID ) {
    unsupp( "Variable of type void", "variable of type pointer to void" );
    goto done;
  }

  if ( *storage == K_REFERENCE ) {
    switch ( c_kind ) {
      case K_FUNCTION:
        unsupp( "Register function", NULL );
        break;
      case K_ARRAY:
        unsupp( "Register array", NULL );
        break;
      case K_ENUM_CLASS_STRUCT_UNION:
        unsupp( "Register struct/class", NULL );
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

  FREE( name );
  FREE( ast );
}

/**
 * Do the "set" command.
 *
 * @param opt The option to set.
 */
static void do_set( char const *opt ) {
  if ( strcmp( opt, "create" ) == 0 )
    opt_make_c = true;
  else if ( strcmp( opt, "nocreate" ) == 0 )
    opt_make_c = false;

  else if ( strcmp( opt, "prompt" ) == 0 )
    prompt = prompt_buf;
  else if ( strcmp( opt, "noprompt" ) == 0 )
    prompt = "";

  else if ( strcmp( opt, "preansi" ) == 0 || strcmp( opt, "knr" ) == 0 )
    opt_lang = LANG_C_KNR;
  else if ( strcmp( opt, "ansi" ) == 0 )
    opt_lang = LANG_C_89;
  else if ( strcmp( opt, "c++" ) == 0 )
    opt_lang = LANG_CPP;
  else if ( strcmp( opt, "c++11" ) == 0 )
    opt_lang = LANG_CPP_11;

#ifdef WITH_CDECL_DEBUG
  else if ( strcmp( opt, "debug" ) == 0 )
    opt_debug = true;
  else if ( strcmp( opt, "nodebug" ) == 0 )
    opt_debug = false;
#endif /* WITH_CDECL_DEBUG */

#ifdef YYDEBUG
  else if ( strcmp( opt, "yydebug" ) == 0 )
    yydebug = true;
  else if ( strcmp( opt, "noyydebug" ) == 0 )
    yydebug = false;
#endif /* YYDEBUG */

  else {
    if ( strcmp( opt, "options" ) != 0 ) {
      printf( "\"%s\": unknown set option\n", opt );
    }
    printf( "Valid set options (and command line equivalents) are:\n" );
    printf( "  options\n" );
    printf( "  create (-c), nocreate\n" );
    printf( "  prompt, noprompt (-q)\n" );
#ifndef WITH_READLINE
    printf( "  interactive (-i), nointeractive\n" );
#endif /* WITH_READLINE */
    printf( "  preansi (-p), ansi (-a), or cplusplus (-+)\n" );
#ifdef WITH_CDECL_DEBUG
    printf( "  debug (-d), nodebug\n" );
#endif /* WITH_CDECL_DEBUG */
#ifdef YYDEBUG
    printf( "  yydebug (-D), noyydebug\n" );
#endif /* YYDEBUG */
    printf( "\nCurrent set values are:\n" );
    printf( "  %screate\n", opt_make_c ? "   " : " no" );
    printf( "  %sinteractive\n", opt_interactive ? "   " : " no" );
    printf( "  %sprompt\n", prompt[0] ? "   " : " no" );
    printf( "  lang=%s\n", lang_name( opt_lang ) );
#ifdef WITH_CDECL_DEBUG
    printf( "  %sdebug\n", opt_debug ? "   " : " no" );
#endif /* WITH_CDECL_DEBUG */
#ifdef YYDEBUG
    printf( "  %syydebug\n", yydebug ? "   " : " no" );
#endif /* YYDEBUG */
  }
}

/**
 * Do the "explain cast" command.
 *
 * @param qualifier1 TODO
 * @param qualifier2 TODO
 * @param type TODO
 * @param cast TODO
 * @param name TODO
 */
static void explain_cast( char const *qualifier1,
                          char const *qualifier2,
                          char const *type, char const *cast,
                          char const *name ) {
  assert( qualifier1 );
  assert( qualifier2 );
  assert( type );
  assert( cast );
  assert( name );

  if ( strcmp( type, "void" ) == 0 ) {
  /*
    if ( c_kind == K_ARRAY )
      unsupp( "array of type void", "array of type pointer to void" );
    else if ( c_kind == K_REFERENCE )
      unsupp( "reference to type void", "pointer to void" );
  */
  }
  printf( "cast %s into %s", name, cast );
  if ( *qualifier1 )
    printf( "%s ", qualifier1 );
  if ( *qualifier2 )
    printf( "%s ", qualifier2 );
  printf( "%s\n", type );
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

#if 0
  if ( type && strcmp( type, "void" ) == 0 ) {
    if ( c_kind == K_NAME )
      unsupp( "Variable of type void", "variable of type pointer to void" );
    else if ( c_kind == K_ARRAY )
      unsupp( "array of type void", "array of type pointer to void" );
    else if ( c_kind == K_REFERENCE )
      unsupp( "reference to type void", "pointer to void" );
  }

  if ( *storage == K_REFERENCE ) {
    switch ( c_kind ) {
      case K_FUNCTION:
        unsupp( "Register function", NULL );
        break;
      case K_ARRAY:
        unsupp( "Register array", NULL );
        break;
      case K_ENUM_CLASS_STRUCT_UNION:
        unsupp( "Register struct/union/enum/class", NULL );
        break;
      default:
        /* suppress warning */;
    } // switch
  }

  printf( "declare %s as ", c_ident );
#endif
}

/**
 * TODO
 *
 * @param lang TODO
 * @param type1 TODO
 * @param type2 TODO
 */
static void illegal( lang_t lang, char const *type1, char const *type2 ) {
  if ( type2 )
    PRINT_ERR(
      "warning: \"%s\" with \"%s\" illegal in %s\n",
      type1, type2, lang_name( lang )
    );
  else
    PRINT_ERR(
      "warning: \"%s\" illegal in %s\n",
      type1, lang_name( lang )
    );
}

static void parse_error( char const *what, char const *msg ) {
  if ( !newlined ) {
    if ( what && *what )
      PRINT_ERR( "\"%s\": ", what );
    PRINT_ERR( "%s\n", msg );
    newlined = true;
  }
}

static void unsupp( char const *s, char const *hint ) {
  illegal( opt_lang, s, NULL );
  if ( hint )
    PRINT_ERR( "\t(maybe you mean \"%s\")\n", hint );
}

static void yyerror( char const *s ) {
  PRINT_ERR( "%s%s\n", (newlined ? "" : "\n"), s );
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
%type   <ast_list>  decl_list_english
%type   <ast_list>  decl_list_opt_english
%type   <ast>       array_decl_english
%type   <number>    array_size_opt_english
%type   <ast>       block_decl_english
%type   <ast>       func_decl_english
%type   <ast>       pointer_english
%type   <ast>       pointer_to_member_english
%type   <ast>       reference_english
%type   <ast>       returning_english
%type   <ast>       var_decl_english

%type   <ast>       cast_c
%type   <ast_list>  cast_list_c
%type   <ast>       array_cast_c
%type   <ast>       block_cast_c
%type   <ast>       func_cast_c
%type   <ast>       ordinary_cast_c
%type   <ast>       pointer_cast_c
%type   <ast>       pointer_to_member_cast_c
%type   <ast>       reference_cast_c

%type   <ast>       decl_c decl2_c
%type   <number>    array_decl_c
%type   <ast>       block_decl_c
%type   <ast>       func_decl_c
%type   <ast>       pointer_decl_c
%type   <ast>       pointer_to_member_decl_c
%type   <ast>       reference_decl_c

%type   <ast>       type_c
%type   <type>      builtin_type_c
%type   <type>      class_struct_type_c
%type   <type>      enum_class_struct_union_type_c
%type   <type>      storage_class_c storage_class_opt_c
%type   <type>      type_modifier_c
%type   <type>      type_modifier_list_c type_modifier_list2_c
%type   <type>      type_qualifier_c
%type   <type>      type_qualifier_list_c type_qualifier_list_opt_c

%type   <name>      name_token_opt

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
  | explain_gibberish
  | help_command
  | set_command
  | quit_command
  | Y_END
  | error Y_END
    {
      parse_error( yytext, ": one of \"cast\", \"declare\", \"explain\", \"help\", or \"set\" expected" );
      yyerrok;
    }
  ;

/*****************************************************************************/
/*  cast                                                                     */
/*****************************************************************************/

cast_english
  : Y_CAST Y_NAME Y_INTO decl_english Y_END
    {
      do_cast( $2, $4 );
    }

  | Y_CAST decl_english Y_END
    {
      do_cast( NULL, $2 );
    }
  ;

/*****************************************************************************/
/*  declare                                                                  */
/*****************************************************************************/

declare_english
  : Y_DECLARE Y_NAME Y_AS storage_class_opt_c decl_english Y_END
    {
      //do_declare( $2, $4 );
      FREE( $2 );
    }
  ;

/*****************************************************************************/
/*  explain                                                                  */
/*****************************************************************************/

explain_gibberish
  : Y_EXPLAIN storage_class_opt_c
    type_qualifier_list_opt_c type_c type_qualifier_list_opt_c decl_c Y_END
    {
      FPRINTF( fout, "EXPLAIN 1\n" );
      $4->as.type |= $2 | $3 | $5;
      c_type_check( $4->as.type );

      CDEBUG( c_ast_json( $4, "type_c", fout ); );
      CDEBUG( c_ast_json( $6, "decl_c", fout ); );

      FPRINTF( fout, "declare %s as ", c_ast_name( $6 ) );
      if ( $6 ) { c_ast_english( $6, fout ); }
      if ( $4 ) { c_ast_english( $4, fout ); }
      FPUTC( '\n', fout );

      c_ast_free( $4 );
      c_ast_free( $6 );
    }

  | Y_EXPLAIN storage_class_c type_qualifier_list_opt_c decl_c Y_END
    {
      FPRINTF( fout, "EXPLAIN 2\n" );
      $4->as.type |= $2 | $3;
      c_type_check( $4->as.type );
      CDEBUG( c_ast_json( $4, "decl_c", fout ); );
      FPRINTF( fout, "declare %s as ", c_ast_name( $4 ) );
      if ( $4 ) { c_ast_english( $4, fout ); }
      FPUTC( '\n', fout );
      c_ast_free( $4 );
    }

  | Y_EXPLAIN storage_class_opt_c type_qualifier_list_c decl_c Y_END
    {
      FPRINTF( fout, "EXPLAIN 3\n" );
      $4->as.type |= $2 | $3;
      c_type_check( $4->as.type );
      CDEBUG( c_ast_json( $4, "decl_c", fout ); );
      FPRINTF( fout, "declare %s as ", c_ast_name( $4 ) );
      if ( $4 ) { c_ast_english( $4, fout ); }
      FPUTC( '\n', fout );
      c_ast_free( $4 );
    }

  | Y_EXPLAIN '(' type_qualifier_list_opt_c type_c type_qualifier_list_opt_c
    cast_c ')' name_token_opt Y_END
    {
      FPRINTF( fout, "EXPLAIN 4\n" );
      $4->as.type |= $3 | $5;
      c_type_check( $4->as.type );
      //CDEBUG( c_ast_json( $4, "cast_c", fout ); );
      if ( $4 ) { c_ast_english( $4, fout ); }
      if ( $6 ) { c_ast_english( $6, fout ); }
      if ( $8 ) printf( "%s\n", $8 );
      FPUTC( '\n', fout );

      c_ast_free( $4 );
      c_ast_free( $6 );
      FREE( $8 );
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
  : Y_SET name_token_opt Y_END    { do_set( $2 ); FREE( $2 ); }
  ;

/*****************************************************************************/
/*  quit                                                                     */
/*****************************************************************************/

quit_command
  : Y_QUIT                        { exit( EX_OK ); }
  ;

/*****************************************************************************/
/*  english productions                                                      */
/*****************************************************************************/

decl_english
  : array_decl_english
  | block_decl_english                  /* Apple extension */
  | func_decl_english
  | pointer_english
  | pointer_to_member_english
  | reference_english
  | type_qualifier_list_opt_c type_c
    {
      $$ = $2;
      $$->as.type |= $1;
    }
  | var_decl_english
  ;

decl_list_english
  : /* empty */
    {
      $$.head_ast = $$.tail_ast = NULL;
    }

  | decl_list_english ',' decl_english
    {
      //$$.head_ast = $1.head_ast;
      //$$.tail_ast = $3.tail_ast;
      //$1.tail_ast->next = $3.head_ast;
    }

  | decl_english
    {
      $$.head_ast = $$.tail_ast = $1;
    }
  ;

array_decl_english
  : Y_ARRAY array_size_opt_english Y_OF decl_english
    {
      switch ( $4->kind ) {
        case K_ARRAY:
          unsupp( "Inner array of unspecified size", "array of pointer" );
          break;
        case K_BUILTIN:
          if ( $4->as.type & T_VOID )
            unsupp( "array of void", "pointer to void" );
          break;
        case K_FUNCTION:
          unsupp( "array of function", "array of pointer to function" );
          break;
        default:
          /* suppress warning */;
      } // switch

      $$ = c_ast_new( K_ARRAY );
      $$->as.array.size = $2;
      $$->as.array.of_ast = $4;
    }
  ;

array_size_opt_english
  : /* empty */                   { $$ = C_ARRAY_NO_SIZE; }
  | Y_NUMBER
  ;

block_decl_english
  : type_qualifier_list_opt_c
    Y_BLOCK { in_attr.y_token = Y_BLOCK; } decl_list_opt_english
    returning_english
    {
      $$ = c_ast_new( K_BLOCK );
      $$->as.block.ret_ast = $5;
      $$->as.block.args = $4;
      $$->as.block.type = $1;
    }
  ;

func_decl_english
  : Y_FUNCTION { in_attr.y_token = Y_FUNCTION; } decl_list_opt_english
    returning_english
    {
      $$ = c_ast_new( K_FUNCTION );
      $$->as.func.ret_ast = $4;
      $$->as.func.args = $3;
    }
  ;

decl_list_opt_english
  : /* empty */                   { $$.head_ast = $$.tail_ast = NULL; }
  | '(' decl_list_english ')'     { $$ = $2; }
  ;

returning_english
  : Y_RETURNING decl_english
    {
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

        unsupp( error_msg, hint_msg );
      }

      $$ = $2;
    }
  ;

pointer_english
  : type_qualifier_list_opt_c Y_POINTER Y_TO decl_english
    {
      if ( $4->kind == K_ARRAY )
        unsupp( "pointer to array of unspecified dimension",
                "pointer to object" );
      $$ = c_ast_new( K_POINTER );
      $$->as.ptr_ref.qualifier = $1;
      $$->as.ptr_ref.to_ast = $4;
    }
  ;

pointer_to_member_english
  : type_qualifier_list_opt_c Y_POINTER Y_TO Y_MEMBER Y_OF
    class_struct_type_c Y_NAME decl_english
    {
      if ( opt_lang != LANG_CPP )
        unsupp( "pointer to member of class", NULL );
/*
      if ( c_kind == K_ARRAY )
        unsupp( "pointer to array of unspecified dimension",
                "pointer to object" );
*/
      $$ = c_ast_new( K_PTR_TO_MEMBER );
      $$->as.ptr_mbr.qualifier = $1;
      $$->as.ptr_mbr.class_name = $7;
      $$->as.ptr_mbr.of_ast = $8;
    }
  ;

reference_english
  : type_qualifier_list_opt_c Y_REFERENCE Y_TO decl_english
    {
      if ( opt_lang != LANG_CPP )
        unsupp( "reference", NULL );
      switch ( $4->kind ) {
        case K_ARRAY:
          unsupp( "reference to array of unspecified dimension",
                  "reference to object" );
          break;
        case K_BUILTIN:
          if ( $4->as.type & T_VOID )
            unsupp( "reference of void", "pointer to void" );
          break;
        default:
          /* suppress warning */;
      } // switch

      $$ = c_ast_new( K_REFERENCE );
      $$->as.ptr_ref.qualifier = $1;
      $$->as.ptr_ref.to_ast = $4;
    }
  ;

var_decl_english
  : Y_NAME Y_AS decl_english
    {
      $$ = $3;
      assert( $$->name == NULL );
      $$->name = $1;
    }

  | Y_NAME
    {
      $$ = c_ast_new( K_NAME );
      $$->name = $1;
    }
  ;

/*****************************************************************************/
/*  cast gibberish productions                                               */
/*****************************************************************************/

cast_list_c
  : cast_list_c ',' cast_list_c
    {
      $$.head_ast = $1.head_ast;
      $$.tail_ast = $3.tail_ast;
      $1.tail_ast->next = $3.head_ast;
    }

  | type_qualifier_list_opt_c type_c cast_c
    {
    }

  | Y_NAME
    {
      c_ast_t *const ast = c_ast_new( K_NAME );
      ast->name = $1;
      $$.head_ast = $$.tail_ast = ast;
    }
  ;

cast_c
  : /* empty */                   { $$ = NULL; }
  | array_cast_c
  | block_cast_c                        /* Apple extension */
  | func_cast_c
  | ordinary_cast_c
  | pointer_cast_c
  | pointer_to_member_cast_c
  | reference_cast_c
  ;

array_cast_c
  : cast_c array_decl_c
    {
      $$ = c_ast_new( K_ARRAY );
      $$->as.array.size = $2;
      $$->as.array.of_ast = $1;
    }
  ;

block_cast_c
  : '(' '^' cast_c ')' '(' ')'
    { // block returning
    }

  | '(' '^' cast_c ')' '(' cast_list_c ')'
    { // block returning
    }
  ;

func_cast_c
  : '(' ')'
    { // function returning
    }

  | '(' cast_c ')' '(' ')'
    { // function returning
    }

  | '(' cast_c ')' '(' cast_list_c ')'
    { // function returning
    }
  ;

ordinary_cast_c
  : '(' cast_c ')'                { $$ = $2; }
  ;

pointer_cast_c
  : '*' cast_c
    {
      $$ = c_ast_new( K_POINTER );
      $$->as.ptr_ref.to_ast = $2;
    }
  ;

pointer_to_member_cast_c
  : Y_NAME Y_COLON_COLON '*' cast_c
    {
      $$ = c_ast_new( K_PTR_TO_MEMBER );
      $$->as.ptr_mbr.class_name = $1;
      $$->as.ptr_mbr.of_ast = $4;
    }
  ;

reference_cast_c
  : '&' cast_c
    {
      $$ = c_ast_new( K_REFERENCE );
      $$->as.ptr_ref.to_ast = $2;
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
  : block_decl_c
  | func_decl_c
  | decl2_c array_decl_c
    {
      $$ = c_ast_new( K_ARRAY );
      $$->as.array.size = $2;
      $$->as.array.of_ast = $1;
    }

  | '(' decl_c ')'                { $$ = $2; }

  | Y_NAME
    {
      $$ = c_ast_new( K_NAME );
      $$->name = $1;
    }
  ;

array_decl_c
  : '[' ']'                       { $$ = C_ARRAY_NO_SIZE; }
  | '[' Y_NUMBER ']'              { $$ = $2; }
  ;

block_decl_c
  : '(' '^' type_qualifier_list_opt_c decl_c ')' '(' ')'
    { // block returning ...
    }

  | '(' '^' type_qualifier_list_opt_c decl_c ')' '(' cast_list_c ')'
    { // block(args) returning
    }
  ;

func_decl_c
  : decl2_c '(' ')'
    {
      $$ = c_ast_new( K_FUNCTION );
      $$->as.func.ret_ast = $1;
      $$->as.func.args.head_ast = NULL;
      $$->as.func.args.tail_ast = NULL;
    }

  | decl2_c '(' cast_list_c ')'
    {
      $$ = c_ast_new( K_FUNCTION );
      $$->as.func.ret_ast = $1;
      $$->as.func.args = $3;
    }
  ;

pointer_decl_c
  : '*' type_qualifier_list_opt_c decl_c
    {
      $$ = c_ast_new( K_POINTER );
      $$->as.ptr_ref.qualifier = $2;
      $$->as.ptr_ref.to_ast = $3;
    }
  ;

pointer_to_member_decl_c
  : Y_NAME Y_COLON_COLON '*' decl_c
    {
      $$ = c_ast_new( K_PTR_TO_MEMBER );
      $$->as.ptr_mbr.class_name = $1;
      $$->as.ptr_mbr.of_ast = $4;
    }
  ;

reference_decl_c
  : '&' type_qualifier_list_opt_c decl_c
    {
      $$ = c_ast_new( K_REFERENCE );
      $$->as.ptr_ref.qualifier = $2;
      $$->as.ptr_ref.to_ast = $3;
    }
  ;

/*****************************************************************************/
/*  type gibberish productions                                               */
/*****************************************************************************/

type_c
  : type_modifier_list_c
    {
      $$ = c_ast_new( K_BUILTIN );
      $$->as.type = $1;
    }
  | type_modifier_list_c builtin_type_c
    {
      $$ = c_ast_new( K_BUILTIN );
      $$->as.type = $1 | $2;
    }
  | builtin_type_c
    {
      $$ = c_ast_new( K_BUILTIN );
      $$->as.type = $1;
    }
  | enum_class_struct_union_type_c Y_NAME
    {
      $$ = c_ast_new( K_ENUM_CLASS_STRUCT_UNION );
      $$->name = $2;
      $$->as.type = $1;
    }
  ;

type_modifier_list_c
  : type_modifier_c type_modifier_list2_c
    {
      $$ = $1 | $2;
    }
  | type_modifier_c
  ;

type_modifier_list2_c
  : type_modifier_list_c
  | type_qualifier_c
  ;

type_modifier_c
  : Y_COMPLEX
  | Y_LONG
  | Y_SHORT
  | Y_SIGNED
  | Y_UNSIGNED
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

enum_class_struct_union_type_c
  : Y_ENUM
  | class_struct_type_c
  | Y_UNION
  ;

class_struct_type_c
  : Y_CLASS
  | Y_STRUCT
  ;

type_qualifier_c
  : Y_CONST
  | Y_RESTRICT
  | Y_VOLATILE
  ;

type_qualifier_list_opt_c
  : /* empty */                   { $$ = T_NONE; }
  | type_qualifier_list_c
  ;

type_qualifier_list_c
  : type_qualifier_c type_qualifier_list_opt_c
    {
      $$ = $1 | $2;
    }
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
  | Y_THREAD_LOCAL
  ;

/*****************************************************************************/
/*  miscellaneous gibberish productions                                      */
/*****************************************************************************/

name_token_opt
  : /* empty */                   { $$ = NULL; }
  | Y_NAME
  ;

%%

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
