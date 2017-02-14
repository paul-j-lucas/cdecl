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

#ifdef WITH_CDECL_DEBUG
#define YYTRACE(...) \
  BLOCK( if ( opt_debug ) PRINT_ERR( "|" __VA_ARGS__ ); )
#else
#define YYTRACE(...)              /* nothing */
#endif /* WITH_CDECL_DEBUG */

#ifdef WITH_YYDEBUG
#define YYDEBUG 1
#endif /* WITH_YYDEBUG */

///////////////////////////////////////////////////////////////////////////////

// external variables
extern char         prompt_buf[];
extern char const  *prompt_ptr;

// extern functions
extern int          yylex( void );

/**
 * The kinds of C identifiers.
 */
enum c_ident_kind {
  C_NONE          = '0',
  C_ARRAY_NO_DIM  = 'a',
  C_ARRAY_DIM     = 'A',
  C_BLOCK         = '^',                // Apple extension
  C_BUILTIN       = 't',                // char, int, etc.
  C_FUNCTION      = 'f',
  C_NAME          = 'n',
  C_POINTER       = '*',
  CXX_REFERENCE   = '&',
  C_STRUCT        = 's',                // or C++ class
  C_VOID          = 'v',
};
typedef enum c_ident_kind c_ident_kind_t;

/**
 * Bits denoting C type.
 */
typedef unsigned c_type_bits_t;

// local constants
static c_type_bits_t const C_TYPE_VOID      = 0x0001;
static c_type_bits_t const C_TYPE_BOOL      = 0x0002;
static c_type_bits_t const C_TYPE_CHAR      = 0x0004;
static c_type_bits_t const C_TYPE_WCHAR_T   = 0x0008;
static c_type_bits_t const C_TYPE_SHORT     = 0x0010;
static c_type_bits_t const C_TYPE_INT       = 0x0020;
static c_type_bits_t const C_TYPE_LONG      = 0x0040;
static c_type_bits_t const C_TYPE_LONG_LONG = 0x0080;
static c_type_bits_t const C_TYPE_SIGNED    = 0x0100;
static c_type_bits_t const C_TYPE_UNSIGNED  = 0x0200;
static c_type_bits_t const C_TYPE_FLOAT     = 0x0400;
static c_type_bits_t const C_TYPE_DOUBLE    = 0x0800;

/**
 * Mapping between C type names and bit representations.
 */
struct c_type_map {
  char const   *name;
  c_type_bits_t bit;
};
typedef struct c_type_map c_type_map_t;

static c_type_map_t const C_TYPE_MAP[] = {
  { "void",       C_TYPE_VOID       },
  { "bool",       C_TYPE_BOOL       },
  { "char",       C_TYPE_CHAR       },
  { "wchar_t",    C_TYPE_WCHAR_T    },
  { "short",      C_TYPE_SHORT      },
  { "int",        C_TYPE_INT        },
  { "long",       C_TYPE_LONG       },
  { "long long",  C_TYPE_LONG_LONG  },
  { "signed",     C_TYPE_SIGNED     },
  { "unsigned",   C_TYPE_UNSIGNED   },
  { "float",      C_TYPE_FLOAT      },
  { "double",     C_TYPE_DOUBLE     }
};

// local constants
static char const     UNKNOWN_NAME[] = "unknown_name";

// local variables
static bool           array_has_dim;
static c_type_bits_t  c_type_bits;
static c_ident_kind_t c_ident_kind;
static char const    *c_ident;

// local functions
static char const*    c_type_name( c_type_bits_t );
static void           illegal( lang_t, char const*, char const* );
static void           unsupp( char const*, char const* );

///////////////////////////////////////////////////////////////////////////////

/**
 * TODO
 *
 * @param type_bit TODO
 */
static void c_type_add( c_type_bits_t type_bit ) {
  if ( type_bit == C_TYPE_LONG && (c_type_bits & C_TYPE_LONG) ) {
    //
    // TODO
    //
    type_bit = C_TYPE_LONG_LONG;
  }

  if ( !(c_type_bits & type_bit) ) {
    c_type_bits |= type_bit;
  } else {
    PRINT_ERR(
      "error: \"%s\" can not be combined with previous declaration\n",
      c_type_name( type_bit )
    );
  }
}

/**
 * TODO
 */
static void c_type_check( void ) {
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
    /*                v b c w s i l L s u f d */
    /* void      */ { _,_,_,_,_,_,_,_,_,_,_,_ },
    /* bool      */ { X,_,_,_,_,_,_,_,_,_,_,_ },
    /* char      */ { X,X,_,_,_,_,_,_,_,_,_,_ },
    /* wchar_t   */ { X,X,X,K,_,_,_,_,_,_,_,_ },
    /* short     */ { X,X,X,X,_,_,_,_,_,_,_,_ },
    /* int       */ { X,X,X,X,_,_,_,_,_,_,_,_ },
    /* long      */ { X,X,X,X,X,_,_,_,_,_,_,_ },
    /* long long */ { X,X,X,X,K,_,_,_,_,_,_,_ },
    /* signed    */ { X,X,K,X,K,K,K,_,_,_,_,_ },
    /* unsigned  */ { X,X,_,X,_,_,_,_,X,_,_,_ },
    /* float     */ { X,X,X,X,X,X,A,X,X,X,_,_ },
    /* double    */ { X,X,X,X,X,X,K,X,X,X,X,_ }
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
                illegal( LANG_C_89, t1, t2 );
              break;
            case KNR:
              if ( opt_lang == LANG_C_KNR )
                illegal( opt_lang, t1, t2 );
              break;
            case NEVER:
              illegal( opt_lang, t1, t2 );
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
 * @param bit TODO
 * @return Returns TODO
 */
static char const* c_type_name( c_type_bits_t bit ) {
  for ( size_t i = 0; i < ARRAY_SIZE( C_TYPE_MAP ); ++i )
    if ( bit == C_TYPE_MAP[i].bit )
      return C_TYPE_MAP[i].name;
  INTERNAL_ERR( "%X: unexpected value for bit", bit );
}

/**
 * Do the "cast" command.
 *
 * @param name TODO
 * @param left TODO
 * @param right TODO
 * @param type TODO
 */
static void do_cast( char const *name, char const *left, char const *right,
                     char const *type ) {
	assert( left );
  assert( right );
  assert( type );

  switch ( c_ident_kind ) {
    case C_FUNCTION:
      unsupp( "Cast into function", "cast into pointer to function" );
      break;
    case C_ARRAY_DIM:
    case C_ARRAY_NO_DIM:
      unsupp( "Cast into array", "cast into pointer" );
      break;
    default: {
      size_t const lenl = strlen( left ), lenr = strlen( right );
      printf(
        "(%s%*s%s)%s\n",
        type, (int)(lenl + lenr ? lenl + 1 : 0),
        left, right, name ? name : "expression"
      );
    }
  } // switch

  free( (void*)left );
  free( (void*)right );
  free( (void*)type );
  if ( name )
    free( (void*)name );
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
static void do_declare( char const *name, char const *storage,
                        char const *left, char const *right,
                        char const *type ) {
  assert( storage );
  assert( left );
  assert( right );
  assert( type );

  if ( c_ident_kind == C_VOID ) {
    unsupp( "Variable of type void", "variable of type pointer to void" );
    goto done;
  }

  if ( *storage == CXX_REFERENCE ) {
    switch ( c_ident_kind ) {
      case C_FUNCTION:
        unsupp( "Register function", NULL );
        break;
      case C_ARRAY_DIM:
      case C_ARRAY_NO_DIM:
        unsupp( "Register array", NULL );
        break;
      case C_STRUCT:
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
    name ? name : (c_ident_kind == C_FUNCTION) ? "f" : "var", right
  );
  if ( opt_make_c ) {
    if ( c_ident_kind == C_FUNCTION && (*storage != 'e') )
      printf( " { }\n" );
    else
      printf( ";\n" );
  } else {
    printf( "\n" );
  }

done:
  free( (void*)storage );
  free( (void*)left );
  free( (void*)right );
  free( (void*)type );
  if ( name )
    free( (void*)name );
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
    prompt_ptr = prompt_buf;
  else if ( strcmp( opt, "noprompt" ) == 0 )
    prompt_ptr = "";
  else if ( strcmp( opt, "preansi" ) == 0 )
    { opt_lang = LANG_C_KNR; }
  else if ( strcmp( opt, "ansi" ) == 0 )
    opt_lang = LANG_C_89;
  else if ( strcmp( opt, "cplusplus" ) == 0 )
    opt_lang = LANG_CXX;
#ifdef WITH_CDECL_DEBUG
  else if ( strcmp( opt, "debug" ) == 0 )
    opt_debug = true;
  else if ( strcmp( opt, "nodebug" ) == 0 )
    opt_debug = false;
#endif /* WITH_CDECL_DEBUG */
#ifdef WITH_YYDEBUG
  else if ( strcmp( opt, "yydebug" ) == 0 )
    yydebug = 1;
  else if ( strcmp( opt, "noyydebug" ) == 0 )
    yydebug = 0;
#endif /* WITH_YYDEBUG */
  else {
    if ( strcmp( opt, UNKNOWN_NAME ) != 0 &&
         strcmp( opt, "options" ) != 0 ) {
      printf( "\"%s\": unknown set option\n", opt );
    }
    printf( "Valid set options (and command line equivalents) are:\n" );
    printf( "\toptions\n" );
    printf( "\tcreate (-c), nocreate\n" );
    printf( "\tprompt, noprompt (-q)\n" );
#ifndef USE_READLINE
    printf( "\tinteractive (-i), nointeractive\n" );
#endif
    printf( "\tpreansi (-p), ansi (-a), or cplusplus (-+)\n" );
#ifdef WITH_CDECL_DEBUG
    printf( "\tdebug (-d), nodebug\n" );
#endif /* WITH_CDECL_DEBUG */
#ifdef WITH_YYDEBUG
    printf( "\tyydebug (-D), noyydebug\n" );
#endif /* WITH_YYDEBUG */

    printf( "\nCurrent set values are:\n" );
    printf( "\t%screate\n", opt_make_c ? "   " : " no" );
    printf( "\t%sinteractive\n", opt_interactive ? "   " : " no" );
    printf( "\t%sprompt\n", prompt_ptr[0] ? "   " : " no" );

    if ( opt_lang == LANG_C_KNR )
      printf( "\t   preansi\n" );
    else
      printf( "\t(nopreansi)\n" );
    if ( opt_lang == LANG_C_89 )
      printf( "\t   ansi\n" );
    else
      printf( "\t(noansi)\n" );
    if ( opt_lang == LANG_CXX )
      printf( "\t   cplusplus\n" );
    else
      printf( "\t(nocplusplus)\n" );
#ifdef WITH_CDECL_DEBUG
    printf( "\t%sdebug\n", opt_debug ? "   " : " no" );
#endif /* WITH_CDECL_DEBUG */
#ifdef WITH_YYDEBUG
    printf( "\t%syydebug\n", yydebug ? "   " : " no" );
#endif /* WITH_YYDEBUG */
  }
}

/**
 * Do the "explain cast" command.
 *
 * @param constvol TODO
 * @param type TODO
 * @param cast TODO
 * @param name TODO
 */
static void explain_cast( char const *constvol, char const *type,
                          char const *cast, char const *name ) {
  assert( constvol );
  assert( type );
  assert( cast );
  assert( name );

  if ( strcmp( type, "void" ) == 0 ) {
    if ( c_ident_kind == C_ARRAY_NO_DIM )
      unsupp( "array of type void", "array of type pointer to void" );
    else if ( c_ident_kind == CXX_REFERENCE )
      unsupp( "reference to type void", "pointer to void" );
  }
  printf( "cast %s into %s", name, cast );
  if ( strlen( constvol ) > 0 )
    printf( "%s ", constvol );
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

  if ( type && strcmp( type, "void" ) == 0 ) {
    if (c_ident_kind == C_NAME)
      unsupp( "Variable of type void", "variable of type pointer to void" );
    else if ( c_ident_kind == C_ARRAY_NO_DIM )
      unsupp( "array of type void", "array of type pointer to void" );
    else if ( c_ident_kind == CXX_REFERENCE )
      unsupp( "reference to type void", "pointer to void" );
  }

  if ( *storage == CXX_REFERENCE ) {
    switch ( c_ident_kind ) {
      case C_FUNCTION:
        unsupp( "Register function", NULL );
        break;
      case C_ARRAY_DIM:
      case C_ARRAY_NO_DIM:
        unsupp( "Register array", NULL );
        break;
      case C_STRUCT:
        unsupp( "Register struct/union/enum/class", NULL );
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

static void print_help( void );

static void unsupp( char const *s, char const *hint ) {
  illegal( opt_lang, s, NULL );
  if ( hint )
    PRINT_ERR( "\t(maybe you mean \"%s\")\n", hint );
}

static void yyerror( char const *s ) {
  PRINT_ERR( "%s\n", s );
}

int yywrap( void ) {
  return 1;
}

///////////////////////////////////////////////////////////////////////////////

%}

%union {
  char const *dynstr;
  struct {
    char const *left;
    char const *right;
    char const *type;
  } halves;
}

%token  T_ARRAY
%token  T_AS
%token  T_BLOCK                         /* Apple extension */
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

%token  <dynstr> T_AUTO
%token  <dynstr> T_BOOL
%token  <dynstr> T_CHAR
%token  <dynstr> T_CLASS
%token  <dynstr> T_CONST_VOLATILE
%token  <dynstr> T_DOUBLE
%token  <dynstr> T_ENUM
%token  <dynstr> T_EXTERN
%token  <dynstr> T_FLOAT
%token  <dynstr> T_INT
%token  <dynstr> T_LONG
%token  <dynstr> T_NAME
%token  <dynstr> T_NUMBER
%token  <dynstr> T_REGISTER
%token  <dynstr> T_SHORT
%token  <dynstr> T_SIGNED
%token  <dynstr> T_STATIC
%token  <dynstr> T_STRUCT
%token  <dynstr> T_UNION
%token  <dynstr> T_UNSIGNED
%token  <dynstr> T_VOID
%token  <dynstr> T_WCHAR_T

%type   <dynstr> array_dimension
%type   <dynstr> cast cast_list
%type   <dynstr> cdecl cdecl1
%type   <dynstr> cdims
%type   <dynstr> const_volatile_list opt_const_volatile_list
%type   <dynstr> class_struct
%type   <dynstr> c_builtin_type
%type   <dynstr> c_type
%type   <halves> decl_english
%type   <dynstr> decl_list_english
%type   <dynstr> enum_class_struct_union
%type   <halves> func_decl_english
%type   <dynstr> mod_list mod_list1 modifier
%type   <dynstr> opt_NAME
%type   <dynstr> storage opt_storage
%type   <dynstr> type

%start command_list

%%

command_list
  : /* empty */
  | command_list command
    {
      c_ident_kind = 0;
    }
  ;

command
  : cast_english
  | declare_english
  | explain_gibberish
  | set_command

  | T_HELP EOL
    {
      YYTRACE( "command: help\n" );
      print_help();
    }

  | EOL
  | error EOL
    {
      yyerrok;
    }
  ;

EOL
  : '\n'
  | ';'
  ;

/*****************************************************************************/
/*  cast                                                                     */
/*****************************************************************************/

cast_english
  : T_CAST T_NAME T_INTO decl_english EOL
    {
      YYTRACE( "cast_english: CAST NAME AS decl_english\n" );
      YYTRACE( "\tNAME='%s'\n", $2 );
      YYTRACE( "\tacdecl.left='%s'\n", $4.left );
      YYTRACE( "\tacdecl.right='%s'\n", $4.right );
      YYTRACE( "\tacdecl.type='%s'\n", $4.type );
      do_cast( $2, $4.left, $4.right, $4.type );
    }

  | T_CAST decl_english EOL
    {
      YYTRACE( "cast_english: CAST decl_english\n" );
      YYTRACE( "\tacdecl.left='%s'\n", $2.left );
      YYTRACE( "\tacdecl.right='%s'\n", $2.right );
      YYTRACE( "\tacdecl.type='%s'\n", $2.type );
      do_cast( NULL, $2.left, $2.right, $2.type );
    }
  ;

/*****************************************************************************/
/*  declare                                                                  */
/*****************************************************************************/

declare_english
  : T_DECLARE T_NAME T_AS opt_storage decl_english EOL
    {
      YYTRACE( "declare_english: DECLARE NAME AS opt_storage decl_english\n" );
      YYTRACE( "\tNAME='%s'\n", $2 );
      YYTRACE( "\topt_storage='%s'\n", $4 );
      YYTRACE( "\tacdecl.left='%s'\n", $5.left );
      YYTRACE( "\tacdecl.right='%s'\n", $5.right );
      YYTRACE( "\tacdecl.type='%s'\n", $5.type );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
      do_declare( $2, $4, $5.left, $5.right, $5.type );
    }

  | T_DECLARE opt_storage decl_english EOL
    {
      YYTRACE( "declare_english: DECLARE opt_storage decl_english\n" );
      YYTRACE( "\topt_storage='%s'\n", $2 );
      YYTRACE( "\tacdecl.left='%s'\n", $3.left );
      YYTRACE( "\tacdecl.right='%s'\n", $3.right );
      YYTRACE( "\tacdecl.type='%s'\n", $3.type );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
      do_declare( NULL, $2, $3.left, $3.right, $3.type );
    }
  ;

/*****************************************************************************/
/*  explain                                                                  */
/*****************************************************************************/

explain_gibberish
  : T_EXPLAIN opt_storage opt_const_volatile_list type opt_const_volatile_list
              cdecl EOL
    {
      YYTRACE( "explain_gibberish: EXPLAIN opt_storage opt_const_volatile_list type cdecl\n" );
      YYTRACE( "\topt_storage='%s'\n", $2 );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $3 );
      YYTRACE( "\ttype='%s'\n", $4 );
      YYTRACE( "\tcdecl='%s'\n", $6 );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
      explain_declaration( $2, $3, $5, $4, $6 );
    }

  | T_EXPLAIN storage opt_const_volatile_list cdecl EOL
    {
      YYTRACE( "explain_gibberish: EXPLAIN storage opt_const_volatile_list cdecl\n" );
      YYTRACE( "\tstorage='%s'\n", $2 );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $3 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
      explain_declaration( $2, $3, NULL, NULL, $4 );
    }

  | T_EXPLAIN opt_storage const_volatile_list cdecl EOL
    {
      YYTRACE( "explain_gibberish: EXPLAIN opt_storage const_volatile_list cdecl\n" );
      YYTRACE( "\topt_storage='%s'\n", $2 );
      YYTRACE( "\tconst_volatile_list='%s'\n", $3 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
      explain_declaration( $2, $3, NULL, NULL, $4 );
    }

  | T_EXPLAIN '(' opt_const_volatile_list type cast ')' opt_NAME EOL
    {
      YYTRACE( "explain_gibberish: EXPLAIN ( opt_const_volatile_list type cast ) opt_NAME\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $3 );
      YYTRACE( "\ttype='%s'\n", $4 );
      YYTRACE( "\tcast='%s'\n", $5 );
      YYTRACE( "\tNAME='%s'\n", $7 );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
      explain_cast( $3, $4, $5, $7 );
    }
  ;

/*****************************************************************************/
/*  set                                                                      */
/*****************************************************************************/

set_command
  : T_SET opt_NAME EOL
    {
      YYTRACE( "set_command: SET opt_NAME\n" );
      YYTRACE( "\topt_NAME='%s'\n", $2 );
      do_set( $2 );
    }
  ;

/*****************************************************************************/
/*  english productions                                                      */
/*****************************************************************************/

decl_english
  : func_decl_english
  | opt_const_volatile_list T_BLOCK T_RETURNING decl_english
    {
      char const *sp = "";
      YYTRACE( "decl_english: opt_const_volatile_list BLOCK RETURNING decl_english\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $1 );
      YYTRACE( "\tdecl_english.left='%s'\n", $4.left );
      YYTRACE( "\tdecl_english.right='%s'\n", $4.right );
      YYTRACE( "\tdecl_english.type='%s'\n", $4.type );
      if (c_ident_kind == C_FUNCTION)
        unsupp( "Block returning function",
                "block returning pointer to function" );
      else if (c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_ARRAY_NO_DIM)
        unsupp( "Block returning array",
                "block returning pointer" );
      if (strlen($1) != 0)
        sp = " ";
      $$.left = cat( $4.left, strdup( "(^" ), strdup( sp ), $1, strdup( sp ), NULL );
      $$.right = cat( strdup( ")()" ), $4.right, NULL );
      $$.type = $4.type;
      c_ident_kind = C_BLOCK;
    }

  | opt_const_volatile_list T_BLOCK '(' decl_list_english ')' T_RETURNING decl_english
    {
      char const *sp = "";
      YYTRACE( "decl_english: opt_const_volatile_list BLOCK RETURNING decl_english\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $1 );
      YYTRACE( "\tdecl_list_english='%s'\n", $4 );
      YYTRACE( "\tdecl_english.left='%s'\n", $7.left );
      YYTRACE( "\tdecl_english.right='%s'\n", $7.right );
      YYTRACE( "\tdecl_english.type='%s'\n", $7.type );
      if (c_ident_kind == C_FUNCTION)
        unsupp( "Block returning function",
                "block returning pointer to function" );
      else if (c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_ARRAY_NO_DIM)
        unsupp( "Block returning array",
                "block returning pointer" );
      if (strlen($1) != 0)
          sp = " ";
      $$.left = cat( $7.left, strdup( "(^" ), strdup( sp ), $1, strdup( sp ), NULL );
      $$.right = cat( strdup( ")(" ), $4, strdup( ")" ), $7.right, NULL );
      $$.type = $7.type;
      c_ident_kind = C_BLOCK;
    }

  | T_ARRAY array_dimension T_OF decl_english
    {
      YYTRACE( "decl_english: ARRAY array_dimension OF decl_english\n" );
      YYTRACE( "\tarray_dimension='%s'\n", $2 );
      YYTRACE( "\tdecl_english.left='%s'\n", $4.left );
      YYTRACE( "\tdecl_english.right='%s'\n", $4.right );
      YYTRACE( "\tdecl_english.type='%s'\n", $4.type );
      if ( c_ident_kind == C_FUNCTION )
        unsupp( "Array of function", "array of pointer to function" );
      else if ( c_ident_kind == C_ARRAY_NO_DIM )
        unsupp( "Inner array of unspecified size", "array of pointer" );
      else if ( c_ident_kind == C_VOID )
        unsupp( "Array of void", "pointer to void" );
      c_ident_kind = array_has_dim ? C_ARRAY_DIM : C_ARRAY_NO_DIM;
      $$.left = $4.left;
      $$.right = cat( $2, $4.right, NULL );
      $$.type = $4.type;
      YYTRACE( "\n\tdecl_english now =\n" );
      YYTRACE( "\t\tdecl_english.left='%s'\n", $$.left );
      YYTRACE( "\t\tdecl_english.right='%s'\n", $$.right );
      YYTRACE( "\t\tdecl_english.type='%s'\n", $$.type );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | opt_const_volatile_list T_POINTER T_TO decl_english
    {
      char const *op = "", *cp = "", *sp = "";

      YYTRACE( "decl_english: opt_const_volatile_list POINTER TO decl_english\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $1 );
      YYTRACE( "\tdecl_english.left='%s'\n", $4.left );
      YYTRACE( "\tdecl_english.right='%s'\n", $4.right );
      YYTRACE( "\tdecl_english.type='%s'\n", $4.type );
      if ( c_ident_kind == C_ARRAY_NO_DIM )
        unsupp( "Pointer to array of unspecified dimension",
                "pointer to object" );
      if ( c_ident_kind == C_ARRAY_NO_DIM || c_ident_kind == C_ARRAY_DIM ||
           c_ident_kind == C_FUNCTION ) {
        op = "(";
        cp = ")";
      }
      if ( strlen( $1 ) > 0 )
        sp = " ";
      $$.left = cat( $4.left, strdup( op ), strdup( "*" ), strdup( sp ), $1, strdup( sp ), NULL );
      $$.right = cat( strdup( cp ), $4.right, NULL );
      $$.type = $4.type;
      c_ident_kind = C_POINTER;
      YYTRACE( "\n\tdecl_english now =\n" );
      YYTRACE( "\t\tdecl_english.left='%s'\n", $$.left );
      YYTRACE( "\t\tdecl_english.right='%s'\n", $$.right );
      YYTRACE( "\t\tdecl_english.type='%s'\n", $$.type );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | opt_const_volatile_list T_POINTER T_TO T_MEMBER T_OF class_struct T_NAME decl_english
    {
      char const *op = "", *cp = "", *sp = "";

      YYTRACE( "decl_english: opt_const_volatile_list POINTER TO MEMBER OF class_struct NAME decl_english\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $1 );
      YYTRACE( "\tclass_struct='%s'\n", $6 );
      YYTRACE( "\tNAME='%s'\n", $7 );
      YYTRACE( "\tdecl_english.left='%s'\n", $8.left );
      YYTRACE( "\tdecl_english.right='%s'\n", $8.right );
      YYTRACE( "\tdecl_english.type='%s'\n", $8.type );
      if (opt_lang != LANG_CXX)
        unsupp( "pointer to member of class", NULL );
      if ( c_ident_kind == C_ARRAY_NO_DIM )
        unsupp( "Pointer to array of unspecified dimension",
                "pointer to object" );
      if ( c_ident_kind == C_ARRAY_DIM || c_ident_kind == C_ARRAY_NO_DIM ||
           c_ident_kind == C_FUNCTION ) {
        op = "(";
        cp = ")";
      }
      if (strlen($1) != 0)
        sp = " ";
      $$.left = cat( $8.left, strdup( op ), $7 ,strdup( "::*" ), strdup( sp ), $1,strdup( sp ), NULL );
      $$.right = cat( strdup( cp ), $8.right, NULL );
      $$.type = $8.type;
      c_ident_kind = C_POINTER;
      YYTRACE( "\n\tdecl_english now =\n" );
      YYTRACE( "\t\tdecl_english.left='%s'\n", $$.left );
      YYTRACE( "\t\tdecl_english.right='%s'\n", $$.right );
      YYTRACE( "\t\tdecl_english.type='%s'\n", $$.type );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | opt_const_volatile_list T_REFERENCE T_TO decl_english
    {
      char const *op = "", *cp = "", *sp = "";

      YYTRACE( "decl_english: opt_const_volatile_list REFERENCE TO decl_english\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $1 );
      YYTRACE( "\tdecl_english.left='%s'\n", $4.left );
      YYTRACE( "\tdecl_english.right='%s'\n", $4.right );
      YYTRACE( "\tdecl_english.type='%s'\n", $4.type );
      if ( opt_lang != LANG_CXX )
        unsupp( "reference", NULL );
      if ( c_ident_kind == C_VOID )
        unsupp( "Reference to void", "pointer to void" );
      else if ( c_ident_kind == C_ARRAY_NO_DIM )
        unsupp( "Reference to array of unspecified dimension",
                "reference to object" );
      if ( c_ident_kind == C_ARRAY_DIM || c_ident_kind == C_ARRAY_NO_DIM ||
           c_ident_kind == C_FUNCTION ) {
        op = "(";
        cp = ")";
      }
      if ( strlen( $1 ) != 0 )
        sp = " ";
      $$.left = cat( $4.left, strdup( op ), strdup( "&" ), strdup( sp ), $1, strdup( sp ), NULL );
      $$.right = cat( strdup( cp ), $4.right, NULL );
      $$.type = $4.type;
      c_ident_kind = CXX_REFERENCE;
      YYTRACE( "\n\tdecl_english now =\n" );
      YYTRACE( "\t\tdecl_english.left='%s'\n", $$.left );
      YYTRACE( "\t\tdecl_english.right='%s'\n", $$.right );
      YYTRACE( "\t\tdecl_english.type='%s'\n", $$.type );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | opt_const_volatile_list type
    {
      YYTRACE( "decl_english: opt_const_volatile_list type\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $1 );
      YYTRACE( "\ttype='%s'\n", $2 );
      $$.left = strdup( "" );
      $$.right = strdup( "" );
      $$.type = cat( $1, strdup( strlen( $1 ) ? " " : "" ), $2, NULL );
      if ( strcmp( $2, "void" ) == 0 )
        c_ident_kind = C_VOID;
      else if ( strncmp( $2, "struct", 6 ) == 0 ||
                strncmp( $2, "class", 5 ) == 0 )
        c_ident_kind = C_STRUCT;
      else
        c_ident_kind = C_BUILTIN;
      YYTRACE( "\n\tdecl_english now =\n" );
      YYTRACE( "\t\tdecl_english.left='%s'\n", $$.left );
      YYTRACE( "\t\tdecl_english.right='%s'\n", $$.right );
      YYTRACE( "\t\tdecl_english.type='%s'\n", $$.type );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind )  ); 
    }
  ;

decl_list_english
  : /* empty */
    {
      YYTRACE( "decl_list_english: EMPTY\n" );
      $$ = strdup( "" );
    }

  | decl_list_english T_COMMA decl_list_english
    {
      YYTRACE( "decl_list_english: decl_list_english1, decl_list_english2\n" );
      YYTRACE( "\tdecl_list_english1='%s'\n", $1 );
      YYTRACE( "\tdecl_list_english2='%s'\n", $3 );
      $$ = cat( $1, strdup( ", " ), $3, NULL );
    }

  | T_NAME
    {
      YYTRACE( "decl_list_english: NAME\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      $$ = $1;
    }

  | decl_english
    {
      YYTRACE( "decl_list_english: decl_english\n" );
      YYTRACE( "\tdeclaration.left='%s'\n", $1.left );
      YYTRACE( "\tdeclaration.right='%s'\n", $1.right );
      YYTRACE( "\tdeclaration.type='%s'\n", $1.type );
      $$ = cat( $1.type, strdup( " " ), $1.left, $1.right, NULL );
    }

  | T_NAME T_AS decl_english
    {
      YYTRACE( "decl_list_english: NAME AS decl_english\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      YYTRACE( "\tdeclaration.left='%s'\n", $3.left );
      YYTRACE( "\tdeclaration.right='%s'\n", $3.right );
      YYTRACE( "\tdeclaration.type='%s'\n", $3.type );
      $$ = cat( $3.type, strdup( " " ), $3.left, $1, $3.right, NULL );
    }
  ;

func_decl_english
  : T_FUNCTION T_RETURNING decl_english
    {
      YYTRACE( "func_decl_english: FUNCTION RETURNING decl_english\n" );
      YYTRACE( "\tdecl_english.left='%s'\n", $3.left );
      YYTRACE( "\tdecl_english.right='%s'\n", $3.right );
      YYTRACE( "\tdecl_english.type='%s'\n", $3.type );
      if ( c_ident_kind == C_FUNCTION )
        unsupp( "Function returning function",
                "function returning pointer to function" );
      else if ( c_ident_kind == C_ARRAY_DIM || c_ident_kind == C_ARRAY_NO_DIM )
        unsupp( "Function returning array",
                "function returning pointer" );
      $$.left = $3.left;
      $$.right = cat( strdup( "()" ), $3.right, NULL );
      $$.type = $3.type;
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\n\tfunc_decl_english now =\n" );
      YYTRACE( "\t\tfunc_decl_english.left='%s'\n", $$.left );
      YYTRACE( "\t\tfunc_decl_english.right='%s'\n", $$.right );
      YYTRACE( "\t\tfunc_decl_english.type='%s'\n", $$.type );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | T_FUNCTION '(' decl_list_english ')' T_RETURNING decl_english
    {
      YYTRACE( "func_decl_english: FUNCTION (decl_list_english) RETURNING decl_english\n" );
      YYTRACE( "\tdecl_list_english='%s'\n", $3 );
      YYTRACE( "\tdecl_english.left='%s'\n", $6.left );
      YYTRACE( "\tdecl_english.right='%s'\n", $6.right );
      YYTRACE( "\tdecl_english.type='%s'\n", $6.type );
      if (c_ident_kind == C_FUNCTION)
        unsupp( "Function returning function",
                "function returning pointer to function" );
      else if (c_ident_kind==C_ARRAY_DIM || c_ident_kind==C_ARRAY_NO_DIM)
        unsupp( "Function returning array",
                "function returning pointer" );
      $$.left = $6.left;
      $$.right = cat( strdup( "(" ), $3, strdup( ")" ), $6.right ,NULL );
      $$.type = $6.type;
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\n\tfunc_decl_english now =\n" );
      YYTRACE( "\t\tfunc_decl_english.left='%s'\n", $$.left );
      YYTRACE( "\t\tfunc_decl_english.right='%s'\n", $$.right );
      YYTRACE( "\t\tfunc_decl_english.type='%s'\n", $$.type );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }
  ;

/*****************************************************************************/
/*  miscellaneous                                                            */
/*****************************************************************************/

cdecl
  : cdecl1
  | '*' opt_const_volatile_list cdecl
    {
      YYTRACE( "cdecl: * opt_const_volatile_list cdecl\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $2 );
      YYTRACE( "\tcdecl='%s'\n", $3 );
      $$ = cat( $3, $2, strdup( strlen( $2 ) ? " pointer to " : "pointer to " ), NULL );
      c_ident_kind = C_POINTER;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | T_NAME T_DOUBLECOLON '*' cdecl
    {
      YYTRACE( "cdecl: NAME DOUBLECOLON '*' cdecl\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      if (opt_lang != LANG_CXX)
        unsupp( "pointer to member of class", NULL );
      $$ = cat( $4, strdup( "pointer to member of class " ), $1, strdup( " " ), NULL );
      c_ident_kind = C_POINTER;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '&' opt_const_volatile_list cdecl
    {
      YYTRACE( "cdecl: & opt_const_volatile_list cdecl\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $2 );
      YYTRACE( "\tcdecl='%s'\n", $3 );
      if (opt_lang != LANG_CXX)
        unsupp( "reference", NULL );
      $$ = cat( $3, $2, strdup( strlen( $2 ) ? " reference to " : "reference to " ), NULL );
      c_ident_kind = CXX_REFERENCE;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }
  ;

cdecl1
  : cdecl1 '(' ')'
    {
      YYTRACE( "cdecl1: cdecl1()\n" );
      YYTRACE( "\tcdecl1='%s'\n", $1 );
      $$ = cat( $1, strdup( "function returning " ), NULL );
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '(' '^' opt_const_volatile_list cdecl ')' '(' ')'
    {
      char const *sp = "";
      YYTRACE( "cdecl1: (^ opt_const_volatile_list cdecl)()\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $3 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      if (strlen($3) > 0)
          sp = " ";
      $$ = cat( $4, $3, strdup( sp ), strdup( "block returning " ), NULL );
      c_ident_kind = C_BLOCK;
    }

  | '(' '^' opt_const_volatile_list cdecl ')' '(' cast_list ')'
    {
      char const *sp = "";
      YYTRACE( "cdecl1: (^ opt_const_volatile_list cdecl)( cast_list )\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $3 );
      YYTRACE( "\tcdecl='%s'\n", $4 );
      YYTRACE( "\tcast_list='%s'\n", $7 );
      if (strlen($3) > 0)
        sp = " ";
      $$ = cat( $4, $3, strdup( sp ), strdup( "block (" ), $7, strdup( ") returning " ), NULL );
      c_ident_kind = C_BLOCK;
    }

  | cdecl1 '(' cast_list ')'
    {
      YYTRACE( "cdecl1: cdecl1(cast_list)\n" );
      YYTRACE( "\tcdecl1='%s'\n", $1 );
      YYTRACE( "\tcast_list='%s'\n", $3 );
      $$ = cat( $1, strdup( "function (" ), $3, strdup( ") returning " ), NULL );
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | cdecl1 cdims
    {
      YYTRACE( "cdecl1: cdecl1 cdims\n" );
      YYTRACE( "\tcdecl1='%s'\n", $1 );
      YYTRACE( "\tcdims='%s'\n", $2 );
      $$ = cat( $1, strdup( "array " ), $2 ,NULL );
      c_ident_kind = C_ARRAY_NO_DIM;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '(' cdecl ')'
    {
      YYTRACE( "cdecl1: (cdecl)\n" );
      YYTRACE( "\tcdecl='%s'\n", $2 );
      $$ = $2;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | T_NAME
    {
      YYTRACE( "cdecl1: NAME\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      c_ident = $1;
      $$ = strdup( "" );
      c_ident_kind = C_NAME;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }
  ;

cast_list
  : cast_list T_COMMA cast_list
    {
      YYTRACE( "cast_list: cast_list1, cast_list2\n" );
      YYTRACE( "\tcast_list1='%s'\n", $1 );
      YYTRACE( "\tcast_list2='%s'\n", $3 );
      $$ = cat( $1, strdup( ", " ), $3, NULL );
    }

  | opt_const_volatile_list type cast
    {
      YYTRACE( "cast_list: opt_const_volatile_list type cast\n" );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $1 );
      YYTRACE( "\ttype='%s'\n", $2 );
      YYTRACE( "\tcast='%s'\n", $3 );
      $$ = cat( $3, $1, strdup( strlen( $1 ) ? " " : "" ), $2, NULL );
    }

  | T_NAME
    {
      $$ = $1;
    }
  ;

cast
  : /* empty */
    {
      YYTRACE( "cast: EMPTY\n" );
      $$ = strdup( "" );
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '(' ')'
    {
      YYTRACE( "cast: ()\n" );
      $$ = strdup( "function returning " );
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '(' cast ')' '(' ')'
    {
      YYTRACE( "cast: (cast)()\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      $$ = cat( $2, strdup( "function returning " ), NULL );
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '(' cast ')' '(' cast_list ')'
    {
      YYTRACE( "cast: (cast)(cast_list)\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      YYTRACE( "\tcast_list='%s'\n", $5 );
      $$ = cat( $2, strdup( "function (" ), $5, strdup( ") returning " ), NULL );
      c_ident_kind = C_FUNCTION;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '(' '^' cast ')' '(' ')'
    {
      YYTRACE( "cast: (^ cast)()\n" );
      YYTRACE( "\tcast='%s'\n", $3 );
      $$ = cat( $3, strdup( "block returning " ), NULL );
      c_ident_kind = C_BLOCK;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '(' '^' cast ')' '(' cast_list ')'
    {
      YYTRACE( "cast: (^ cast)(cast_list)\n" );
      YYTRACE( "\tcast='%s'\n", $3 );
      $$ = cat( $3, strdup( "block (" ), $6, strdup( ") returning " ), NULL );
      c_ident_kind = C_BLOCK;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '(' cast ')'
    {
      YYTRACE( "cast: (cast)\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      $$ = $2;
      /* c_ident_kind = c_ident_kind; */
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | T_NAME T_DOUBLECOLON '*' cast
    {
      YYTRACE( "cast: NAME::*cast\n" );
      YYTRACE( "\tcast='%s'\n", $4 );
      if (opt_lang != LANG_CXX)
        unsupp( "pointer to member of class", NULL );
      $$ = cat( $4, strdup( "pointer to member of class " ), $1, strdup( " " ), NULL );
      c_ident_kind = C_POINTER;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '*' cast
    {
      YYTRACE( "cast: *cast\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      $$ = cat( $2, strdup( "pointer to " ), NULL );
      c_ident_kind = C_POINTER;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | '&' cast
    {
      YYTRACE( "cast: &cast\n" );
      YYTRACE( "\tcast='%s'\n", $2 );
      if ( opt_lang != LANG_CXX )
        unsupp( "reference", NULL );
      $$ = cat( $2, strdup( "reference to " ), NULL );
      c_ident_kind = CXX_REFERENCE;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
    }

  | cast cdims
    {
      YYTRACE( "cast: cast cdims\n" );
      YYTRACE( "\tcast='%s'\n", $1 );
      YYTRACE( "\tcdims='%s'\n", $2 );
      $$ = cat( $1, strdup( "array " ), $2, NULL );
      c_ident_kind = C_ARRAY_NO_DIM;
      YYTRACE( "\tc_ident_kind = '%s'\n", visible( c_ident_kind ) );
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
      $$ = cat( $2, strdup( " of " ), NULL );
    }
  ;

array_dimension
  : /* empty */
    {
      YYTRACE( "array_dimension: EMPTY\n" );
      array_has_dim = false;
      $$ = strdup( "[]" );
    }

  | T_NUMBER
    {
      YYTRACE( "array_dimension: NUMBER\n" );
      YYTRACE( "\tNUMBER='%s'\n", $1 );
      array_has_dim = true;
      $$ = cat( strdup( "[" ), $1, strdup( "]" ), NULL );
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

  | c_builtin_type
    {
      YYTRACE( "c_type: c_builtin_type\n" );
      YYTRACE( "\tc_builtin_type='%s'\n", $1 );
      $$ = $1;
    }

  | mod_list c_builtin_type
    {
      YYTRACE( "c_type: mod_list c_builtin_type\n" );
      YYTRACE( "\tmod_list='%s'\n", $1 );
      YYTRACE( "\tc_builtin_type='%s'\n", $2 );
      $$ = cat( $1, strdup(" "), $2, NULL );
    }

  | enum_class_struct_union T_NAME
    {
      YYTRACE( "c_type: enum_class_struct_union NAME\n" );
      YYTRACE( "\tenum_class_struct_union='%s'\n", $1 );
      YYTRACE( "\tNAME='%s'\n", $2 );
      $$ = cat( $1, strdup( " " ), $2, NULL );
    }
  ;

enum_class_struct_union
  : class_struct
  | T_ENUM
  | T_UNION
    {
      $$ = $1;
    }
  ;

class_struct
  : T_STRUCT
  | T_CLASS
    {
      $$ = $1;
    }
  ;

c_builtin_type
  : T_BOOL
    {
      YYTRACE( "c_builtin_type: BOOL\n" );
      YYTRACE( "\tBOOL='%s'\n", $1 );
      c_type_add( C_TYPE_BOOL );
      $$ = $1;
    }

  | T_CHAR
    {
      YYTRACE( "c_builtin_type: CHAR\n" );
      YYTRACE( "\tCHAR='%s'\n", $1 );
      c_type_add( C_TYPE_CHAR );
      $$ = $1;
    }

  | T_WCHAR_T
    {
      YYTRACE( "c_builtin_type: WCHAR_T\n" );
      YYTRACE( "\tWCHAR_T='%s'\n", $1 );
      if ( opt_lang == LANG_C_KNR )
        illegal( opt_lang, $1, NULL );
      c_type_add( C_TYPE_WCHAR_T );
      $$ = $1;
    }

  | T_INT
    {
      YYTRACE( "c_builtin_type: INT\n" );
      YYTRACE( "\tINT='%s'\n", $1 );
      c_type_add( C_TYPE_INT );
      $$ = $1;
    }

  | T_FLOAT
    {
      YYTRACE( "c_builtin_type: FLOAT\n" );
      YYTRACE( "\tFLOAT='%s'\n", $1 );
      c_type_add( C_TYPE_FLOAT );
      $$ = $1;
    }

  | T_DOUBLE
    {
      YYTRACE( "c_builtin_type: DOUBLE\n" );
      YYTRACE( "\tDOUBLE='%s'\n", $1 );
      c_type_add( C_TYPE_DOUBLE );
      $$ = $1;
    }

  | T_VOID
    {
      YYTRACE( "c_builtin_type: VOID\n" );
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
      $$ = cat( $1, strdup( " " ), $2, NULL );
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
      YYTRACE( "mod_list1: CONST_VOLATILE\n" );
      YYTRACE( "\tCONST_VOLATILE='%s'\n", $1 );
      if ( opt_lang == LANG_C_KNR )
        illegal( opt_lang, $1, NULL );
      else if ( strcmp( $1, "noalias" ) == 0 && opt_lang == LANG_CXX )
        unsupp( $1, NULL );
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

const_volatile_list
  : T_CONST_VOLATILE opt_const_volatile_list
    {
      YYTRACE( "const_volatile_list: CONST_VOLATILE opt_const_volatile_list\n" );
      YYTRACE( "\tCONST_VOLATILE='%s'\n", $1 );
      YYTRACE( "\topt_const_volatile_list='%s'\n", $2 );
      if ( opt_lang == LANG_C_KNR )
        illegal( opt_lang, $1, NULL );
      else if ( strcmp( $1, "noalias" ) == 0 && opt_lang == LANG_CXX )
        unsupp( $1, NULL );
      $$ = cat( $1, strdup( strlen( $2 ) ? " " : "" ), $2, NULL );
    }
  ;

opt_const_volatile_list
  : /* empty */
    {
      YYTRACE( "opt_const_volatile_list: EMPTY\n" );
      $$ = strdup( "" );
    }
  | const_volatile_list
  ;

opt_NAME
  : /* empty */
    {
      YYTRACE( "opt_NAME: EMPTY\n" );
      $$ = strdup( UNKNOWN_NAME );
    }

  | T_NAME
    {
      YYTRACE( "opt_NAME: NAME\n" );
      YYTRACE( "\tNAME='%s'\n", $1 );
      $$ = $1;
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
  : /* empty */
    {
      YYTRACE( "opt_storage: EMPTY\n" );
      $$ = strdup( "" );
    }

  | storage
    {
      YYTRACE( "opt_storage: storage=%s\n", $1 );
      $$ = $1;
    }
  ;

%%

///////////////////////////////////////////////////////////////////////////////

/* the help messages */
struct help_text {
  char const *text;                     // generic text 
  char const *cpp_text;                 // C++ specific text 
};
typedef struct help_text help_text_t;

/**
 * Help text (limited to 80 columns and 23 lines so it fits on an 80x24
 * screen).
 */
static help_text_t const HELP_TEXT[] = {
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
/* 19 */{ "decllist: a comma separated list of <name>, <english>, or <name> as <english>", NULL },
/* 20 */{ "name: a C identifier", NULL },
/* 21 */{ "gibberish: a C declaration, like 'int *x', or cast, like '(int *)x'", NULL },
/* 22 */{ "storage-class: auto, extern, register, or static",
          "storage-class: extern, register, or static" },
/* 23 */{ "C-type: bool, int, char, wchar_t, float, double, or void", NULL },
/* 24 */{ "modifier: short, long, signed, unsigned, const, volatile, or noalias",
          "modifier: short, long, signed, unsigned, const, or volatile" },
  { NULL, NULL }
};

/**
 * Prints the help message to standard output.
 */
static void print_help( void ) {
  char const *const fmt = opt_lang == LANG_CXX ? " %s\n" : "  %s\n";

  for ( help_text_t const *ht = HELP_TEXT; ht->text; ++ht ) {
    if ( opt_lang == LANG_CXX && ht->cpp_text )
      printf( fmt, ht->cpp_text );
    else
      printf( fmt, ht->text );
  } // for
}

///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */
