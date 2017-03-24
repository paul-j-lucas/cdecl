/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     Y_CAST = 258,
     Y_DECLARE = 259,
     Y_EXPLAIN = 260,
     Y_HELP = 261,
     Y_SET = 262,
     Y_QUIT = 263,
     Y_ARRAY = 264,
     Y_AS = 265,
     Y_BLOCK = 266,
     Y_FUNCTION = 267,
     Y_INTO = 268,
     Y_MEMBER = 269,
     Y_OF = 270,
     Y_POINTER = 271,
     Y_REFERENCE = 272,
     Y_RETURNING = 273,
     Y_TO = 274,
     Y_AUTO = 275,
     Y_CHAR = 276,
     Y_DOUBLE = 277,
     Y_EXTERN = 278,
     Y_FLOAT = 279,
     Y_INT = 280,
     Y_LONG = 281,
     Y_REGISTER = 282,
     Y_SHORT = 283,
     Y_STATIC = 284,
     Y_STRUCT = 285,
     Y_TYPEDEF = 286,
     Y_UNION = 287,
     Y_UNSIGNED = 288,
     Y_CONST = 289,
     Y_ENUM = 290,
     Y_SIGNED = 291,
     Y_VOID = 292,
     Y_VOLATILE = 293,
     Y_BOOL = 294,
     Y_COMPLEX = 295,
     Y_RESTRICT = 296,
     Y_WCHAR_T = 297,
     Y_NORETURN = 298,
     Y_THREAD_LOCAL = 299,
     Y_CLASS = 300,
     Y_COLON_COLON = 301,
     Y_CHAR16_T = 302,
     Y_CHAR32_T = 303,
     Y___BLOCK = 304,
     Y_END = 305,
     Y_ERROR = 306,
     Y_NAME = 307,
     Y_NUMBER = 308
   };
#endif
/* Tokens.  */
#define Y_CAST 258
#define Y_DECLARE 259
#define Y_EXPLAIN 260
#define Y_HELP 261
#define Y_SET 262
#define Y_QUIT 263
#define Y_ARRAY 264
#define Y_AS 265
#define Y_BLOCK 266
#define Y_FUNCTION 267
#define Y_INTO 268
#define Y_MEMBER 269
#define Y_OF 270
#define Y_POINTER 271
#define Y_REFERENCE 272
#define Y_RETURNING 273
#define Y_TO 274
#define Y_AUTO 275
#define Y_CHAR 276
#define Y_DOUBLE 277
#define Y_EXTERN 278
#define Y_FLOAT 279
#define Y_INT 280
#define Y_LONG 281
#define Y_REGISTER 282
#define Y_SHORT 283
#define Y_STATIC 284
#define Y_STRUCT 285
#define Y_TYPEDEF 286
#define Y_UNION 287
#define Y_UNSIGNED 288
#define Y_CONST 289
#define Y_ENUM 290
#define Y_SIGNED 291
#define Y_VOID 292
#define Y_VOLATILE 293
#define Y_BOOL 294
#define Y_COMPLEX 295
#define Y_RESTRICT 296
#define Y_WCHAR_T 297
#define Y_NORETURN 298
#define Y_THREAD_LOCAL 299
#define Y_CLASS 300
#define Y_COLON_COLON 301
#define Y_CHAR16_T 302
#define Y_CHAR32_T 303
#define Y___BLOCK 304
#define Y_END 305
#define Y_ERROR 306
#define Y_NAME 307
#define Y_NUMBER 308




/* Copy the first part of user declarations.  */
#line 6 "parser.y"

// local
#include "config.h"                     /* must come first */
#include "ast.h"
#include "ast_util.h"
#include "color.h"
#include "common.h"
#include "keywords.h"
#include "lang.h"
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

#ifdef WITH_CDECL_DEBUG
#define CDEBUG(...) BLOCK( if ( opt_debug ) { __VA_ARGS__ } )
#else
#define CDEBUG(...)                     /* nothing */
#endif /* WITH_CDECL_DEBUG */

#define C_AST_CHECK(AST,CHECK) BLOCK( \
  if ( !c_ast_check( (AST), (CHECK) ) ) PARSE_CLEANUP(); )

#define C_TYPE_ADD(DST,SRC,LOC) BLOCK( \
  if ( !c_type_add( (DST), (SRC), &(LOC) ) ) PARSE_CLEANUP(); )

#define C_TYPE_CHECK(TYPE,LOC) BLOCK( \
  if ( !c_type_check( TYPE, &(LOC) ) ) PARSE_CLEANUP(); )

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

#define PARSE_CLEANUP()   BLOCK( parse_cleanup(); yyclearin; YYABORT; )
#define PARSE_ERROR(...)  BLOCK( parse_error( __VA_ARGS__ ); PARSE_CLEANUP(); )

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
  qualifier_link_t *qualifier_head;
  c_ast_t *type_ast;
  int y_token;
};
typedef struct in_attr in_attr_t;

// extern functions
extern void         print_help( void );
extern void         set_option( char const* );
extern int          yylex();

// local variables
static in_attr_t    in_attr;
static bool         newlined = true;
static unsigned     ast_depth;

// local functions
static void         qualifier_clear( void );

////////// inline functions ///////////////////////////////////////////////////

/**
 * Peeks at the type AST at the head of the type AST inherited attribute stack.
 *
 * @return Returns said AST.
 */
static inline c_ast_t* type_peek( void ) {
  return in_attr.type_ast;
}

/**
 * Pops a type AST from the type AST inherited attribute stack.
 *
 * @return Returns said AST.
 */
static inline c_ast_t* type_pop( void ) {
  --ast_depth;
  return LINK_POP( c_ast_t, &in_attr.type_ast );
}

/**
 * Pushes a type AST onto the type AST inherited attribute stack.
 *
 * @param ast The AST to push.
 */
static inline void type_push( c_ast_t *ast ) {
  LINK_PUSH( &in_attr.type_ast, ast );
  ++ast_depth;
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
 */
static void parse_cleanup( void ) {
  c_ast_gc();
  qualifier_clear();
  memset( &in_attr, 0, sizeof in_attr );
}

/**
 * Prints a parsing error message to standard error.
 *
 * @param format A \c printf() style format string.
 */
static void parse_error( char const *format, ... ) {
  if ( !newlined ) {
    PRINT_ERR( ": " );
    if ( *yytext )
      PRINT_ERR( "\"%s\": ", (*yytext == '\n' ? "\\n" : yytext) );
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );
    PRINT_ERR( "\n" );
    newlined = true;
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
  assert( loc );
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
 * Called by bison to print a parsing error message.
 *
 * @param msg The error message to print.
 */
static void yyerror( char const *msg ) {
  print_caret( CARET_CURRENT_LEX_COL );
  PRINT_ERR( "%s%d: ", (newlined ? "" : "\n"), error_column() + 1 );
  SGR_START_COLOR( stderr, error );
  FPUTS( msg, stderr );
  SGR_END_COLOR( stderr );
  newlined = false;
  parse_cleanup();
}

///////////////////////////////////////////////////////////////////////////////


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 253 "parser.y"
{
  c_ast_list_t  ast_list; /* for function arguments */
  c_ast_pair_t  ast_pair; /* for the AST being built */
  char const   *name;     /* name being declared or explained */
  int           number;   /* for array sizes */
  c_type_t      type;     /* built-in types, storage classes, & qualifiers */
}
/* Line 193 of yacc.c.  */
#line 457 "parser.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 482 "parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   381

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  62
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  88
/* YYNRULES -- Number of rules.  */
#define YYNRULES  182
/* YYNRULES -- Number of states.  */
#define YYNSTATES  255

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   308

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    51,     2,
      24,    25,    21,     2,    20,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    22,     2,    23,    56,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      52,    53,    54,    55,    57,    58,    59,    60,    61
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     9,    10,    12,    14,    16,    18,
      20,    22,    24,    26,    29,    30,    36,    40,    44,    51,
      54,    58,    59,    65,    66,    75,    78,    82,    85,    86,
      88,    90,    92,    94,    96,    98,   100,   103,   104,   113,
     116,   117,   124,   126,   129,   134,   137,   138,   142,   143,
     145,   149,   151,   152,   156,   158,   160,   162,   164,   166,
     171,   172,   174,   176,   177,   182,   183,   188,   189,   193,
     194,   196,   198,   202,   205,   207,   208,   212,   214,   216,
     218,   220,   222,   225,   228,   231,   238,   241,   245,   250,
     256,   260,   263,   267,   269,   271,   273,   275,   277,   279,
     281,   283,   285,   287,   290,   293,   297,   301,   302,   312,
     317,   319,   320,   326,   327,   328,   332,   335,   336,   340,
     344,   345,   349,   352,   355,   357,   358,   360,   363,   365,
     367,   369,   371,   373,   375,   377,   379,   381,   383,   387,
     390,   392,   393,   395,   398,   400,   402,   404,   406,   408,
     410,   412,   414,   416,   418,   420,   422,   424,   427,   430,
     432,   434,   436,   438,   440,   441,   444,   446,   448,   450,
     451,   453,   455,   457,   459,   461,   463,   465,   467,   469,
     471,   473,   474
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
      63,     0,    -1,    -1,    63,    64,    65,    66,    -1,    -1,
      67,    -1,    68,    -1,    69,    -1,    71,    -1,    73,    -1,
      74,    -1,    75,    -1,    58,    -1,     1,    58,    -1,    -1,
       3,    60,    13,    91,    58,    -1,     3,    60,     1,    -1,
       3,    91,    58,    -1,     4,    60,    10,   145,    91,    58,
      -1,     4,     1,    -1,     4,    60,     1,    -1,    -1,     5,
     135,    70,   110,    58,    -1,    -1,     5,    24,   135,    72,
      76,    25,   149,    58,    -1,     6,    58,    -1,     7,   149,
      58,    -1,     8,    58,    -1,    -1,    77,    -1,    78,    -1,
      80,    -1,    82,    -1,    83,    -1,    84,    -1,    85,    -1,
      76,   113,    -1,    -1,    24,    56,    79,    76,    25,    24,
      87,    25,    -1,    24,    25,    -1,    -1,    24,   120,    81,
      76,    25,    86,    -1,    60,    -1,    21,    76,    -1,    60,
      53,   148,    76,    -1,    51,    76,    -1,    -1,    24,    87,
      25,    -1,    -1,    88,    -1,    88,   147,    89,    -1,    89,
      -1,    -1,   135,    90,    76,    -1,    60,    -1,    92,    -1,
      96,    -1,   102,    -1,   109,    -1,     9,    93,    15,    91,
      -1,    -1,    61,    -1,     1,    -1,    -1,    11,    95,    98,
     101,    -1,    -1,    12,    97,    98,   101,    -1,    -1,    24,
      99,    25,    -1,    -1,   100,    -1,    91,    -1,   100,   147,
      91,    -1,    18,    91,    -1,     1,    -1,    -1,   143,   103,
     104,    -1,    94,    -1,   105,    -1,   107,    -1,   108,    -1,
     130,    -1,   106,    91,    -1,    16,    19,    -1,    16,     1,
      -1,   106,    14,    15,   142,    60,    91,    -1,   106,     1,
      -1,   106,    14,     1,    -1,   106,    14,    15,     1,    -1,
     106,    14,    15,   142,     1,    -1,    17,    19,    91,    -1,
      17,     1,    -1,    60,    10,    91,    -1,    60,    -1,   111,
      -1,   121,    -1,   124,    -1,   127,    -1,   112,    -1,   114,
      -1,   116,    -1,   117,    -1,   118,    -1,   111,   113,    -1,
      22,    23,    -1,    22,    61,    23,    -1,    22,     1,    23,
      -1,    -1,    24,    56,   115,   143,   110,    25,    24,    87,
      25,    -1,   111,    24,    87,    25,    -1,    60,    -1,    -1,
      24,   120,   119,   110,    25,    -1,    -1,    -1,   123,   122,
     110,    -1,    21,   143,    -1,    -1,   126,   125,   110,    -1,
      60,    53,   148,    -1,    -1,   129,   128,   110,    -1,    51,
     143,    -1,   131,   134,    -1,   132,    -1,    -1,   132,    -1,
     131,   133,    -1,   133,    -1,    46,    -1,    32,    -1,    34,
      -1,    42,    -1,    39,    -1,    33,    -1,   139,    -1,   141,
      -1,   137,    -1,   137,   139,   136,    -1,   139,   136,    -1,
     140,    -1,    -1,   137,    -1,   137,   138,    -1,   138,    -1,
     133,    -1,   144,    -1,   146,    -1,    43,    -1,    45,    -1,
      27,    -1,    54,    -1,    55,    -1,    48,    -1,    31,    -1,
      30,    -1,    28,    -1,   141,    60,    -1,   141,     1,    -1,
      41,    -1,   142,    -1,    38,    -1,    52,    -1,    36,    -1,
      -1,   143,   144,    -1,    40,    -1,    47,    -1,    44,    -1,
      -1,   146,    -1,    26,    -1,    57,    -1,    29,    -1,    35,
      -1,    37,    -1,    50,    -1,    20,    -1,     1,    -1,    21,
      -1,     1,    -1,    -1,    60,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   398,   398,   400,   405,   411,   412,   413,   414,   415,
     416,   417,   418,   419,   427,   437,   451,   456,   474,   490,
     495,   506,   506,   529,   529,   559,   567,   575,   583,   584,
     585,   586,   587,   588,   589,   590,   594,   622,   621,   649,
     662,   662,   683,   700,   717,   737,   758,   759,   763,   764,
     768,   781,   795,   795,   810,   828,   829,   830,   831,   835,
     868,   869,   870,   874,   874,   895,   895,   914,   915,   929,
     930,   934,   935,   951,   996,  1000,  1000,  1017,  1018,  1019,
    1020,  1021,  1025,  1042,  1043,  1050,  1074,  1079,  1084,  1091,
    1101,  1129,  1136,  1150,  1172,  1173,  1174,  1175,  1179,  1180,
    1181,  1182,  1183,  1187,  1213,  1214,  1215,  1223,  1222,  1253,
    1271,  1288,  1288,  1305,  1312,  1312,  1328,  1345,  1345,  1361,
    1379,  1379,  1394,  1415,  1431,  1447,  1448,  1452,  1466,  1470,
    1471,  1472,  1473,  1474,  1480,  1484,  1497,  1516,  1531,  1551,
    1567,  1571,  1572,  1576,  1591,  1599,  1600,  1601,  1605,  1606,
    1607,  1608,  1609,  1610,  1611,  1612,  1613,  1617,  1634,  1643,
    1644,  1645,  1649,  1650,  1654,  1655,  1672,  1673,  1674,  1678,
    1679,  1683,  1684,  1685,  1687,  1688,  1689,  1697,  1698,  1702,
    1703,  1707,  1708
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "Y_CAST", "Y_DECLARE", "Y_EXPLAIN",
  "Y_HELP", "Y_SET", "Y_QUIT", "Y_ARRAY", "Y_AS", "Y_BLOCK", "Y_FUNCTION",
  "Y_INTO", "Y_MEMBER", "Y_OF", "Y_POINTER", "Y_REFERENCE", "Y_RETURNING",
  "Y_TO", "','", "'*'", "'['", "']'", "'('", "')'", "Y_AUTO", "Y_CHAR",
  "Y_DOUBLE", "Y_EXTERN", "Y_FLOAT", "Y_INT", "Y_LONG", "Y_REGISTER",
  "Y_SHORT", "Y_STATIC", "Y_STRUCT", "Y_TYPEDEF", "Y_UNION", "Y_UNSIGNED",
  "Y_CONST", "Y_ENUM", "Y_SIGNED", "Y_VOID", "Y_VOLATILE", "Y_BOOL",
  "Y_COMPLEX", "Y_RESTRICT", "Y_WCHAR_T", "Y_NORETURN", "Y_THREAD_LOCAL",
  "'&'", "Y_CLASS", "Y_COLON_COLON", "Y_CHAR16_T", "Y_CHAR32_T", "'^'",
  "Y___BLOCK", "Y_END", "Y_ERROR", "Y_NAME", "Y_NUMBER", "$accept",
  "command_list", "command_init", "command", "command_cleanup",
  "cast_english", "declare_english", "explain_declaration_c", "@1",
  "explain_cast_c", "@2", "help_command", "set_command", "quit_command",
  "cast_c", "array_cast_c", "block_cast_c", "@3", "func_cast_c", "@4",
  "name_cast_c", "pointer_cast_c", "pointer_to_member_cast_c",
  "reference_cast_c", "paren_arg_list_opt_opt_c", "arg_list_opt_c",
  "arg_list_c", "arg_c", "@5", "decl_english", "array_decl_english",
  "array_size_opt_english", "block_decl_english", "@6",
  "func_decl_english", "@7", "paren_decl_list_opt_english",
  "decl_list_opt_english", "decl_list_english", "returning_english",
  "qualified_decl_english", "@8", "qualifiable_decl_english",
  "pointer_decl_english", "pointer_to", "pointer_to_member_decl_english",
  "reference_decl_english", "var_decl_english", "decl_c", "decl2_c",
  "array_decl_c", "array_size_c", "block_decl_c", "@9", "func_decl_c",
  "name_decl_c", "nested_decl_c", "@10", "placeholder_type_c",
  "pointer_decl_c", "@11", "pointer_decl_type_c",
  "pointer_to_member_decl_c", "@12", "pointer_to_member_decl_type_c",
  "reference_decl_c", "@13", "reference_decl_type_c", "type_english",
  "type_modifier_list_opt_english", "type_modifier_list_english",
  "type_modifier_english", "unmodified_type_english", "type_c",
  "type_modifier_list_opt_c", "type_modifier_list_c", "type_modifier_c",
  "builtin_type_c", "named_enum_class_struct_union_type_c",
  "enum_class_struct_union_type_c", "class_struct_type_c",
  "type_qualifier_list_opt_c", "type_qualifier_c", "storage_class_opt_c",
  "storage_class_c", "expect_comma", "expect_star", "name_token_opt", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
      44,    42,    91,    93,    40,    41,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,    38,   300,   301,   302,   303,    94,   304,   305,   306,
     307,   308
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    62,    63,    63,    64,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    66,    67,    67,    67,    68,    68,
      68,    70,    69,    72,    71,    73,    74,    75,    76,    76,
      76,    76,    76,    76,    76,    76,    77,    79,    78,    80,
      81,    80,    82,    83,    84,    85,    86,    86,    87,    87,
      88,    88,    90,    89,    89,    91,    91,    91,    91,    92,
      93,    93,    93,    95,    94,    97,    96,    98,    98,    99,
      99,   100,   100,   101,   101,   103,   102,   104,   104,   104,
     104,   104,   105,   106,   106,   107,   107,   107,   107,   107,
     108,   108,   109,   109,   110,   110,   110,   110,   111,   111,
     111,   111,   111,   112,   113,   113,   113,   115,   114,   116,
     117,   119,   118,   120,   122,   121,   123,   125,   124,   126,
     128,   127,   129,   130,   130,   131,   131,   132,   132,   133,
     133,   133,   133,   133,   133,   134,   134,   135,   135,   135,
     135,   136,   136,   137,   137,   138,   138,   138,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   140,   140,   141,
     141,   141,   142,   142,   143,   143,   144,   144,   144,   145,
     145,   146,   146,   146,   146,   146,   146,   147,   147,   148,
     148,   149,   149
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     4,     0,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     0,     5,     3,     3,     6,     2,
       3,     0,     5,     0,     8,     2,     3,     2,     0,     1,
       1,     1,     1,     1,     1,     1,     2,     0,     8,     2,
       0,     6,     1,     2,     4,     2,     0,     3,     0,     1,
       3,     1,     0,     3,     1,     1,     1,     1,     1,     4,
       0,     1,     1,     0,     4,     0,     4,     0,     3,     0,
       1,     1,     3,     2,     1,     0,     3,     1,     1,     1,
       1,     1,     2,     2,     2,     6,     2,     3,     4,     5,
       3,     2,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     3,     3,     0,     9,     4,
       1,     0,     5,     0,     0,     3,     2,     0,     3,     3,
       0,     3,     2,     2,     1,     0,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     2,
       1,     0,     1,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     1,
       1,     1,     1,     1,     0,     2,     1,     1,     1,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     4,     1,     0,     0,   164,     0,     0,     0,   181,
       0,    12,    14,     5,     6,     7,     8,     9,    10,    11,
      13,     0,    65,     0,     0,    55,    56,    57,    58,    75,
      19,     0,     0,   171,   150,   156,   173,   155,   154,   130,
     134,   131,   174,   163,   175,   161,   133,   166,   159,   132,
     148,   168,   149,   129,   167,   153,   176,   162,   151,   152,
     172,   145,    21,   137,   144,   141,   140,     0,   160,   146,
     147,    25,   182,     0,    27,     3,    62,    61,     0,    67,
      16,   164,   164,    17,   125,   165,    20,   169,    23,     0,
     143,   141,   139,   142,   158,   157,    26,   164,   164,     0,
      93,    92,     0,    63,     0,     0,    77,    76,    78,     0,
      79,    80,    81,     0,   126,   128,   164,   170,    28,   164,
     113,   164,   110,     0,    94,    98,    99,   100,   101,   102,
      95,   114,    96,   117,    97,   120,   138,    59,    71,     0,
       0,    74,   164,    66,    15,    67,    84,    83,    91,   164,
      86,     0,    82,   127,   123,   135,   136,     0,    28,   113,
      28,    42,     0,    29,    30,    31,    32,    33,    34,    35,
     116,   107,   111,   122,     0,    22,     0,    48,   103,     0,
       0,     0,    68,   178,   177,   164,    73,     0,    90,    87,
       0,    18,    43,    39,    37,    40,    45,     0,   181,    36,
     164,     0,   180,   179,   119,     0,   104,     0,    54,     0,
       0,    51,    52,   115,   118,   121,    72,    64,    88,     0,
      28,    28,    28,     0,     0,     0,   106,   105,   109,     0,
      28,    89,   164,     0,     0,    44,    24,     0,   112,    50,
      53,    85,     0,    46,     0,    48,    48,    41,    48,     0,
       0,     0,    38,    47,   108
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     3,    12,    75,    13,    14,    15,    89,    16,
     118,    17,    18,    19,   162,   163,   164,   220,   165,   221,
     166,   167,   168,   169,   247,   209,   210,   211,   230,    24,
      25,    78,   106,   145,    26,    79,    99,   139,   140,   143,
      27,    84,   107,   108,   109,   110,   111,    28,   123,   124,
     125,   199,   126,   200,   127,   128,   129,   201,   172,   130,
     179,   131,   132,   180,   133,   134,   181,   135,   112,   113,
     114,    61,   154,   212,    92,    63,    64,    65,    66,    67,
      68,    29,    69,   116,    70,   185,   204,    73
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -155
static const yytype_int16 yypact[] =
{
    -155,    26,  -155,    38,   -27,    91,    12,   207,   -24,    -7,
      40,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
    -155,     9,  -155,    39,    66,  -155,  -155,  -155,  -155,    74,
    -155,   116,   239,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,   271,  -155,   298,  -155,    13,  -155,  -155,
    -155,  -155,  -155,    67,  -155,  -155,  -155,  -155,   122,   106,
    -155,    97,    97,  -155,   151,  -155,  -155,   137,  -155,    99,
    -155,   298,  -155,   298,  -155,  -155,  -155,    97,     0,    20,
     126,  -155,    89,  -155,    18,   109,  -155,  -155,  -155,    47,
    -155,  -155,  -155,   326,     7,  -155,    97,  -155,   110,  -155,
      92,  -155,   100,    96,   147,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,   135,
     107,  -155,    97,  -155,  -155,   106,  -155,  -155,  -155,    97,
    -155,   114,  -155,  -155,  -155,  -155,  -155,   123,   110,    -9,
     110,   111,   121,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
      74,  -155,  -155,    74,    14,  -155,    10,   172,  -155,    99,
      99,    99,  -155,  -155,  -155,    97,  -155,    20,  -155,  -155,
     103,  -155,   143,  -155,  -155,  -155,   143,    14,    -7,  -155,
    -155,    99,  -155,  -155,  -155,   150,  -155,   163,  -155,   164,
     115,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,    22,
     110,   110,   110,   130,    98,   166,  -155,  -155,  -155,   172,
     110,  -155,    97,   155,   157,   143,  -155,   167,  -155,  -155,
     143,  -155,   197,   199,   201,   172,   172,  -155,   172,   171,
     203,   205,  -155,  -155,  -155
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,  -155,  -154,  -155,  -155,  -155,  -155,  -155,
    -155,  -155,  -155,  -155,  -155,   -70,  -155,    27,  -155,   -80,
    -155,  -155,  -155,  -155,  -155,  -155,   113,  -155,  -155,    73,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,   -68,  -155,
    -155,   139,  -155,  -155,  -155,  -155,  -155,  -155,   129,  -155,
    -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,  -155,
    -155,   -62,  -155,    23,   204,   -37,   -43,   -56,  -155,   177,
     102,  -116,   -29,  -155,   220,   112,   132,   125
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -165
static const yytype_int16 yytable[] =
{
      85,   101,   102,   170,   192,   173,   196,    91,  -124,    21,
      76,   205,    22,    30,    94,   202,   193,   137,   138,   146,
      90,   141,   115,   231,   -60,   -69,     2,  -124,    93,   152,
      62,    20,  -124,   206,    71,   203,   157,   147,   142,     4,
      80,     5,     6,     7,     8,     9,    10,   194,   150,    81,
      90,   153,    82,    72,    93,    88,    21,   155,  -164,    22,
     100,   151,   186,  -164,  -164,  -124,   233,   234,   235,   188,
      77,   207,    31,    95,  -164,  -164,   240,  -164,  -164,  -164,
    -164,  -164,   232,  -164,   224,  -164,  -164,  -164,  -164,  -164,
    -164,  -164,  -164,  -164,  -164,  -164,    11,   -93,    74,  -164,
      21,  -164,  -164,    22,   218,   216,    21,   100,   183,    22,
     148,   213,   214,   215,    47,   189,   183,    86,    51,   119,
     119,    54,   120,   120,    83,    96,    87,   184,   149,   190,
      98,   158,   -70,   225,   159,   184,    81,    97,    47,    43,
     -49,    85,    51,   176,    85,    54,   198,   144,   171,   121,
     121,    23,   241,   174,   175,    57,   237,   100,   122,   122,
     182,   160,   103,    33,   197,   176,    36,   104,   105,   176,
     161,   177,    42,   226,    44,   249,   250,   176,   251,   176,
     242,   191,   243,    39,    40,    41,   227,    56,   236,   228,
      46,   238,   244,    49,    60,    85,   252,    53,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,   245,    56,   246,    57,   248,    58,    59,   253,    60,
     254,    32,   208,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,   239,    56,   187,    57,
     217,    58,    59,   178,    60,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,   195,    56,
     156,    57,   219,    58,    59,   136,    60,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,   117,    44,     0,
      46,    47,     0,    49,    50,    51,    52,    53,    54,    55,
       0,    56,   229,   223,    33,    58,    59,    36,    60,   222,
      39,    40,    41,    42,     0,    44,     0,    46,    47,     0,
      49,     0,    51,     0,    53,    54,     0,     0,    56,     0,
       0,     0,     0,    34,    35,    60,    37,    38,    39,    40,
      41,     0,    43,     0,    45,    46,     0,    48,    49,    50,
       0,    52,    53,     0,    55,     0,     0,     0,    57,     0,
      58,    59
};

static const yytype_int16 yycheck[] =
{
      29,    81,    82,   119,   158,   121,   160,    63,     1,     9,
       1,     1,    12,     1,     1,     1,    25,    97,    98,     1,
      63,     1,    84,     1,    15,    25,     0,    20,    65,   109,
       7,    58,    25,    23,    58,    21,   116,    19,    18,     1,
       1,     3,     4,     5,     6,     7,     8,    56,     1,    10,
      93,   113,    13,    60,    91,    32,     9,   113,    11,    12,
      60,    14,   142,    16,    17,    58,   220,   221,   222,   149,
      61,    61,    60,    60,    27,    28,   230,    30,    31,    32,
      33,    34,    60,    36,   200,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    58,    58,    58,    52,
       9,    54,    55,    12,     1,   185,     9,    60,     1,    12,
       1,   179,   180,   181,    40,     1,     1,     1,    44,    21,
      21,    47,    24,    24,    58,    58,    10,    20,    19,    15,
      24,    21,    25,   201,    24,    20,    10,    15,    40,    36,
      25,   170,    44,    22,   173,    47,    25,    58,    56,    51,
      51,    60,   232,    53,    58,    52,   224,    60,    60,    60,
      25,    51,    11,    26,    53,    22,    29,    16,    17,    22,
      60,    24,    35,    23,    37,   245,   246,    22,   248,    22,
      25,    58,    25,    32,    33,    34,    23,    50,    58,    25,
      39,    25,    25,    42,    57,   224,    25,    46,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    24,    50,    24,    52,    24,    54,    55,    25,    57,
      25,    24,    60,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,   229,    50,   145,    52,
     187,    54,    55,   124,    57,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,   159,    50,
     113,    52,   190,    54,    55,    91,    57,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    87,    37,    -1,
      39,    40,    -1,    42,    43,    44,    45,    46,    47,    48,
      -1,    50,   210,   198,    26,    54,    55,    29,    57,   197,
      32,    33,    34,    35,    -1,    37,    -1,    39,    40,    -1,
      42,    -1,    44,    -1,    46,    47,    -1,    -1,    50,    -1,
      -1,    -1,    -1,    27,    28,    57,    30,    31,    32,    33,
      34,    -1,    36,    -1,    38,    39,    -1,    41,    42,    43,
      -1,    45,    46,    -1,    48,    -1,    -1,    -1,    52,    -1,
      54,    55
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    63,     0,    64,     1,     3,     4,     5,     6,     7,
       8,    58,    65,    67,    68,    69,    71,    73,    74,    75,
      58,     9,    12,    60,    91,    92,    96,   102,   109,   143,
       1,    60,    24,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    50,    52,    54,    55,
      57,   133,   135,   137,   138,   139,   140,   141,   142,   144,
     146,    58,    60,   149,    58,    66,     1,    61,    93,    97,
       1,    10,    13,    58,   103,   144,     1,    10,   135,    70,
     138,   139,   136,   137,     1,    60,    58,    15,    24,    98,
      60,    91,    91,    11,    16,    17,    94,   104,   105,   106,
     107,   108,   130,   131,   132,   133,   145,   146,    72,    21,
      24,    51,    60,   110,   111,   112,   114,   116,   117,   118,
     121,   123,   124,   126,   127,   129,   136,    91,    91,    99,
     100,     1,    18,   101,    58,    95,     1,    19,     1,    19,
       1,    14,    91,   133,   134,   139,   141,    91,    21,    24,
      51,    60,    76,    77,    78,    80,    82,    83,    84,    85,
     143,    56,   120,   143,    53,    58,    22,    24,   113,   122,
     125,   128,    25,     1,    20,   147,    91,    98,    91,     1,
      15,    58,    76,    25,    56,   120,    76,    53,    25,   113,
     115,   119,     1,    21,   148,     1,    23,    61,    60,    87,
      88,    89,   135,   110,   110,   110,    91,   101,     1,   142,
      79,    81,   148,   149,   143,   110,    23,    23,    25,   147,
      90,     1,    60,    76,    76,    76,    58,   110,    25,    89,
      76,    91,    25,    25,    25,    24,    24,    86,    24,    87,
      87,    87,    25,    25,    25
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:
#line 405 "parser.y"
    {
      newlined = true;
    }
    break;

  case 13:
#line 420 "parser.y"
    {
      PARSE_ERROR( "unexpected token" );
    }
    break;

  case 14:
#line 427 "parser.y"
    {
      parse_cleanup();
    }
    break;

  case 15:
#line 438 "parser.y"
    {
      DUMP_START( "cast_english", "CAST NAME INTO decl_english END" );
      DUMP_NAME( "> NAME", (yyvsp[(2) - (5)].name) );
      DUMP_AST( "> decl_english", (yyvsp[(4) - (5)].ast_pair).top_ast );
      DUMP_END();

      C_AST_CHECK( (yyvsp[(4) - (5)].ast_pair).top_ast, CHECK_CAST );
      FPUTC( '(', fout );
      c_ast_gibberish_cast( (yyvsp[(4) - (5)].ast_pair).top_ast, fout );
      FPRINTF( fout, ")%s\n", (yyvsp[(2) - (5)].name) );
      FREE( (yyvsp[(2) - (5)].name) );
    }
    break;

  case 16:
#line 452 "parser.y"
    {
      PARSE_ERROR( "\"%s\" expected", L_INTO );
    }
    break;

  case 17:
#line 457 "parser.y"
    {
      DUMP_START( "cast_english", "CAST decl_english END" );
      DUMP_AST( "> decl_english", (yyvsp[(2) - (3)].ast_pair).top_ast );
      DUMP_END();

      C_AST_CHECK( (yyvsp[(2) - (3)].ast_pair).top_ast, CHECK_CAST );
      FPUTC( '(', fout );
      c_ast_gibberish_cast( (yyvsp[(2) - (3)].ast_pair).top_ast, fout );
      FPUTS( ")\n", fout );
    }
    break;

  case 18:
#line 475 "parser.y"
    {
      DUMP_START( "declare_english",
                  "DECLARE NAME AS storage_class_opt_c decl_english END" );
      (yyvsp[(5) - (6)].ast_pair).top_ast->name = (yyvsp[(2) - (6)].name);
      DUMP_NAME( "> NAME", (yyvsp[(2) - (6)].name) );
      DUMP_TYPE( "> storage_class_opt_c", (yyvsp[(4) - (6)].type) );
      DUMP_AST( "> decl_english", (yyvsp[(5) - (6)].ast_pair).top_ast );
      DUMP_END();

      C_TYPE_ADD( &(yyvsp[(5) - (6)].ast_pair).top_ast->type, (yyvsp[(4) - (6)].type), (yylsp[(4) - (6)]) );
      C_AST_CHECK( (yyvsp[(5) - (6)].ast_pair).top_ast, CHECK_DECL );
      c_ast_gibberish_declare( (yyvsp[(5) - (6)].ast_pair).top_ast, fout );
      FPUTC( '\n', fout );
    }
    break;

  case 19:
#line 491 "parser.y"
    {
      PARSE_ERROR( "name expected" );
    }
    break;

  case 20:
#line 496 "parser.y"
    {
      PARSE_ERROR( "\"%s\" expected", L_AS );
    }
    break;

  case 21:
#line 506 "parser.y"
    { type_push( (yyvsp[(2) - (2)].ast_pair).top_ast ); }
    break;

  case 22:
#line 507 "parser.y"
    {
      type_pop();

      DUMP_START( "explain_declaration_c", "EXPLAIN type_c decl_c END" );
      DUMP_AST( "> type_c", (yyvsp[(2) - (5)].ast_pair).top_ast );
      DUMP_AST( "> decl_c", (yyvsp[(4) - (5)].ast_pair).top_ast );
      DUMP_END();

      c_ast_t *const ast = c_ast_patch_none( (yyvsp[(2) - (5)].ast_pair).top_ast, (yyvsp[(4) - (5)].ast_pair).top_ast );
      C_AST_CHECK( ast, CHECK_DECL );
      char const *const name = c_ast_take_name( ast );
      assert( name );
      FPRINTF( fout, "%s %s %s ", L_DECLARE, name, L_AS );
      if ( c_ast_take_typedef( ast ) )
        FPRINTF( fout, "%s ", L_TYPE );
      c_ast_english( ast, fout );
      FPUTC( '\n', fout );
      FREE( name );
    }
    break;

  case 23:
#line 529 "parser.y"
    { type_push( (yyvsp[(3) - (3)].ast_pair).top_ast ); }
    break;

  case 24:
#line 531 "parser.y"
    {
      type_pop();

      DUMP_START( "explain_cast_t",
                  "EXPLAIN '(' type_c cast_c ')' name_token_opt END" );
      DUMP_AST( "> type_c", (yyvsp[(3) - (8)].ast_pair).top_ast );
      DUMP_AST( "> cast_c", (yyvsp[(5) - (8)].ast_pair).top_ast );
      DUMP_NAME( "> name_token_opt", (yyvsp[(7) - (8)].name) );
      DUMP_END();

      c_ast_t *const ast = c_ast_patch_none( (yyvsp[(3) - (8)].ast_pair).top_ast, (yyvsp[(5) - (8)].ast_pair).top_ast );
      C_AST_CHECK( ast, CHECK_CAST );
      FPUTS( L_CAST, fout );
      if ( (yyvsp[(7) - (8)].name) ) {
        FPRINTF( fout, " %s", (yyvsp[(7) - (8)].name) );
        FREE( (yyvsp[(7) - (8)].name) );
      }
      FPRINTF( fout, " %s ", L_INTO );
      c_ast_english( ast, fout );
      FPUTC( '\n', fout );
    }
    break;

  case 25:
#line 559 "parser.y"
    { print_help(); }
    break;

  case 26:
#line 567 "parser.y"
    { set_option( (yyvsp[(2) - (3)].name) ); FREE( (yyvsp[(2) - (3)].name) ); }
    break;

  case 27:
#line 575 "parser.y"
    { quit(); }
    break;

  case 28:
#line 583 "parser.y"
    { (yyval.ast_pair).top_ast = (yyval.ast_pair).target_ast = NULL; }
    break;

  case 36:
#line 595 "parser.y"
    {
      DUMP_START( "array_cast_c", "cast_c array_cast_c" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_AST( "> cast_c", (yyvsp[(1) - (2)].ast_pair).top_ast );
      DUMP_NUM( "> array_size_c", (yyvsp[(2) - (2)].number) );

      if ( (yyvsp[(1) - (2)].ast_pair).target_ast )
        DUMP_AST( "> target_ast", (yyvsp[(1) - (2)].ast_pair).target_ast );

      c_ast_t *const array = c_ast_new( K_ARRAY, ast_depth, &(yyloc) );
      array->as.array.size = (yyvsp[(2) - (2)].number);
      c_ast_set_parent( c_ast_new( K_NONE, ast_depth, &(yylsp[(1) - (2)]) ), array );
      if ( (yyvsp[(1) - (2)].ast_pair).target_ast ) {
        (yyval.ast_pair).top_ast = (yyvsp[(1) - (2)].ast_pair).top_ast;
        (yyval.ast_pair).target_ast = c_ast_add_array( (yyvsp[(1) - (2)].ast_pair).target_ast, array );
      } else {
        (yyval.ast_pair).top_ast = c_ast_add_array( (yyvsp[(1) - (2)].ast_pair).top_ast, array );
        (yyval.ast_pair).target_ast = NULL;
      }

      DUMP_AST( "< array_cast_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 37:
#line 622 "parser.y"
    {
      //
      // A block AST has to be the type inherited attribute for cast_c so we
      // have to create it here.
      //
      type_push( c_ast_new( K_BLOCK, ast_depth, &(yyloc) ) );
    }
    break;

  case 38:
#line 630 "parser.y"
    {
      c_ast_t *const block = type_pop();

      DUMP_START( "block_cast_c",
                  "'(' '^' cast_c ')' '(' arg_list_opt_c ')'" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_AST( "> cast_c", (yyvsp[(4) - (8)].ast_pair).top_ast );
      DUMP_AST_LIST( "> arg_list_opt_c", (yyvsp[(7) - (8)].ast_list) );

      (yyval.ast_pair).top_ast->as.block.args = (yyvsp[(7) - (8)].ast_list);
      (yyval.ast_pair).top_ast = c_ast_add_func( (yyvsp[(4) - (8)].ast_pair).top_ast, type_peek(), block );
      (yyval.ast_pair).target_ast = block->as.block.ret_ast;

      DUMP_AST( "< block_cast_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 39:
#line 650 "parser.y"
    {
      DUMP_START( "func_cast_c", "'(' ')'" );
      DUMP_AST( "^ type_c", type_peek() );

      (yyval.ast_pair).top_ast = c_ast_new( K_FUNCTION, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      c_ast_set_parent( type_peek(), (yyval.ast_pair).top_ast );

      DUMP_AST( "< func_cast_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 40:
#line 662 "parser.y"
    { type_push( (yyvsp[(2) - (2)].ast_pair).top_ast ); }
    break;

  case 41:
#line 664 "parser.y"
    {
      type_pop();
      DUMP_START( "func_cast_c", "'(' cast_c ')' '(' arg_list_opt_c ')'" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_AST( "> placeholder_type_c", (yyvsp[(2) - (6)].ast_pair).top_ast );
      DUMP_AST( "> cast_c", (yyvsp[(4) - (6)].ast_pair).top_ast );
      DUMP_AST_LIST( "> arg_list_opt_c", (yyvsp[(6) - (6)].ast_list) );

      c_ast_t *const func = c_ast_new( K_FUNCTION, ast_depth, &(yyloc) );
      func->as.func.args = (yyvsp[(6) - (6)].ast_list);
      (yyval.ast_pair).top_ast = c_ast_add_func( (yyvsp[(4) - (6)].ast_pair).top_ast, type_peek(), func );
      (yyval.ast_pair).target_ast = NULL;

      DUMP_AST( "< func_cast_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 42:
#line 684 "parser.y"
    {
      DUMP_START( "name_cast_c", "NAME" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_NAME( "> NAME", (yyvsp[(1) - (1)].name) );

      (yyval.ast_pair).top_ast = type_peek();
      (yyval.ast_pair).target_ast = NULL;
      assert( (yyval.ast_pair).top_ast->name == NULL );
      (yyval.ast_pair).top_ast->name = (yyvsp[(1) - (1)].name);

      DUMP_AST( "< name_cast_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 43:
#line 701 "parser.y"
    {
      DUMP_START( "pointer_cast_c", "'*' cast_c" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_AST( "> cast_c", (yyvsp[(2) - (2)].ast_pair).top_ast );

      (yyval.ast_pair).top_ast = c_ast_new( K_POINTER, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      // TODO: do something with $2
      c_ast_set_parent( type_peek(), (yyval.ast_pair).top_ast );

      DUMP_AST( "< pointer_cast_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 44:
#line 718 "parser.y"
    {
      DUMP_START( "pointer_to_member_cast_c", "NAME COLON_COLON '*' cast_c" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_NAME( "> NAME", (yyvsp[(1) - (4)].name) );
      DUMP_AST( "> cast_c", (yyvsp[(4) - (4)].ast_pair).top_ast );

      (yyval.ast_pair).top_ast = c_ast_new( K_POINTER_TO_MEMBER, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = T_CLASS;
      c_ast_set_parent( type_peek(), (yyval.ast_pair).top_ast );
      (yyval.ast_pair).top_ast->as.ptr_mbr.class_name = (yyvsp[(1) - (4)].name);
      // TODO: do something with $4

      DUMP_AST( "< pointer_to_member_cast_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 45:
#line 738 "parser.y"
    {
      DUMP_START( "reference_cast_c", "'&' cast_c" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_AST( "> cast_c", (yyvsp[(2) - (2)].ast_pair).top_ast );

      (yyval.ast_pair).top_ast = c_ast_new( K_REFERENCE, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      // TODO: do something with $2
      c_ast_set_parent( type_peek(), (yyval.ast_pair).top_ast );

      DUMP_AST( "< reference_cast_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 46:
#line 758 "parser.y"
    { (yyval.ast_list).head_ast = (yyval.ast_list).tail_ast = NULL; }
    break;

  case 47:
#line 759 "parser.y"
    { (yyval.ast_list) = (yyvsp[(2) - (3)].ast_list); }
    break;

  case 48:
#line 763 "parser.y"
    { (yyval.ast_list).head_ast = (yyval.ast_list).tail_ast = NULL; }
    break;

  case 50:
#line 769 "parser.y"
    {
      DUMP_START( "arg_list_c", "arg_list_c ',' cast_c" );
      DUMP_AST_LIST( "> arg_list_c", (yyvsp[(1) - (3)].ast_list) );
      DUMP_AST( "> cast_c", (yyvsp[(3) - (3)].ast_pair).top_ast );

      (yyval.ast_list) = (yyvsp[(1) - (3)].ast_list);
      c_ast_list_append( &(yyval.ast_list), (yyvsp[(3) - (3)].ast_pair).top_ast );

      DUMP_AST_LIST( "< arg_list_c", (yyval.ast_list) );
      DUMP_END();
    }
    break;

  case 51:
#line 782 "parser.y"
    {
      DUMP_START( "arg_list_c", "arg_c" );
      DUMP_AST( "> arg_c", (yyvsp[(1) - (1)].ast_pair).top_ast );

      (yyval.ast_list).head_ast = (yyval.ast_list).tail_ast = NULL;
      c_ast_list_append( &(yyval.ast_list), (yyvsp[(1) - (1)].ast_pair).top_ast );

      DUMP_AST_LIST( "< arg_list_c", (yyval.ast_list) );
      DUMP_END();
    }
    break;

  case 52:
#line 795 "parser.y"
    { type_push( (yyvsp[(1) - (1)].ast_pair).top_ast ); }
    break;

  case 53:
#line 796 "parser.y"
    {
      type_pop();
      DUMP_START( "arg_c", "type_c cast_c" );
      DUMP_AST( "> type_c", (yyvsp[(1) - (3)].ast_pair).top_ast );
      DUMP_AST( "> cast_c", (yyvsp[(3) - (3)].ast_pair).top_ast );

      (yyval.ast_pair) = (yyvsp[(3) - (3)].ast_pair).top_ast ? (yyvsp[(3) - (3)].ast_pair) : (yyvsp[(1) - (3)].ast_pair);
      if ( (yyval.ast_pair).top_ast->name == NULL )
        (yyval.ast_pair).top_ast->name = check_strdup( c_ast_name( (yyval.ast_pair).top_ast, V_DOWN ) );

      DUMP_AST( "< arg_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 54:
#line 811 "parser.y"
    {
      DUMP_START( "argc", "NAME" );
      DUMP_NAME( "> NAME", (yyvsp[(1) - (1)].name) );

      (yyval.ast_pair).top_ast = c_ast_new( K_NAME, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->name = check_strdup( (yyvsp[(1) - (1)].name) );

      DUMP_END();
    }
    break;

  case 59:
#line 836 "parser.y"
    {
      DUMP_START( "array_decl_english",
                  "ARRAY array_size_opt_english OF decl_english" );
      DUMP_NUM( "> array_size_opt_english", (yyvsp[(2) - (4)].number) );
      DUMP_AST( "> decl_english", (yyvsp[(4) - (4)].ast_pair).top_ast );

      switch ( (yyvsp[(4) - (4)].ast_pair).top_ast->kind ) {
        case K_BUILTIN:
          if ( (yyvsp[(4) - (4)].ast_pair).top_ast->type & T_VOID ) {
            print_error( &(yylsp[(4) - (4)]), "array of void" );
            print_hint( "pointer to void" );
          }
          break;
        case K_FUNCTION:
          print_error( &(yylsp[(4) - (4)]), "array of function" );
          print_hint( "array of pointer to function" );
          break;
        default:
          /* suppress warning */;
      } // switch

      (yyval.ast_pair).top_ast = c_ast_new( K_ARRAY, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->as.array.size = (yyvsp[(2) - (4)].number);
      c_ast_set_parent( (yyvsp[(4) - (4)].ast_pair).top_ast, (yyval.ast_pair).top_ast );

      DUMP_AST( "< array_decl_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 60:
#line 868 "parser.y"
    { (yyval.number) = C_ARRAY_NO_SIZE; }
    break;

  case 62:
#line 870 "parser.y"
    { PARSE_ERROR( "array size expected" ); }
    break;

  case 63:
#line 874 "parser.y"
    { in_attr.y_token = Y_BLOCK; }
    break;

  case 64:
#line 876 "parser.y"
    {
      DUMP_START( "block_decl_english",
                  "BLOCK paren_decl_list_opt_english returning_english" );
      DUMP_TYPE( "^ qualifier", qualifier_peek() );
      DUMP_AST_LIST( "> paren_decl_list_opt_english", (yyvsp[(3) - (4)].ast_list) );
      DUMP_AST( "> returning_english", (yyvsp[(4) - (4)].ast_pair).top_ast );

      (yyval.ast_pair).top_ast = c_ast_new( K_BLOCK, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = qualifier_peek();
      c_ast_set_parent( (yyvsp[(4) - (4)].ast_pair).top_ast, (yyval.ast_pair).top_ast );
      (yyval.ast_pair).top_ast->as.block.args = (yyvsp[(3) - (4)].ast_list);

      DUMP_AST( "< block_decl_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 65:
#line 895 "parser.y"
    { in_attr.y_token = Y_FUNCTION; }
    break;

  case 66:
#line 897 "parser.y"
    {
      DUMP_START( "func_decl_english",
                  "FUNCTION paren_decl_list_opt_english returning_english" );
      DUMP_AST_LIST( "> decl_list_opt_english", (yyvsp[(3) - (4)].ast_list) );
      DUMP_AST( "> returning_english", (yyvsp[(4) - (4)].ast_pair).top_ast );

      (yyval.ast_pair).top_ast = c_ast_new( K_FUNCTION, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      c_ast_set_parent( (yyvsp[(4) - (4)].ast_pair).top_ast, (yyval.ast_pair).top_ast );
      (yyval.ast_pair).top_ast->as.func.args = (yyvsp[(3) - (4)].ast_list);

      DUMP_AST( "< func_decl_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 67:
#line 914 "parser.y"
    { (yyval.ast_list).head_ast = (yyval.ast_list).tail_ast = NULL; }
    break;

  case 68:
#line 916 "parser.y"
    {
      DUMP_START( "paren_decl_list_opt_english",
                  "'(' decl_list_opt_english ')'" );
      DUMP_AST_LIST( "> decl_list_opt_english", (yyvsp[(2) - (3)].ast_list) );

      (yyval.ast_list) = (yyvsp[(2) - (3)].ast_list);

      DUMP_AST_LIST( "< paren_decl_list_opt_english", (yyval.ast_list) );
      DUMP_END();
    }
    break;

  case 69:
#line 929 "parser.y"
    { (yyval.ast_list).head_ast = (yyval.ast_list).tail_ast = NULL; }
    break;

  case 71:
#line 934 "parser.y"
    { (yyval.ast_list).head_ast = (yyval.ast_list).tail_ast = (yyvsp[(1) - (1)].ast_pair).top_ast; }
    break;

  case 72:
#line 936 "parser.y"
    {
      DUMP_START( "decl_list_opt_english",
                  "decl_list_opt_english ',' decl_english" );
      DUMP_AST_LIST( "> decl_list_opt_english", (yyvsp[(1) - (3)].ast_list) );
      DUMP_AST( "> decl_english", (yyvsp[(3) - (3)].ast_pair).top_ast );

      (yyval.ast_list) = (yyvsp[(1) - (3)].ast_list);
      c_ast_list_append( &(yyval.ast_list), (yyvsp[(3) - (3)].ast_pair).top_ast );

      DUMP_AST_LIST( "< decl_list_opt_english", (yyval.ast_list) );
      DUMP_END();
    }
    break;

  case 73:
#line 952 "parser.y"
    {
      DUMP_START( "returning_english", "RETURNING decl_english" );
      DUMP_AST( "> decl_english", (yyvsp[(2) - (2)].ast_pair).top_ast );

      c_keyword_t const *keyword;
      switch ( (yyvsp[(2) - (2)].ast_pair).top_ast->kind ) {
        case K_ARRAY:
        case K_FUNCTION:
          keyword = c_keyword_find_token( in_attr.y_token );
        default:
          keyword = NULL;
      } // switch

      if ( keyword ) {
        char error_msg[ 80 ];
        char const *hint;

        switch ( (yyvsp[(2) - (2)].ast_pair).top_ast->kind ) {
          case K_ARRAY:
            hint = "pointer";
            break;
          case K_FUNCTION:
            hint = "pointer to function";
            break;
          default:
            hint = NULL;
        } // switch

        snprintf( error_msg, sizeof error_msg,
          "%s returning %s",
          keyword->literal, c_kind_name( (yyvsp[(2) - (2)].ast_pair).top_ast->kind )
        );

        print_error( &(yylsp[(2) - (2)]), error_msg );
        if ( hint )
          print_hint( "%s returning %s", keyword->literal, hint );
      }

      (yyval.ast_pair) = (yyvsp[(2) - (2)].ast_pair);

      DUMP_AST( "< returning_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 74:
#line 996 "parser.y"
    { PARSE_ERROR( "\"%s\" expected", L_RETURNING ); }
    break;

  case 75:
#line 1000 "parser.y"
    { qualifier_push( (yyvsp[(1) - (1)].type), &(yylsp[(1) - (1)]) ); }
    break;

  case 76:
#line 1002 "parser.y"
    {
      qualifier_pop();
      DUMP_START( "qualified_decl_english",
                  "type_qualifier_list_opt_c qualifiable_decl_english" );
      DUMP_TYPE( "> type_qualifier_list_opt_c", (yyvsp[(1) - (3)].type) );
      DUMP_AST( "> qualifiable_decl_english", (yyvsp[(3) - (3)].ast_pair).top_ast );

      (yyval.ast_pair) = (yyvsp[(3) - (3)].ast_pair);

      DUMP_AST( "< qualified_decl_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 82:
#line 1026 "parser.y"
    {
      DUMP_START( "pointer_decl_english", "POINTER TO decl_english" );
      DUMP_TYPE( "^ qualifier", qualifier_peek() );
      DUMP_AST( "> decl_english", (yyvsp[(2) - (2)].ast_pair).top_ast );

      (yyval.ast_pair).top_ast = c_ast_new( K_POINTER, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      c_ast_set_parent( (yyvsp[(2) - (2)].ast_pair).top_ast, (yyval.ast_pair).top_ast );
      (yyval.ast_pair).top_ast->as.ptr_ref.qualifier = qualifier_peek();

      DUMP_AST( "< pointer_decl_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 84:
#line 1044 "parser.y"
    {
      PARSE_ERROR( "\"%s\" expected", L_TO );
    }
    break;

  case 85:
#line 1051 "parser.y"
    {
      DUMP_START( "pointer_to_member_decl_english",
                  "POINTER TO MEMBER OF "
                  "class_struct_type_c NAME decl_english" );
      DUMP_TYPE( "^ qualifier", qualifier_peek() );
      DUMP_TYPE( "> class_struct_type_c", (yyvsp[(4) - (6)].type) );
      DUMP_NAME( "> NAME", (yyvsp[(5) - (6)].name) );
      DUMP_AST( "> decl_english", (yyvsp[(6) - (6)].ast_pair).top_ast );

      if ( opt_lang < LANG_CPP_MIN )
        print_warning( &(yyloc), "pointer to member of class" );

      (yyval.ast_pair).top_ast = c_ast_new( K_POINTER_TO_MEMBER, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = (yyvsp[(4) - (6)].type);
      c_ast_set_parent( (yyvsp[(6) - (6)].ast_pair).top_ast, (yyval.ast_pair).top_ast );
      (yyval.ast_pair).top_ast->as.ptr_ref.qualifier = qualifier_peek();
      (yyval.ast_pair).top_ast->as.ptr_mbr.class_name = (yyvsp[(5) - (6)].name);

      DUMP_AST( "< pointer_to_member_decl_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 86:
#line 1075 "parser.y"
    {
      PARSE_ERROR( "\"%s\" expected", L_MEMBER );
    }
    break;

  case 87:
#line 1080 "parser.y"
    {
      PARSE_ERROR( "\"%s\" expected", L_OF );
    }
    break;

  case 88:
#line 1085 "parser.y"
    {
      PARSE_ERROR(
        "\"%s\", \"%s\", or \"%s\" expected", L_CLASS, L_STRUCT, L_UNION
      );
    }
    break;

  case 89:
#line 1092 "parser.y"
    {
      PARSE_ERROR(
        "\"%s\", \"%s\", or \"%s\" name expected",
        L_CLASS, L_STRUCT, L_UNION
      );
    }
    break;

  case 90:
#line 1102 "parser.y"
    {
      DUMP_START( "reference_decl_english", "REFERENCE TO decl_english" );
      DUMP_TYPE( "^ qualifier", qualifier_peek() );
      DUMP_AST( "> decl_english", (yyvsp[(3) - (3)].ast_pair).top_ast );

      if ( opt_lang < LANG_CPP_MIN )
        print_warning( &(yyloc), "reference" );
      switch ( (yyvsp[(3) - (3)].ast_pair).top_ast->kind ) {
        case K_BUILTIN:
          if ( (yyvsp[(3) - (3)].ast_pair).top_ast->type & T_VOID ) {
            print_error( &(yylsp[(3) - (3)]), "reference of void" );
            print_hint( "pointer to void" );
          }
          break;
        default:
          /* suppress warning */;
      } // switch

      (yyval.ast_pair).top_ast = c_ast_new( K_REFERENCE, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      c_ast_set_parent( (yyvsp[(3) - (3)].ast_pair).top_ast, (yyval.ast_pair).top_ast );
      (yyval.ast_pair).top_ast->as.ptr_ref.qualifier = qualifier_peek();

      DUMP_AST( "< reference_decl_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 91:
#line 1130 "parser.y"
    {
      PARSE_ERROR( "\"%s\" expected", L_TO );
    }
    break;

  case 92:
#line 1137 "parser.y"
    {
      DUMP_START( "var_decl_english", "NAME AS decl_english" );
      DUMP_NAME( "> NAME", (yyvsp[(1) - (3)].name) );
      DUMP_AST( "> decl_english", (yyvsp[(3) - (3)].ast_pair).top_ast );

      (yyval.ast_pair) = (yyvsp[(3) - (3)].ast_pair);
      assert( (yyval.ast_pair).top_ast->name == NULL );
      (yyval.ast_pair).top_ast->name = (yyvsp[(1) - (3)].name);

      DUMP_AST( "< var_decl_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 93:
#line 1151 "parser.y"
    {
      DUMP_START( "var_decl_english", "NAME" );
      DUMP_NAME( "> NAME", (yyvsp[(1) - (1)].name) );

      if ( opt_lang > LANG_C_KNR )
        print_warning( &(yyloc), "missing function prototype" );

      (yyval.ast_pair).top_ast = c_ast_new( K_NAME, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->name = (yyvsp[(1) - (1)].name);

      DUMP_AST( "< var_decl_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 103:
#line 1188 "parser.y"
    {
      DUMP_START( "array_decl_c", "decl2_c array_size_c" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_AST( "> decl2_c", (yyvsp[(1) - (2)].ast_pair).top_ast );
      DUMP_NUM( "> array_size_c", (yyvsp[(2) - (2)].number) );
      if ( (yyvsp[(1) - (2)].ast_pair).target_ast )
        DUMP_AST( "> target_ast", (yyvsp[(1) - (2)].ast_pair).target_ast );

      c_ast_t *const array = c_ast_new( K_ARRAY, ast_depth, &(yyloc) );
      array->as.array.size = (yyvsp[(2) - (2)].number);
      c_ast_set_parent( c_ast_new( K_NONE, ast_depth, &(yylsp[(1) - (2)]) ), array );
      if ( (yyvsp[(1) - (2)].ast_pair).target_ast ) {
        (yyval.ast_pair).top_ast = (yyvsp[(1) - (2)].ast_pair).top_ast;
        (yyval.ast_pair).target_ast = c_ast_add_array( (yyvsp[(1) - (2)].ast_pair).target_ast, array );
      } else {
        (yyval.ast_pair).top_ast = c_ast_add_array( (yyvsp[(1) - (2)].ast_pair).top_ast, array );
        (yyval.ast_pair).target_ast = NULL;
      }

      DUMP_AST( "< array_decl_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 104:
#line 1213 "parser.y"
    { (yyval.number) = C_ARRAY_NO_SIZE; }
    break;

  case 105:
#line 1214 "parser.y"
    { (yyval.number) = (yyvsp[(2) - (3)].number); }
    break;

  case 106:
#line 1216 "parser.y"
    {
      PARSE_ERROR( "integer expected for array size" );
    }
    break;

  case 107:
#line 1223 "parser.y"
    {
      //
      // A block AST has to be the type inherited attribute for decl_c so we
      // have to create it here.
      //
      type_push( c_ast_new( K_BLOCK, ast_depth, &(yyloc) ) );
    }
    break;

  case 108:
#line 1231 "parser.y"
    {
      c_ast_t *const block = type_pop();

      DUMP_START( "block_decl_c",
                  "'(' '^' type_qualifier_list_opt_c decl_c ')' "
                  "'(' arg_list_opt_c ')'" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_TYPE( "> type_qualifier_list_opt_c", (yyvsp[(4) - (9)].type) );
      DUMP_AST( "> decl_c", (yyvsp[(5) - (9)].ast_pair).top_ast );
      DUMP_AST_LIST( "> arg_list_opt_c", (yyvsp[(8) - (9)].ast_list) );

      C_TYPE_ADD( &block->type, (yyvsp[(4) - (9)].type), (yylsp[(4) - (9)]) );
      block->as.block.args = (yyvsp[(8) - (9)].ast_list);
      (yyval.ast_pair).top_ast = c_ast_add_func( (yyvsp[(5) - (9)].ast_pair).top_ast, type_peek(), block );
      (yyval.ast_pair).target_ast = block->as.block.ret_ast;

      DUMP_AST( "< block_decl_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 109:
#line 1254 "parser.y"
    {
      DUMP_START( "func_decl_c", "decl2_c '(' arg_list_opt_c ')'" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_AST( "> decl2_c", (yyvsp[(1) - (4)].ast_pair).top_ast );
      DUMP_AST_LIST( "> arg_list_opt_c", (yyvsp[(3) - (4)].ast_list) );

      c_ast_t *const func = c_ast_new( K_FUNCTION, ast_depth, &(yyloc) );
      func->as.func.args = (yyvsp[(3) - (4)].ast_list);
      (yyval.ast_pair).top_ast = c_ast_add_func( (yyvsp[(1) - (4)].ast_pair).top_ast, type_peek(), func );
      (yyval.ast_pair).target_ast = func->as.func.ret_ast;

      DUMP_AST( "< func_decl_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 110:
#line 1272 "parser.y"
    {
      DUMP_START( "name_decl_c", "NAME" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_NAME( "> NAME", (yyvsp[(1) - (1)].name) );

      (yyval.ast_pair).top_ast = type_peek();
      (yyval.ast_pair).target_ast = NULL;
      assert( (yyval.ast_pair).top_ast->name == NULL );
      (yyval.ast_pair).top_ast->name = (yyvsp[(1) - (1)].name);

      DUMP_AST( "< name_decl_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 111:
#line 1288 "parser.y"
    { type_push( (yyvsp[(2) - (2)].ast_pair).top_ast ); }
    break;

  case 112:
#line 1289 "parser.y"
    {
      type_pop();

      DUMP_START( "nested_decl_c", "'(' placeholder_type_c decl_c ')'" );
      DUMP_AST( "> placeholder_type_c", (yyvsp[(2) - (5)].ast_pair).top_ast );
      DUMP_AST( "> decl_c", (yyvsp[(4) - (5)].ast_pair).top_ast );

      (yyval.ast_pair) = (yyvsp[(4) - (5)].ast_pair);

      DUMP_AST( "< nested_decl_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 113:
#line 1305 "parser.y"
    {
      (yyval.ast_pair).top_ast = c_ast_new( K_NONE, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
    }
    break;

  case 114:
#line 1312 "parser.y"
    { type_push( (yyvsp[(1) - (1)].ast_pair).top_ast ); }
    break;

  case 115:
#line 1313 "parser.y"
    {
      type_pop();
      DUMP_START( "pointer_decl_c", "pointer_decl_type_c decl_c" );
      DUMP_AST( "> pointer_decl_type_c", (yyvsp[(1) - (3)].ast_pair).top_ast );
      DUMP_AST( "> decl_c", (yyvsp[(3) - (3)].ast_pair).top_ast );

      c_ast_patch_none( (yyvsp[(1) - (3)].ast_pair).top_ast, (yyvsp[(3) - (3)].ast_pair).top_ast );
      (yyval.ast_pair) = (yyvsp[(3) - (3)].ast_pair);

      DUMP_AST( "< pointer_decl_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 116:
#line 1329 "parser.y"
    {
      DUMP_START( "pointer_decl_type_c", "'*' type_qualifier_list_opt_c" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_TYPE( "> type_qualifier_list_opt_c", (yyvsp[(2) - (2)].type) );

      (yyval.ast_pair).top_ast = c_ast_new( K_POINTER, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->as.ptr_ref.qualifier = (yyvsp[(2) - (2)].type);
      c_ast_set_parent( type_peek(), (yyval.ast_pair).top_ast );

      DUMP_AST( "< pointer_decl_type_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 117:
#line 1345 "parser.y"
    { type_push( (yyvsp[(1) - (1)].ast_pair).top_ast ); }
    break;

  case 118:
#line 1346 "parser.y"
    {
      type_pop();
      DUMP_START( "pointer_to_member_decl_c",
                  "pointer_to_member_decl_type_c decl_c" );
      DUMP_AST( "> pointer_to_member_decl_type_c", (yyvsp[(1) - (3)].ast_pair).top_ast );
      DUMP_AST( "> decl_c", (yyvsp[(3) - (3)].ast_pair).top_ast );

      (yyval.ast_pair) = (yyvsp[(3) - (3)].ast_pair);

      DUMP_AST( "< pointer_to_member_decl_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 119:
#line 1362 "parser.y"
    {
      DUMP_START( "pointer_to_member_decl_type_c", "NAME COLON_COLON '*'" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_NAME( "> NAME", (yyvsp[(1) - (3)].name) );

      (yyval.ast_pair).top_ast = c_ast_new( K_POINTER_TO_MEMBER, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = T_CLASS;
      (yyval.ast_pair).top_ast->as.ptr_mbr.class_name = (yyvsp[(1) - (3)].name);
      c_ast_set_parent( type_peek(), (yyval.ast_pair).top_ast );

      DUMP_AST( "< pointer_to_member_decl_type_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 120:
#line 1379 "parser.y"
    { type_push( (yyvsp[(1) - (1)].ast_pair).top_ast ); }
    break;

  case 121:
#line 1380 "parser.y"
    {
      type_pop();
      DUMP_START( "reference_decl_c", "reference_decl_type_c decl_c" );
      DUMP_AST( "> reference_decl_type_c", (yyvsp[(1) - (3)].ast_pair).top_ast );
      DUMP_AST( "> decl_c", (yyvsp[(3) - (3)].ast_pair).top_ast );

      (yyval.ast_pair) = (yyvsp[(3) - (3)].ast_pair);

      DUMP_AST( "< reference_decl_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 122:
#line 1395 "parser.y"
    {
      DUMP_START( "reference_decl_type_c", "'&' type_qualifier_list_opt_c" );
      DUMP_AST( "^ type_c", type_peek() );
      DUMP_TYPE( "> type_qualifier_list_opt_c", (yyvsp[(2) - (2)].type) );

      (yyval.ast_pair).top_ast = c_ast_new( K_REFERENCE, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->as.ptr_ref.qualifier = (yyvsp[(2) - (2)].type);
      c_ast_set_parent( type_peek(), (yyval.ast_pair).top_ast );

      DUMP_AST( "< reference_decl_type_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 123:
#line 1416 "parser.y"
    {
      DUMP_START( "type_english",
                  "type_modifier_list_opt_english unmodified_type_english" );
      DUMP_TYPE( "^ qualifier", qualifier_peek() );
      DUMP_TYPE( "> type_modifier_list_opt_english", (yyvsp[(1) - (2)].type) );
      DUMP_AST( "> unmodified_type_english", (yyvsp[(2) - (2)].ast_pair).top_ast );

      (yyval.ast_pair) = (yyvsp[(2) - (2)].ast_pair);
      C_TYPE_ADD( &(yyval.ast_pair).top_ast->type, qualifier_peek(), qualifier_peek_loc() );
      C_TYPE_ADD( &(yyval.ast_pair).top_ast->type, (yyvsp[(1) - (2)].type), (yylsp[(1) - (2)]) );

      DUMP_AST( "< type_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 124:
#line 1432 "parser.y"
    {
      DUMP_START( "type_english", "type_modifier_list_english" );
      DUMP_TYPE( "> type_modifier_list_english", (yyvsp[(1) - (1)].type) );

      (yyval.ast_pair).top_ast = c_ast_new( K_BUILTIN, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = T_INT;
      C_TYPE_ADD( &(yyval.ast_pair).top_ast->type, (yyvsp[(1) - (1)].type), (yylsp[(1) - (1)]) );

      DUMP_AST( "< type_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 125:
#line 1447 "parser.y"
    { (yyval.type) = T_NONE; }
    break;

  case 127:
#line 1453 "parser.y"
    {
      DUMP_START( "type_modifier_list_opt_english",
                  "type_modifier_list_opt_english type_modifier_english" );
      DUMP_TYPE( "> type_modifier_list_opt_english", (yyvsp[(1) - (2)].type) );
      DUMP_TYPE( "> type_modifier_english", (yyvsp[(2) - (2)].type) );

      (yyval.type) = (yyvsp[(1) - (2)].type);
      C_TYPE_ADD( &(yyval.type), (yyvsp[(2) - (2)].type), (yylsp[(2) - (2)]) );

      DUMP_TYPE( "< type_modifier_list_opt_english", (yyval.type) );
      DUMP_END();
    }
    break;

  case 135:
#line 1485 "parser.y"
    {
      DUMP_START( "unmodified_type_english", "builtin_type_c" );
      DUMP_TYPE( "> builtin_type_c", (yyvsp[(1) - (1)].type) );

      (yyval.ast_pair).top_ast = c_ast_new( K_BUILTIN, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = (yyvsp[(1) - (1)].type);

      DUMP_AST( "< unmodified_type_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 136:
#line 1498 "parser.y"
    {
      DUMP_START( "unmodified_type_english", "enum_class_struct_union_type_c" );
      DUMP_TYPE( "> enum_class_struct_union_type_c", (yyvsp[(1) - (1)].type) );

      (yyval.ast_pair).top_ast = c_ast_new( K_ENUM_CLASS_STRUCT_UNION, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = (yyvsp[(1) - (1)].type);

      DUMP_AST( "< unmodified_type_english", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 137:
#line 1517 "parser.y"
    {
      DUMP_START( "type_c", "type_modifier_list_c" );
      DUMP_TYPE( "> type_modifier_list_c", (yyvsp[(1) - (1)].type) );

      (yyval.ast_pair).top_ast = c_ast_new( K_BUILTIN, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = T_INT;
      C_TYPE_ADD( &(yyval.ast_pair).top_ast->type, (yyvsp[(1) - (1)].type), (yylsp[(1) - (1)]) );
      C_TYPE_CHECK( (yyval.ast_pair).top_ast->type, (yylsp[(1) - (1)]) );

      DUMP_AST( "< type_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 138:
#line 1532 "parser.y"
    {
      DUMP_START( "type_c",
                  "type_modifier_list_c builtin_type_c "
                  "type_modifier_list_opt_c" );
      DUMP_TYPE( "> type_modifier_list_c", (yyvsp[(1) - (3)].type) );
      DUMP_TYPE( "> builtin_type_c", (yyvsp[(2) - (3)].type) );
      DUMP_TYPE( "> type_modifier_list_opt_c", (yyvsp[(3) - (3)].type) );

      (yyval.ast_pair).top_ast = c_ast_new( K_BUILTIN, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = (yyvsp[(2) - (3)].type);
      C_TYPE_ADD( &(yyval.ast_pair).top_ast->type, (yyvsp[(1) - (3)].type), (yylsp[(1) - (3)]) );
      C_TYPE_ADD( &(yyval.ast_pair).top_ast->type, (yyvsp[(3) - (3)].type), (yylsp[(3) - (3)]) );
      C_TYPE_CHECK( (yyval.ast_pair).top_ast->type, (yylsp[(1) - (3)]) );

      DUMP_AST( "< type_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 139:
#line 1552 "parser.y"
    {
      DUMP_START( "type_c", "builtin_type_c type_modifier_list_opt_c" );
      DUMP_TYPE( "> builtin_type_c", (yyvsp[(1) - (2)].type) );
      DUMP_TYPE( "> type_modifier_list_opt_c", (yyvsp[(2) - (2)].type) );

      (yyval.ast_pair).top_ast = c_ast_new( K_BUILTIN, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = (yyvsp[(1) - (2)].type);
      C_TYPE_ADD( &(yyval.ast_pair).top_ast->type, (yyvsp[(2) - (2)].type), (yylsp[(2) - (2)]) );
      C_TYPE_CHECK( (yyval.ast_pair).top_ast->type, (yylsp[(1) - (2)]) );

      DUMP_AST( "< type_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 141:
#line 1571 "parser.y"
    { (yyval.type) = T_NONE; }
    break;

  case 143:
#line 1577 "parser.y"
    {
      DUMP_START( "type_modifier_list_c",
                  "type_modifier_list_c type_modifier_c" );
      DUMP_TYPE( "> type_modifier_list_c", (yyvsp[(1) - (2)].type) );
      DUMP_TYPE( "> type_modifier_c", (yyvsp[(2) - (2)].type) );

      (yyval.type) = (yyvsp[(1) - (2)].type);
      C_TYPE_ADD( &(yyval.type), (yyvsp[(2) - (2)].type), (yylsp[(2) - (2)]) );
      C_TYPE_CHECK( (yyval.type), (yylsp[(2) - (2)]) );

      DUMP_TYPE( "< type_modifier_list_c", (yyval.type) );
      DUMP_END();
    }
    break;

  case 144:
#line 1592 "parser.y"
    {
      (yyval.type) = (yyvsp[(1) - (1)].type);
      C_TYPE_CHECK( (yyval.type), (yylsp[(1) - (1)]) );
    }
    break;

  case 157:
#line 1618 "parser.y"
    {
      DUMP_START( "named_enum_class_struct_union_type_c",
                  "enum_class_struct_union_type_c NAME" );
      DUMP_TYPE( "> enum_class_struct_union_type_c", (yyvsp[(1) - (2)].type) );
      DUMP_NAME( "> NAME", (yyvsp[(2) - (2)].name) );

      (yyval.ast_pair).top_ast = c_ast_new( K_ENUM_CLASS_STRUCT_UNION, ast_depth, &(yyloc) );
      (yyval.ast_pair).target_ast = NULL;
      (yyval.ast_pair).top_ast->type = (yyvsp[(1) - (2)].type);
      (yyval.ast_pair).top_ast->as.ecsu.ecsu_name = (yyvsp[(2) - (2)].name);
      C_TYPE_CHECK( (yyval.ast_pair).top_ast->type, (yylsp[(1) - (2)]) );

      DUMP_AST( "< named_enum_class_struct_union_type_c", (yyval.ast_pair).top_ast );
      DUMP_END();
    }
    break;

  case 158:
#line 1635 "parser.y"
    {
      PARSE_ERROR(
        "%s name expected", c_kind_name( K_ENUM_CLASS_STRUCT_UNION )
      );
    }
    break;

  case 164:
#line 1654 "parser.y"
    { (yyval.type) = T_NONE; }
    break;

  case 165:
#line 1656 "parser.y"
    {
      DUMP_START( "type_qualifier_list_opt_c",
                  "type_qualifier_list_opt_c type_qualifier_c" );
      DUMP_TYPE( "> type_qualifier_list_opt_c", (yyvsp[(1) - (2)].type) );
      DUMP_TYPE( "> type_qualifier_c", (yyvsp[(2) - (2)].type) );

      (yyval.type) = (yyvsp[(1) - (2)].type);
      C_TYPE_ADD( &(yyval.type), (yyvsp[(2) - (2)].type), (yylsp[(2) - (2)]) );
      C_TYPE_CHECK( (yyval.type), (yylsp[(2) - (2)]) );

      DUMP_TYPE( "< type_qualifier_list_opt_c", (yyval.type) );
      DUMP_END();
    }
    break;

  case 169:
#line 1678 "parser.y"
    { (yyval.type) = T_NONE; }
    break;

  case 178:
#line 1698 "parser.y"
    { PARSE_ERROR( "',' expected" ); }
    break;

  case 180:
#line 1703 "parser.y"
    { PARSE_ERROR( "'*' expected" ); }
    break;

  case 181:
#line 1707 "parser.y"
    { (yyval.name) = NULL; }
    break;


/* Line 1267 of yacc.c.  */
#line 3292 "parser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 1711 "parser.y"


///////////////////////////////////////////////////////////////////////////////
/* vim:set et sw=2 ts=2: */

