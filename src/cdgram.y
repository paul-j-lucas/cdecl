%{
/*
**    cdecl -- C gibberish translator
**    src/cdgram.y
*/

// local
#include "config.h"
#include "options.h"
#include "util.h"

// standard
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WITH_CDECL_DEBUG 1

#ifdef WITH_CDECL_DEBUG
#define YYTRACE(...) \
  BLOCK( if ( opt_debug ) PRINT_ERR( "|" __VA_ARGS__ ); )
#else
#define YYTRACE(...)              /* nothing */
#endif /* WITH_CDECL_DEBUG */

extern char const   unknown_name[];
extern bool         opt_debug;
extern bool         prompting;
extern char         prompt_buf[];
extern char const  *prompt_ptr;

// extern functions
extern int    yylex( void );

/**
 * The kinds of C identifiers.
 */
enum c_ident_kind {
  C_NONE          = '0',
  C_ARRAY_NO_DIM  = 'a',
  C_ARRAY_DIM     = 'A',
  C_BLOCK         = '^',                // Apple
  C_BUILTIN       = 't',                // char, int, etc.
  C_FUNCTION      = 'f',
  C_NAME          = 'n',
  C_POINTER       = '*',
  CXX_REFERENCE   = '&',
  C_STRUCT        = 's',                // or C++ class
  C_VOID          = 'v',
};
typedef enum c_ident_kind c_ident_kind_t;

typedef unsigned c_type_bits_t;

static c_type_bits_t const C_TYPE_VOID     = 0x0001;
static c_type_bits_t const C_TYPE_BOOL     = 0x0002;
static c_type_bits_t const C_TYPE_CHAR     = 0x0004;
static c_type_bits_t const C_TYPE_WCHAR_T  = 0x0008;
static c_type_bits_t const C_TYPE_SHORT    = 0x0010;
static c_type_bits_t const C_TYPE_INT      = 0x0020;
static c_type_bits_t const C_TYPE_LONG     = 0x0040;
static c_type_bits_t const C_TYPE_SIGNED   = 0x0080;
static c_type_bits_t const C_TYPE_UNSIGNED = 0x0100;
static c_type_bits_t const C_TYPE_FLOAT    = 0x0200;
static c_type_bits_t const C_TYPE_DOUBLE   = 0x0400;

// local variables
static bool           array_has_dim;
static c_type_bits_t  c_type_bits;
static c_ident_kind_t c_ident_kind;
static char const    *c_ident;

// local functions
static void   not_supported( char const*, char const*, char const* );
static void   unsupp( char const*, char const* );

///////////////////////////////////////////////////////////////////////////////

#if 0
char const crosscheck_old[9][9] = {
  /*              L, I, S, C, V, U, S, F, D, */
  /* long */      _, _, _, _, _, _, _, _, _,
  /* int */       _, _, _, _, _, _, _, _, _,
  /* short */     X, _, _, _, _, _, _, _, _,
  /* char */      X, X, X, _, _, _, _, _, _,
  /* void */      X, X, X, X, _, _, _, _, _,
  /* unsigned */  R, _, R, R, X, _, _, _, _,
  /* signed */    K, K, K, K, X, X, _, _, _,
  /* float */     A, X, X, X, X, X, X, _, _,
  /* double */    K, X, X, X, X, X, X, X, _
};
#endif

/**
 * TODO
 *
 * @param bit TODO
 */
static void c_type_add( c_type_bits_t bit ) {
  if ( c_type_bits & bit )
    /* complain */;
  else
    c_type_bits |= bit;
}

/**
 * TODO
 */
static void c_type_check( void ) {
  struct c_type_map {
    char const   *name;
    c_type_bits_t bit;
  };
  typedef struct c_type_map c_type_map_t;

  static c_type_map_t const C_TYPE_MAP[] = {
    { "void",     C_TYPE_VOID     },
    { "bool",     C_TYPE_BOOL     },
    { "char",     C_TYPE_CHAR     },
    { "wchar_t",  C_TYPE_WCHAR_T  },
    { "short",    C_TYPE_SHORT    },
    { "int",      C_TYPE_INT      },
    { "long",     C_TYPE_LONG     },
    { "signed",   C_TYPE_SIGNED   },
    { "unsigned", C_TYPE_UNSIGNED },
    { "float",    C_TYPE_FLOAT    },
    { "double",   C_TYPE_DOUBLE   }
  };

  enum restriction {
    NONE,
    NEVER,                              // never allowed
    KNR,                                // not allowed in K&R C
    ANSI                                // not allowed in ANSI C
  };
  typedef enum restriction restriction_t;

#define _ NONE
#define X NEVER
#define K KNR
#define A ANSI

  static restriction_t const RESTRICTIONS[][ ARRAY_SIZE( C_TYPE_MAP ) ] = {
    /*               v b c w s i l s u f d */
    /* void     */ { _,_,_,_,_,_,_,_,_,_,_ },
    /* bool     */ { X,_,_,_,_,_,_,_,_,_,_ },
    /* char     */ { X,X,_,_,_,_,_,_,_,_,_ },
    /* wchar_t  */ { X,X,X,K,_,_,_,_,_,_,_ },
    /* short    */ { X,X,X,X,_,_,_,_,_,_,_ },
    /* int      */ { X,X,X,X,_,_,_,_,_,_,_ },
    /* long     */ { X,X,X,X,X,_,_,_,_,_,_ },
    /* signed   */ { X,X,K,X,K,K,K,_,_,_,_ },
    /* unsigned */ { X,X,_,X,_,_,_,X,_,_,_ },
    /* float    */ { X,X,X,X,X,X,A,X,X,_,_ },
    /* double   */ { X,X,X,X,X,X,K,X,X,X,_ }
  };

#undef _
#undef X
#undef K
#undef A

  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_MAP ); ++i ) {
    if ( c_type_bits & C_TYPE_MAP[i].bit ) {
      for ( size_t j = 0; j < i; ++j ) {
        if ( c_type_bits & C_TYPE_MAP[j].bit ) {
          char const *const t1 = C_TYPE_MAP[i].name;
          char const *const t2 = C_TYPE_MAP[j].name;
          switch ( RESTRICTIONS[i][j] ) {
            case NONE:
              break;
            case ANSI:
              if ( opt_lang != LANG_C_KNR )
                not_supported( " (ANSI Compiler)", t1, t2 );
              break;
            case KNR:
              if ( opt_lang == LANG_C_KNR )
                not_supported( " (Pre-ANSI Compiler)", t1, t2 );
              break;
            case NEVER:
              not_supported( "", t1, t2 );
              break;
          } // switch
        }
      } // for
    }
  } // for
}

/**
 * TODO
 *
 */
static void do_cast( char const *name, char const *left, char const *right,
                     char const *type ) {
	assert( left );
  assert( right );
  assert( type );

  size_t const lenl = strlen( left ), lenr = strlen( right );

  if ( c_ident_kind == C_FUNCTION )
    unsupp( "Cast into function", "cast into pointer to function" );
  else if (c_ident_kind==C_ARRAY_DIM || c_ident_kind=='a')
    unsupp( "Cast into array","cast into pointer" );
  printf(
    "(%s%*s%s)%s\n",
    type, (int)(lenl + lenr ? lenl + 1 : 0),
    left, right, name ? name : "expression"
  );
  free( (void*)left );
  free( (void*)right );
  free( (void*)type );
  if ( name )
    free( (void*)name );
}

/**
 * TODO
 *
 * @param name TODO
 * @param storage TODO
 * @param left TODO
 * @param right TODO
 * @param type TODO
 */
static void do_declare( char const *name, char const *storage,
                        char const *left, char const *right,
                        char const *type ) {
  assert( storage );
  assert( left );
  assert( right );
  assert( type );

  if ( c_ident_kind == C_VOID )
    unsupp("Variable of type void", "variable of type pointer to void");

  if ( *storage == CXX_REFERENCE ) {
    switch ( c_ident_kind ) {
      case C_FUNCTION:
        unsupp("Register function", NULL);
        break;
      case C_ARRAY_DIM:
      case C_ARRAY_NO_DIM:
        unsupp("Register array", NULL);
          break;
      case C_STRUCT:
        unsupp("Register struct/class", NULL);
        break;
      default:
        /* suppress warning */;
    } // switch
  }

  if ( *storage )
    printf( "%s ", storage );

  printf(
    "%s %s%s%s", type, left,
    name ? name : (c_ident_kind == C_FUNCTION) ? "f" : "var", right
  );
  if ( opt_make_c ) {
    if ( (c_ident_kind == C_FUNCTION) && (*storage != 'e') )
      printf( " { }\n" );
    else
      printf( ";\n" );
  } else {
    printf( "\n" );
  }

  free( (void*)storage );
  free( (void*)left );
  free( (void*)right );
  free( (void*)type );
  if ( name )
    free( (void*)name );
}

/**
 */
static void do_set( char const *opt ) {
  if (strcmp(opt, "create") == 0)
    { opt_make_c = true; }
  else if (strcmp(opt, "nocreate") == 0)
    { opt_make_c = false; }
  else if (strcmp(opt, "prompt") == 0)
    { prompting = true; prompt_ptr = prompt_buf; }
  else if (strcmp(opt, "noprompt") == 0)
    { prompting = false; prompt_ptr = ""; }
  else if (strcmp(opt, "preansi") == 0)
    { opt_lang = LANG_C_KNR; }
  else if (strcmp(opt, "ansi") == 0)
    { opt_lang = LANG_C_ANSI; }
  else if (strcmp(opt, "cplusplus") == 0)
    { opt_lang = LANG_CXX; }
#ifdef WITH_CDECL_DEBUG
  else if (strcmp(opt, "debug") == 0)
    { opt_debug = 1; }
  else if (strcmp(opt, "nodebug") == 0)
    { opt_debug = 0; }
#endif /* WITH_CDECL_DEBUG */
#ifdef doyydebug
  else if (strcmp(opt, "yydebug") == 0)
    { yydebug = 1; }
  else if (strcmp(opt, "noyydebug") == 0)
    { yydebug = 0; }
#endif /* doyydebug */
  else {
    if ((strcmp(opt, unknown_name) != 0) &&
        (strcmp(opt, "options") != 0))
      printf("Unknown set option: '%s'\n", opt);

    printf("Valid set options (and command line equivalents) are:\n");
    printf("\toptions\n");
    printf("\tcreate (-c), nocreate\n");
    printf("\tprompt, noprompt (-q)\n");
#ifndef USE_READLINE
    printf("\tinteractive (-i), nointeractive\n");
#endif
    printf("\tpreansi (-p), ansi (-a) or cplusplus (-+)\n");
#ifdef WITH_CDECL_DEBUG
    printf("\tdebug (-d), nodebug\n");
#endif /* WITH_CDECL_DEBUG */
#ifdef doyydebug
    printf("\tyydebug (-D), noyydebug\n");
#endif /* doyydebug */

    printf("\nCurrent set values are:\n");
    printf("\t%screate\n", opt_make_c ? "   " : " no");
    printf("\t%sprompt\n", prompt_ptr[0] ? "   " : " no");
    printf("\t%sinteractive\n", opt_interactive ? "   " : " no");
    if (opt_lang == LANG_C_KNR)
      printf("\t   preansi\n");
    else
      printf("\t(nopreansi)\n");
    if ( opt_lang == LANG_C_ANSI )
      printf("\t   ansi\n");
    else
      printf("\t(noansi)\n");
    if ( opt_lang == LANG_CXX )
      printf("\t   cplusplus\n");
    else
      printf("\t(nocplusplus)\n");
#ifdef WITH_CDECL_DEBUG
    printf("\t%sdebug\n", opt_debug ? "   " : " no");
#endif /* WITH_CDECL_DEBUG */
#ifdef doyydebug
    printf("\t%syydebug\n", yydebug ? "   " : " no");
#endif /* doyydebug */
  }
}

static void explain_cast( char const *constvol, char const *type,
                          char const *cast, char const *name ) {
  assert( constvol );
  assert( type );
  assert( cast );
  assert( name );

  if ( strcmp( type, "void" ) == 0 ) {
    if ( c_ident_kind == 'a' )
      unsupp("array of type void", "array of type pointer to void");
    else if ( c_ident_kind == CXX_REFERENCE )
      unsupp( "reference to type void", "pointer to void" );
  }
  printf( "cast %s into %s", name, cast );
  if ( strlen( constvol ) > 0 )
    printf( "%s ", constvol );
  printf( "%s\n", type );
}

void explain_declaration( char const *storage, char const *constvol1,
                          char const *constvol2, char const *type,
                          char const *decl ) {
  assert( storage );
  assert( constvol1 );
  assert( constvol2 );
  assert( decl );

  if ( type && strcmp( type, "void" ) == 0 ) {
    if (c_ident_kind == C_NAME)
      unsupp("Variable of type void", "variable of type pointer to void");
    else if ( c_ident_kind == 'a' )
      unsupp("array of type void", "array of type pointer to void");
    else if ( c_ident_kind == CXX_REFERENCE )
      unsupp("reference to type void", "pointer to void");
  }

  if ( *storage == CXX_REFERENCE ) {
    switch ( c_ident_kind ) {
      case C_FUNCTION:
        unsupp("Register function", NULL);
        break;
      case C_ARRAY_DIM:
      case C_ARRAY_NO_DIM:
        unsupp("Register array", NULL);
        break;
      case C_STRUCT:
        unsupp("Register struct/union/enum/class", NULL);
        break;
      default:
        /* suppress warning */;
    } // switch
  }

  printf( "declare %s as ", c_ident );
  if ( *storage )
    printf( "%s ", storage );
  printf( "%s", decl );
  if ( *constvol1 )
    printf( "%s ", constvol1 );
  if ( *constvol2 )
    printf( "%s ", constvol2 );
  printf( "%s\n", type ? type : "int" );
}

static void print_help( void );

static void not_supported( char const *compiler, char const *type1,
                           char const *type2 ) {
  if ( type2 )
    PRINT_ERR(
      "Warning: Unsupported in%s C%s -- '%s' with '%s'\n",
      compiler, opt_lang == LANG_CXX ? "++" : "", type1, type2
    );
  else
    PRINT_ERR(
      "Warning: Unsupported in%s C%s -- '%s'\n",
      compiler, opt_lang == LANG_CXX ? "++" : "", type1
    );
}

/* Write out a message about something */
/* being unsupported, possibly with a hint. */
static void unsupp( char const *s, char const *hint ) {
  not_supported( "", s, NULL );
  if ( hint )
    PRINT_ERR( "\t(maybe you mean \"%s\")\n", hint );
}

static void yyerror( char const *s ) {
  PRINT_ERR( "%s\n", s );
  //YYTRACE( "yychar=%d\n", yychar );
}

int yywrap( void ) {
  return 1;
}

///////////////////////////////////////////////////////////////////////////////

%}

%union {
  char *dynstr;
  struct {
    char *left;
    char *right;
    char *type;
  } halves;
}

%token  T_ARRAY
%token  T_AS
%token  T_BLOCK
%token  T_CAST
%token  T_COMMA
%token  T_DECLARE
%token  T_DOUBLECOLON
%token  T_EXPLAIN
%token  T_FUNCTION
%token  T_HELP
%token  T_INTO
%token  T_MEMBER
%token  T_OF
%token  T_POINTER
%token  T_REFERENCE
%token  T_RETURNING
%token  T_SET
%token  T_TO

%token  <dynstr> T_VOID
%token  <dynstr> T_BOOL
%token  <dynstr> T_CHAR
%token  <dynstr> T_WCHAR_T
%token  <dynstr> T_SHORT T_INT T_LONG T_FLOAT T_DOUBLE
%token  <dynstr> T_SIGNED
%token  <dynstr> T_UNSIGNED
%token  <dynstr> T_CLASS
%token  <dynstr> T_CONST_VOLATILE
%token  <dynstr> T_ENUM
%token  <dynstr> T_NAME
%token  <dynstr> T_STRUCT T_UNION
%token  <dynstr> T_NUMBER

%token  <dynstr> T_AUTO T_EXTERN T_REGISTER T_STATIC
%type   <dynstr> adecllist adims c_type cast castlist cdecl cdecl1 cdims
%type   <dynstr> constvol_list ClassStruct mod_list mod_list1 modifier
%type   <dynstr> opt_constvol_list optNAME opt_storage storage StrClaUniEnum
%type   <dynstr> c_type_name type
%type   <halves> adecl

%start prog

%%

prog
  : /* empty */
  | prog statement
    {
      c_ident_kind = 0;
    }
  ;

statement
  : T_HELP NL
    {
      YYTRACE( "statement: help\n" );
      print_help();
    }

  | T_DECLARE T_NAME T_AS opt_storage adecl NL
    {
      YYTRACE( "statement: DECLARE NAME AS opt_storage adecl\n" );
      YYTRACE( "\tNAME='%s'\n", $2 );
      YYTRACE( "\topt_storage='%s'\n", $4 );
      YYTRACE( "\tacdecl.left='%s'\n", $5.left );
      YYTRACE( "\tacdecl.right='%s'\n", $5.right );
      YYTRACE( "\tacdecl.type='%s'\n", $5.type );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
      do_declare($2, $4, $5.left, $5.right, $5.type);
    }

  | T_DECLARE opt_storage adecl NL
    {
      YYTRACE( "statement: DECLARE opt_storage adecl\n" );
      YYTRACE( "\topt_storage='%s'\n", $2 );
      YYTRACE( "\tacdecl.left='%s'\n", $3.left );
      YYTRACE( "\tacdecl.right='%s'\n", $3.right );
      YYTRACE( "\tacdecl.type='%s'\n", $3.type );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
      do_declare(NULL, $2, $3.left, $3.right, $3.type);
    }

  | T_CAST T_NAME T_INTO adecl NL
    {
      YYTRACE( "statement: CAST NAME AS adecl\n" );
      YYTRACE( "\tNAME='%s'\n", $2 );
      YYTRACE( "\tacdecl.left='%s'\n", $4.left );
      YYTRACE( "\tacdecl.right='%s'\n", $4.right );
      YYTRACE( "\tacdecl.type='%s'\n", $4.type );
      do_cast($2, $4.left, $4.right, $4.type);
    }

  | T_CAST adecl NL
    {
      YYTRACE( "statement: CAST adecl\n" );
      YYTRACE( "\tacdecl.left='%s'\n", $2.left );
      YYTRACE( "\tacdecl.right='%s'\n", $2.right );
      YYTRACE( "\tacdecl.type='%s'\n", $2.type );
      do_cast(NULL, $2.left, $2.right, $2.type);
    }

  | T_EXPLAIN opt_storage opt_constvol_list type opt_constvol_list cdecl NL
    {
      YYTRACE( "statement: EXPLAIN opt_storage opt_constvol_list type cdecl\n" );
      YYTRACE( "\topt_storage='%s'\n", $2 );
      YYTRACE( "\topt_constvol_list='%s'\n", $3 );
      YYTRACE( "\ttype='%s'\n", $4 );
      YYTRACE( "\tcdecl='%s'\n", $6 );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
      explain_declaration($2, $3, $5, $4, $6);
    }

  | T_EXPLAIN storage opt_constvol_list cdecl NL
    {
      YYTRACE( "statement: EXPLAIN storage opt_constvol_list cdecl\n" );
      YYTRACE( "\tstorage='%s'\n", $2 );
      YYTRACE( "\topt_constvol_list='%s'\n", $3 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
      explain_declaration($2, $3, NULL, NULL, $4);
    }

  | T_EXPLAIN opt_storage constvol_list cdecl NL
    {
      YYTRACE( "statement: EXPLAIN opt_storage constvol_list cdecl\n" );
      YYTRACE( "\topt_storage='%s'\n", $2 );
      YYTRACE( "\tconstvol_list='%s'\n", $3 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
      explain_declaration($2, $3, NULL, NULL, $4);
    }

  | T_EXPLAIN '(' opt_constvol_list type cast ')' optNAME NL
    {
      YYTRACE( "statement: EXPLAIN ( opt_constvol_list type cast ) optNAME\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $3 );
      YYTRACE( "\ttype='%s'\n", $4 );
      YYTRACE( "\tcast='%s'\n", $5 );
      YYTRACE( "\tNAME='%s'\n", $7 );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
      explain_cast($3, $4, $5, $7);
    }

  | T_SET optNAME NL
    {
      YYTRACE( "statement: SET optNAME\n" );
      YYTRACE( "\toptNAME='%s'\n", $2 );
      do_set($2);
    }

  | NL
  | error NL
    {
      yyerrok;
    }
  ;

NL
  : '\n'
    {
      prompting = true;
    }
  | ';'
    {
      prompting = false;
    }
  ;

optNAME
  : T_NAME
    {
      YYTRACE( "optNAME: NAME\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      $$ = $1;
    }

  | /* empty */
    {
      YYTRACE( "optNAME: EMPTY\n" );
      $$ = strdup(unknown_name);
    }
  ;

cdecl
  : cdecl1
  | '*' opt_constvol_list cdecl
    {
      YYTRACE( "cdecl: * opt_constvol_list cdecl\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $2 );
      YYTRACE( "\tcdecl='%s'\n", $3 );
      $$ = cat($3,$2,strdup(strlen($2)?" pointer to ":"pointer to "),NULL);
      c_ident_kind = C_POINTER;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | T_NAME T_DOUBLECOLON '*' cdecl
    {
      YYTRACE( "cdecl: NAME DOUBLECOLON '*' cdecl\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      if (opt_lang != LANG_CXX)
        unsupp("pointer to member of class", NULL);
      $$ = cat($4,strdup("pointer to member of class "),$1,strdup(" "),NULL);
      c_ident_kind = C_POINTER;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '&' opt_constvol_list cdecl
    {
      YYTRACE( "cdecl: & opt_constvol_list cdecl\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $2 );
      YYTRACE( "\tcdecl='%s'\n", $3 );
      if (opt_lang != LANG_CXX)
        unsupp("reference", NULL);
      $$ = cat($3,$2,strdup(strlen($2)?" reference to ":"reference to "),NULL);
      c_ident_kind = CXX_REFERENCE;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }
  ;

cdecl1
  : cdecl1 '(' ')'
    {
      YYTRACE( "cdecl1: cdecl1()\n" );
      YYTRACE( "\tcdecl1='%s'\n", $1 );
      $$ = cat($1,strdup("function returning "),NULL);
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '(' '^' opt_constvol_list cdecl ')' '(' ')'
    {
      char const *sp = "";
      YYTRACE( "cdecl1: (^ opt_constvol_list cdecl)()\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $3 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      if (strlen($3) > 0)
          sp = " ";
      $$ = cat($4, $3, strdup(sp), strdup("block returning "), NULL);
      c_ident_kind = C_BLOCK;
    }

  | '(' '^' opt_constvol_list cdecl ')' '(' castlist ')'
    {
      char const *sp = "";
      YYTRACE( "cdecl1: (^ opt_constvol_list cdecl)( castlist )\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $3 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      YYTRACE( "\tcastlist='%s'\n", $7 );
      if (strlen($3) > 0)
        sp = " ";
      $$ = cat($4, $3, strdup(sp), strdup("block ("), $7, strdup(") returning "), NULL);
      c_ident_kind = C_BLOCK;
    }

  | cdecl1 '(' castlist ')'
    {
      YYTRACE( "cdecl1: cdecl1(castlist)\n" );
      YYTRACE( "\tcdecl1='%s'\n", $1 );
      YYTRACE( "\tcastlist='%s'\n", $3 );
      $$ = cat($1, strdup("function ("),
          $3, strdup(") returning "), NULL);
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | cdecl1 cdims
    {
      YYTRACE( "cdecl1: cdecl1 cdims\n" );
      YYTRACE( "\tcdecl1='%s'\n", $1 );
      YYTRACE( "\tcdims='%s'\n", $2 );
      $$ = cat($1,strdup("array "),$2,NULL);
      c_ident_kind = 'a';
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '(' cdecl ')'
    {
      YYTRACE( "cdecl1: (cdecl)\n" );
      YYTRACE( "\tcdecl='%s'\n", $2 );
      $$ = $2;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | T_NAME
    {
      YYTRACE( "cdecl1: NAME\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      c_ident = $1;
      $$ = strdup("");
      c_ident_kind = C_NAME;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }
  ;

castlist
  : castlist T_COMMA castlist
    {
      YYTRACE( "castlist: castlist1, castlist2\n" );
      YYTRACE( "\tcastlist1='%s'\n", $1 );
      YYTRACE( "\tcastlist2='%s'\n", $3 );
      $$ = cat($1, strdup(", "), $3, NULL);
    }

  | opt_constvol_list type cast
    {
      YYTRACE( "castlist: opt_constvol_list type cast\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $1 );
      YYTRACE( "\ttype='%s'\n", $2 );
      YYTRACE( "\tcast='%s'\n", $3 );
      $$ = cat($3, $1, strdup(strlen($1) ? " " : ""), $2, NULL);
    }

  | T_NAME
    {
      $$ = $1;
    }
  ;

adecllist
  : /* empty */
    {
      YYTRACE( "adecllist: EMPTY\n" );
      $$ = strdup("");
    }

  | adecllist T_COMMA adecllist
    {
      YYTRACE( "adecllist: adecllist1, adecllist2\n" );
      YYTRACE( "\tadecllist1='%s'\n", $1 );
      YYTRACE( "\tadecllist2='%s'\n", $3 );
      $$ = cat($1, strdup(", "), $3, NULL);
    }

  | T_NAME
    {
      YYTRACE( "adecllist: NAME\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      $$ = $1;
    }

  | adecl
    {
      YYTRACE( "adecllist: adecl\n" );
      YYTRACE( "\tadecl.left='%s'\n", $1.left );
      YYTRACE( "\tadecl.right='%s'\n", $1.right );
      YYTRACE( "\tadecl.type='%s'\n", $1.type );
      $$ = cat($1.type, strdup(" "), $1.left, $1.right, NULL);
    }

  | T_NAME T_AS adecl
    {
      YYTRACE( "adecllist: NAME AS adecl\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      YYTRACE( "\tadecl.left='%s'\n", $3.left );
      YYTRACE( "\tadecl.right='%s'\n", $3.right );
      YYTRACE( "\tadecl.type='%s'\n", $3.type );
      $$ = cat($3.type, strdup(" "), $3.left, $1, $3.right, NULL);
    }
  ;

cast
  : /* empty */
    {
      YYTRACE( "cast: EMPTY\n" );
      $$ = strdup("");
      /* c_ident_kind = c_ident_kind; */
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '(' ')'
    {
      YYTRACE( "cast: ()\n" );
      $$ = strdup("function returning ");
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '(' cast ')' '(' ')'
    {
      YYTRACE( "cast: (cast)()\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      $$ = cat($2,strdup("function returning "),NULL);
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '(' cast ')' '(' castlist ')'
    {
      YYTRACE( "cast: (cast)(castlist)\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      YYTRACE( "\tcastlist='%s'\n", $5 );
      $$ = cat($2,strdup("function ("),$5,strdup(") returning "),NULL);
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '(' '^' cast ')' '(' ')'
    {
      YYTRACE( "cast: (^ cast)()\n" );
      YYTRACE( "\tcast='%s'\n", $3 );
      $$ = cat($3,strdup("block returning "),NULL);
      c_ident_kind = C_BLOCK;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '(' '^' cast ')' '(' castlist ')'
    {
      YYTRACE( "cast: (^ cast)(castlist)\n" );
      YYTRACE( "\tcast='%s'\n", $3 );
      $$ = cat($3,strdup("block ("), $6, strdup(") returning "),NULL);
      c_ident_kind = C_BLOCK;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '(' cast ')'
    {
      YYTRACE( "cast: (cast)\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      $$ = $2;
      /* c_ident_kind = c_ident_kind; */
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | T_NAME T_DOUBLECOLON '*' cast
    {
      YYTRACE( "cast: NAME::*cast\n" );
      YYTRACE( "\tcast='%s'\n", $4 );
      if (opt_lang != LANG_CXX)
        unsupp("pointer to member of class", NULL);
      $$ = cat($4,strdup("pointer to member of class "),$1,strdup(" "),NULL);
      c_ident_kind = C_POINTER;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '*' cast
    {
      YYTRACE( "cast: *cast\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      $$ = cat($2,strdup("pointer to "),NULL);
      c_ident_kind = C_POINTER;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | '&' cast
    {
      YYTRACE( "cast: &cast\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      if (opt_lang != LANG_CXX)
        unsupp("reference", NULL);
      $$ = cat($2,strdup("reference to "),NULL);
      c_ident_kind = CXX_REFERENCE;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | cast cdims
    {
      YYTRACE( "cast: cast cdims\n" );
      YYTRACE( "\tcast='%s'\n", $1 );
      YYTRACE( "\tcdims='%s'\n", $2 );
      $$ = cat($1,strdup("array "),$2,NULL);
      c_ident_kind = C_ARRAY_NO_DIM;
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }
  ;

cdims
  : '[' ']'
    {
      YYTRACE( "cdims: []\n" );
      $$ = strdup("of ");
    }

  | '[' T_NUMBER ']'
    {
      YYTRACE( "cdims: [NUMBER]\n" );
      YYTRACE( "\tNUMBER='%s'\n", $2 );
      $$ = cat($2,strdup(" of "),NULL);
    }
  ;

adecl
  : T_FUNCTION T_RETURNING adecl
    {
      YYTRACE( "adecl: FUNCTION RETURNING adecl\n" );
      YYTRACE( "\tadecl.left='%s'\n", $3.left );
      YYTRACE( "\tadecl.right='%s'\n", $3.right );
      YYTRACE( "\tadecl.type='%s'\n", $3.type );
      if (c_ident_kind == C_FUNCTION)
        unsupp("Function returning function",
                "function returning pointer to function");
      else if (c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_ARRAY_NO_DIM)
        unsupp("Function returning array",
                "function returning pointer");
      $$.left = $3.left;
      $$.right = cat(strdup("()"),$3.right,NULL);
      $$.type = $3.type;
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\n\tadecl now =\n" );
      YYTRACE( "\t\tadecl.left='%s'\n", $$.left );
      YYTRACE( "\t\tadecl.right='%s'\n", $$.right );
      YYTRACE( "\t\tadecl.type='%s'\n", $$.type );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | T_FUNCTION '(' adecllist ')' T_RETURNING adecl
    {
      YYTRACE( "adecl: FUNCTION (adecllist) RETURNING adecl\n" );
      YYTRACE( "\tadecllist='%s'\n", $3 );
      YYTRACE( "\tadecl.left='%s'\n", $6.left );
      YYTRACE( "\tadecl.right='%s'\n", $6.right );
      YYTRACE( "\tadecl.type='%s'\n", $6.type );
      if (c_ident_kind == C_FUNCTION)
        unsupp("Function returning function",
                "function returning pointer to function");
      else if (c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_ARRAY_NO_DIM)
        unsupp("Function returning array",
                "function returning pointer");
      $$.left = $6.left;
      $$.right = cat(strdup("("),$3,strdup(")"),$6.right,NULL);
      $$.type = $6.type;
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\n\tadecl now =\n" );
      YYTRACE( "\t\tadecl.left='%s'\n", $$.left );
      YYTRACE( "\t\tadecl.right='%s'\n", $$.right );
      YYTRACE( "\t\tadecl.type='%s'\n", $$.type );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | opt_constvol_list T_BLOCK T_RETURNING adecl
    {
      char const *sp = "";
      YYTRACE( "adecl: opt_constvol_list BLOCK RETURNING adecl\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $1 );
      YYTRACE( "\tadecl.left='%s'\n", $4.left );
      YYTRACE( "\tadecl.right='%s'\n", $4.right );
      YYTRACE( "\tadecl.type='%s'\n", $4.type );
      if (c_ident_kind == C_FUNCTION)
        unsupp("Block returning function",
               "block returning pointer to function");
      else if (c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_ARRAY_NO_DIM)
        unsupp("Block returning array",
               "block returning pointer");
      if (strlen($1) != 0)
        sp = " ";
      $$.left = cat($4.left, strdup("(^"), strdup(sp), $1, strdup(sp), NULL);
      $$.right = cat(strdup(")()"),$4.right,NULL);
      $$.type = $4.type;
      c_ident_kind = C_BLOCK;
    }

  | opt_constvol_list T_BLOCK '(' adecllist ')' T_RETURNING adecl
    {
      char const *sp = "";
      YYTRACE( "adecl: opt_constvol_list BLOCK RETURNING adecl\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $1 );
      YYTRACE( "\tadecllist='%s'\n", $4 );
      YYTRACE( "\tadecl.left='%s'\n", $7.left );
      YYTRACE( "\tadecl.right='%s'\n", $7.right );
      YYTRACE( "\tadecl.type='%s'\n", $7.type );
      if (c_ident_kind == C_FUNCTION)
        unsupp("Block returning function",
               "block returning pointer to function");
      else if (c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_ARRAY_NO_DIM)
        unsupp("Block returning array",
               "block returning pointer");
      if (strlen($1) != 0)
          sp = " ";
      $$.left = cat($7.left, strdup("(^"), strdup(sp), $1, strdup(sp), NULL);
      $$.right = cat(strdup(")("), $4, strdup(")"), $7.right, NULL);
      $$.type = $7.type;
      c_ident_kind = C_BLOCK;
    }

  | T_ARRAY adims T_OF adecl
    {
      YYTRACE( "adecl: ARRAY adims OF adecl\n" );
      YYTRACE( "\tadims='%s'\n", $2 );
      YYTRACE( "\tadecl.left='%s'\n", $4.left );
      YYTRACE( "\tadecl.right='%s'\n", $4.right );
      YYTRACE( "\tadecl.type='%s'\n", $4.type );
      if ( c_ident_kind == C_FUNCTION )
        unsupp("Array of function", "array of pointer to function");
      else if ( c_ident_kind == C_ARRAY_NO_DIM )
        unsupp("Inner array of unspecified size", "array of pointer");
      else if ( c_ident_kind == C_VOID )
        unsupp("Array of void", "pointer to void");
      c_ident_kind = array_has_dim ? C_ARRAY_DIM : C_ARRAY_NO_DIM;
      $$.left = $4.left;
      $$.right = cat( $2, $4.right, NULL );
      $$.type = $4.type;
      YYTRACE( "\n\tadecl now =\n" );
      YYTRACE( "\t\tadecl.left='%s'\n", $$.left );
      YYTRACE( "\t\tadecl.right='%s'\n", $$.right );
      YYTRACE( "\t\tadecl.type='%s'\n", $$.type );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | opt_constvol_list T_POINTER T_TO adecl
    {
      char const *op = "", *cp = "", *sp = "";

      YYTRACE( "adecl: opt_constvol_list POINTER TO adecl\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $1 );
      YYTRACE( "\tadecl.left='%s'\n", $4.left );
      YYTRACE( "\tadecl.right='%s'\n", $4.right );
      YYTRACE( "\tadecl.type='%s'\n", $4.type );
      if (c_ident_kind == C_ARRAY_NO_DIM)
        unsupp("Pointer to array of unspecified dimension",
                "pointer to object");
      if (c_ident_kind==C_ARRAY_NO_DIM || c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_FUNCTION) {
        op = "(";
        cp = ")";
      }
      if ( strlen( $1 ) > 0 )
        sp = " ";
      $$.left = cat($4.left,strdup(op),strdup("*"), strdup(sp),$1,strdup(sp),NULL);
      $$.right = cat(strdup(cp),$4.right,NULL);
      $$.type = $4.type;
      c_ident_kind = C_POINTER;
      YYTRACE( "\n\tadecl now =\n" );
      YYTRACE( "\t\tadecl.left='%s'\n", $$.left );
      YYTRACE( "\t\tadecl.right='%s'\n", $$.right );
      YYTRACE( "\t\tadecl.type='%s'\n", $$.type );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | opt_constvol_list T_POINTER T_TO T_MEMBER T_OF ClassStruct T_NAME adecl
    {
      char const *op = "", *cp = "", *sp = "";

      YYTRACE( "adecl: opt_constvol_list POINTER TO MEMBER OF ClassStruct NAME adecl\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $1 );
      YYTRACE( "\tClassStruct='%s'\n", $6 );
      YYTRACE( "\tNAME='%s'\n", $7 );
      YYTRACE( "\tadecl.left='%s'\n", $8.left );
      YYTRACE( "\tadecl.right='%s'\n", $8.right );
      YYTRACE( "\tadecl.type='%s'\n", $8.type );
      if (opt_lang != LANG_CXX)
        unsupp("pointer to member of class", NULL);
      if (c_ident_kind == C_ARRAY_NO_DIM)
        unsupp("Pointer to array of unspecified dimension",
                "pointer to object");
      if (c_ident_kind==C_ARRAY_NO_DIM || c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_FUNCTION) {
        op = "(";
        cp = ")";
      }
      if (strlen($1) != 0)
        sp = " ";
      $$.left = cat($8.left,strdup(op),$7,strdup("::*"), strdup(sp),$1,strdup(sp),NULL);
      $$.right = cat(strdup(cp),$8.right,NULL);
      $$.type = $8.type;
      c_ident_kind = C_POINTER;
      YYTRACE( "\n\tadecl now =\n" );
      YYTRACE( "\t\tadecl.left='%s'\n", $$.left );
      YYTRACE( "\t\tadecl.right='%s'\n", $$.right );
      YYTRACE( "\t\tadecl.type='%s'\n", $$.type );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | opt_constvol_list T_REFERENCE T_TO adecl
    {
      char const *op = "", *cp = "", *sp = "";

      YYTRACE( "adecl: opt_constvol_list REFERENCE TO adecl\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $1 );
      YYTRACE( "\tadecl.left='%s'\n", $4.left );
      YYTRACE( "\tadecl.right='%s'\n", $4.right );
      YYTRACE( "\tadecl.type='%s'\n", $4.type );
      if (opt_lang != LANG_CXX)
        unsupp("reference", NULL);
      if (c_ident_kind == C_VOID)
        unsupp("Reference to void",
                "pointer to void");
      else if (c_ident_kind == C_ARRAY_NO_DIM)
        unsupp("Reference to array of unspecified dimension",
                "reference to object");
      if (c_ident_kind==C_ARRAY_NO_DIM || c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_FUNCTION) {
        op = "(";
        cp = ")";
      }
      if (strlen($1) != 0)
        sp = " ";
      $$.left = cat($4.left,strdup(op),strdup("&"), strdup(sp),$1,strdup(sp),NULL);
      $$.right = cat(strdup(cp),$4.right,NULL);
      $$.type = $4.type;
      c_ident_kind = CXX_REFERENCE;
      YYTRACE( "\n\tadecl now =\n" );
      YYTRACE( "\t\tadecl.left='%s'\n", $$.left );
      YYTRACE( "\t\tadecl.right='%s'\n", $$.right );
      YYTRACE( "\t\tadecl.type='%s'\n", $$.type );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }

  | opt_constvol_list type
    {
      YYTRACE( "adecl: opt_constvol_list type\n" );
      YYTRACE( "\topt_constvol_list='%s'\n", $1 );
      YYTRACE( "\ttype='%s'\n", $2 );
      $$.left = strdup("");
      $$.right = strdup("");
      $$.type = cat($1,strdup(strlen($1)?" ":""),$2,NULL);
      if (strcmp($2, "void") == 0)
          c_ident_kind = C_VOID;
      else if ((strncmp($2, "struct", 6) == 0) ||
                (strncmp($2, "class", 5) == 0))
          c_ident_kind = C_STRUCT;
      else
          c_ident_kind = C_BUILTIN;
      YYTRACE( "\n\tadecl now =\n" );
      YYTRACE( "\t\tadecl.left='%s'\n", $$.left );
      YYTRACE( "\t\tadecl.right='%s'\n", $$.right );
      YYTRACE( "\t\tadecl.type='%s'\n", $$.type );
      YYTRACE( "\tprev = '%s'\n", visible(c_ident_kind) );
    }
  ;

adims
  : /* empty */
    {
      YYTRACE( "adims: EMPTY\n" );
      array_has_dim = false;
      $$ = strdup("[]");
    }

  | T_NUMBER
    {
      YYTRACE( "adims: NUMBER\n" );
      YYTRACE( "\tNUMBER='%s'\n", $1 );
      array_has_dim = true;
      $$ = cat(strdup("["),$1,strdup("]"),NULL);
    }
  ;

type
  : c_type_init c_type
    {
      YYTRACE( "type: c_type_init c_type\n" );
      YYTRACE( "\tc_type_init=''\n" );
      YYTRACE( "\tc_type='%s'\n", $2 );
      c_type_check();
      $$ = $2;
    }
  ;

c_type_init
  : /* empty */
    {
      YYTRACE( "c_type_init: EMPTY\n" );
      c_type_bits = 0;
    }
  ;

c_type
  : mod_list
    {
      YYTRACE( "c_type: mod_list\n" );
      YYTRACE( "\tmod_list='%s'\n", $1 );
      $$ = $1;
    }

  | c_type_name
    {
      YYTRACE( "c_type: c_type_name\n" );
      YYTRACE( "\tc_type_name='%s'\n", $1 );
      $$ = $1;
    }

  | mod_list c_type_name
    {
      YYTRACE( "c_type: mod_list c_type_name\n" );
      YYTRACE( "\tmod_list='%s'\n", $1 );
      YYTRACE( "\tc_type_name='%s'\n", $2 );
      $$ = cat( $1, strdup(" "), $2, NULL );
    }

  | StrClaUniEnum T_NAME
    {
      YYTRACE( "c_type: StrClaUniEnum NAME\n" );
      YYTRACE( "\tStrClaUniEnum='%s'\n", $1 );
      YYTRACE( "\tNAME='%s'\n", $2 );
      $$ = cat($1,strdup(" "),$2,NULL);
    }
  ;

StrClaUniEnum
  : ClassStruct
  | T_ENUM
  | T_UNION
    {
      $$ = $1;
    }
  ;

ClassStruct
  : T_STRUCT
  | T_CLASS
    {
      $$ = $1;
    }
  ;

c_type_name
  : T_INT
    {
      YYTRACE( "c_type_name: INT\n" );
      YYTRACE( "\tINT='%s'\n", $1 );
      c_type_add( C_TYPE_INT );
      $$ = $1;
    }

  | T_BOOL
    {
      YYTRACE( "c_type_name: BOOL\n" );
      YYTRACE( "\tCHAR='%s'\n", $1 );
      c_type_add( C_TYPE_BOOL );
      $$ = $1;
    }

  | T_CHAR
    {
      YYTRACE( "c_type_name: CHAR\n" );
      YYTRACE( "\tCHAR='%s'\n", $1 );
      c_type_add( C_TYPE_CHAR );
      $$ = $1;
    }

  | T_WCHAR_T
    {
      YYTRACE( "c_type_name: WCHAR_T\n" );
      YYTRACE( "\tCHAR='%s'\n", $1 );
      if ( opt_lang == LANG_C_KNR )
        not_supported(" (Pre-ANSI Compiler)", $1, NULL);
      else
        c_type_add( C_TYPE_WCHAR_T );
      $$ = $1;
    }

  | T_FLOAT
    {
      YYTRACE( "c_type_name: FLOAT\n" );
      YYTRACE( "\tFLOAT='%s'\n", $1 );
      c_type_add( C_TYPE_FLOAT );
      $$ = $1;
    }

  | T_DOUBLE
    {
      YYTRACE( "c_type_name: DOUBLE\n" );
      YYTRACE( "\tDOUBLE='%s'\n", $1 );
      c_type_add( C_TYPE_DOUBLE );
      $$ = $1;
    }

  | T_VOID
    {
      YYTRACE( "c_type_name: VOID\n" );
      YYTRACE( "\tVOID='%s'\n", $1 );
      c_type_add( C_TYPE_VOID );
      $$ = $1;
    }
  ;

mod_list
  : modifier mod_list1
    {
      YYTRACE( "mod_list: modifier mod_list1\n" );
      YYTRACE( "\tmodifier='%s'\n", $1 );
      YYTRACE( "\tmod_list1='%s'\n", $2 );
      $$ = cat($1,strdup(" "),$2,NULL);
    }

  | modifier
    {
      YYTRACE( "mod_list: modifier\n" );
      YYTRACE( "\tmodifier='%s'\n", $1 );
      $$ = $1;
    }
  ;

mod_list1
  : mod_list
    {
      YYTRACE( "mod_list1: mod_list\n" );
      YYTRACE( "\tmod_list='%s'\n", $1 );
      $$ = $1;
    }

  | T_CONST_VOLATILE
    {
      YYTRACE( "mod_list1: CONSTVOLATILE\n" );
      YYTRACE( "\tCONSTVOLATILE='%s'\n", $1 );
      if ( opt_lang == LANG_C_KNR )
        not_supported(" (Pre-ANSI Compiler)", $1, NULL);
      else if ((strcmp($1, "noalias") == 0) && opt_lang == LANG_CXX)
        unsupp($1, NULL);
      $$ = $1;
    }
  ;

modifier
  : T_UNSIGNED
    {
      YYTRACE( "modifier: UNSIGNED\n" );
      YYTRACE( "\tUNSIGNED='%s'\n", $1 );
      c_type_add( C_TYPE_UNSIGNED );
      $$ = $1;
    }

  | T_SIGNED
    {
      YYTRACE( "modifier: SIGNED\n" );
      YYTRACE( "\tSIGNED='%s'\n", $1 );
      c_type_add( C_TYPE_SIGNED );
      $$ = $1;
    }

  | T_LONG
    {
      YYTRACE( "modifier: LONG\n" );
      YYTRACE( "\tLONG='%s'\n", $1 );
      c_type_add( C_TYPE_LONG );
      $$ = $1;
    }

  | T_SHORT
    {
      YYTRACE( "modifier: SHORT\n" );
      YYTRACE( "\tSHORT='%s'\n", $1 );
      c_type_add( C_TYPE_SHORT );
      $$ = $1;
    }
  ;

opt_constvol_list
  : T_CONST_VOLATILE opt_constvol_list
    {
      YYTRACE( "opt_constvol_list: CONSTVOLATILE opt_constvol_list\n" );
      YYTRACE( "\tCONSTVOLATILE='%s'\n", $1 );
      YYTRACE( "\topt_constvol_list='%s'\n", $2 );
      if (opt_lang == LANG_C_KNR)
        not_supported(" (Pre-ANSI Compiler)", $1, NULL);
      else if ((strcmp($1, "noalias") == 0) && opt_lang == LANG_CXX)
        unsupp($1, NULL);
      $$ = cat($1,strdup(strlen($2) ? " " : ""),$2,NULL);
    }

  | /* empty */
    {
      YYTRACE( "opt_constvol_list: EMPTY\n" );
      $$ = strdup("");
    }
  ;

constvol_list
  : T_CONST_VOLATILE opt_constvol_list
    {
      YYTRACE( "constvol_list: CONSTVOLATILE opt_constvol_list\n" );
      YYTRACE( "\tCONSTVOLATILE='%s'\n", $1 );
      YYTRACE( "\topt_constvol_list='%s'\n", $2 );
      if (opt_lang == LANG_C_KNR)
        not_supported(" (Pre-ANSI Compiler)", $1, NULL);
      else if ((strcmp($1, "noalias") == 0) && opt_lang == LANG_CXX)
        unsupp($1, NULL);
      $$ = cat($1,strdup(strlen($2) ? " " : ""),$2,NULL);
    }
  ;

storage
  : T_AUTO
  | T_EXTERN
  | T_REGISTER
  | T_STATIC
    {
      YYTRACE( "storage: AUTO,EXTERN,STATIC,REGISTER (%s)\n", $1 );
      $$ = $1;
    }
  ;

opt_storage
  : storage
    {
      YYTRACE( "opt_storage: storage=%s\n", $1 );
      $$ = $1;
    }

  | /* empty */
    {
      YYTRACE( "opt_storage: EMPTY\n" );
      $$ = strdup("");
    }
  ;

%%

///////////////////////////////////////////////////////////////////////////////

/* the help messages */
struct help_text {
  char const *text;                     // generic text 
  char const *cpptext;                  // C++ specific text 
};
typedef struct help_text help_text_t;

static help_text_t const HELP_TEXT[] = {
  // up-to 23 lines of help text so it fits on (24x80) screens

/*  1 */ { "[] means optional; {} means 1 or more; <> means defined elsewhere", NULL },
/*  2 */ { "  commands are separated by ';' and newlines", NULL },
/*  3 */ { "command:", NULL },
/*  4 */ { "  declare <name> as <english>", NULL },
/*  5 */ { "  cast <name> into <english>", NULL },
/*  6 */ { "  explain <gibberish>", NULL },
/*  7 */ { "  set or set options", NULL },
/*  8 */ { "  help, ?", NULL },
/*  9 */ { "  quit or exit", NULL },
/* 10 */ { "english:", NULL },
/* 11 */ { "  function [( <decl-list> )] returning <english>", NULL },
/* 12 */ { "  block [( <decl-list> )] returning <english>", NULL },
/* 13 */ { "  array [<number>] of <english>", NULL },
/* 14 */ { "  [{ const | volatile | noalias }] pointer to <english>",
    "  [{const|volatile}] {pointer|reference} to [member of class <name>] <english>" },
/* 15 */{ "  <type>", NULL },
/* 16 */{ "type:", NULL },
/* 17 */{ "  {[<storage-class>] [{<modifier>}] [<C-type>]}", NULL },
/* 18 */{ "  { struct | union | enum } <name>",
    "  {struct|class|union|enum} <name>" },
/* 19 */{ "decllist: a comma separated list of <name>, <english> or <name> as <english>", NULL },
/* 20 */{ "name: a C identifier", NULL },
/* 21 */{ "gibberish: a C declaration, like 'int *x', or cast, like '(int *)x'", NULL },
/* 22 */{ "storage-class: extern, static, auto, register", NULL },
/* 23 */{ "C-type: int, char, float, double, or void", NULL },
/* 24 */{ "modifier: short, long, signed, unsigned, const, volatile, or noalias",
    "modifier: short, long, signed, unsigned, const, or volatile" },
  { NULL, NULL }
};

static void print_help( void ) {
  char const *const fmt = opt_lang == LANG_CXX ? " %s\n" : "  %s\n";

  for ( help_text_t const *p = HELP_TEXT; p->text; p++ ) {
    if (opt_lang == LANG_CXX && p->cpptext)
      printf( fmt, p->cpptext );
    else
      printf( fmt, p->text );
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
